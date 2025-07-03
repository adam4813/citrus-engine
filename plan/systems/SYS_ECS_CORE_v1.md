# SYS_ECS_CORE_v1

> **System-Level Design Document for Entity Component System Core**

## Executive Summary

This document defines the core Entity Component System (ECS) architecture for the modern C++20 game engine. The ECS
provides a data-oriented, performance-focused foundation for managing thousands of game entities with high-frequency
updates at 60+ FPS. The design emphasizes cache-friendly component storage, flexible system composition, and clean
separation between generic engine systems and game-specific AI logic. The architecture supports both multi-threaded
execution on native platforms and single-threaded fallback for WebAssembly builds.

## Scope and Objectives

### In Scope

- [ ] Entity lifecycle management with generation-based IDs
- [ ] Component storage with structure-of-arrays layout for cache efficiency
- [ ] System scheduling with dependency resolution and parallel execution
- [ ] Component query API using C++20 concepts for type safety
- [ ] Event system for inter-system communication
- [ ] Performance profiling and debugging hooks
- [ ] Single-threaded fallback support for WebAssembly
- [ ] Update rate throttling mechanisms

### Out of Scope

- [ ] Game-specific AI systems (Utility AI, GOAP) - these integrate with but are not part of core ECS
- [ ] Specific game components (ActorComponent, StructureComponent) - examples only
- [ ] Asset loading and management (handled by separate asset system)
- [ ] Rendering implementation details (handled by rendering system)

### Primary Objectives

1. **Performance**: Support thousands of entities at 60+ FPS with efficient memory access patterns
2. **Flexibility**: Enable diverse game systems through component composition
3. **Type Safety**: Leverage C++20 concepts for compile-time validation of component operations
4. **Threading**: Support parallel system execution with automatic dependency resolution

### Secondary Objectives

- Clean integration points for game-specific AI systems
- Comprehensive debugging and profiling capabilities
- Memory-efficient component storage for WASM builds
- Extensible architecture for future engine features

## Architecture/Design

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Game-Specific Layer                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │ Utility AI  │  │    GOAP     │  │  Game Logic │             │
│  │   System    │  │   System    │  │   Systems   │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
│         │                │                │                    │
│         └────────────────┼────────────────┘                    │
│                          │                                     │
├─────────────────────────────────────────────────────────────────┤
│                      ECS Core Layer                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │   Entity    │  │ Component   │  │   System    │             │
│  │  Manager    │  │  Manager    │  │ Scheduler   │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
│         │                │                │                    │
│         └────────────────┼────────────────┘                    │
│                          │                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │  Movement   │  │  Collision  │  │   Render    │             │
│  │   System    │  │   System    │  │   System    │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Entity Manager

- **Purpose**: Centralized entity lifecycle management with memory-efficient ID generation
- **Responsibilities**: Entity creation/destruction, ID generation/reuse, entity validation
- **Key Classes/Interfaces**: `EntityManager`, `EntityId`, `EntityGeneration`
- **Data Flow**: Game systems request entities → Manager assigns IDs → Components associated → Systems process

#### Component 2: Component Manager

- **Purpose**: High-performance component storage using structure-of-arrays layout
- **Responsibilities**: Component registration, storage allocation, component queries, memory management
- **Key Classes/Interfaces**: `ComponentManager`, `ComponentArray<T>`, `ComponentTraits<T>`
- **Data Flow**: Systems query components → Manager returns views → Systems process data → Manager handles updates

#### Component 3: System Scheduler

- **Purpose**: Dependency-aware system execution with parallel processing capabilities
- **Responsibilities**: System registration, dependency resolution, parallel execution batches, update rate control
- **Key Classes/Interfaces**: `SystemScheduler`, `ISystem`, `ThreadingRequirements`, `SystemDependency`
- **Data Flow**: Scheduler analyzes dependencies → Creates execution batches → Executes systems in parallel →
  Synchronizes results

#### Component 4: World Container

