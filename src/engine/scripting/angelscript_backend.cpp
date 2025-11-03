module;

#include <any>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

#include <angelscript.h>

module engine.scripting;

namespace engine::scripting {
    class AngelScriptBackend : public IScriptingBackend {
    private:
        asIScriptEngine *engine_ = nullptr;
        asIScriptContext *context_ = nullptr;
        std::map<std::string, std::function<ScriptValue(const std::vector<ScriptValue>&)>> registered_functions_;

        // Helper to convert ScriptValue to AngelScript
        void PushValue(asIScriptContext *ctx, int arg_index, const ScriptValue &value) {
            if (value.Is<int>()) {
                ctx->SetArgDWord(arg_index, static_cast<asDWORD>(value.As<int>()));
            } else if (value.Is<float>()) {
                ctx->SetArgFloat(arg_index, value.As<float>());
            } else if (value.Is<double>()) {
                ctx->SetArgDouble(arg_index, value.As<double>());
            } else if (value.Is<bool>()) {
                ctx->SetArgByte(arg_index, value.As<bool>() ? 1 : 0);
            } else if (value.Is<std::string>()) {
                // String arguments are not supported; throw an error to prevent silent failure
                throw std::invalid_argument("AngelScriptBackend: String arguments are not supported. Please avoid passing std::string to scripts, or implement proper string registration.");
            }
        }

        // Helper to get return value from AngelScript
        ScriptValue GetReturnValue(asIScriptContext *ctx, int type_id) {
            ScriptValue result;
            
            if (type_id == asTYPEID_INT32) {
                result.value = static_cast<int>(ctx->GetReturnDWord());
            } else if (type_id == asTYPEID_FLOAT) {
                result.value = ctx->GetReturnFloat();
            } else if (type_id == asTYPEID_DOUBLE) {
                result.value = ctx->GetReturnDouble();
            } else if (type_id == asTYPEID_BOOL) {
                result.value = ctx->GetReturnByte() != 0;
            }
            // String return handling would be more complex
            
            return result;
        }

        // Generic wrapper for registered C++ functions
        static void GenericFunctionWrapper(asIScriptGeneric *gen) {
            // Get the function from auxiliary pointer (stored during registration)
            auto *func_ptr = static_cast<std::function<ScriptValue(const std::vector<ScriptValue>&)>*>(
                gen->GetAuxiliary()
            );

            // Convert arguments
            std::vector<ScriptValue> args;
            const int arg_count = gen->GetArgCount();
            for (int i = 0; i < arg_count; ++i) {
                ScriptValue arg;
                const int type_id = gen->GetArgTypeId(i);
                
                if (type_id == asTYPEID_INT32) {
                    arg.value = static_cast<int>(gen->GetArgDWord(i));
                } else if (type_id == asTYPEID_FLOAT) {
                    arg.value = gen->GetArgFloat(i);
                } else if (type_id == asTYPEID_DOUBLE) {
                    arg.value = gen->GetArgDouble(i);
                } else if (type_id == asTYPEID_BOOL) {
                    arg.value = gen->GetArgByte(i) != 0;
                }
                
                args.push_back(arg);
            }

            // Call the C++ function
            ScriptValue result = (*func_ptr)(args);

            // Set return value
            const int return_type_id = gen->GetReturnTypeId();
            if (return_type_id == asTYPEID_INT32 && result.Is<int>()) {
                gen->SetReturnDWord(static_cast<asDWORD>(result.As<int>()));
            } else if (return_type_id == asTYPEID_FLOAT && result.Is<float>()) {
                gen->SetReturnFloat(result.As<float>());
            } else if (return_type_id == asTYPEID_DOUBLE && result.Is<double>()) {
                gen->SetReturnDouble(result.As<double>());
            } else if (return_type_id == asTYPEID_BOOL && result.Is<bool>()) {
                gen->SetReturnByte(result.As<bool>() ? 1 : 0);
            }
        }

