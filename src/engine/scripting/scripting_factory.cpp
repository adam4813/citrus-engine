module;

#include <memory>
#include <stdexcept>

module engine.scripting;

namespace engine::scripting {
    // Forward declarations of factory helper functions
    std::unique_ptr<IScriptingBackend> CreateLuaBackend();
    std::unique_ptr<IScriptingBackend> CreateAngelScriptBackend();

    // Factory function implementation
    std::unique_ptr<IScriptingBackend> CreateScriptingBackend(ScriptLanguage language) {
        switch (language) {
            case ScriptLanguage::Lua:
                return CreateLuaBackend();
            case ScriptLanguage::AngelScript:
                return CreateAngelScriptBackend();
            case ScriptLanguage::Python:
                // TODO: Implement Python backend
                throw std::runtime_error("Python backend not yet implemented");
            default:
                throw std::runtime_error("Unknown scripting language");
        }
    }
}