- **Purpose**: Top-level coordination between entities, components, and systems
- **Responsibilities**: System integration, frame execution, resource cleanup, performance monitoring
- **Key Classes/Interfaces**: `World`, `FrameContext`, `WorldConfig`
- **Data Flow**: Game loop calls World::Update() → World orchestrates all subsystems → Returns control to game

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Generation-based IDs prevent use-after-free errors and enable safe entity references
- **Entity Queries**: Systems specify component requirements and receive filtered entity lists
- **Component Dependencies**: Automatic validation ensures entities have required components before system processing

#### Component Design

```cpp
// Generic transform component - spatial data
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    bool dirty = true;  // Optimization flag for transform updates
};

// Movement component - velocity and targets
struct VelocityComponent {
    glm::vec3 velocity{0.0f};
    float max_speed = 5.0f;
    float acceleration = 10.0f;
    float drag = 0.95f;
};

// Target component - AI integration point
struct TargetComponent {
    glm::vec3 target_position{0.0f};
    EntityId target_entity = INVALID_ENTITY;
    enum class Type { Position, Entity, None } type = Type::None;
    float arrival_threshold = 0.1f;
};

// Collision shape component - physics integration
struct ShapeComponent {
    enum class Type { Box, Circle, Polygon };
    Type shape_type = Type::Box;
    glm::vec2 size{1.0f};  // width/height for box, radius for circle
    glm::vec2 offset{0.0f};  // Local offset from transform
    bool is_trigger = false;
    uint32_t collision_mask = 0xFFFFFFFF;
};

// Render component - visual representation
struct RenderComponent {
    uint32_t texture_id = 0;
    uint32_t material_id = 0;
    glm::vec4 tint{1.0f};
    float z_order = 0.0f;
    bool visible = true;
    bool cast_shadow = false;
};

// Component traits for ECS optimization
template<typename T>
struct ComponentTraits {
    static constexpr bool is_trivially_copyable = std::is_trivially_copyable_v<T>;
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(T);
    static constexpr bool enable_pooling = true;
};

// Specialized traits for high-frequency components
template<>
struct ComponentTraits<TransformComponent> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 50000;  // Higher limit for common component
    static constexpr size_t alignment = 16;  // SIMD alignment
    static constexpr bool enable_pooling = true;
};
```

#### System Integration

