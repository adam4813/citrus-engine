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

export import :backend;

export namespace engine::scripting {

    // Helper to convert C++ types to ScriptValue
    template<typename T>
    ScriptValue ToScriptValue(const T &value) {
        ScriptValue result;
        result.value = value;
        return result;
    }

    // Helper to build signature string from C++ types (e.g., "int(int,int)")
    template<typename T>
    constexpr const char* TypeToString() {
        if constexpr (std::is_void_v<T>) return "void";
        else if constexpr (std::is_same_v<T, int>) return "int";
        else if constexpr (std::is_same_v<T, float>) return "float";
        else if constexpr (std::is_same_v<T, double>) return "double";
        else if constexpr (std::is_same_v<T, bool>) return "bool";
        else if constexpr (std::is_same_v<T, std::string>) return "string";
        else return "unknown";
    }

    template<typename ReturnType, typename... Args>
    std::string MakeSignatureString() {
        std::string sig = TypeToString<ReturnType>();
        sig += "(";
        if constexpr (sizeof...(Args) > 0) {
            ((sig += TypeToString<Args>(), sig += ","), ...);
            sig.pop_back(); // Remove trailing comma
        }
        sig += ")";
        return sig;
    }

    // Helper to convert script arguments to C++ types (shared between ScriptingSystem and ClassRegistration)
    namespace detail {
        // Forward declaration
        template<typename... Args, std::size_t... Indices>
        std::tuple<Args...> ConvertArgsImpl(
            const std::vector<ScriptValue> &script_args,
            std::index_sequence<Indices...>
        );

        template<typename... Args>
        std::tuple<Args...> ConvertArgs(const std::vector<ScriptValue> &script_args) {
            return ConvertArgsImpl<Args...>(script_args, std::index_sequence_for<Args...>{});
        }

        template<typename... Args, std::size_t... Indices>
        std::tuple<Args...> ConvertArgsImpl(
            const std::vector<ScriptValue> &script_args,
            std::index_sequence<Indices...>
        ) {
            return std::make_tuple(script_args[Indices].As<Args>()...);
        }
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
                    std::apply(func, detail::ConvertArgs<Args...>(args));
                    return ScriptValue{};
                } else {
                    ReturnType result = std::apply(func, detail::ConvertArgs<Args...>(args));
                    return ToScriptValue(result);
                }
            };

            // Register the wrapped function with string signature
            const std::string full_name = class_name_ + "_" + method_name;
            std::string signature = MakeSignatureString<ReturnType, Args...>();
            backend_->RegisterGlobalFunction(full_name, wrapper, signature);
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
            if (!backend_ || !backend_->Initialize()) {
                throw std::runtime_error("Failed to initialize scripting backend");
            }
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
                    std::apply(func, detail::ConvertArgs<Args...>(args));
                    return ScriptValue{};
                } else {
                    ReturnType result = std::apply(func, detail::ConvertArgs<Args...>(args));
                    return ToScriptValue(result);
                }
            };

            // Create string signature from template parameters (e.g., "int(int,int)")
            std::string signature = MakeSignatureString<ReturnType, Args...>();
            backend_->RegisterGlobalFunction(name, wrapper, signature);
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

        // Change the scripting language at runtime
        // Note: If the new language is the same as the current one, this is a no-op and the backend is not reinitialized.
        // If the language is changed, this will shutdown the current backend and reinitialize with the new one.
        // All registered functions and state will be lost when the language is changed.
        bool SetLanguage(ScriptLanguage new_language) {
            if (new_language == language_) {
                return true; // Already using this language; no reinitialization occurs
            }

            // Shutdown current backend
            if (backend_) {
                backend_->Shutdown();
            }

            // Create and initialize new backend
            try {
                backend_ = CreateScriptingBackend(new_language);
                if (!backend_ || !backend_->Initialize()) {
                    // Revert to previous language on failure
                    backend_ = CreateScriptingBackend(language_);
                    if (backend_) {
                        backend_->Initialize();
                    }
                    return false;
                }
                language_ = new_language;
                return true;
            } catch (...) {
                // Revert to previous language on exception
                backend_ = CreateScriptingBackend(language_);
                if (backend_) {
                    backend_->Initialize();
                }
                return false;
            }
        }

    };
}
