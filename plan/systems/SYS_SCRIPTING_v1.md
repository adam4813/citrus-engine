# SYS_SCRIPTING_v1

> **System-Level Design Document for Multi-Language Scripting Interface**

## Executive Summary

This document defines a comprehensive scripting interface system for the modern C++20 game engine, designed to provide
unified access to engine functionality across multiple scripting languages (Lua, Python, AngelScript). The system
emphasizes a clean C++ interface layer that scripting language modules can bind to in language-specific ways, enabling
customizable developer experiences while maintaining consistent underlying functionality. The design prioritizes
automatic binding generation, hot-reload capabilities, and seamless integration with the ECS architecture and threading
model.

## Scope and Objectives

### In Scope

- [ ] Unified C++ scripting interface abstracting engine functionality for external binding
- [ ] Multi-language support architecture enabling Lua, Python, and AngelScript modules
- [ ] Automatic binding generation system reducing manual binding maintenance overhead
- [ ] Hot-reload scripting with runtime code updates and error recovery
- [ ] ECS integration enabling script-driven entity manipulation and system logic
- [ ] Threading-safe script execution with proper synchronization and isolation
- [ ] Asset integration allowing scripts to reference and manipulate game assets
- [ ] Performance profiling and debugging tools for script execution analysis
- [ ] Cross-platform compatibility ensuring identical scripting behavior on all targets
- [ ] Development workflow optimization with IDE integration and debugging support

### Out of Scope

- [ ] Domain-specific scripting languages or custom language implementation
- [ ] Real-time script compilation requiring JIT infrastructure
- [ ] Networking-aware script distribution or remote script execution
- [ ] Script security sandbox beyond basic error isolation
- [ ] Advanced script optimization requiring custom virtual machines
- [ ] Integration with external script hosting services or cloud execution

### Primary Objectives

1. **Language Flexibility**: Support multiple scripting languages through unified interface architecture
2. **Development Velocity**: Enable rapid gameplay prototyping and iteration through scripting
3. **API Consistency**: Maintain identical functionality across all supported scripting languages
4. **Performance Balance**: Minimize script/native boundary overhead while maintaining flexibility
5. **Hot-Reload Workflow**: Support seamless script updates during development and runtime

### Secondary Objectives

- Comprehensive error handling with detailed debugging information across language boundaries
- Future extensibility for additional scripting languages and binding patterns
- Integration with existing development tools and workflow optimization
- Memory-efficient script execution suitable for WebAssembly deployment constraints
- Comprehensive documentation and examples for each supported scripting language

## Architecture/Design

### High-Level Overview

```
Multi-Language Scripting Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                   Scripting Language Layer                      │
│                                                                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │    Lua      │ │   Python    │ │ AngelScript │               │
│  │   Module    │ │   Module    │ │   Module    │               │
│  │             │ │             │ │             │               │
│  │ Language-   │ │ Language-   │ │ Language-   │               │
│  │ Specific    │ │ Specific    │ │ Specific    │               │
│  │ Bindings    │ │ Bindings    │ │ Bindings    │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                 Unified Scripting Interface                     │
├─────────────────────────────────────────────────────────────────┤
│  Entity API    │ Component    │ System API   │ Asset API       │
│                │ API          │              │                 │
│  Create        │ Add/Remove   │ Enable/      │ Load/Unload     │
│  Destroy       │ Get/Set      │ Disable      │ Reference       │
│  Query         │ Update       │ Schedule     │ Hot-Reload      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Binding Generation & Management                     │
├─────────────────────────────────────────────────────────────────┤
│  Reflection    │ Code         │ Type         │ Error           │
│  System        │ Generation   │ Mapping      │ Handling        │
│                │              │              │                 │
│  C++ Metadata  │ Auto-Binding │ Type Safety  │ Exception       │
│  Extraction    │ Generation   │ Validation   │ Translation     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Engine Foundation                            │
├─────────────────────────────────────────────────────────────────┤
│  ECS Core     │ Threading    │ Assets       │ Platform         │
│               │              │              │                  │
│  Components   │ Job System   │ Asset Mgr    │ File System      │
│  Systems      │ Thread Pool  │ Hot-Reload   │ Timing           │
│  World        │ Sync         │ Loading      │ Memory           │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Unified Scripting Interface

- **Purpose**: Provide consistent C++ API that all scripting language modules can bind to uniformly
- **Responsibilities**: Engine functionality exposure, type-safe parameter handling, error boundary management
- **Key Classes/Interfaces**: `ScriptInterface`, `ScriptEntityAPI`, `ScriptComponentAPI`, `ScriptSystemAPI`
- **Data Flow**: Scripts call API → Interface validates → Engine functions execute → Results returned to scripts

#### Component 2: Language Module System

- **Purpose**: Pluggable architecture enabling different scripting languages with language-specific optimizations
- **Responsibilities**: Language-specific binding generation, runtime management, debugging integration
- **Key Classes/Interfaces**: `IScriptLanguage`, `LuaModule`, `PythonModule`, `AngelScriptModule`
- **Data Flow**: Module registration → Binding generation → Runtime initialization → Script execution management

#### Component 3: Binding Generation System

- **Purpose**: Automatic generation of language bindings from C++ interface declarations
- **Responsibilities**: C++ reflection, binding code generation, type mapping, documentation generation
- **Key Classes/Interfaces**: `BindingGenerator`, `TypeReflector`, `CodeGenerator`, `BindingRegistry`
- **Data Flow**: C++ analysis → Metadata extraction → Binding generation → Language module integration

#### Component 4: Hot-Reload and Development Tools

- **Purpose**: Development workflow optimization with runtime script updates and debugging support
- **Responsibilities**: File watching, script reloading, error recovery, performance profiling, debug integration
- **Key Classes/Interfaces**: `ScriptHotReloader`, `ScriptDebugger`, `ScriptProfiler`, `ScriptErrorHandler`
- **Data Flow**: File changes → Hot-reload trigger → Script recompilation → Runtime update → Error handling

### Unified Scripting Interface Design

#### Core Interface Architecture

```cpp
namespace engine::scripting {

// Primary scripting interface - this is what language modules bind to
class ScriptInterface {
public:
    // Entity management interface
    [[nodiscard]] auto GetEntityAPI() -> ScriptEntityAPI&;
    [[nodiscard]] auto GetComponentAPI() -> ScriptComponentAPI&;
    [[nodiscard]] auto GetSystemAPI() -> ScriptSystemAPI&;
    [[nodiscard]] auto GetAssetAPI() -> ScriptAssetAPI&;
    [[nodiscard]] auto GetInputAPI() -> ScriptInputAPI&;
    