```cpp
// Base system interface with standardized lifecycle
class ISystem {
public:
    virtual ~ISystem() = default;
    
    // Lifecycle methods (consistent across all systems)
    [[nodiscard]] virtual auto Initialize() -> std::optional<std::string> = 0;
    virtual void Shutdown() noexcept = 0;
    
    // Frame execution
    virtual void Update(double delta_time, const FrameContext& context) = 0;
    
    // Threading and dependency information
    [[nodiscard]] virtual auto GetThreadingRequirements() const -> ThreadingRequirements = 0;
    [[nodiscard]] virtual auto GetSystemName() const -> std::string_view = 0;
    [[nodiscard]] virtual auto GetSystemVersion() const -> uint32_t = 0;
    
    // Performance monitoring
    [[nodiscard]] virtual auto GetPerformanceMetrics() const -> SystemMetrics = 0;
};

// Example basic physics system
class PhysicsSystem : public ISystem {
public:
    [[nodiscard]] auto Initialize() -> std::optional<std::string> override {
        return std::nullopt;  // Success
    }
    
    void Shutdown() noexcept override {
        // Cleanup resources
    }
    
    void Update(double delta_time, const FrameContext& context) override {
        // Simple physics integration using existing components
        auto movement_query = context.world->Query<TransformComponent, VelocityComponent>();
        
        for (auto [entity, transform, velocity] : movement_query) {
            // Basic physics integration
            transform.position += velocity.velocity * static_cast<float>(delta_time);
            
            // Apply drag
            velocity.velocity *= velocity.drag;
            
            transform.dirty = true;
        }
        
        // Handle AI-driven movement targets with physics
        auto target_query = context.world->Query<TransformComponent, VelocityComponent, TargetComponent>();
        for (auto [entity, transform, velocity, target] : target_query) {
            if (target.type == TargetComponent::Type::Position) {
                glm::vec3 direction = target.target_position - transform.position;
                float distance = glm::length(direction);
                
                if (distance > target.arrival_threshold) {
                    // Apply acceleration toward target
                    direction = glm::normalize(direction);
                    velocity.velocity += direction * velocity.acceleration * static_cast<float>(delta_time);
                    
                    // Clamp to max speed
                    if (glm::length(velocity.velocity) > velocity.max_speed) {
                        velocity.velocity = glm::normalize(velocity.velocity) * velocity.max_speed;
                    }
                } else {
                    // Target reached - stop and remove target
                    velocity.velocity = glm::vec3{0.0f};
                    context.world->RemoveComponent<TargetComponent>(entity);
                    
                    // Publish event for AI systems
                    context.world->GetEventSystem().Publish(EntityTargetReachedEvent{
                        .entity = entity,
                        .target_position = target.target_position,
                        .timestamp = context.current_time
                    });
                }
            }
        }
        
        // Basic collision detection using shape components
        auto collision_query = context.world->Query<TransformComponent, ShapeComponent>();
        std::vector<EntityId> entities_with_shapes;
        
        for (auto [entity, transform, shape] : collision_query) {
            entities_with_shapes.push_back(entity);
        }
        
        // Simple O(n²) collision detection - sufficient for lightweight physics
        for (size_t i = 0; i < entities_with_shapes.size(); ++i) {
            for (size_t j = i + 1; j < entities_with_shapes.size(); ++j) {
                if (CheckCollision(entities_with_shapes[i], entities_with_shapes[j], context.world)) {
                    HandleCollision(entities_with_shapes[i], entities_with_shapes[j], context.world);
                }
            }
        }
    }
    
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .execution_phase = ExecutionPhase::Update,
            .can_run_parallel = false,  // Keep simple for now - collision detection is not thread-safe
            .read_components = {ComponentType::Get<TargetComponent>(), ComponentType::Get<ShapeComponent>()},
            .write_components = {ComponentType::Get<TransformComponent>(), ComponentType::Get<VelocityComponent>()}
        };
    }
    
    [[nodiscard]] auto GetSystemName() const -> std::string_view override {
        return "PhysicsSystem";
    }
    
    [[nodiscard]] auto GetSystemVersion() const -> uint32_t override {
        return 1;
    }
    
    [[nodiscard]] auto GetPerformanceMetrics() const -> SystemMetrics override {
        return system_metrics_;
    }

private:
    bool CheckCollision(EntityId entity_a, EntityId entity_b, const World* world) const {
        auto transform_a = world->GetComponent<TransformComponent>(entity_a);
        auto transform_b = world->GetComponent<TransformComponent>(entity_b);
        auto shape_a = world->GetComponent<ShapeComponent>(entity_a);
        auto shape_b = world->GetComponent<ShapeComponent>(entity_b);
        
        if (!transform_a || !transform_b || !shape_a || !shape_b) {
            return false;
        }
        
        // Simple AABB collision for box shapes
        if (shape_a->shape_type == ShapeComponent::Type::Box && 
            shape_b->shape_type == ShapeComponent::Type::Box) {
            
            glm::vec3 pos_a = transform_a->position + glm::vec3(shape_a->offset, 0.0f);
            glm::vec3 pos_b = transform_b->position + glm::vec3(shape_b->offset, 0.0f);
            
            return (std::abs(pos_a.x - pos_b.x) < (shape_a->size.x + shape_b->size.x) * 0.5f) &&
                   (std::abs(pos_a.y - pos_b.y) < (shape_a->size.y + shape_b->size.y) * 0.5f);
        }
        
        return false;
    }
    
    void HandleCollision(EntityId entity_a, EntityId entity_b, World* world) const {
        // Simple collision response - stop entities
        if (auto velocity_a = world->GetComponent<VelocityComponent>(entity_a)) {
            velocity_a->velocity *= 0.5f;  // Dampen velocity
        }
        if (auto velocity_b = world->GetComponent<VelocityComponent>(entity_b)) {
            velocity_b->velocity *= 0.5f;  // Dampen velocity
        }
        
        // Publish collision event for AI systems
        if (auto transform_a = world->GetComponent<TransformComponent>(entity_a);
            auto transform_b = world->GetComponent<TransformComponent>(entity_b)) {
            
            glm::vec3 collision_point = (transform_a->position + transform_b->position) * 0.5f;
            glm::vec3 collision_normal = glm::normalize(transform_b->position - transform_a->position);
            
            world->GetEventSystem().Publish(CollisionEvent{
                .entity_a = entity_a,
                .entity_b = entity_b,
                .collision_point = collision_point,
                .collision_normal = collision_normal
            });
        }
    }
    
    SystemMetrics system_metrics_;
};
```

