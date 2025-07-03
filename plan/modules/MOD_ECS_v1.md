# MOD_ECS_v1

> **Entity Component System Core - Foundation Module**

## Executive Summary

The `engine.ecs` module implements a high-performance Entity Component System that serves as the foundational
architecture for all game object management in the Colony Game Engine. This module provides sparse set storage,
efficient component pools, cache-friendly queries, and parallel system execution, enabling scalable game logic with
deterministic multi-threaded performance for colony simulation scenarios requiring thousands of entities.

## Scope and Objectives

### In Scope

- [ ] Entity creation, destruction, and lifecycle management
- [ ] Component registration, storage, and access patterns
- [ ] System scheduling and parallel execution framework
- [ ] Query system for efficient component iteration
- [ ] Event system for inter-system communication
- [ ] Memory-efficient sparse set storage architecture

### Out of Scope

- [ ] Specific game logic components (handled by game-layer modules)
- [ ] Rendering-specific components (handled by engine.rendering)
- [ ] Physics integration (handled by engine.physics)
- [ ] Serialization of specific component types

### Primary Objectives

1. **High-Performance Entity Management**: Support 10,000+ entities with sub-millisecond query performance
2. **Cache-Friendly Component Access**: Ensure component iteration achieves >90% cache hit rates
3. **Parallel System Execution**: Enable concurrent system execution with automatic dependency resolution

### Secondary Objectives

- Zero-allocation entity queries during runtime
- Hot-swappable component types for development workflow
- Memory usage under 10MB for 5,000 entities with 8 component types

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Core ECS entity lifecycle management with generation-based validation
- **Entity Queries**: Provides foundational query system for all other engine systems
- **Component Dependencies**: No dependencies; serves as foundation for all other component systems

#### Component Design

```cpp
// Core ECS component for entity identification
struct EntityInfo {
    Entity entity_id{0};
    std::uint32_t generation{0};
    bool is_active{true};
    std::string debug_name;
};

// Example component structure for reference
struct ExampleComponent {
    float value{0.0f};
    std::vector<int> data_array;
    bool is_active{true};
};

// Component traits for ECS integration
template<>
struct ComponentTraits<ExampleComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};
```

#### System Integration

- **System Dependencies**: ECS is the foundation - no dependencies on other systems
- **Component Access Patterns**: Provides read/write access patterns for all engine systems
- **Inter-System Communication**: Provides event system and resource management for inter-system communication

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Foundation - ECS coordinates all parallel system execution
- **Thread Safety**: All ECS operations are thread-safe with lock-free component access
- **Data Dependencies**: No dependencies; coordinates dependencies for all other systems

#### Parallel Execution Strategy

```cpp
// Multi-threaded ECS with automatic system dependency resolution
class ECSSystem : public ISystem {
public:
    // System scheduling with dependency analysis
    [[nodiscard]] auto ScheduleSystems(std::span<ISystem*> systems) -> ExecutionGraph;
    
    // Parallel entity query execution
    template<typename... Components>
    void ExecuteParallelQuery(Query<Components...>& query,
                             std::function<void(Entity, Components&...)> func,
                             const ThreadContext& context);
    
    // Thread-safe component access
    template<typename T>
    [[nodiscard]] auto GetComponent(Entity entity) const -> std::optional<T*>;

private:
    struct SystemDependency {
        ISystem* system;
        std::vector<ComponentType> read_components;
        std::vector<ComponentType> write_components;
    };
    
    // Lock-free component storage
    std::unordered_map<ComponentType, std::unique_ptr<IComponentStorage>> storage_;
    mutable std::shared_mutex storage_mutex_;
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Components stored in contiguous arrays for optimal iteration
- **Memory Ordering**: Component updates use acquire-release semantics for consistency
- **Lock-Free Sections**: Entity queries and component iteration are lock-free

### Public APIs

#### Primary Interface: `ECSInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>

