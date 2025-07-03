# MOD_PHYSICS_v1

> **2D Physics Simulation with Spatial Optimization - Engine Module**

## Executive Summary

The `engine.physics` module provides a comprehensive 2D physics simulation system optimized for real-time gameplay with
rigid body dynamics, collision detection, spatial optimization, and seamless ECS integration. This module implements
efficient broad-phase collision detection, continuous collision detection, and constraint solving to deliver realistic
physics simulation for the Colony Game Engine's interactive elements including dynamic objects, collision detection, and
environmental interactions across desktop and WebAssembly platforms.

## Scope and Objectives

### In Scope

- [ ] 2D rigid body dynamics with linear and angular physics
- [ ] Efficient collision detection with broad-phase spatial optimization
- [ ] Contact resolution and constraint solving
- [ ] Multiple collision shape types (circle, box, polygon, edge)
- [ ] Physics material system with friction and restitution
- [ ] Continuous collision detection for fast-moving objects

### Out of Scope

- [ ] 3D physics simulation (Colony Game is 2D-focused)
- [ ] Soft body dynamics and fluid simulation
- [ ] Advanced constraint types (joints, springs)
- [ ] Physics-based networking and prediction

### Primary Objectives

1. **Performance**: Simulate 1,000+ physics bodies at 60 FPS with deterministic results
2. **Accuracy**: Stable collision resolution with minimal penetration (< 0.1 units)
3. **Integration**: Seamless ECS integration with zero-copy data sharing

### Secondary Objectives

- Sub-frame collision detection for fast-moving objects
- Memory usage under 64MB for typical simulation loads
- Hot-reload physics configuration during development

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages physics body components for entities with physical behavior
- **Entity Queries**: Queries entities with RigidBody, Collider, and Transform components
- **Component Dependencies**: Requires Transform for positioning; optional PhysicsMaterial for surface properties

#### Component Design

```cpp
// Physics-related components for ECS integration
struct RigidBody {
    BodyType type{BodyType::Dynamic};
    float mass{1.0f};
    float inertia{1.0f};
    Vec2 linear_velocity{0.0f, 0.0f};
    float angular_velocity{0.0f};
    Vec2 force{0.0f, 0.0f};
    float torque{0.0f};
    float linear_damping{0.1f};
    float angular_damping{0.1f};
    bool is_sleeping{false};
    std::uint32_t body_id{0}; // Physics engine body ID
};

struct Collider {
    CollisionShapeId shape_id{0};
    CollisionCategory category{CollisionCategory::Default};
    CollisionMask mask{0xFFFFFFFF}; // What this collider can collide with
    bool is_sensor{false}; // Trigger-only, no collision response
    bool is_enabled{true};
    Vec2 offset{0.0f, 0.0f}; // Offset from entity position
};

struct PhysicsMaterial {
    float friction{0.5f};
    float restitution{0.0f}; // Bounciness (0 = no bounce, 1 = perfect bounce)
    float density{1.0f};
    SurfaceType surface_type{SurfaceType::Default};
};

struct ContactInfo {
    Entity other_entity{0};
    Vec2 contact_point{0.0f, 0.0f};
    Vec2 contact_normal{0.0f, 0.0f};
    float penetration_depth{0.0f};
    float impulse_magnitude{0.0f};
};

// Component traits for physics
template<>
struct ComponentTraits<RigidBody> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};

template<>
struct ComponentTraits<Collider> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};
```

#### System Integration

- **System Dependencies**: Runs after transform updates, before rendering and AI systems
- **Component Access Patterns**: Read-write access to RigidBody; read-only to Transform and Collider
- **Inter-System Communication**: Provides collision events for AI and gameplay systems

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Physics batch - executes on dedicated physics thread for consistent timing
- **Thread Safety**: Physics simulation isolated to physics thread; component updates are thread-safe
- **Data Dependencies**: Reads Transform/Collider data; writes to RigidBody state and contact information

#### Parallel Execution Strategy

