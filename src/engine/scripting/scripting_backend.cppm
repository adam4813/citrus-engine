module;

#include <any>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

export module engine.scripting:backend;

export namespace engine::scripting {
    // Enum for supported scripting languages
    enum class ScriptLanguage {
        Lua,
        AngelScript,
        Python
    };

    // Variant type for script values
    struct ScriptValue {
        std::any value;
        
        template<typename T>
        T As() const {
            return std::any_cast<T>(value);
        }
        
        template<typename T>
        bool Is() const {
            return value.type() == typeid(T);
        }
    };

    // Interface for scripting backend implementations
    class IScriptingBackend {
    public:
        virtual ~IScriptingBackend() = default;

        // Initialize the scripting backend
        virtual bool Initialize() = 0;

        // Shutdown the scripting backend
        virtual void Shutdown() = 0;

        // Execute a script from a string
        virtual bool ExecuteString(const std::string &script) = 0;

        // Execute a script from a file
        virtual bool ExecuteFile(const std::string &filepath) = 0;

        // Register a global function (no class binding)
        // Signature is a simple string that all 3 languages can understand (e.g., "int(int,int)")
        virtual void RegisterGlobalFunction(
            const std::string &name,
            std::function<ScriptValue(const std::vector<ScriptValue>&)> func,
            const std::string &signature = ""
        ) = 0;

        // Call a script function and get the return value
        virtual ScriptValue CallFunction(
            const std::string &name,
            const std::vector<ScriptValue> &args = {}
        ) = 0;

        // Get the language type
        [[nodiscard]] virtual ScriptLanguage GetLanguage() const = 0;
    };

    // Factory function to create scripting backends
    [[nodiscard]] std::unique_ptr<IScriptingBackend> CreateScriptingBackend(ScriptLanguage language);
}
