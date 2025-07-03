# MOD_SCRIPTING_v1

> **Multi-Language Scripting System - Engine Module**

## Executive Summary

The `engine.scripting` module provides a comprehensive multi-language scripting system supporting Lua, Python, and
AngelScript for gameplay logic, configuration, and rapid prototyping with hot-reload capabilities, seamless C++
integration, performance optimization, and secure sandboxing. This module enables dynamic game logic modification,
user-generated content support, and rapid iteration for the Colony Game Engine's complex simulation systems requiring
flexible scripting capabilities for AI behaviors, game rules, and content creation across desktop and WebAssembly
platforms.

## Scope and Objectives

### In Scope

- [ ] Multi-language scripting support (Lua, Python, AngelScript)
- [ ] Hot-reload scripting with automatic dependency tracking
- [ ] Type-safe C++ to script bindings with automatic generation
- [ ] Script sandboxing and security for user-generated content
- [ ] Performance monitoring and script profiling
- [ ] Integrated debugging and error reporting

### Out of Scope

- [ ] Visual scripting editors and node-based systems
- [ ] Complex script networking and synchronization
- [ ] Script compilation to native code
- [ ] Advanced IDE integration beyond basic debugging

### Primary Objectives

1. **Performance**: Execute 10,000+ script function calls per frame with minimal overhead
2. **Safety**: Secure script execution with sandboxing preventing system access
3. **Integration**: Seamless C++ binding with type safety and automatic marshalling

### Secondary Objectives

- Hot-reload scripts within 100ms of file changes
- Memory usage under 32MB for typical script loads
- Cross-platform script compatibility (desktop and WebAssembly)

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages script component attachments to entities for behavior
- **Entity Queries**: Queries entities with ScriptBehavior, ScriptComponent, or ScriptController components
- **Component Dependencies**: Provides scripting interfaces for all engine systems

#### Component Design

```cpp
// Script-related components for ECS integration
struct ScriptBehavior {
    AssetId script_asset{0};
    ScriptLanguage language{ScriptLanguage::Lua};
    std::string entry_function{"update"};
    std::unordered_map<std::string, ScriptValue> variables;
    ScriptContextId context_id{0};
    bool is_enabled{true};
    float execution_time_budget{2.0f}; // Max ms per frame
};

struct ScriptComponent {
    std::string component_name;
    std::unordered_map<std::string, ScriptValue> properties;
    std::vector<std::string> event_handlers;
    bool auto_serialize{true};
};

struct ScriptController {
    std::vector<AssetId> script_modules;
    std::string controller_name;
    ScriptPriority priority{ScriptPriority::Normal};
    std::unordered_set<std::string> exposed_systems;
    bool is_active{true};
};

// Component traits for scripting
template<>
struct ComponentTraits<ScriptBehavior> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};

template<>
struct ComponentTraits<ScriptComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 20000;
};
```

#### System Integration

- **System Dependencies**: Runs after all other systems to process script-driven behaviors
- **Component Access Patterns**: Read-write access to ScriptBehavior; provides interfaces to all systems
- **Inter-System Communication**: Exposes all engine systems through type-safe script bindings

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Script batch - executes on main thread with cooperative multitasking
- **Thread Safety**: Script execution is single-threaded; bindings use thread-safe interfaces
- **Data Dependencies**: Accesses all engine systems through thread-safe script interfaces

#### Parallel Execution Strategy

```cpp
// Multi-threaded script processing with time-sliced execution
class ScriptSystem : public ISystem {
public:
    // Main thread: Execute scripts with time budgets
    void ExecuteScripts(const ComponentManager& components,
                       std::span<EntityId> entities,
                       const ThreadContext& context) override;
    
    // Background thread: Hot-reload and compilation
    void ProcessScriptReloads(const ThreadContext& context);
    
    // Main thread: Script event dispatch and callbacks
    void DispatchScriptEvents(const EventQueue& events,
                             const ThreadContext& context);

private:
    struct ScriptTask {
        Entity entity;
        ScriptFunction function;
        std::chrono::nanoseconds time_budget;
        ScriptPriority priority;
    };
    
    // Time-sliced script execution queue
    std::priority_queue<ScriptTask> execution_queue_;
    std::atomic<bool> reload_pending_{false};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Script bytecode and data stored contiguously for optimal execution
- **Memory Ordering**: Script state updates use relaxed ordering for performance
- **Lock-Free Sections**: Script execution and event dispatch are single-threaded

### Public APIs

#### Primary Interface: `ScriptEngineInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <functional>

