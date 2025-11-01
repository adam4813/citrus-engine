module;

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

export module engine.scripting;

import engine.scripting.backend;

export namespace engine::scripting {
    // Re-export commonly used types
    using ScriptValue;
    using ScriptLanguage;

    // Helper to convert C++ types to ScriptValue
    template<typename T>
    ScriptValue ToScriptValue(const T &value) {
        ScriptValue result;
        result.value = value;
        return result;
    }

    // Class registration helper (for fluent interface)
    class ClassRegistration {
    private:
        std::string class_name_;
        IScriptingBackend *backend_;

    public:
        ClassRegistration(std::string name, IScriptingBackend *backend)
            : class_name_(std::move(name)), backend_(backend) {}

        // Register a method with automatic type conversion
        template<typename ReturnType, typename... Args>
        ClassRegistration& Method(
            const std::string &method_name,
            std::function<ReturnType(Args...)> func
        ) {
            // Wrap the function to handle type conversions
            auto wrapper = [func](const std::vector<ScriptValue> &args) -> ScriptValue {
                if (args.size() != sizeof...(Args)) {
                    // Argument count mismatch - return default value
                    if constexpr (!std::is_void_v<ReturnType>) {
                        return ToScriptValue(ReturnType{});
                    } else {
                        return ScriptValue{};
                    }
                }

                // Call the function with converted arguments
                if constexpr (std::is_void_v<ReturnType>) {
                    std::apply(func, ConvertArgs<Args...>(args));
                    return ScriptValue{};
                } else {
                    ReturnType result = std::apply(func, ConvertArgs<Args...>(args));
                    return ToScriptValue(result);
                }
            };

            // Register the wrapped function
            const std::string full_name = class_name_ + "_" + method_name;
            backend_->RegisterGlobalFunction(full_name, wrapper);
            return *this;
        }

        // Register a static method (same as Method but clearer naming)
        template<typename ReturnType, typename... Args>
        ClassRegistration& StaticMethod(
            const std::string &method_name,
            std::function<ReturnType(Args...)> func
        ) {
            return Method(method_name, func);
        }

    private:
        // Helper to convert script arguments to C++ types
        template<typename... Args>
        static std::tuple<Args...> ConvertArgs(const std::vector<ScriptValue> &script_args) {
            return ConvertArgsImpl<Args...>(script_args, std::index_sequence_for<Args...>{});
        }

        template<typename... Args, std::size_t... Indices>
        static std::tuple<Args...> ConvertArgsImpl(
            const std::vector<ScriptValue> &script_args,
            std::index_sequence<Indices...>
        ) {
            return std::make_tuple(script_args[Indices].As<Args>()...);
        }
    };

    // Main scripting system
    class ScriptingSystem {
    private:
        std::unique_ptr<IScriptingBackend> backend_;
        ScriptLanguage language_;

    public:
        // Initialize with a specific language
        explicit ScriptingSystem(ScriptLanguage language = ScriptLanguage::Lua)
            : language_(language) {
            backend_ = CreateScriptingBackend(language);
            backend_->Initialize();
        }

        ~ScriptingSystem() {
            if (backend_) {
                backend_->Shutdown();
            }
        }

        // Disable copy
        ScriptingSystem(const ScriptingSystem&) = delete;
        ScriptingSystem& operator=(const ScriptingSystem&) = delete;

        // Enable move
        ScriptingSystem(ScriptingSystem&&) noexcept = default;
        ScriptingSystem& operator=(ScriptingSystem&&) noexcept = default;

        // Register a class (returns helper for fluent interface)
        ClassRegistration RegisterClass(const std::string &class_name) {
            return ClassRegistration(class_name, backend_.get());
        }

        // Register a global function with automatic type conversion
        template<typename ReturnType, typename... Args>
        void RegisterGlobalFunction(
            const std::string &name,
            std::function<ReturnType(Args...)> func
        ) {
            auto wrapper = [func](const std::vector<ScriptValue> &args) -> ScriptValue {
                if (args.size() != sizeof...(Args)) {
                    if constexpr (!std::is_void_v<ReturnType>) {
                        return ToScriptValue(ReturnType{});
                    } else {
                        return ScriptValue{};
                    }
                }

                if constexpr (std::is_void_v<ReturnType>) {
                    std::apply(func, ConvertArgs<Args...>(args));
                    return ScriptValue{};
                } else {
                    ReturnType result = std::apply(func, ConvertArgs<Args...>(args));
                    return ToScriptValue(result);
                }
            };

            backend_->RegisterGlobalFunction(name, wrapper);
        }

        // Convenience overload for function pointers
        template<typename ReturnType, typename... Args>
        void RegisterGlobalFunction(
            const std::string &name,
            ReturnType(*func)(Args...)
        ) {
            RegisterGlobalFunction(name, std::function<ReturnType(Args...)>(func));
        }

        // Execute a script string
        bool ExecuteString(const std::string &script) {
            return backend_->ExecuteString(script);
        }

        // Execute a script file
        bool ExecuteFile(const std::string &filepath) {
            return backend_->ExecuteFile(filepath);
        }

        // Call a script function
        template<typename ReturnType = void, typename... Args>
        ReturnType CallFunction(const std::string &name, Args&&... args) {
            std::vector<ScriptValue> script_args;
            (script_args.push_back(ToScriptValue(std::forward<Args>(args))), ...);
            
            ScriptValue result = backend_->CallFunction(name, script_args);
            
            if constexpr (!std::is_void_v<ReturnType>) {
                return result.As<ReturnType>();
            }
        }

        // Get the active language
        [[nodiscard]] ScriptLanguage GetLanguage() const {
            return language_;
        }

    private:
        template<typename... Args>
        static std::tuple<Args...> ConvertArgs(const std::vector<ScriptValue> &script_args) {
            return ConvertArgsImpl<Args...>(script_args, std::index_sequence_for<Args...>{});
        }

        template<typename... Args, std::size_t... Indices>
        static std::tuple<Args...> ConvertArgsImpl(
            const std::vector<ScriptValue> &script_args,
            std::index_sequence<Indices...>
        ) {
            return std::make_tuple(script_args[Indices].As<Args>()...);
        }
    };
}