namespace engine::ecs {

template<typename T>
concept Component = requires(T t) {
    typename T::ComponentType;
    requires std::is_trivially_destructible_v<T>;
};

class ECSInterface {
public:
    [[nodiscard]] auto Initialize(const ECSConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Entity management
    [[nodiscard]] auto CreateEntity() -> Entity;
    void DestroyEntity(Entity entity);
    [[nodiscard]] auto IsEntityValid(Entity entity) const -> bool;
    
    // Component management
    template<Component T>
    void RegisterComponent();
    
    template<Component T>
    T& AddComponent(Entity entity, T&& component);
    
    template<Component T>
    void RemoveComponent(Entity entity);
    
    template<Component T>
    [[nodiscard]] auto GetComponent(Entity entity) -> std::optional<T*>;
    
    template<Component T>
    [[nodiscard]] auto HasComponent(Entity entity) const -> bool;
    
    // Query system
    template<Component... Components>
    [[nodiscard]] auto CreateQuery() -> Query<Components...>;
    
    // System management
    void RegisterSystem(std::unique_ptr<ISystem> system);
    void UpdateSystems(float delta_time);
    
    // Event system
    template<typename T>
    void SendEvent(const T& event);
    
    template<typename T>
    [[nodiscard]] auto RegisterEventHandler(std::function<void(const T&)> handler) -> CallbackId;
    
    // Performance monitoring
    [[nodiscard]] auto GetECSStats() const -> ECSStatistics;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<ECSImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::ecs
```

#### Scripting Interface Requirements

```cpp
// ECS scripting interface for dynamic entity management
class ECSScriptInterface {
public:
    // Type-safe entity management from scripts
    [[nodiscard]] auto CreateEntity() -> std::uint32_t;
    void DestroyEntity(std::uint32_t entity_id);
    [[nodiscard]] auto IsEntityValid(std::uint32_t entity_id) const -> bool;
    
    // Component access (type-erased for scripting)
    void AddComponent(std::uint32_t entity_id, std::string_view component_type, const std::string& data);
    void RemoveComponent(std::uint32_t entity_id, std::string_view component_type);
    [[nodiscard]] auto HasComponent(std::uint32_t entity_id, std::string_view component_type) const -> bool;
    
    // Entity queries
    [[nodiscard]] auto FindEntitiesWithComponent(std::string_view component_type) -> std::vector<std::uint32_t>;
    [[nodiscard]] auto GetEntityCount() const -> std::uint32_t;
    
    // Event system
    void SendEvent(std::string_view event_type, const std::string& data);
    [[nodiscard]] auto RegisterEventHandler(std::string_view event_type, std::string_view callback_function) -> bool;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<ECSInterface> ecs_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Entity Management**: Create, destroy, and validate 100,000+ entities efficiently
- [ ] **Component System**: Register and manage unlimited component types with type safety
- [ ] **Query Performance**: Execute complex queries on 10,000+ entities under 1ms

### Performance Requirements

- [ ] **Entity Operations**: Entity creation/destruction under 1 microsecond
- [ ] **Query Speed**: Component queries achieve >90% cache hit rates
- [ ] **Memory Usage**: Total ECS memory under 10MB for 5,000 entities with 8 components each
- [ ] **System Scheduling**: Parallel system execution with <5% scheduling overhead

### Quality Requirements

- [ ] **Thread Safety**: Zero race conditions in multi-threaded access patterns
- [ ] **Maintainability**: All ECS code covered by comprehensive unit tests
- [ ] **Testability**: Mock ECS interfaces for system testing
- [ ] **Documentation**: Complete ECS architecture guide with usage examples

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(ECSTest, EntityCreationPerformance) {
    auto ecs = engine::ecs::ECS{};
    ecs.Initialize(ECSConfig{});
    
    // Measure entity creation speed
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Entity> entities;
    for (int i = 0; i < 100000; ++i) {
        entities.push_back(ecs.CreateEntity());
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();
    auto avg_time = total_time / 100000.0;
    
    EXPECT_LT(avg_time, 1.0); // Under 1 microsecond per entity
}

TEST(ECSTest, QueryPerformance) {
    auto ecs = engine::ecs::ECS{};
    ecs.Initialize(ECSConfig{});
    ecs.RegisterComponent<Position>();
    ecs.RegisterComponent<Velocity>();
    
    // Create test entities
    for (int i = 0; i < 10000; ++i) {
        auto entity = ecs.CreateEntity();
        ecs.AddComponent(entity, Position{static_cast<float>(i), 0.0f});
        if (i % 2 == 0) {
            ecs.AddComponent(entity, Velocity{1.0f, 0.0f});
        }
    }
    
    // Measure query performance
    auto start = std::chrono::high_resolution_clock::now();
    auto query = ecs.CreateQuery<Position, Velocity>();
    int count = 0;
    for (auto [entity, pos, vel] : query) {
        pos.x += vel.x;
        count++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();
    
    EXPECT_LT(query_time, 1000); // Under 1ms
    EXPECT_EQ(count, 5000); // Half the entities have both components
}

TEST(ECSTest, MemoryUsage) {
    auto ecs = engine::ecs::ECS{};
    ecs.Initialize(ECSConfig{});
    ecs.RegisterComponent<Position>();
    ecs.RegisterComponent<Velocity>();
    
    // Create 5000 entities with components
    for (int i = 0; i < 5000; ++i) {
        auto entity = ecs.CreateEntity();
        ecs.AddComponent(entity, Position{static_cast<float>(i), static_cast<float>(i)});
        ecs.AddComponent(entity, Velocity{1.0f, 1.0f});
    }
    
    auto stats = ecs.GetECSStats();
    EXPECT_LT(stats.total_memory_bytes, 10 * 1024 * 1024); // Under 10MB
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Entity Management (Estimated: 4 days)

- [ ] **Task 1.1**: Implement entity creation, destruction, and generation validation
- [ ] **Task 1.2**: Create sparse set storage for efficient entity-component mapping
- [ ] **Task 1.3**: Add basic component registration and storage
- [ ] **Deliverable**: Basic ECS with entity and component management

#### Phase 2: Query System (Estimated: 3 days)

- [ ] **Task 2.1**: Implement component queries with multiple component types
- [ ] **Task 2.2**: Add query caching and optimization for repeated queries
- [ ] **Task 2.3**: Create query builder pattern for complex queries
- [ ] **Deliverable**: Efficient query system with caching

#### Phase 3: System Management (Estimated: 3 days)

- [ ] **Task 3.1**: Implement system registration and dependency analysis
- [ ] **Task 3.2**: Add parallel system execution with automatic scheduling
- [ ] **Task 3.3**: Create system ordering based on component dependencies
- [ ] **Deliverable**: Complete system management with parallel execution

#### Phase 4: Events and Resources (Estimated: 2 days)

- [ ] **Task 4.1**: Implement event system for inter-system communication
- [ ] **Task 4.2**: Add resource management for global game state
- [ ] **Task 4.3**: Create performance monitoring and debugging tools
- [ ] **Deliverable**: Complete ECS with events, resources, and debugging

### File Structure

```
src/engine/ecs/
├── ecs.cppm                    // Primary module interface
├── entity_manager.cpp         // Entity lifecycle management
├── component_pool.cpp         // Component storage implementation
├── component_registry.cpp     // Component type management
├── query_system.cpp          // Query execution and caching
├── system_manager.cpp        // System registration and execution
├── world.cpp                 // World container and coordination
├── event_dispatcher.cpp      // Event handling system
├── resource_manager.cpp      // Global resource management
└── tests/
    ├── entity_tests.cpp
    ├── component_tests.cpp
    ├── query_tests.cpp
    └── ecs_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::ecs`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with specialized ECS components
- **Build Integration**: Foundation module with no external engine dependencies

### Testing Strategy

- **Unit Tests**: Comprehensive testing of all ECS components with known patterns
- **Integration Tests**: Full ECS testing with complex entity scenarios
- **Performance Tests**: Benchmarking entity operations, queries, and memory usage
- **Thread Safety Tests**: Multi-threaded access pattern validation

## Risk Assessment

### Technical Risks

| Risk                        | Probability | Impact | Mitigation                                               |
|-----------------------------|-------------|--------|----------------------------------------------------------|
| **Performance Degradation** | Low         | High   | Efficient sparse set storage, comprehensive benchmarking |
| **Memory Fragmentation**    | Medium      | Medium | Pool allocators, contiguous component storage            |
| **Threading Complexity**    | Medium      | High   | Lock-free designs, careful synchronization               |
| **API Complexity**          | Low         | Medium | Clean template interfaces, comprehensive documentation   |

### Integration Risks

- **System Dependencies**: Risk of circular dependencies between systems
    - *Mitigation*: Clear dependency analysis, topological system ordering
- **Component Explosion**: Risk of too many component types impacting performance
    - *Mitigation*: Component type limits, archetype optimization

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (memory management, timing, threading primitives)

- **Optional Systems**: None (ECS is the foundation)

### External Dependencies

- **Standard Library Features**: C++20 concepts, templates, containers, algorithms
- **Platform APIs**: None (pure C++ implementation)

### Build System Dependencies

- **CMake Targets**: Foundation target with no dependencies
- **vcpkg Packages**: None required for core ECS functionality
- **Platform-Specific**: Platform-optimized memory allocators where available
- **Module Dependencies**: Imports only engine.platform and standard library

