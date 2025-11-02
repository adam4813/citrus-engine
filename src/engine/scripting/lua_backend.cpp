module;

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

module engine.scripting;

namespace engine::scripting {
    class LuaBackend : public IScriptingBackend {
    private:
        lua_State *lua_state_ = nullptr;

        // Helper to push a ScriptValue onto the Lua stack
        void PushValue(const ScriptValue &value) {
            if (value.Is<int>()) {
                lua_pushinteger(lua_state_, value.As<int>());
            } else if (value.Is<double>()) {
                lua_pushnumber(lua_state_, value.As<double>());
            } else if (value.Is<float>()) {
                lua_pushnumber(lua_state_, static_cast<double>(value.As<float>()));
            } else if (value.Is<std::string>()) {
                lua_pushstring(lua_state_, value.As<std::string>().c_str());
            } else if (value.Is<bool>()) {
                lua_pushboolean(lua_state_, value.As<bool>());
            } else {
                lua_pushnil(lua_state_);
            }
        }

        // Helper to pop a value from Lua stack and convert to ScriptValue
        ScriptValue PopValue() {
            ScriptValue result;
            const int value_type = lua_type(lua_state_, -1);

            switch (value_type) {
                case LUA_TNUMBER:
                    if (lua_isinteger(lua_state_, -1)) {
                        result.value = static_cast<int>(lua_tointeger(lua_state_, -1));
                    } else {
                        result.value = lua_tonumber(lua_state_, -1);
                    }
                    break;
                case LUA_TBOOLEAN:
                    result.value = static_cast<bool>(lua_toboolean(lua_state_, -1));
                    break;
                case LUA_TSTRING:
                    result.value = std::string(lua_tostring(lua_state_, -1));
                    break;
                case LUA_TNIL:
                default:
                    // For nil/unsupported types, leave as empty any
                    break;
            }

            lua_pop(lua_state_, 1);
            return result;
        }

        // Static destructor for cleaning up function objects in Lua userdata
        static int FunctionDestructor(lua_State *L) {
            auto *ptr = static_cast<std::function<ScriptValue(const std::vector<ScriptValue>&)>*>(
                lua_touserdata(L, 1)
            );
            ptr->~function();
            return 0;
        }

        // C function wrapper for Lua callbacks
        static int LuaCFunctionWrapper(lua_State *L) {
            // Get the function pointer from upvalue
            auto *func = static_cast<std::function<ScriptValue(const std::vector<ScriptValue>&)>*>(
                lua_touserdata(L, lua_upvalueindex(1))
            );

            // Get number of arguments
            const int argc = lua_gettop(L);

            // Convert Lua arguments to ScriptValues
            std::vector<ScriptValue> args;
            args.reserve(argc);
            for (int i = 1; i <= argc; ++i) {
                ScriptValue arg;
                const int value_type = lua_type(L, i);
                
                switch (value_type) {
                    case LUA_TNUMBER:
                        if (lua_isinteger(L, i)) {
                            arg.value = static_cast<int>(lua_tointeger(L, i));
                        } else {
                            arg.value = lua_tonumber(L, i);
                        }
                        break;
                    case LUA_TBOOLEAN:
                        arg.value = static_cast<bool>(lua_toboolean(L, i));
                        break;
                    case LUA_TSTRING:
                        arg.value = std::string(lua_tostring(L, i));
                        break;
                    default:
                        // For nil/unsupported types, leave as empty any
                        break;
                }
                args.push_back(arg);
            }

            // Call the C++ function
            ScriptValue result = (*func)(args);

            // Push result onto Lua stack
            if (result.Is<int>()) {
                lua_pushinteger(L, result.As<int>());
            } else if (result.Is<double>()) {
                lua_pushnumber(L, result.As<double>());
            } else if (result.Is<float>()) {
                lua_pushnumber(L, static_cast<double>(result.As<float>()));
            } else if (result.Is<std::string>()) {
                lua_pushstring(L, result.As<std::string>().c_str());
            } else if (result.Is<bool>()) {
                lua_pushboolean(L, result.As<bool>());
            } else {
                lua_pushnil(L);
            }

            return 1; // Number of return values
        }