    // Global engine controls
    void SetTimeScale(float scale);
    [[nodiscard]] auto GetFrameTime() const -> float;
    [[nodiscard]] auto GetTotalTime() const -> double;
    
    // Debug and development
    void LogMessage(LogLevel level, std::string_view message);
    void DebugBreak();
    [[nodiscard]] auto GetPerformanceStats() const -> PerformanceStats;
    
    // Script lifecycle management
    void RegisterScriptCleanupCallback(std::function<void()> callback);
    void UnregisterAllCallbacks();
};

// Entity manipulation API designed for scripting
class ScriptEntityAPI {
public:
    // Entity lifecycle
    [[nodiscard]] auto CreateEntity() -> EntityId;
    void DestroyEntity(EntityId entity);
    [[nodiscard]] auto IsEntityValid(EntityId entity) const -> bool;
    
    // Entity queries with script-friendly interfaces
    [[nodiscard]] auto FindEntitiesWithComponent(std::string_view component_name) -> std::vector<EntityId>;
    [[nodiscard]] auto FindEntitiesInRadius(glm::vec3 center, float radius) -> std::vector<EntityId>;
    [[nodiscard]] auto FindEntitiesByTag(std::string_view tag) -> std::vector<EntityId>;
    
    // Bulk operations for performance
    void DestroyEntities(std::span<const EntityId> entities);
    [[nodiscard]] auto CreateEntitiesFromTemplate(std::string_view template_name, uint32_t count) -> std::vector<EntityId>;

private:
    World* world_;
    SceneManager* scene_manager_;
};

// Component manipulation with type-safe script interface
class ScriptComponentAPI {
public:
    // Generic component operations using string names for script flexibility
    void AddComponent(EntityId entity, std::string_view component_name, const ScriptValue& data);
    void RemoveComponent(EntityId entity, std::string_view component_name);
    [[nodiscard]] auto HasComponent(EntityId entity, std::string_view component_name) const -> bool;
    [[nodiscard]] auto GetComponent(EntityId entity, std::string_view component_name) -> std::optional<ScriptValue>;
    void SetComponent(EntityId entity, std::string_view component_name, const ScriptValue& data);
    
    // Specialized component APIs for common operations
    void SetPosition(EntityId entity, glm::vec3 position);
    [[nodiscard]] auto GetPosition(EntityId entity) const -> std::optional<glm::vec3>;
    void SetRotation(EntityId entity, glm::quat rotation);
    [[nodiscard]] auto GetRotation(EntityId entity) const -> std::optional<glm::quat>;
    
    // Batch operations for performance
    void SetPositions(std::span<const EntityId> entities, std::span<const glm::vec3> positions);
    [[nodiscard]] auto GetPositions(std::span<const EntityId> entities) -> std::vector<glm::vec3>;

private:
    ComponentManager* component_manager_;
    TypeRegistry* type_registry_;
};

// System control interface for script-driven system management
class ScriptSystemAPI {
public:
    // System lifecycle control
    void EnableSystem(std::string_view system_name);
    void DisableSystem(std::string_view system_name);
    [[nodiscard]] auto IsSystemEnabled(std::string_view system_name) const -> bool;
    
    // System scheduling and timing
    void SetSystemPriority(std::string_view system_name, int32_t priority);
    void SetSystemUpdateRate(std::string_view system_name, float updates_per_second);
    