        // Parse signature string to AngelScript declaration
        std::string ConvertSignatureToAS(const std::string &name, const std::string &signature) {
            // Simple signature format: "returntype(arg1,arg2,...)"
            // Convert to AngelScript: "returntype name(arg1, arg2, ...)"
            if (signature.empty()) {
                return "void " + name + "()";
            }

            size_t paren_pos = signature.find('(');
            if (paren_pos == std::string::npos) {
                return "void " + name + "()";
            }

            std::string return_type = signature.substr(0, paren_pos);
            std::string params = signature.substr(paren_pos);

            return return_type + " " + name + params;
        }

    public:
        bool Initialize() override {
            engine_ = asCreateScriptEngine();
            if (!engine_) {
                return false;
            }

            // Set message callback for errors (using C function for simplicity)
            // Note: Could implement proper error handling later

            // Create context for executing scripts
            context_ = engine_->CreateContext();
            if (!context_) {
                return false;
            }

            return true;
        }

        void Shutdown() override {
            if (context_) {
                context_->Release();
                context_ = nullptr;
            }
            if (engine_) {
                engine_->ShutDownAndRelease();
                engine_ = nullptr;
            }
            registered_functions_.clear();
        }

        bool ExecuteString(const std::string &script) override {
            if (!engine_ || !context_) {
                return false;
            }

            asIScriptModule *mod = engine_->GetModule("script", asGM_ALWAYS_CREATE);
            if (!mod) {
                return false;
            }

            // Add script section
            int r = mod->AddScriptSection("inline", script.c_str());
            if (r < 0) {
                return false;
            }

            // Build the module
            r = mod->Build();
            if (r < 0) {
                return false;
            }

            return true;
        }

        bool ExecuteFile(const std::string &filepath) override {
            // Not yet implemented: AngelScript file execution is not supported.
            (void)filepath;
            throw std::runtime_error("AngelScriptBackend::ExecuteFile is not yet supported.");
        }

        void RegisterGlobalFunction(
            const std::string &name,
            std::function<ScriptValue(const std::vector<ScriptValue>&)> func,
            const std::string &signature
        ) override {
            if (!engine_) {
                return;
            }

            // Store the function
            registered_functions_[name] = func;
            auto *func_ptr = &registered_functions_[name];

            // Convert signature to AngelScript format
            std::string as_decl = ConvertSignatureToAS(name, signature);

            // Register as generic function
            int r = engine_->RegisterGlobalFunction(
                as_decl.c_str(),
                asFUNCTION(GenericFunctionWrapper),
                asCALL_GENERIC,
                func_ptr
            );

            if (r < 0) {
                // Registration failed
                registered_functions_.erase(name);
            }
        }

        ScriptValue CallFunction(
            const std::string &name,
            const std::vector<ScriptValue> &args
        ) override {
            if (!engine_ || !context_) {
                return ScriptValue{};
            }

            asIScriptModule *mod = engine_->GetModule("script");
            if (!mod) {
                return ScriptValue{};
            }

            asIScriptFunction *func = mod->GetFunctionByName(name.c_str());
            if (!func) {
                return ScriptValue{};
            }

            // Prepare context
            context_->Prepare(func);

            // Set arguments
            for (size_t i = 0; i < args.size(); ++i) {
                PushValue(context_, static_cast<int>(i), args[i]);
            }

            // Execute
            int r = context_->Execute();
            if (r != asEXECUTION_FINISHED) {
                return ScriptValue{};
            }

            // Get return value
            const int return_type_id = func->GetReturnTypeId();
            return GetReturnValue(context_, return_type_id);
        }

        [[nodiscard]] ScriptLanguage GetLanguage() const override {
            return ScriptLanguage::AngelScript;
        }

        ~AngelScriptBackend() override {
            Shutdown();
        }
    };

    // Factory helper function
    std::unique_ptr<IScriptingBackend> CreateAngelScriptBackend() {
        return std::make_unique<AngelScriptBackend>();
    }
}