    public:
        bool Initialize() override {
            lua_state_ = luaL_newstate();
            if (!lua_state_) {
                return false;
            }
            luaL_openlibs(lua_state_);
            return true;
        }

        void Shutdown() override {
            if (lua_state_) {
                lua_close(lua_state_);
                lua_state_ = nullptr;
            }
        }

        bool ExecuteString(const std::string &script) override {
            if (!lua_state_) {
                return false;
            }

            if (luaL_dostring(lua_state_, script.c_str()) != LUA_OK) {
                // Retrieve and log the error message
                const char* error_msg = lua_tostring(lua_state_, -1);
                spdlog::error("[Lua] Script execution failed: {}", error_msg ? error_msg : "unknown error");
                lua_pop(lua_state_, 1);
                return false;
            }

            return true;
        }

        bool ExecuteFile(const std::string &filepath) override {
            if (!lua_state_) {
                return false;
            }

            if (luaL_dofile(lua_state_, filepath.c_str()) != LUA_OK) {
                // Retrieve and log the error message
                const char* error_msg = lua_tostring(lua_state_, -1);
                spdlog::error("[Lua] Script file execution failed ({}): {}", filepath, error_msg ? error_msg : "unknown error");
                lua_pop(lua_state_, 1);
                return false;
            }

            return true;
        }

        void RegisterGlobalFunction(
            const std::string &name,
            std::function<ScriptValue(const std::vector<ScriptValue>&)> func,
            const std::string &signature
        ) override {
            // Note: Lua doesn't use the signature for type checking, but it's available for documentation
            (void)signature; // Suppress unused parameter warning
            if (!lua_state_) {
                return;
            }

            // Allocate storage for the function object on the heap
            // This will be owned by Lua's garbage collector
            using FuncType = std::function<ScriptValue(const std::vector<ScriptValue>&)>;
            constexpr size_t func_size = sizeof(FuncType);
            constexpr size_t func_align = alignof(FuncType);
            
            // Ensure proper alignment by allocating extra space if needed
            void* raw_ptr = lua_newuserdata(lua_state_, func_size);
            
            // Check alignment (should be valid for most platforms, but good practice)
            if (reinterpret_cast<uintptr_t>(raw_ptr) % func_align != 0) {
                spdlog::error("[Lua] Alignment check failed for function userdata. Expected alignment: {}, got address: {}", 
                              func_align, reinterpret_cast<uintptr_t>(raw_ptr));
                lua_pop(lua_state_, 1);
                return; // Alignment issue - this should not happen with lua_newuserdata
            }
            
            auto *func_ptr = static_cast<FuncType*>(raw_ptr);
            new (func_ptr) FuncType(std::move(func));

            // Create a metatable for cleanup
            lua_newtable(lua_state_);
            lua_pushcfunction(lua_state_, FunctionDestructor);
            lua_setfield(lua_state_, -2, "__gc");
            lua_setmetatable(lua_state_, -2);

            // Push the C closure with the function as an upvalue
            lua_pushcclosure(lua_state_, LuaCFunctionWrapper, 1);

            // Set as global
            lua_setglobal(lua_state_, name.c_str());
        }

        ScriptValue CallFunction(
            const std::string &name,
            const std::vector<ScriptValue> &args
        ) override {
            if (!lua_state_) {
                return ScriptValue{};
            }

            // Get the function
            lua_getglobal(lua_state_, name.c_str());
            if (!lua_isfunction(lua_state_, -1)) {
                lua_pop(lua_state_, 1);
                return ScriptValue{};
            }

            // Push arguments
            for (const auto &arg : args) {
                PushValue(arg);
            }

            // Call the function
            if (lua_pcall(lua_state_, static_cast<int>(args.size()), 1, 0) != LUA_OK) {
                lua_pop(lua_state_, 1);
                return ScriptValue{};
            }

            // Get the result
            return PopValue();
        }

        [[nodiscard]] ScriptLanguage GetLanguage() const override {
            return ScriptLanguage::Lua;
        }

        ~LuaBackend() override {
            Shutdown();
        }
    };

    // Factory helper function
    std::unique_ptr<IScriptingBackend> CreateLuaBackend() {
        return std::make_unique<LuaBackend>();
    }
}