    // Custom script system registration
    void RegisterScriptSystem(std::string_view system_name, std::function<void(float)> update_function);
    void UnregisterScriptSystem(std::string_view system_name);
    
    // Performance monitoring
    [[nodiscard]] auto GetSystemPerformanceData(std::string_view system_name) -> SystemPerformanceData;
    [[nodiscard]] auto GetAllSystemsPerformance() -> std::vector<SystemPerformanceData>;

private:
    SystemScheduler* system_scheduler_;
    std::unordered_map<std::string, std::function<void(float)>> script_systems_;
};

// Asset management interface optimized for script usage
class ScriptAssetAPI {
public:
    // Asset loading with script-friendly return types
    [[nodiscard]] auto LoadAsset(std::string_view path, std::string_view type_name) -> std::optional<AssetId>;
    void UnloadAsset(AssetId asset_id);
    [[nodiscard]] auto IsAssetLoaded(AssetId asset_id) const -> bool;
    
    // Asset queries and metadata
    [[nodiscard]] auto GetAssetPath(AssetId asset_id) const -> std::optional<std::string>;
    [[nodiscard]] auto GetAssetType(AssetId asset_id) const -> std::optional<std::string>;
    [[nodiscard]] auto FindAssetsByType(std::string_view type_name) -> std::vector<AssetId>;
    
    // Hot-reload integration
    void ReloadAsset(AssetId asset_id);
    void ReloadAllAssetsOfType(std::string_view type_name);
    void SetAssetHotReloadCallback(AssetId asset_id, std::function<void()> callback);

private:
    AssetManager* asset_manager_;
    std::unordered_map<AssetId, std::function<void()>> reload_callbacks_;
};

// Input system integration for script-driven input handling
class ScriptInputAPI {
public:
    // Action-based input queries
    [[nodiscard]] auto IsActionPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto IsActionJustPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto IsActionJustReleased(std::string_view action_name) const -> bool;
    [[nodiscard]] auto GetActionValue(std::string_view action_name) const -> float;
    
    // Input context management
    void PushInputContext(std::string_view context_name);
    void PopInputContext();
    [[nodiscard]] auto GetActiveInputContext() const -> std::string;
    
    // Custom input handling
    void RegisterInputCallback(std::string_view action_name, std::function<void(float)> callback);
    void UnregisterInputCallback(std::string_view action_name);

private:
    InputManager* input_manager_;
    std::unordered_map<std::string, std::function<void(float)>> input_callbacks_;
};

// Script-friendly value type for cross-language data exchange
class ScriptValue {
public:
    enum class Type { Null, Bool, Int, Float, String, Vector3, Quaternion, Table };
    
    ScriptValue() = default;
    ScriptValue(bool value);
    ScriptValue(int32_t value);
    ScriptValue(float value);
    ScriptValue(std::string value);
    ScriptValue(glm::vec3 value);
    ScriptValue(glm::quat value);
    ScriptValue(std::unordered_map<std::string, ScriptValue> table);
    
    [[nodiscard]] auto GetType() const -> Type;
    [[nodiscard]] auto AsBool() const -> bool;
    [[nodiscard]] auto AsInt() const -> int32_t;
    [[nodiscard]] auto AsFloat() const -> float;
    [[nodiscard]] auto AsString() const -> const std::string&;
    [[nodiscard]] auto AsVector3() const -> glm::vec3;
    [[nodiscard]] auto AsQuaternion() const -> glm::quat;
    [[nodiscard]] auto AsTable() const -> const std::unordered_map<std::string, ScriptValue>&;
    
    // Conversion utilities for language-specific types
    template<typename T>
    [[nodiscard]] auto As() const -> T;
    
    template<typename T>
    static auto FromNative(const T& value) -> ScriptValue;

private:
    Type type_ = Type::Null;
    std::variant<std::monostate, bool, int32_t, float, std::string, 
                glm::vec3, glm::quat, std::unordered_map<std::string, ScriptValue>> data_;
};

} // namespace engine::scripting
```

### Language Module Architecture

#### Pluggable Language Interface

```cpp
namespace engine::scripting {

// Abstract interface that all scripting language modules must implement
class IScriptLanguage {
public:
    virtual ~IScriptLanguage() = default;
    
    // Module lifecycle
    [[nodiscard]] virtual auto Initialize(const ScriptInterface& engine_interface) -> bool = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual auto GetLanguageName() const -> std::string_view = 0;
    [[nodiscard]] virtual auto GetLanguageVersion() const -> std::string_view = 0;
    
    // Script execution
    [[nodiscard]] virtual auto ExecuteScript(std::string_view script_content, std::string_view source_name = "") -> ScriptResult = 0;
    [[nodiscard]] virtual auto ExecuteScriptFile(const std::filesystem::path& script_path) -> ScriptResult = 0;
    [[nodiscard]] virtual auto CallFunction(std::string_view function_name, std::span<const ScriptValue> args) -> ScriptResult = 0;
    