### Multi-Threading Design

#### Threading Model

- **Native Platforms**: Parallel system execution with dependency-based batching
- **WebAssembly**: Single-threaded fallback with same API for consistency
- **Thread Safety**: Lock-free component access through immutable views and exclusive write access
- **Data Dependencies**: Automatic analysis of component read/write patterns

#### Parallel Execution Strategy

```cpp
// Threading requirements declaration
struct ThreadingRequirements {
    ExecutionPhase execution_phase = ExecutionPhase::Update;
    bool can_run_parallel = true;
    std::vector<ComponentTypeId> read_components;
    std::vector<ComponentTypeId> write_components;
    std::vector<std::string> system_dependencies;  // Systems that must run before this one
    float estimated_cost = 1.0f;  // Relative execution cost for load balancing
};

// System scheduler with dependency resolution
class SystemScheduler {
public:
    template<typename SystemType, typename... Args>
    void RegisterSystem(Args&&... args) {
        auto system = std::make_unique<SystemType>(std::forward<Args>(args)...);
        auto requirements = system->GetThreadingRequirements();
        
        // Validate component access patterns
        ValidateComponentAccess(requirements);
        
        // Add to dependency graph
        dependency_graph_.AddSystem(std::move(system), requirements);
    }
    
    void ExecuteFrame(double delta_time, const FrameContext& context) {
        // Throttle update rate if needed
        if (ShouldThrottleUpdate(delta_time)) {
            return;
        }
        
        // Get execution batches based on dependencies
        auto batches = dependency_graph_.GetExecutionBatches();
        
        for (const auto& batch : batches) {
            if (context.threading_enabled && batch.can_parallelize) {
                // Execute systems in parallel
                ExecuteBatchParallel(batch, delta_time, context);
            } else {
                // Execute systems sequentially (WASM fallback)
                ExecuteBatchSequential(batch, delta_time, context);
            }
        }
        
        // Update performance metrics
        UpdatePerformanceMetrics();
    }

private:
    void ValidateComponentAccess(const ThreadingRequirements& requirements) {
        // Ensure no component is both read and written by same system
        for (const auto& read_comp : requirements.read_components) {
            for (const auto& write_comp : requirements.write_components) {
                if (read_comp == write_comp) {
                    throw std::runtime_error("System cannot both read and write same component");
                }
            }
        }
    }
    
    bool ShouldThrottleUpdate(double delta_time) {
        // Implement update rate throttling
        static constexpr double MIN_FRAME_TIME = 1.0 / 120.0;  // 120 FPS cap
        return delta_time < MIN_FRAME_TIME;
    }
    
    SystemDependencyGraph dependency_graph_;
    PerformanceProfiler profiler_;
};
```

#### Memory Access Patterns

- **Structure-of-Arrays**: Components stored in contiguous arrays for cache efficiency
- **Component Views**: Immutable views prevent data races during parallel access
- **Memory Pools**: Pre-allocated component storage for predictable performance
- **SIMD Opportunities**: 16-byte aligned component storage enables vectorized operations

### Component Query API