```cpp
// Multi-threaded physics processing with deterministic simulation
class PhysicsSystem : public ISystem {
public:
    // Main thread: Collect physics data from ECS
    void CollectPhysicsData(const ComponentManager& components,
                          std::span<EntityId> entities,
                          const ThreadContext& context) override;
    
    // Physics thread: Run simulation step
    void SimulatePhysics(float delta_time, const ThreadContext& context);
    
    // Main thread: Apply results back to ECS
    void ApplyPhysicsResults(const ComponentManager& components,
                           std::span<EntityId> entities,
                           const ThreadContext& context);

private:
    struct PhysicsCommand {
        enum Type { AddBody, RemoveBody, ApplyForce, SetVelocity };
        Type type;
        Entity entity;
        union {
            RigidBodyDefinition body_def;
            Vec2 force;
            Vec2 velocity;
        } data;
    };
    
    // Lock-free command queue between main and physics threads
    moodycamel::ConcurrentQueue<PhysicsCommand> command_queue_;
    std::atomic<bool> physics_thread_running_{false};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Physics data stored in contiguous arrays for optimal iteration
- **Memory Ordering**: Physics updates use acquire-release semantics for thread safety
- **Lock-Free Sections**: Broad-phase collision detection and constraint solving are lock-free

### Public APIs

#### Primary Interface: `PhysicsSystemInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <functional>