    // Hot-reload support
    virtual void ReloadScript(std::string_view source_name) = 0;
    virtual void ReloadAllScripts() = 0;
    [[nodiscard]] virtual auto SupportsHotReload() const -> bool = 0;
    
    // Debugging and development
    virtual void SetDebugMode(bool enabled) = 0;
    [[nodiscard]] virtual auto GetErrorInfo() const -> std::optional<ScriptError> = 0;
    [[nodiscard]] virtual auto GetPerformanceStats() const -> ScriptPerformanceStats = 0;
    
    // Memory management
    virtual void CollectGarbage() = 0;
    [[nodiscard]] virtual auto GetMemoryUsage() const -> size_t = 0;
};

// Script execution result with comprehensive error information
struct ScriptResult {
    enum class Status { Success, CompilationError, RuntimeError, Timeout };
    
    Status status = Status::Success;
    ScriptValue return_value;
    std::optional<ScriptError> error;
    std::chrono::microseconds execution_time{0};
};

struct ScriptError {
    enum class Type { Syntax, Runtime, Timeout, OutOfMemory, Unknown };
    
    Type type;
    std::string message;
    std::string source_name;
    uint32_t line_number = 0;
    uint32_t column_number = 0;
    std::vector<std::string> stack_trace;
};

struct ScriptPerformanceStats {
    std::chrono::microseconds total_execution_time{0};
    std::chrono::microseconds average_execution_time{0};
    uint64_t total_executions = 0;
    uint64_t compilation_count = 0;
    uint64_t garbage_collections = 0;
    size_t peak_memory_usage = 0;
    size_t current_memory_usage = 0;
};

// Language module factory for dynamic loading
class ScriptLanguageFactory {
public:
    template<typename LanguageModule>
    static void RegisterLanguage() {
        auto creator = []() -> std::unique_ptr<IScriptLanguage> {
            return std::make_unique<LanguageModule>();
        };
        
        registered_languages_[LanguageModule::GetStaticLanguageName()] = creator;
    }
    
    [[nodiscard]] static auto CreateLanguage(std::string_view language_name) -> std::unique_ptr<IScriptLanguage> {
        auto it = registered_languages_.find(std::string(language_name));
        if (it != registered_languages_.end()) {
            return it->second();
        }
        return nullptr;
    }
    
    [[nodiscard]] static auto GetAvailableLanguages() -> std::vector<std::string> {
        std::vector<std::string> languages;
        for (const auto& [name, creator] : registered_languages_) {
            languages.push_back(name);
        }
        return languages;
    }

private:
    static inline std::unordered_map<std::string, std::function<std::unique_ptr<IScriptLanguage>()>> registered_languages_;
};

} // namespace engine::scripting
```

#### Example Language Module Implementation

```cpp
// Example Lua module implementation
class LuaScriptLanguage final : public IScriptLanguage {
public:
    [[nodiscard]] static auto GetStaticLanguageName() -> std::string_view { return "Lua"; }
    
    [[nodiscard]] auto Initialize(const ScriptInterface& engine_interface) -> bool override {
        lua_state_ = luaL_newstate();
        if (!lua_state_) {
            return false;
        }
        
        luaL_openlibs(lua_state_);
        
        // Generate and execute Lua bindings for the engine interface
        auto binding_generator = LuaBindingGenerator(engine_interface);
        auto binding_code = binding_generator.GenerateBindings();
        
        auto result = luaL_dostring(lua_state_, binding_code.c_str());
        if (result != LUA_OK) {
            last_error_ = ExtractLuaError();
            return false;
        }
        
        engine_interface_ = &engine_interface;
        return true;
    }
    
    void Shutdown() override {
        if (lua_state_) {
            lua_close(lua_state_);
            lua_state_ = nullptr;
        }
    }
    
    [[nodiscard]] auto GetLanguageName() const -> std::string_view override { return "Lua"; }
    [[nodiscard]] auto GetLanguageVersion() const -> std::string_view override { return LUA_VERSION; }
    