```cpp
// C++20 concepts for type-safe component queries
template<typename T>
concept Component = requires {
    typename ComponentTraits<T>;
    std::is_trivially_destructible_v<T>;
};

template<typename... Components>
concept ValidQuery = (Component<Components> && ...);

// World query interface
class World {
public:
    // Single component query
    template<Component T>
    [[nodiscard]] auto Query() -> ComponentView<T> {
        return component_manager_.GetView<T>();
    }
    
    // Multi-component query with automatic iteration
    template<ValidQuery... Components>
    [[nodiscard]] auto Query() -> MultiComponentView<Components...> {
        return component_manager_.GetMultiView<Components...>();
    }
    
    // Query with entity filtering
    template<ValidQuery... Components>
    [[nodiscard]] auto QueryWhere(std::function<bool(EntityId)> predicate) -> FilteredView<Components...> {
        return component_manager_.GetFilteredView<Components...>(predicate);
    }
    
    // Component existence check
    template<Component T>
    [[nodiscard]] bool HasComponent(EntityId entity) const {
        return component_manager_.HasComponent<T>(entity);
    }
    
    // Component manipulation
    template<Component T>
    void AddComponent(EntityId entity, T&& component) {
        component_manager_.AddComponent(entity, std::forward<T>(component));
    }
    
    template<Component T>
    void RemoveComponent(EntityId entity) {
        component_manager_.RemoveComponent<T>(entity);
    }
    
    template<Component T>
    [[nodiscard]] auto GetComponent(EntityId entity) -> std::optional<T*> {
        return component_manager_.GetComponent<T>(entity);
    }

private:
    EntityManager entity_manager_;
    ComponentManager component_manager_;
    SystemScheduler system_scheduler_;
    EventSystem event_system_;
};
```

### Event System for Inter-System Communication

```cpp
// Event system for loose coupling between systems
class EventSystem {
public:
    template<typename EventType>
    void Subscribe(std::function<void(const EventType&)> handler) {
        auto type_id = GetEventTypeId<EventType>();
        event_handlers_[type_id].emplace_back([handler](const void* event) {
            handler(*static_cast<const EventType*>(event));
        });
    }
    
    template<typename EventType>
    void Publish(const EventType& event) {
        auto type_id = GetEventTypeId<EventType>();
        if (auto it = event_handlers_.find(type_id); it != event_handlers_.end()) {
            for (const auto& handler : it->second) {
                handler(&event);
            }
        }
    }
    
    void ProcessPendingEvents() {
        // Process queued events during safe points
        for (auto& event : pending_events_) {
            event();
        }
        pending_events_.clear();
    }

private:
    template<typename EventType>
    static constexpr uint32_t GetEventTypeId() {
        return TypeIdGenerator<EventType>::value;
    }
    
    std::unordered_map<uint32_t, std::vector<std::function<void(const void*)>>> event_handlers_;
    std::vector<std::function<void()>> pending_events_;
};

// Example events for AI system integration
struct EntityTargetReachedEvent {
    EntityId entity;
    glm::vec3 target_position;
    double timestamp;
};

struct CollisionEvent {
    EntityId entity_a;
    EntityId entity_b;
    glm::vec3 collision_point;
    glm::vec3 collision_normal;
};
```

## Dependencies

### Internal Dependencies

- **Required Systems**: None (ECS is foundational)
- **Optional Systems**: Event system enhances but is not required for basic operation
- **Circular Dependencies**: None by design

### External Dependencies

- **Third-Party Libraries**:
    - `glm` for mathematics (header-only)
    - Standard library threading primitives (`std::thread`, `std::mutex`)
- **Standard Library Features**: C++20 concepts, ranges, optional, variant
- **Platform APIs**: None directly (threading handled by standard library)

### Build System Dependencies

- **CMake Targets**: Engine core, math utilities
- **vcpkg Packages**: No additional packages required
- **Platform-Specific**: Threading support detection for WASM builds
- **Module Dependencies**: Will be exposed through `engine.ecs.cppm` module interface

### Asset Pipeline Dependencies

- **Asset Formats**: None directly (systems may depend on specific assets)
- **Configuration Files**: System configuration through engine config
- **Resource Loading**: Component data may reference assets through IDs

### Reference Implementation Examples

- **Existing Code Patterns**: Current entity system in `src/state/` provides patterns for entity lifecycle
- **Anti-Patterns**: Avoid deep inheritance from existing entity classes
- **Integration Points**: Clean interfaces for AI systems to write component data

## Success Criteria

### Functional Requirements

- [ ] **Entity Management**: Create/destroy 50,000+ entities efficiently with generation-based IDs
- [ ] **Component Storage**: Store and query components with <1μs access time per entity
- [ ] **System Execution**: Execute 20+ systems with automatic dependency resolution
- [ ] **Threading**: Parallel system execution on native platforms, single-threaded fallback for WASM
- [ ] **Memory Efficiency**: Component storage with <10% memory overhead vs naive storage
- [ ] **Type Safety**: Compile-time validation of component queries and system dependencies