namespace engine::physics {

template<typename T>
concept PhysicsShape = requires(T t) {
    { t.GetBoundingBox() } -> std::convertible_to<BoundingBox>;
    { t.GetArea() } -> std::convertible_to<float>;
    { t.GetType() } -> std::convertible_to<ShapeType>;
};

class PhysicsSystemInterface {
public:
    [[nodiscard]] auto Initialize(const PhysicsConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Physics world management
    void SetGravity(const Vec2& gravity) noexcept;
    [[nodiscard]] auto GetGravity() const noexcept -> Vec2;
    void SetTimeScale(float time_scale) noexcept;
    
    // Body management
    [[nodiscard]] auto CreateRigidBody(Entity entity, const RigidBodyDef& definition) -> std::optional<PhysicsBodyId>;
    void DestroyRigidBody(PhysicsBodyId body_id) noexcept;
    
    // Shape management  
    template<PhysicsShape T>
    [[nodiscard]] auto CreateShape(const T& shape_def) -> std::optional<CollisionShapeId>;
    void DestroyShape(CollisionShapeId shape_id) noexcept;
    
    // Forces and impulses
    void ApplyForce(PhysicsBodyId body_id, const Vec2& force, const Vec2& point) noexcept;
    void ApplyImpulse(PhysicsBodyId body_id, const Vec2& impulse, const Vec2& point) noexcept;
    void SetVelocity(PhysicsBodyId body_id, const Vec2& linear_velocity, float angular_velocity) noexcept;
    
    // Collision queries
    [[nodiscard]] auto RaycastFirst(const Vec2& origin, const Vec2& direction, 
                                   float max_distance) const -> std::optional<RaycastHit>;
    [[nodiscard]] auto RaycastAll(const Vec2& origin, const Vec2& direction, 
                                 float max_distance) const -> std::vector<RaycastHit>;
    [[nodiscard]] auto OverlapShape(const CollisionShape& shape, 
                                   const Transform2D& transform) const -> std::vector<Entity>;
    
    // Collision event system
    [[nodiscard]] auto RegisterCollisionCallback(std::function<void(const ContactInfo&)> callback) -> CallbackId;
    void UnregisterCollisionCallback(CallbackId id) noexcept;
    
    // Performance monitoring
    [[nodiscard]] auto GetPhysicsStats() const noexcept -> PhysicsStatistics;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<PhysicsSystemImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::physics
```

#### Scripting Interface Requirements

```cpp
// Physics scripting interface for dynamic physics control
class PhysicsScriptInterface {
public:
    // Type-safe physics manipulation from scripts
    void ApplyForce(std::uint32_t entity_id, float force_x, float force_y);
    void ApplyImpulse(std::uint32_t entity_id, float impulse_x, float impulse_y);
    void SetVelocity(std::uint32_t entity_id, float vel_x, float vel_y);
    
    // Physics queries
    [[nodiscard]] auto GetVelocity(std::uint32_t entity_id) const -> std::pair<float, float>;
    [[nodiscard]] auto GetMass(std::uint32_t entity_id) const -> float;
    
    // Collision detection
    [[nodiscard]] auto Raycast(float origin_x, float origin_y, float dir_x, float dir_y, 
                              float max_distance) const -> std::optional<std::uint32_t>;
    [[nodiscard]] auto CheckCollision(std::uint32_t entity1, std::uint32_t entity2) const -> bool;
    
    // World settings
    void SetGravity(float gravity_x, float gravity_y);
    [[nodiscard]] auto GetGravity() const -> std::pair<float, float>;
    
    // Event registration
    [[nodiscard]] auto OnCollision(std::uint32_t entity_id, 
                                  std::string_view callback_function) -> bool;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<PhysicsSystemInterface> physics_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Rigid Body Dynamics**: Accurate linear and angular physics simulation with proper momentum conservation
- [ ] **Collision Detection**: Reliable collision detection between all supported shape types
- [ ] **Constraint Solving**: Stable contact resolution with minimal penetration artifacts

### Performance Requirements

- [ ] **Simulation Scale**: Handle 1,000+ physics bodies at 60 FPS with deterministic results
- [ ] **Collision Accuracy**: Collision penetration depth under 0.1 world units
- [ ] **Memory Usage**: Total physics memory under 64MB for typical gameplay scenarios
- [ ] **Update Time**: Physics simulation under 8ms per frame (50% of 16ms budget)

### Quality Requirements

- [ ] **Determinism**: Identical simulation results across multiple runs with same inputs
- [ ] **Stability**: Zero NaN or infinite values in physics calculations
- [ ] **Maintainability**: All physics code covered by unit tests
- [ ] **Documentation**: Complete physics parameter reference and tuning guide

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(PhysicsTest, SimulationPerformance) {
    auto physics_system = engine::physics::PhysicsSystem{};
    physics_system.Initialize(PhysicsConfig{});
    
    // Create many physics bodies
    for (int i = 0; i < 1000; ++i) {
        auto entity = CreateTestEntity();
        RigidBodyDef body_def;
        body_def.type = BodyType::Dynamic;
        body_def.position = Vec2{static_cast<float>(i % 100), static_cast<float>(i / 100)};
        
        auto body_id = physics_system.CreateRigidBody(entity, body_def);
        ASSERT_TRUE(body_id.has_value());
    }
    
    // Measure simulation performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 60; ++frame) {
        physics_system.Update(1.0f / 60.0f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    auto avg_frame_time = total_time / 60.0f;
    
    EXPECT_LT(avg_frame_time, 8.0f); // Under 8ms per frame
}

TEST(PhysicsTest, CollisionAccuracy) {
    auto physics_system = engine::physics::PhysicsSystem{};
    physics_system.Initialize(PhysicsConfig{});
    
    // Create two overlapping boxes
    auto entity1 = CreateTestEntity();
    auto entity2 = CreateTestEntity();
    
    RigidBodyDef body1_def;
    body1_def.position = Vec2{0.0f, 0.0f};
    auto body1_id = physics_system.CreateRigidBody(entity1, body1_def);
    
    RigidBodyDef body2_def;
    body2_def.position = Vec2{0.5f, 0.0f}; // Slight overlap
    auto body2_id = physics_system.CreateRigidBody(entity2, body2_def);
    
    // Run simulation until separation
    for (int i = 0; i < 120; ++i) { // 2 seconds at 60 FPS
        physics_system.Update(1.0f / 60.0f);
    }
    
    // Check final separation
    auto body1_pos = physics_system.GetBodyPosition(*body1_id);
    auto body2_pos = physics_system.GetBodyPosition(*body2_id);
    float separation = glm::distance(body1_pos, body2_pos);
    
    EXPECT_GT(separation, 1.0f); // Bodies should be separated by at least 1 unit
    EXPECT_LT(separation, 2.0f); // But not too far apart (stable resolution)
}

TEST(PhysicsTest, Determinism) {
    auto physics_system1 = engine::physics::PhysicsSystem{};
    auto physics_system2 = engine::physics::PhysicsSystem{};
    
    PhysicsConfig config;
    config.deterministic_mode = true;
    physics_system1.Initialize(config);
    physics_system2.Initialize(config);
    
    // Create identical scenarios in both systems
    SetupIdenticalPhysicsScenario(physics_system1);
    SetupIdenticalPhysicsScenario(physics_system2);
    
    // Run identical simulations
    for (int frame = 0; frame < 600; ++frame) { // 10 seconds
        physics_system1.Update(1.0f / 60.0f);
        physics_system2.Update(1.0f / 60.0f);
    }
    
    // Compare final states
    auto state1 = ExtractPhysicsState(physics_system1);
    auto state2 = ExtractPhysicsState(physics_system2);
    
    EXPECT_EQ(state1, state2); // Identical results
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Physics Engine (Estimated: 7 days)

- [ ] **Task 1.1**: Implement basic rigid body dynamics and integration
- [ ] **Task 1.2**: Add collision shape primitives (circle, box, polygon)
- [ ] **Task 1.3**: Create broad-phase collision detection with spatial hashing
- [ ] **Deliverable**: Basic physics simulation with simple shapes

#### Phase 2: Collision Resolution (Estimated: 5 days)

- [ ] **Task 2.1**: Implement narrow-phase collision detection algorithms
- [ ] **Task 2.2**: Add contact constraint solver with position correction
- [ ] **Task 2.3**: Implement physics materials and surface properties
- [ ] **Deliverable**: Stable collision response and resolution

#### Phase 3: ECS Integration (Estimated: 4 days)

- [ ] **Task 3.1**: Create physics component system and data synchronization
- [ ] **Task 3.2**: Implement thread-safe communication between physics and main threads
- [ ] **Task 3.3**: Add collision event system for gameplay integration
- [ ] **Deliverable**: Full ECS integration with event-driven collision handling

#### Phase 4: Optimization & Features (Estimated: 3 days)

- [ ] **Task 4.1**: Add continuous collision detection for fast objects
- [ ] **Task 4.2**: Implement spatial query system (raycasting, overlap tests)
- [ ] **Task 4.3**: Add physics debugging visualization and performance profiling
- [ ] **Deliverable**: Production-ready physics with debugging tools

### File Structure

```
src/engine/physics/
├── physics.cppm                // Primary module interface
├── physics_world.cpp          // Physics world management
├── rigid_body.cpp             // Rigid body dynamics and integration
├── collision_detection.cpp    // Broad and narrow-phase collision detection
├── constraint_solver.cpp      // Contact constraint resolution
├── collision_shapes.cpp       // Shape primitives and queries
├── physics_materials.cpp      // Material properties and surface types
├── spatial_hash.cpp           // Spatial partitioning for broad-phase
├── continuous_collision.cpp   // CCD for fast-moving objects
└── tests/
    ├── physics_world_tests.cpp
    ├── collision_tests.cpp
    └── physics_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::physics`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with specialized physics algorithms
- **Build Integration**: Links with engine.ecs, engine.scene, engine.threading

### Testing Strategy

- **Unit Tests**: Isolated testing of physics algorithms with known expected results
- **Integration Tests**: Full physics simulation testing with ECS components
- **Performance Tests**: Simulation scale testing and timing benchmarks
- **Determinism Tests**: Cross-platform identical result validation

## Risk Assessment

### Technical Risks

| Risk                        | Probability | Impact | Mitigation                                                         |
|-----------------------------|-------------|--------|--------------------------------------------------------------------|
| **Numerical Instability**   | Medium      | High   | Stable integration schemes, constraint stabilization               |
| **Performance Degradation** | Medium      | High   | Spatial optimization, efficient broad-phase algorithms             |
| **Threading Complexity**    | Low         | Medium | Clear thread boundaries, lock-free data structures                 |
| **Platform Differences**    | Low         | Low    | Deterministic floating-point operations, consistent math libraries |

### Integration Risks

- **ECS Performance**: Risk that physics component updates impact frame rate
    - *Mitigation*: Efficient component synchronization, batched updates
- **Memory Usage**: Risk of excessive memory usage with many physics bodies
    - *Mitigation*: Object pooling, spatial optimization, sleeping bodies

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (timing, memory management)
    - engine.ecs (component access, entity management)
    - engine.threading (physics thread management)
    - engine.scene (spatial queries, transform synchronization)

- **Optional Systems**:
    - engine.profiling (physics performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 atomic operations, mathematical functions
- **Math Libraries**: GLM or similar for vector/matrix operations

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.threading, engine.scene
- **vcpkg Packages**: glm (for mathematical operations)
- **Platform-Specific**: Platform-optimized math libraries where available
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.threading, engine.scene

### Asset Pipeline Dependencies

- **Physics Materials**: JSON configuration for surface properties and physics parameters
- **Configuration Files**: Physics settings following existing JSON patterns
- **Resource Loading**: Physics configuration loaded through engine.assets