    [[nodiscard]] auto ExecuteScript(std::string_view script_content, std::string_view source_name) -> ScriptResult override {
        if (!lua_state_) {
            return ScriptResult{.status = ScriptResult::Status::RuntimeError};
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Load script
        auto load_result = luaL_loadbuffer(lua_state_, script_content.data(), script_content.size(), source_name.data());
        if (load_result != LUA_OK) {
            auto error = ExtractLuaError();
            return ScriptResult{
                .status = ScriptResult::Status::CompilationError,
                .error = error
            };
        }
        
        // Execute script
        auto exec_result = lua_pcall(lua_state_, 0, LUA_MULTRET, 0);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        ScriptResult result;
        result.execution_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (exec_result != LUA_OK) {
            result.status = ScriptResult::Status::RuntimeError;
            result.error = ExtractLuaError();
        } else {
            result.status = ScriptResult::Status::Success;
            if (lua_gettop(lua_state_) > 0) {
                result.return_value = LuaToScriptValue(lua_state_, -1);
                lua_pop(lua_state_, 1);
            }
        }
        
        return result;
    }
    
    [[nodiscard]] auto ExecuteScriptFile(const std::filesystem::path& script_path) -> ScriptResult override {
        auto file_content = platform::FileSystem::ReadFile(script_path.string());
        if (!file_content) {
            return ScriptResult{
                .status = ScriptResult::Status::CompilationError,
                .error = ScriptError{
                    .type = ScriptError::Type::Syntax,
                    .message = "Failed to read script file",
                    .source_name = script_path.string()
                }
            };
        }
        
        std::string content(file_content->begin(), file_content->end());
        return ExecuteScript(content, script_path.string());
    }
    
    [[nodiscard]] auto CallFunction(std::string_view function_name, std::span<const ScriptValue> args) -> ScriptResult override {
        if (!lua_state_) {
            return ScriptResult{.status = ScriptResult::Status::RuntimeError};
        }
        
        // Get function from global table
        lua_getglobal(lua_state_, function_name.data());
        if (!lua_isfunction(lua_state_, -1)) {
            lua_pop(lua_state_, 1);
            return ScriptResult{
                .status = ScriptResult::Status::RuntimeError,
                .error = ScriptError{
                    .type = ScriptError::Type::Runtime,
                    .message = std::format("Function '{}' not found", function_name)
                }
            };
        }
        
        // Push arguments
        for (const auto& arg : args) {
            ScriptValueToLua(lua_state_, arg);
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Call function
        auto call_result = lua_pcall(lua_state_, static_cast<int>(args.size()), 1, 0);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        
        ScriptResult result;
        result.execution_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (call_result != LUA_OK) {
            result.status = ScriptResult::Status::RuntimeError;
            result.error = ExtractLuaError();
        } else {
            result.status = ScriptResult::Status::Success;
            if (lua_gettop(lua_state_) > 0) {
                result.return_value = LuaToScriptValue(lua_state_, -1);
                lua_pop(lua_state_, 1);
            }
        }
        
        return result;
    }
    
    void ReloadScript(std::string_view source_name) override {
        // Implementation depends on how scripts are tracked and stored
        auto it = loaded_scripts_.find(std::string(source_name));
        if (it != loaded_scripts_.end()) {
            ExecuteScriptFile(it->second);
        }
    }
    
    void ReloadAllScripts() override {
        for (const auto& [name, path] : loaded_scripts_) {
            ExecuteScriptFile(path);
        }
    }
    
    [[nodiscard]] auto SupportsHotReload() const -> bool override { return true; }
    
    void SetDebugMode(bool enabled) override {
        debug_enabled_ = enabled;
        // Enable Lua debug hooks if supported
    }
    
    [[nodiscard]] auto GetErrorInfo() const -> std::optional<ScriptError> override {
        return last_error_;
    }
    
    [[nodiscard]] auto GetPerformanceStats() const -> ScriptPerformanceStats override {
        return performance_stats_;
    }
    
    void CollectGarbage() override {
        if (lua_state_) {
            lua_gc(lua_state_, LUA_GCCOLLECT, 0);
            performance_stats_.garbage_collections++;
        }
    }
    
    [[nodiscard]] auto GetMemoryUsage() const -> size_t override {
        if (lua_state_) {
            return lua_gc(lua_state_, LUA_GCCOUNT, 0) * 1024; // Convert KB to bytes
        }
        return 0;
    }

private:
    lua_State* lua_state_ = nullptr;
    const ScriptInterface* engine_interface_ = nullptr;
    bool debug_enabled_ = false;
    std::optional<ScriptError> last_error_;
    ScriptPerformanceStats performance_stats_;
    std::unordered_map<std::string, std::filesystem::path> loaded_scripts_;
    
    [[nodiscard]] auto ExtractLuaError() -> ScriptError;
    [[nodiscard]] auto LuaToScriptValue(lua_State* L, int index) -> ScriptValue;
    void ScriptValueToLua(lua_State* L, const ScriptValue& value);
};
```

### Binding Generation System

#### Automatic Binding Generation

```cpp
namespace engine::scripting {

// Automatic binding generator using C++ reflection
class BindingGenerator {
public:
    explicit BindingGenerator(const ScriptInterface& interface) : interface_(interface) {}
    
    // Generate bindings for a specific language
    [[nodiscard]] auto GenerateBindingsForLanguage(std::string_view language) -> std::string {
        if (language == "Lua") {
            return GenerateLuaBindings();
        } else if (language == "Python") {
            return GeneratePythonBindings();
        } else if (language == "AngelScript") {
            return GenerateAngelScriptBindings();
        }
        return "";
    }
    
    // Register additional types for binding generation
    template<typename T>
    void RegisterType() {
        type_registry_.RegisterType<T>();
    }
    
    // Register additional functions for binding generation
    template<typename Func>
    void RegisterFunction(std::string_view name, Func&& function) {
        function_registry_.RegisterFunction(name, std::forward<Func>(function));
    }

private:
    const ScriptInterface& interface_;
    TypeRegistry type_registry_;
    FunctionRegistry function_registry_;
    
    [[nodiscard]] auto GenerateLuaBindings() -> std::string;
    [[nodiscard]] auto GeneratePythonBindings() -> std::string;
    [[nodiscard]] auto GenerateAngelScriptBindings() -> std::string;
};

// Type reflection system for automatic binding generation
class TypeRegistry {
public:
    template<typename T>
    void RegisterType() {
        auto type_info = std::make_unique<TypeInfo>();
        type_info->name = typeid(T).name();
        type_info->size = sizeof(T);
        type_info->alignment = alignof(T);
        
        // Extract member information using reflection
        ExtractMembers<T>(*type_info);
        
        registered_types_[type_info->name] = std::move(type_info);
    }
    
    [[nodiscard]] auto GetTypeInfo(std::string_view type_name) const -> const TypeInfo* {
        auto it = registered_types_.find(std::string(type_name));
        return it != registered_types_.end() ? it->second.get() : nullptr;
    }
    
    [[nodiscard]] auto GetAllTypes() const -> std::vector<const TypeInfo*> {
        std::vector<const TypeInfo*> types;
        for (const auto& [name, info] : registered_types_) {
            types.push_back(info.get());
        }
        return types;
    }

private:
    struct MemberInfo {
        std::string name;
        std::string type_name;
        size_t offset;
        bool is_readable;
        bool is_writable;
    };
    
    struct TypeInfo {
        std::string name;
        size_t size;
        size_t alignment;
        std::vector<MemberInfo> members;
        std::vector<std::string> methods;
    };
    
    std::unordered_map<std::string, std::unique_ptr<TypeInfo>> registered_types_;
    
    template<typename T>
    void ExtractMembers(TypeInfo& type_info);
};

} // namespace engine::scripting
```

### Threading Integration

#### Thread-Safe Script Execution

```cpp
namespace engine::scripting {

// Thread-safe script manager for multi-threaded execution
class ThreadSafeScriptManager {
public:
    explicit ThreadSafeScriptManager(std::unique_ptr<IScriptLanguage> language)
        : language_(std::move(language)) {}
    
    // Execute script on any thread safely
    [[nodiscard]] auto ExecuteScript(std::string_view script_content, std::string_view source_name = "") -> ScriptResult {
        std::unique_lock lock(execution_mutex_);
        return language_->ExecuteScript(script_content, source_name);
    }
    
    // Execute script asynchronously with future return
    [[nodiscard]] auto ExecuteScriptAsync(std::string_view script_content, std::string_view source_name = "") 
        -> std::future<ScriptResult> {
        
        auto promise = std::make_shared<std::promise<ScriptResult>>();
        auto future = promise->get_future();
        
        auto script_job = [this, content = std::string(script_content), name = std::string(source_name), promise]() {
            auto result = ExecuteScript(content, name);
            promise->set_value(result);
        };
        
        job_system_->Submit(std::move(script_job), "ScriptExecution");
        return future;
    }
    
    // Call function with thread safety
    [[nodiscard]] auto CallFunction(std::string_view function_name, std::span<const ScriptValue> args) -> ScriptResult {
        std::unique_lock lock(execution_mutex_);
        return language_->CallFunction(function_name, args);
    }
    
    // Thread-safe hot-reload
    void ReloadScript(std::string_view source_name) {
        std::unique_lock lock(execution_mutex_);
        language_->ReloadScript(source_name);
    }

private:
    std::unique_ptr<IScriptLanguage> language_;
    std::mutex execution_mutex_;
    JobSystem* job_system_ = nullptr; // Injected dependency
};

// Script system integration with ECS threading
class ScriptSystem : public ISystem {
public:
    explicit ScriptSystem(std::unique_ptr<ThreadSafeScriptManager> script_manager)
        : script_manager_(std::move(script_manager)) {}
    
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                ComponentTypeId::GetId<ScriptComponent>(),
                ComponentTypeId::GetId<TransformComponent>()
            },
            .write_components = {
                ComponentTypeId::GetId<ScriptComponent>()
            },
            .execution_phase = ExecutionPhase::Update,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Process all entities with script components
        world.ForEachComponent<ScriptComponent>([this, delta_time](EntityId entity, ScriptComponent& script) {
            if (!script.enabled || script.script_file.empty()) {
                return;
            }
            
            // Prepare script context
            auto script_context = PrepareScriptContext(entity, delta_time);
            
            // Execute entity script update function
            auto args = std::array{
                ScriptValue(static_cast<int32_t>(entity)),
                ScriptValue(delta_time)
            };
            
            auto result = script_manager_->CallFunction(script.update_function_name, args);
            
            // Handle script execution results
            if (result.status != ScriptResult::Status::Success && result.error) {
                LogScriptError(entity, *result.error);
            }
        });
    }

private:
    std::unique_ptr<ThreadSafeScriptManager> script_manager_;
    
    [[nodiscard]] auto PrepareScriptContext(EntityId entity, float delta_time) -> ScriptContext;
    void LogScriptError(EntityId entity, const ScriptError& error);
};

// Component for entities that have associated scripts
struct ScriptComponent {
    std::string script_file;
    std::string update_function_name = "update";
    bool enabled = true;
    bool auto_reload = true;
    