### Performance Requirements

- [ ] **Frame Rate**: Maintain 60+ FPS with 5,000+ active entities
- [ ] **Entity Creation**: Create entities in <1μs each
- [ ] **Component Queries**: Query 10,000 entities in <100μs
- [ ] **System Updates**: Execute typical system in <5ms
- [ ] **Memory Usage**: Component storage under 50MB for 10,000 entities
- [ ] **Cache Efficiency**: >90% cache hit rate for component iterations

### Quality Requirements

- [ ] **Reliability**: Zero memory leaks during 24-hour stress testing
- [ ] **Maintainability**: System registration requires <10 lines of code
- [ ] **Testability**: Unit test coverage above 85% for core ECS functionality
- [ ] **Documentation**: Complete API documentation with usage examples

### Acceptance Tests

```cpp
// Performance validation tests
TEST(ECSCore, HandlesLargeEntityCounts) {
    World world;
    
    // Create 10,000 entities with transform and velocity components
    std::vector<EntityId> entities;
    for (int i = 0; i < 10000; ++i) {
        auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity, {});
        world.AddComponent<VelocityComponent>(entity, {});
        entities.push_back(entity);
    }
    
    // Measure query performance
    auto start = std::chrono::high_resolution_clock::now();
    auto query = world.Query<TransformComponent, VelocityComponent>();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    EXPECT_LT(query_time.count(), 100);  // <100μs for query setup
    
    // Measure iteration performance
    start = std::chrono::high_resolution_clock::now();
    for (auto [entity, transform, velocity] : query) {
        transform.position += velocity.velocity * 0.016f;  // Simulate 60 FPS update
        transform.dirty = true;
    }
    end = std::chrono::high_resolution_clock::now();
    
    auto iteration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    EXPECT_LT(iteration_time.count(), 5000);  // <5ms for iteration
}

// Thread safety validation
TEST(ECSCore, ThreadSafeSystemExecution) {
    World world;
    
    // Register systems that can run in parallel
    world.RegisterSystem<PhysicsSystem>();
    world.RegisterSystem<RenderSystem>();  // Placeholder for future rendering system
    
    // Create entities with overlapping component sets
    for (int i = 0; i < 1000; ++i) {
        auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity, {});
        world.AddComponent<VelocityComponent>(entity, {});
        world.AddComponent<ShapeComponent>(entity, {});
        world.AddComponent<RenderComponent>(entity, {});
    }
    
    // Execute multiple frames - note: PhysicsSystem is single-threaded for now
    FrameContext context{.threading_enabled = true};
    for (int frame = 0; frame < 100; ++frame) {
        EXPECT_NO_THROW(world.Update(0.016, context));
    }
}

// Memory efficiency validation
TEST(ECSCore, EfficientComponentStorage) {
    World world;
    
    // Measure memory usage before adding components
    size_t initial_memory = GetMemoryUsage();
    
    // Add 10,000 transform components
    std::vector<EntityId> entities;
    for (int i = 0; i < 10000; ++i) {
        auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity, {});
        entities.push_back(entity);
    }
    
    size_t final_memory = GetMemoryUsage();
    size_t component_memory = final_memory - initial_memory;
    
    // Should be close to theoretical minimum: 10,000 * sizeof(TransformComponent)
    size_t theoretical_minimum = 10000 * sizeof(TransformComponent);
    size_t overhead = component_memory - theoretical_minimum;
    
    EXPECT_LT(overhead, theoretical_minimum * 0.1);  // <10% overhead
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core ECS Foundation (Estimated: 5 days)

- [ ] **Task 1.1**: Entity manager with generation-based IDs (1 day)
- [ ] **Task 1.2**: Component manager with SoA storage (2 days)
- [ ] **Task 1.3**: Basic system scheduler (single-threaded) (1 day)
- [ ] **Task 1.4**: World container and basic queries (1 day)
- [ ] **Deliverable**: Working ECS that can create entities, add components, and run systems

#### Phase 2: Advanced Features (Estimated: 4 days)

- [ ] **Task 2.1**: Multi-threaded system execution with dependency resolution (2 days)
- [ ] **Task 2.2**: C++20 concepts for type-safe queries (1 day)
- [ ] **Task 2.3**: Event system for inter-system communication (1 day)
- [ ] **Deliverable**: Full-featured ECS with parallel execution and events

#### Phase 3: Performance and Integration (Estimated: 3 days)

- [ ] **Task 3.1**: Performance profiling and optimization (1 day)
- [ ] **Task 3.2**: WASM single-threaded fallback (1 day)
- [ ] **Task 3.3**: Integration with existing Game::Update() flow (1 day)
- [ ] **Deliverable**: Production-ready ECS integrated with colony game

### File Structure

```
src/engine/ecs/
├── include/
│   ├── EntityManager.h
│   ├── ComponentManager.h
│   ├── SystemScheduler.h
│   ├── World.h
│   ├── ECSTypes.h
│   └── ECSConcepts.h
├── src/
│   ├── EntityManager.cpp
│   ├── ComponentManager.cpp
│   ├── SystemScheduler.cpp
│   ├── World.cpp
│   └── EventSystem.cpp
└── tests/
    ├── EntityManagerTests.cpp
    ├── ComponentManagerTests.cpp
    ├── SystemSchedulerTests.cpp
    ├── WorldTests.cpp
    └── PerformanceTests.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::ecs` for all ECS-related classes
- **Header Guards**: Use `#pragma once` for all headers
- **Module Structure**: Public interfaces exposed through `engine.ecs.cppm`
- **Build Integration**: ECS as separate CMake target `EngineECS`