namespace engine::scripting {

template<typename T>
concept ScriptBindable = requires(T t) {
    typename T::ScriptBindings;
    { T::GetScriptTypeName() } -> std::convertible_to<std::string_view>;
};

class ScriptEngineInterface {
public:
    [[nodiscard]] auto Initialize(const ScriptConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Script loading and compilation
    [[nodiscard]] auto LoadScript(const AssetPath& script_path, 
                                 ScriptLanguage language) -> std::optional<ScriptId>;
    [[nodiscard]] auto CompileScript(std::string_view source_code, 
                                    ScriptLanguage language) -> std::optional<ScriptId>;
    void UnloadScript(ScriptId script_id) noexcept;
    
    // Script execution
    [[nodiscard]] auto CallFunction(ScriptId script_id, std::string_view function_name,
                                   std::span<const ScriptValue> args) -> std::optional<ScriptValue>;
    [[nodiscard]] auto CreateContext(ScriptId script_id) -> std::optional<ScriptContextId>;
    void DestroyContext(ScriptContextId context_id) noexcept;
    
    // Variable management
    void SetGlobalVariable(std::string_view name, const ScriptValue& value);
    [[nodiscard]] auto GetGlobalVariable(std::string_view name) const -> std::optional<ScriptValue>;
    void SetContextVariable(ScriptContextId context_id, std::string_view name, const ScriptValue& value);
    
    // Type binding system
    template<ScriptBindable T>
    void RegisterType();
    
    template<typename Func>
    void RegisterFunction(std::string_view name, Func&& function);
    
    void RegisterNamespace(std::string_view name);
    
    // Hot-reload and debugging
    void EnableHotReload(bool enable) noexcept;
    [[nodiscard]] auto RegisterReloadCallback(ScriptId script_id,
                                             std::function<void(ScriptId)> callback) -> CallbackId;
    void SetBreakpoint(ScriptId script_id, std::uint32_t line_number);
    
    // Performance monitoring
    [[nodiscard]] auto GetScriptStats() const noexcept -> ScriptStatistics;
    void SetExecutionTimeLimit(std::chrono::nanoseconds limit) noexcept;
    
    // Scripting interface exposure (self-referential for script access to scripting)
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<ScriptEngineImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::scripting
```

#### Scripting Interface Requirements

```cpp
// Script-accessible interface for engine interaction
class EngineScriptInterface {
public:
    // Entity management from scripts
    [[nodiscard]] auto CreateEntity() -> std::uint32_t;
    void DestroyEntity(std::uint32_t entity_id);
    [[nodiscard]] auto IsEntityValid(std::uint32_t entity_id) const -> bool;
    
    // Component access
    template<typename T>
    void AddComponent(std::uint32_t entity_id, const T& component);
    
    template<typename T>
    [[nodiscard]] auto GetComponent(std::uint32_t entity_id) -> std::optional<T>;
    
    template<typename T>
    void RemoveComponent(std::uint32_t entity_id);
    
    // System access
    [[nodiscard]] auto GetInputSystem() -> InputScriptInterface&;
    [[nodiscard]] auto GetAudioSystem() -> AudioScriptInterface&;
    [[nodiscard]] auto GetRenderingSystem() -> RenderingScriptInterface&;
    [[nodiscard]] auto GetPhysicsSystem() -> PhysicsScriptInterface&;
    
    // Logging and debugging
    void LogInfo(std::string_view message);
    void LogWarning(std::string_view message);
    void LogError(std::string_view message);
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<ScriptEngineInterface> script_engine_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Multi-Language Support**: Execute Lua, Python, and AngelScript with consistent APIs
- [ ] **Hot-Reload**: Automatically reload changed scripts without application restart
- [ ] **Type Safety**: Prevent type mismatches between C++ and script code with compile-time checks

### Performance Requirements

- [ ] **Execution Speed**: Execute 10,000+ script function calls per frame with <1ms overhead
- [ ] **Memory Usage**: Total script memory under 32MB for typical game scenarios
- [ ] **Reload Time**: Hot-reload scripts within 100ms of file modification
- [ ] **Startup Time**: Initialize script engine under 200ms on application start

### Quality Requirements

- [ ] **Security**: Sandbox script execution preventing file system and network access
- [ ] **Maintainability**: All script binding code covered by automated tests
- [ ] **Testability**: Mock script engines for deterministic testing
- [ ] **Documentation**: Complete scripting API reference with examples

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(ScriptingTest, ExecutionPerformance) {
    auto script_engine = engine::scripting::ScriptEngine{};
    script_engine.Initialize(ScriptConfig{});
    
    // Load performance test script
    auto script_id = script_engine.CompileScript(R"(
        function test_function(value)
            return value * 2
        end
    )", ScriptLanguage::Lua);
    ASSERT_TRUE(script_id.has_value());
    
    // Measure execution performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        ScriptValue args[] = {ScriptValue{static_cast<float>(i)}};
        auto result = script_engine.CallFunction(*script_id, "test_function", args);
        ASSERT_TRUE(result.has_value());
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(execution_time, 1); // Under 1ms for 10,000 calls
}

TEST(ScriptingTest, HotReloadSpeed) {
    auto script_engine = engine::scripting::ScriptEngine{};
    script_engine.Initialize(ScriptConfig{});
    script_engine.EnableHotReload(true);
    
    // Load initial script from file
    auto script_id = script_engine.LoadScript("test_script.lua", ScriptLanguage::Lua);
    ASSERT_TRUE(script_id.has_value());
    
    // Call initial function
    auto initial_result = script_engine.CallFunction(*script_id, "get_value", {});
    ASSERT_TRUE(initial_result.has_value());
    
    // Modify script file
    ModifyScriptFile("test_script.lua", "function get_value() return 42 end");
    
    // Measure reload time
    auto start = std::chrono::high_resolution_clock::now();
    WaitForScriptReload(*script_id);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto reload_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(reload_time, 100); // Under 100ms
    
    // Verify new behavior
    auto new_result = script_engine.CallFunction(*script_id, "get_value", {});
    ASSERT_TRUE(new_result.has_value());
    EXPECT_EQ(new_result->AsFloat(), 42.0f);
}

TEST(ScriptingTest, TypeSafety) {
    auto script_engine = engine::scripting::ScriptEngine{};
    script_engine.Initialize(ScriptConfig{});
    
    // Register type-safe bindings
    script_engine.RegisterType<Vec3>();
    script_engine.RegisterFunction("CreateVector", [](float x, float y, float z) -> Vec3 {
        return Vec3{x, y, z};
    });
    
    auto script_id = script_engine.CompileScript(R"(
        function test_vector()
            local vec = CreateVector(1.0, 2.0, 3.0)
            return vec
        end
    )", ScriptLanguage::Lua);
    ASSERT_TRUE(script_id.has_value());
    
    // Test type-safe return value
    auto result = script_engine.CallFunction(*script_id, "test_vector", {});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->Is<Vec3>());
    
    auto vec = result->As<Vec3>();
    EXPECT_EQ(vec.x, 1.0f);
    EXPECT_EQ(vec.y, 2.0f);
    EXPECT_EQ(vec.z, 3.0f);
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Scripting Engine (Estimated: 8 days)

- [ ] **Task 1.1**: Implement base script engine architecture and interfaces
- [ ] **Task 1.2**: Add Lua integration with basic C++ bindings
- [ ] **Task 1.3**: Create script value marshalling and type system
- [ ] **Deliverable**: Basic Lua scripting with simple function calls

#### Phase 2: Multi-Language Support (Estimated: 6 days)

- [ ] **Task 2.1**: Add Python integration with embedded interpreter
- [ ] **Task 2.2**: Implement AngelScript integration
- [ ] **Task 2.3**: Create unified scripting interface across languages
- [ ] **Deliverable**: Multi-language scripting with consistent APIs

#### Phase 3: ECS Integration (Estimated: 5 days)

- [ ] **Task 3.1**: Create script component system and entity bindings
- [ ] **Task 3.2**: Implement automatic C++ to script binding generation
- [ ] **Task 3.3**: Add script behavior execution and event handling
- [ ] **Deliverable**: Full ECS integration with script-driven entities

#### Phase 4: Advanced Features (Estimated: 4 days)

- [ ] **Task 4.1**: Implement hot-reload with dependency tracking
- [ ] **Task 4.2**: Add script debugging and performance profiling
- [ ] **Task 4.3**: Create sandboxing and security features
- [ ] **Deliverable**: Production-ready scripting with debugging and security

### File Structure

```
src/engine/scripting/
├── scripting.cppm              // Primary module interface
├── script_engine.cpp          // Core script engine management
├── script_value.cpp           // Value marshalling and type conversion
├── script_bindings.cpp        // C++ to script binding generation
├── script_context.cpp         // Script execution contexts and isolation
├── hot_reload.cpp             // Script hot-reload and dependency tracking
├── languages/
│   ├── lua_engine.cpp         // Lua integration
│   ├── python_engine.cpp      // Python integration
│   └── angelscript_engine.cpp // AngelScript integration
├── bindings/
│   ├── ecs_bindings.cpp       // ECS system bindings
│   ├── engine_bindings.cpp    // Engine system bindings
│   └── math_bindings.cpp      // Math type bindings
└── tests/
    ├── script_engine_tests.cpp
    ├── binding_tests.cpp
    └── scripting_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::scripting`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with language-specific implementations
- **Build Integration**: Links with Lua, Python, and AngelScript libraries

### Testing Strategy

- **Unit Tests**: Isolated testing of script engines with mock C++ bindings
- **Integration Tests**: Full scripting integration with ECS and engine systems
- **Performance Tests**: Script execution speed and memory usage benchmarks
- **Security Tests**: Sandbox validation and privilege escalation prevention

## Risk Assessment

### Technical Risks

| Risk                         | Probability | Impact | Mitigation                                                 |
|------------------------------|-------------|--------|------------------------------------------------------------|
| **Performance Overhead**     | Medium      | High   | Efficient binding generation, script compilation caching   |
| **Memory Leaks**             | Medium      | Medium | RAII patterns, automatic garbage collection integration    |
| **Security Vulnerabilities** | Low         | High   | Strict sandboxing, capability-based security model         |
| **Cross-Platform Issues**    | Medium      | Medium | Consistent script engine builds, WebAssembly compatibility |

### Integration Risks

- **ECS Performance**: Risk that script component updates impact frame rate
    - *Mitigation*: Time-sliced execution, performance budgets, efficient bindings
- **Hot-Reload Stability**: Risk of crashes during script reloading
    - *Mitigation*: Safe reload mechanisms, dependency validation, rollback support

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (file I/O, memory management)
    - engine.ecs (component access, entity management)
    - engine.assets (script file loading and hot-reload)
    - engine.threading (background compilation, time-slicing)

- **Optional Systems**:
    - engine.profiling (script performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 concepts, variant, optional, functional
- **Scripting Languages**: Lua 5.4, Python 3.x, AngelScript 2.x

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.assets, engine.threading
- **vcpkg Packages**: lua, python3, angelscript
- **Platform-Specific**: WebAssembly-compatible builds for browser deployment
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.assets, engine.threading

### Asset Pipeline Dependencies

- **Script Files**: Lua, Python, and AngelScript source files
- **Configuration Files**: Script binding configuration in JSON format
- **Resource Loading**: Script assets loaded through engine.assets with hot-reload