    // Script-specific data storage
    std::unordered_map<std::string, ScriptValue> script_data;
    
    // Performance tracking
    std::chrono::microseconds total_execution_time{0};
    uint64_t execution_count = 0;
};

template<>
struct ComponentTraits<ScriptComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(std::string);
};

} // namespace engine::scripting
```

### Hot-Reload and Development Tools

#### Development Workflow Integration

```cpp
namespace engine::scripting {

// Hot-reload system for script development
class ScriptHotReloader {
public:
    explicit ScriptHotReloader(ThreadSafeScriptManager& script_manager)
        : script_manager_(script_manager) {}
    
    void Initialize(const std::filesystem::path& script_directory) {
        script_directory_ = script_directory;
        
        // Set up file watching using platform abstraction
        file_watcher_ = platform::FileSystem::CreateWatcher(
            script_directory.string(),
            [this](const std::string& changed_file) {
                OnScriptFileChanged(changed_file);
            }
        );
        
        is_initialized_ = true;
    }
    
    void Shutdown() {
        if (file_watcher_) {
            file_watcher_.reset();
        }
        is_initialized_ = false;
    }
    
    // Manual reload trigger for specific scripts
    void ReloadScript(const std::filesystem::path& script_path) {
        if (!is_initialized_) return;
        
        try {
            auto relative_path = std::filesystem::relative(script_path, script_directory_);
            script_manager_.ReloadScript(relative_path.string());
            
            LogMessage(LogLevel::Info, std::format("Successfully reloaded script: {}", relative_path.string()));
            
            // Notify reload callbacks
            auto it = reload_callbacks_.find(relative_path.string());
            if (it != reload_callbacks_.end()) {
                for (const auto& callback : it->second) {
                    callback();
                }
            }
        } catch (const std::exception& e) {
            LogMessage(LogLevel::Error, std::format("Failed to reload script {}: {}", script_path.string(), e.what()));
        }
    }
    