### Testing Strategy

- **Unit Tests**: Individual component testing for all manager classes
- **Integration Tests**: Full ECS workflow validation
- **Performance Tests**: Benchmarking with 1000+ entities and multiple systems
- **Thread Safety Tests**: Concurrent access validation with race condition detection

## Risk Assessment

### Technical Risks

| Risk                     | Probability | Impact | Mitigation                                 |
|--------------------------|-------------|--------|--------------------------------------------|
| **Memory Fragmentation** | Medium      | High   | Use memory pools and component arrays      |
| **Cache Misses**         | Medium      | High   | SoA layout and SIMD alignment              |
| **Threading Deadlocks**  | Low         | High   | Lock-free design and dependency analysis   |
| **WASM Performance**     | High        | Medium | Single-threaded optimization and profiling |

### Integration Risks

- **AI System Integration**: Risk of tight coupling with game-specific logic
- **Existing Code Compatibility**: Risk of breaking current entity system
- **Performance Regression**: Risk of slower performance than current implementation

### Resource Risks

- **Development Time**: Complex threading may extend timeline
- **Memory Usage**: SoA storage may increase memory footprint initially
- **Testing Complexity**: Multi-threaded testing requires sophisticated tooling

### Contingency Plans

- **Plan B**: Implement single-threaded version first, add threading later
- **Scope Reduction**: Remove advanced features like events if timeline pressures
- **Performance Fallback**: Keep existing entity system as backup during transition

## Decision Rationale

### Architectural Decisions

#### Decision 1: Structure-of-Arrays Component Storage

- **Options Considered**: Array-of-Structures (AoS), Structure-of-Arrays (SoA), hybrid approaches
- **Selection Rationale**: SoA provides better cache locality for system iteration patterns
- **Trade-offs**: More complex implementation for significantly better performance
- **Future Impact**: Enables SIMD optimizations and scales to thousands of entities

#### Decision 2: Generation-Based Entity IDs

- **Options Considered**: Simple integer IDs, UUID-based IDs, generation-based IDs
- **Selection Rationale**: Prevents use-after-free errors while maintaining performance
- **Trade-offs**: Slightly more complex ID management for much safer entity references
- **Future Impact**: Enables safe entity references across system boundaries

#### Decision 3: Compile-Time Component Validation

- **Options Considered**: Runtime type checking, compile-time concepts, hybrid validation
- **Selection Rationale**: C++20 concepts provide zero-cost compile-time validation
- **Trade-offs**: Requires modern compiler for better type safety and performance
- **Future Impact**: Prevents entire classes of runtime errors and improves performance

#### Decision 4: Event System for AI Integration

- **Options Considered**: Direct system coupling, shared state, event messaging
- **Selection Rationale**: Events provide loose coupling between ECS and AI systems
- **Trade-offs**: Additional complexity for clean separation of concerns
- **Future Impact**: Enables flexible AI architectures without ECS core dependencies

### Performance Decisions

- **Component Alignment**: 16-byte alignment enables SIMD operations on transform data
- **Memory Pooling**: Pre-allocated component arrays prevent allocation overhead
- **Update Throttling**: Frame rate capping prevents unnecessary CPU usage
- **Single-threaded Fallback**: WASM compatibility without performance cliffs

## References

### Related Planning Documents

- `ARCH_ENGINE_CORE_v1.md` - High-level engine architecture and threading model
- `SYS_THREADING_v1.md` - Threading system implementation
- `SYS_RENDERING_v1.md` - Rendering system integration

### External Resources

- [ECS Design Patterns](https://github.com/SanderMertens/ecs-faq) - Industry best practices
- [Data-Oriented Design](https://www.dataorienteddesign.com/) - Performance optimization principles
- [C++20 Concepts Tutorial](https://en.cppreference.com/w/cpp/language/concepts) - Type safety implementation

### Existing Code References

- `src/state/EntitySystem.h` - Current entity management patterns
- `src/AI_ARCHITECTURE.md` - AI system design for integration planning
- `src/GOAP.md` - Goal-oriented action planning integration points

## Appendices

### A. Component Memory Layout Analysis

Structure-of-Arrays layout for 10,000 TransformComponent instances:

```
Traditional AoS:          SoA Layout:
Entity[0]: [pos|rot|scl]  positions: [pos0|pos1|pos2|...]  <- Cache line
Entity[1]: [pos|rot|scl]  rotations: [rot0|rot1|rot2|...]  <- Cache line  
Entity[2]: [pos|rot|scl]  scales:    [scl0|scl1|scl2|...]  <- Cache line
...                       ...
```

SoA provides 3x better cache utilization for position-only system iterations.

### B. Threading Model for Colony Game

```
Native Build:                    WASM Build:
┌─────────────┐                 ┌─────────────┐
│ Main Thread │                 │ Main Thread │
│ (Events)    │                 │ (All Logic) │
└─────────────┘                 └─────────────┘
       │                               │
┌─────────────┐                 ┌─────────────┐
│Render Thread│                 │Same Thread  │
│┌───────────┐│                 │┌───────────┐│
││ECS Systems││                 ││ECS Systems││
│└───────────┘│                 │└───────────┘│
│┌───────────┐│                 │┌───────────┐│
││  OpenGL   ││                 ││  OpenGL   ││
│└───────────┘│                 │└───────────┘│
└─────────────┘                 └─────────────┘
```

### C. AI System Integration Examples

```cpp
// Game-specific AI system that uses ECS components
class UtilityAISystem {
public:
    void Update(double delta_time, World& world) {
        // Query entities that need AI decisions
        auto query = world.Query<TransformComponent, AIStateComponent>();
        
        for (auto [entity, transform, ai_state] : query) {
            // Run utility AI to decide next action
            auto best_action = EvaluateUtilities(entity, world);
            
            // Set target component based on AI decision
            if (best_action.type == ActionType::MoveTo) {
                world.AddComponent<TargetComponent>(entity, {
                    .target_position = best_action.target_position,
                    .type = TargetComponent::Type::Position
                });
            }
            
            // Publish event for other systems
            world.GetEventSystem().Publish(AIDecisionEvent{
                .entity = entity,
                .action = best_action,
                .confidence = best_action.utility_score
            });
        }
    }
    
private:
    AIAction EvaluateUtilities(EntityId entity, const World& world) {
        // Utility AI implementation...
        return AIAction{};
    }
};
```

---

**Document Status**: Draft  
**Last Updated**: June 30, 2025  
**Next Review**: July 7, 2025  
**Reviewers**: [To be assigned]