    // Register callback for script reload events
    void RegisterReloadCallback(const std::filesystem::path& script_path, std::function<void()> callback) {
        auto relative_path = std::filesystem::relative(script_path, script_directory_);
        reload_callbacks_[relative_path.string()].push_back(callback);
    }
    
    // Enable/disable hot-reload for performance
    void SetEnabled(bool enabled) {
        hot_reload_enabled_ = enabled;
    }

private:
    ThreadSafeScriptManager& script_manager_;
    std::filesystem::path script_directory_;
    std::unique_ptr<platform::FileWatcher> file_watcher_;
    bool is_initialized_ = false;
    bool hot_reload_enabled_ = true;
    
    std::unordered_map<std::string, std::vector<std::function<void()>>> reload_callbacks_;
    
    void OnScriptFileChanged(const std::string& changed_file) {
        if (!hot_reload_enabled_) return;
        
        // Add delay to handle multiple rapid file changes
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto script_path = script_directory_ / changed_file;
        ReloadScript(script_path);
    }
};

// Script debugging and profiling tools
class ScriptDebugger {
public:
    explicit ScriptDebugger(ThreadSafeScriptManager& script_manager)
        : script_manager_(script_manager) {}
    
    // Enable debugging for specific scripts
    void EnableDebugging(bool enabled) {
        debug_enabled_ = enabled;
    }
    
    // Set breakpoints for script debugging
    void SetBreakpoint(std::string_view script_name, uint32_t line_number) {
        breakpoints_[std::string(script_name)].insert(line_number);
    }
    
    void RemoveBreakpoint(std::string_view script_name, uint32_t line_number) {
        auto it = breakpoints_.find(std::string(script_name));
        if (it != breakpoints_.end()) {
            it->second.erase(line_number);
        }
    }
    
    // Performance profiling
    [[nodiscard]] auto GetProfilingData() const -> ScriptProfilingData {
        ScriptProfilingData data;
        data.total_scripts_executed = total_executions_;
        data.total_execution_time = total_execution_time_;
        data.average_execution_time = total_executions_ > 0 ? 
            total_execution_time_ / total_executions_ : std::chrono::microseconds{0};
        data.peak_memory_usage = peak_memory_usage_;
        data.current_memory_usage = script_manager_.GetMemoryUsage();
        
        return data;
    }
    
    void ResetProfilingData() {
        total_executions_ = 0;
        total_execution_time_ = std::chrono::microseconds{0};
        peak_memory_usage_ = 0;
    }
    
    // Error logging and analysis
    void LogScriptError(const ScriptError& error) {
        error_history_.push_back({
            .error = error,
            .timestamp = std::chrono::steady_clock::now()
        });
        
        // Keep only recent errors
        constexpr size_t max_error_history = 1000;
        if (error_history_.size() > max_error_history) {
            error_history_.erase(error_history_.begin());
        }
    }
    
    [[nodiscard]] auto GetRecentErrors(std::chrono::minutes time_window = std::chrono::minutes{10}) const -> std::vector<ScriptError> {
        auto cutoff_time = std::chrono::steady_clock::now() - time_window;
        std::vector<ScriptError> recent_errors;
        
        for (const auto& entry : error_history_) {
            if (entry.timestamp >= cutoff_time) {
                recent_errors.push_back(entry.error);
            }
        }
        
        return recent_errors;
    }

private:
    ThreadSafeScriptManager& script_manager_;
    bool debug_enabled_ = false;
    
    std::unordered_map<std::string, std::unordered_set<uint32_t>> breakpoints_;
    
    // Profiling data
    uint64_t total_executions_ = 0;
    std::chrono::microseconds total_execution_time_{0};
    size_t peak_memory_usage_ = 0;
    
    struct ErrorEntry {
        ScriptError error;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::vector<ErrorEntry> error_history_;
};

struct ScriptProfilingData {
    uint64_t total_scripts_executed = 0;
    std::chrono::microseconds total_execution_time{0};
    std::chrono::microseconds average_execution_time{0};
    size_t peak_memory_usage = 0;
    size_t current_memory_usage = 0;
};

} // namespace engine::scripting
```

## Integration Points

### ECS Integration

The scripting system integrates deeply with the ECS foundation:

- **ScriptComponent**: Entities can have associated scripts for custom behavior
- **ScriptSystem**: ECS system that executes scripts during the update phase
- **Component Access**: Scripts can manipulate entity components through the unified interface
- **System Integration**: Script systems can be registered and managed alongside native C++ systems

### Asset System Integration

Scripts are treated as first-class assets:

- **Script Loading**: Scripts loaded through the asset pipeline with dependency tracking
- **Hot-Reload**: Script changes detected and reloaded automatically during development
- **Asset References**: Scripts can reference and manipulate other engine assets
- **Streaming**: Large script libraries can be streamed and loaded on demand

### Threading Model Integration

The scripting system respects the engine's threading architecture:

- **Thread Safety**: Script execution properly synchronized for multi-threaded environments
- **Job System**: Scripts can be executed asynchronously using the engine's job system
- **Frame Coherency**: Script execution synchronized with ECS update phases
- **Performance**: Script execution batched and optimized for parallel processing

### Platform Abstraction Integration

Cross-platform script execution leverages platform services:

- **File Watching**: Script hot-reload uses platform file system monitoring
- **Memory Management**: Script memory tracking integrates with platform memory utilities
- **Error Handling**: Script errors logged through platform debugging infrastructure
- **Performance Timing**: Script profiling uses platform high-resolution timing

## Performance Requirements

### Target Specifications

- **Script Execution**: Individual script functions execute in <1ms for typical gameplay logic
- **Hot-Reload Time**: Script reloading completes within 100ms for immediate development feedback
- **Memory Overhead**: Scripting system adds <50MB overhead to engine memory usage
- **Cross-Language Parity**: <5% performance difference between equivalent scripts in different languages
- **Binding Generation**: Automatic binding generation completes in <10s for full engine interface

### Optimization Strategies

1. **Binding Optimization**: Pre-generate bindings at build time rather than runtime generation
2. **Script Caching**: Cache compiled scripts to avoid recompilation overhead
3. **Memory Pooling**: Use memory pools for script value allocation and exchange
4. **Batch Execution**: Execute multiple script functions in batches to amortize overhead
5. **JIT Integration**: Support JIT compilation where available (LuaJIT, PyPy) for performance

## Success Criteria

### Functional Requirements

- **Multi-Language Support**: Lua, Python, and AngelScript modules all functional with identical APIs
- **ECS Integration**: Scripts can create, modify, and query entities and components seamlessly
- **Hot-Reload Workflow**: Scripts reload automatically during development with <100ms latency
- **Error Handling**: Comprehensive error reporting with source location and stack traces
- **Performance Profiling**: Real-time performance monitoring and debugging tools available

### Quality Requirements

- **API Consistency**: Identical functionality available across all supported scripting languages
- **Thread Safety**: Zero race conditions during concurrent script execution testing
- **Memory Safety**: No memory leaks during script loading, execution, and hot-reload cycles
- **Cross-Platform**: 100% feature parity between native and WebAssembly script execution

### Development Workflow

- **IDE Integration**: Syntax highlighting and debugging support for all supported languages
- **Documentation**: Auto-generated API documentation for each scripting language
- **Example Coverage**: Comprehensive examples demonstrating ECS, asset, and input integration
- **Testing Framework**: Unit tests covering all scripting interfaces and language modules

This scripting interface system provides the foundation for flexible, multi-language game development while maintaining
performance and consistency across your engine architecture. The unified interface approach ensures that adding new
scripting languages requires minimal engine changes while providing language-specific optimizations and developer
experiences.
