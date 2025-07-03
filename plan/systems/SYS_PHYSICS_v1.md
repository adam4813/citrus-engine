# SYS_PHYSICS_v1

> **System-Level Design Document for Physics System**

## Executive Summary

This document defines a lightweight, efficient physics system designed specifically for colony simulation games with
thousands of entities. The physics system integrates seamlessly with the ECS architecture, providing movement, collision
detection, and basic force simulation while maintaining 60+ FPS performance. The design emphasizes simplicity and
performance over complex physics simulations, focusing on the specific needs of top-down 2D games with AI-driven
entities that require pathfinding, collision avoidance, and target-seeking behaviors.

## Scope and Objectives

### In Scope

- [ ] 2D physics simulation with position, velocity, and acceleration
- [ ] AABB (Axis-Aligned Bounding Box) collision detection with spatial optimization
- [ ] AI-driven target seeking with proper acceleration and arrival behaviors
- [ ] Simple collision response with velocity dampening and separation
- [ ] Spatial partitioning using grid-based optimization for collision detection
- [ ] Force application system for AI movement and environmental effects
- [ ] Physics event system for collision notifications and target achievements
- [ ] Integration with existing ECS components (Transform, Velocity, Shape, Target)

### Out of Scope

- [ ] Complex 3D physics simulation (focus on 2D top-down gameplay)
- [ ] Realistic physics materials and friction coefficients
- [ ] Advanced collision shapes (polygons, circles) in initial implementation
- [ ] Physics joints and constraints systems
- [ ] Fluid dynamics or particle physics
- [ ] Integration with external physics engines (Box2D, Bullet, etc.)

### Primary Objectives

1. **Performance**: Handle 5,000+ entities with collision detection at 60+ FPS
2. **AI Integration**: Seamless integration with utility AI and GOAP systems through events
3. **Simplicity**: Lightweight implementation focused on colony game requirements
4. **Scalability**: Spatial optimization allows for growing entity counts

### Secondary Objectives

- Deterministic behavior for save/load consistency
- Debug visualization integration for development
- Tunable physics parameters through configuration
- Future extensibility for more complex physics features

## Architecture/Design

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Game AI Layer                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │ Utility AI  │  │    GOAP     │  │ Pathfinding │             │
│  │   System    │  │   System    │  │   System    │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
│         │                │                │                    │
│         └── Target ──────┼─── Force ──────┘                    │
│           Component      │ Component                           │
├─────────────────────────────────────────────────────────────────┤
│                    Physics System                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │  Movement   │  │  Collision  │  │   Force     │             │
│  │ Integration │  │ Detection   │  │Application  │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
│         │                │                │                    │
│         └────────────── ECS ──────────────┘                    │
│                    Components                                  │
├─────────────────────────────────────────────────────────────────┤
│                  Spatial Optimization                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │    Grid     │  │   Broad     │  │   Narrow    │             │
│  │ Partitioning│  │   Phase     │  │    Phase    │             │
│  └─────────────┘  └─────────────┘  └─────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Movement Integration

- **Purpose**: Apply velocities and forces to entity positions using physics integration
- **Responsibilities**: Position updates, velocity integration, drag application, speed limiting
- **Key Classes/Interfaces**: `MovementIntegrator`, `ForceAccumulator`
- **Data Flow**: Forces accumulated → Velocity updated → Position integrated → Transform updated

#### Component 2: Collision Detection

- **Purpose**: Efficient detection of entity collisions using spatial optimization
- **Responsibilities**: Spatial grid management, broad-phase collision detection, narrow-phase AABB testing
- **Key Classes/Interfaces**: `CollisionDetector`, `SpatialGrid`, `CollisionPair`
- **Data Flow**: Entities in grid → Broad phase candidates → Narrow phase testing → Collision events

#### Component 3: Force Application

- **Purpose**: Apply various forces to entities for AI movement and environmental effects
- **Responsibilities**: Target seeking forces, separation forces, environmental forces, force management
- **Key Classes/Interfaces**: `ForceGenerator`, `TargetSeeker`, `SeparationForce`
- **Data Flow**: AI sets targets → Forces calculated → Forces accumulated → Applied to velocity

#### Component 4: Spatial Optimization

- **Purpose**: Optimize collision detection performance through spatial partitioning
- **Responsibilities**: Grid cell management, entity tracking, spatial queries, performance monitoring
- **Key Classes/Interfaces**: `SpatialGrid`, `GridCell`, `SpatialQuery`
- **Data Flow**: Entities update positions → Grid updated → Collision queries optimized

### Physics Architecture Design

#### Component Integration with ECS

```cpp
// Enhanced physics components building on ECS foundation
struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    bool dirty = true;
    
    // Physics integration
    glm::vec3 previous_position{0.0f};  // For collision response
};

struct VelocityComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 acceleration{0.0f};
    float max_speed = 5.0f;
    float drag = 0.98f;  // Velocity multiplier per frame (0.98 = 2% drag)
    
    // Physics state
    bool kinematic = false;  // If true, not affected by forces
    float mass = 1.0f;      // For future force calculations
};

struct ShapeComponent {
    enum class Type { Box, Circle };
    Type shape_type = Type::Box;
    glm::vec2 size{1.0f};     // width/height for box, radius for circle
    glm::vec2 offset{0.0f};   // Local offset from transform center
    
    // Collision properties
    bool is_trigger = false;         // Ghost collision (events only)
    bool is_static = false;          // Never moves (structures)
    uint32_t collision_layer = 1;    // What layer this object is on
    uint32_t collision_mask = 0xFFFFFFFF;  // What layers it collides with
    
    // Physics material
    float restitution = 0.0f;        // Bounciness (0 = no bounce, 1 = perfect bounce)
    float friction = 0.5f;           // Surface friction for sliding
};

struct TargetComponent {
    glm::vec3 target_position{0.0f};
    EntityId target_entity = INVALID_ENTITY;
    enum class Type { Position, Entity, None } type = Type::None;
    
    // Steering behavior parameters
    float arrival_threshold = 0.5f;   // Stop when this close to target
    float arrival_radius = 2.0f;      // Start slowing down at this distance
    float target_speed = 3.0f;        // Desired speed when moving to target
    float steering_force = 10.0f;     // How strong the steering force is
    
    // State tracking
    bool target_reached = false;
    float time_at_target = 0.0f;
};

// New component for force accumulation
struct ForceComponent {
    glm::vec3 accumulated_force{0.0f};
    
    // Force parameters
    float max_force = 15.0f;          // Maximum total force that can be applied
    bool reset_each_frame = true;     // Clear forces after each physics update
    
    // Debug information
    std::vector<std::pair<std::string, glm::vec3>> debug_forces;  // For visualization
};

// Physics-specific component traits
template<>
struct ComponentTraits<ForceComponent> {
    static constexpr bool is_trivially_copyable = false;  // Contains std::vector
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(glm::vec3);
    static constexpr bool enable_pooling = true;
};
```

#### Physics System Implementation

```cpp
class PhysicsSystem : public ISystem {
public:
    [[nodiscard]] auto Initialize() -> std::optional<std::string> override {
        // Initialize spatial grid based on expected world size
        spatial_grid_ = std::make_unique<SpatialGrid>(config_.world_bounds, config_.grid_cell_size);
        
        // Initialize physics configuration
        physics_config_ = {
            .gravity = glm::vec3{0.0f, 0.0f, 0.0f},  // No gravity for top-down
            .time_step = 1.0f / 60.0f,               // Fixed 60 FPS timestep
            .position_iterations = 1,                // Simple integration
            .velocity_iterations = 1
        };
        
        return std::nullopt;  // Success
    }
    
    void Shutdown() noexcept override {
        spatial_grid_.reset();
    }
    
    void Update(double delta_time, const FrameContext& context) override {
        PROFILE_SCOPE("PhysicsSystem::Update");
        
        // Step 1: Apply forces and update velocities
        UpdateForces(delta_time, context);
        
        // Step 2: Integrate movement (velocity to position)
        IntegrateMovement(delta_time, context);
        
        // Step 3: Update spatial grid with new positions
        UpdateSpatialGrid(context);
        
        // Step 4: Detect and resolve collisions
        DetectCollisions(context);
        
        // Step 5: Publish physics events
        PublishPhysicsEvents(context);
        
        // Step 6: Clear accumulated forces for next frame
        ClearForces(context);
        
        // Update performance metrics
        UpdatePerformanceMetrics(delta_time);
    }
    
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .execution_phase = ExecutionPhase::Update,
            .can_run_parallel = false,  // Single-threaded for spatial grid consistency
            .read_components = {
                ComponentType::Get<TargetComponent>(), 
                ComponentType::Get<ShapeComponent>()
            },
            .write_components = {
                ComponentType::Get<TransformComponent>(), 
                ComponentType::Get<VelocityComponent>(),
                ComponentType::Get<ForceComponent>()
            }
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
    void UpdateForces(double delta_time, const FrameContext& context) {
        PROFILE_SCOPE("PhysicsSystem::UpdateForces");
        
        // Apply target seeking forces
        auto target_query = context.world->Query<TransformComponent, VelocityComponent, TargetComponent, ForceComponent>();
        
        for (auto [entity, transform, velocity, target, force] : target_query) {
            if (target.type == TargetComponent::Type::Position) {
                auto steering_force = CalculateSteeringForce(transform, velocity, target);
                force.accumulated_force += steering_force;
                
                // Debug tracking
                if constexpr (PHYSICS_DEBUG_ENABLED) {
                    force.debug_forces.emplace_back("Steering", steering_force);
                }
            }
        }
        
        // Apply separation forces (avoid crowding)
        auto crowd_query = context.world->Query<TransformComponent, VelocityComponent, ForceComponent, ShapeComponent>();
        
        for (auto [entity, transform, velocity, force, shape] : crowd_query) {
            if (!shape.is_static) {
                auto separation_force = CalculateSeparationForce(entity, transform, context);
                force.accumulated_force += separation_force;
                
                if constexpr (PHYSICS_DEBUG_ENABLED) {
                    force.debug_forces.emplace_back("Separation", separation_force);
                }
            }
        }
    }
    
    void IntegrateMovement(double delta_time, const FrameContext& context) {
        PROFILE_SCOPE("PhysicsSystem::IntegrateMovement");
        
        auto movement_query = context.world->Query<TransformComponent, VelocityComponent, ForceComponent>();
        
        for (auto [entity, transform, velocity, force] : movement_query) {
            if (velocity.kinematic) {
                continue;  // Kinematic objects don't respond to forces
            }
            
            // Store previous position for collision response
            transform.previous_position = transform.position;
            
            // Apply accumulated forces to acceleration (F = ma, assume mass = 1 for simplicity)
            velocity.acceleration = force.accumulated_force / velocity.mass;
            
            // Clamp acceleration to reasonable limits
            float max_acceleration = 50.0f;  // Configurable
            if (glm::length(velocity.acceleration) > max_acceleration) {
                velocity.acceleration = glm::normalize(velocity.acceleration) * max_acceleration;
            }
            
            // Integrate acceleration to velocity
            velocity.velocity += velocity.acceleration * static_cast<float>(delta_time);
            
            // Apply drag
            velocity.velocity *= velocity.drag;
            
            // Clamp velocity to max speed
            if (glm::length(velocity.velocity) > velocity.max_speed) {
                velocity.velocity = glm::normalize(velocity.velocity) * velocity.max_speed;
            }
            
            // Integrate velocity to position
            transform.position += velocity.velocity * static_cast<float>(delta_time);
            transform.dirty = true;
        }
    }
    
    void UpdateSpatialGrid(const FrameContext& context) {
        PROFILE_SCOPE("PhysicsSystem::UpdateSpatialGrid");
        
        // Clear previous frame's grid
        spatial_grid_->Clear();
        
        // Add all entities with shapes to the spatial grid
        auto shape_query = context.world->Query<TransformComponent, ShapeComponent>();
        
        for (auto [entity, transform, shape] : shape_query) {
            // Calculate AABB for the entity
            auto bounds = CalculateAABB(transform, shape);
            spatial_grid_->Insert(entity, bounds);
        }
    }
    
    void DetectCollisions(const FrameContext& context) {
        PROFILE_SCOPE("PhysicsSystem::DetectCollisions");
        
        collision_pairs_.clear();
        
        // Get all entities that moved this frame
        auto moving_query = context.world->Query<TransformComponent, VelocityComponent, ShapeComponent>();
        
        for (auto [entity, transform, velocity, shape] : moving_query) {
            if (glm::length(velocity.velocity) < 0.001f) {
                continue;  // Skip stationary entities
            }
            
            // Query spatial grid for potential collisions
            auto bounds = CalculateAABB(transform, shape);
            auto candidates = spatial_grid_->Query(bounds);
            
            for (EntityId candidate : candidates) {
                if (candidate == entity) continue;
                
                // Avoid duplicate pairs
                if (entity < candidate && 
                    collision_pairs_.find({entity, candidate}) == collision_pairs_.end()) {
                    
                    if (TestCollision(entity, candidate, context)) {
                        collision_pairs_.insert({entity, candidate});
                        ResolveCollision(entity, candidate, context);
                    }
                }
            }
        }
    }
    
    glm::vec3 CalculateSteeringForce(const TransformComponent& transform, 
                                   const VelocityComponent& velocity,
                                   const TargetComponent& target) {
        glm::vec3 to_target = target.target_position - transform.position;
        float distance = glm::length(to_target);
        
        if (distance < target.arrival_threshold) {
            // Close enough - stop
            return -velocity.velocity * 5.0f;  // Braking force
        }
        
        // Calculate desired velocity
        glm::vec3 desired_velocity;
        if (distance < target.arrival_radius) {
            // Slow down as we approach
            float speed_ratio = distance / target.arrival_radius;
            desired_velocity = glm::normalize(to_target) * target.target_speed * speed_ratio;
        } else {
            // Full speed ahead
            desired_velocity = glm::normalize(to_target) * target.target_speed;
        }
        
        // Steering force = desired velocity - current velocity
        glm::vec3 steering = desired_velocity - velocity.velocity;
        
        // Limit steering force
        if (glm::length(steering) > target.steering_force) {
            steering = glm::normalize(steering) * target.steering_force;
        }
        
        return steering;
    }
    
    glm::vec3 CalculateSeparationForce(EntityId entity, const TransformComponent& transform, 
                                     const FrameContext& context) {
        glm::vec3 separation_force{0.0f};
        constexpr float separation_radius = 1.5f;  // Configurable
        constexpr float separation_strength = 8.0f;
        
        // Query nearby entities
        AABB query_bounds = {
            transform.position - glm::vec3{separation_radius},
            transform.position + glm::vec3{separation_radius}
        };
        
        auto nearby_entities = spatial_grid_->Query(query_bounds);
        
        for (EntityId other : nearby_entities) {
            if (other == entity) continue;
            
            auto other_transform = context.world->GetComponent<TransformComponent>(other);
            if (!other_transform) continue;
            
            glm::vec3 to_other = other_transform->position - transform.position;
            float distance = glm::length(to_other);
            
            if (distance > 0.001f && distance < separation_radius) {
                // Apply separation force (inverse square falloff)
                float force_magnitude = separation_strength / (distance * distance);
                glm::vec3 force_direction = -glm::normalize(to_other);  // Away from other
                separation_force += force_direction * force_magnitude;
            }
        }
        
        return separation_force;
    }
    
    bool TestCollision(EntityId entity_a, EntityId entity_b, const FrameContext* context) {
        auto transform_a = context->world->GetComponent<TransformComponent>(entity_a);
        auto transform_b = context->world->GetComponent<TransformComponent>(entity_b);
        auto shape_a = context->world->GetComponent<ShapeComponent>(entity_a);
        auto shape_b = context->world->GetComponent<ShapeComponent>(entity_b);
        
        if (!transform_a || !transform_b || !shape_a || !shape_b) {
            return false;
        }
        
        // Check collision layers
        if ((shape_a->collision_layer & shape_b->collision_mask) == 0 &&
            (shape_b->collision_layer & shape_a->collision_mask) == 0) {
            return false;
        }
        
        // Simple AABB collision test
        auto bounds_a = CalculateAABB(*transform_a, *shape_a);
        auto bounds_b = CalculateAABB(*transform_b, *shape_b);
        
        return (bounds_a.min.x < bounds_b.max.x && bounds_a.max.x > bounds_b.min.x &&
                bounds_a.min.y < bounds_b.max.y && bounds_a.max.y > bounds_b.min.y);
    }
    
    void ResolveCollision(EntityId entity_a, EntityId entity_b, const FrameContext& context) {
        auto transform_a = context.world->GetComponent<TransformComponent>(entity_a);
        auto transform_b = context.world->GetComponent<TransformComponent>(entity_b);
        auto velocity_a = context.world->GetComponent<VelocityComponent>(entity_a);
        auto velocity_b = context.world->GetComponent<VelocityComponent>(entity_b);
        auto shape_a = context.world->GetComponent<ShapeComponent>(entity_a);
        auto shape_b = context.world->GetComponent<ShapeComponent>(entity_b);
        
        if (!transform_a || !transform_b || !velocity_a || !velocity_b || !shape_a || !shape_b) {
            return;
        }
        
        // Handle trigger collisions (no physical response, just events)
        if (shape_a->is_trigger || shape_b->is_trigger) {
            pending_collision_events_.emplace_back(CollisionEvent{
                .entity_a = entity_a,
                .entity_b = entity_b,
                .collision_point = (transform_a->position + transform_b->position) * 0.5f,
                .collision_normal = glm::normalize(transform_b->position - transform_a->position),
                .is_trigger = true
            });
            return;
        }
        
        // Calculate collision response
        glm::vec3 collision_normal = glm::normalize(transform_a->position - transform_b->position);
        
        // Simple collision response - separate entities and dampen velocities
        float separation_distance = 0.1f;  // Small separation to prevent overlap
        
        if (!shape_a->is_static && !shape_b->is_static) {
            // Both objects are dynamic - separate equally
            transform_a->position += collision_normal * separation_distance * 0.5f;
            transform_b->position -= collision_normal * separation_distance * 0.5f;
            
            // Exchange some velocity (simple elastic collision approximation)
            float restitution = (shape_a->restitution + shape_b->restitution) * 0.5f;
            float relative_velocity = glm::dot(velocity_a->velocity - velocity_b->velocity, collision_normal);
            
            if (relative_velocity > 0) {  // Objects moving apart
                return;
            }
            
            float impulse = -(1 + restitution) * relative_velocity * 0.5f;
            velocity_a->velocity += collision_normal * impulse;
            velocity_b->velocity -= collision_normal * impulse;
            
        } else if (shape_a->is_static) {
            // Entity A is static, only move B
            transform_b->position -= collision_normal * separation_distance;
            
            // Reflect B's velocity
            float restitution = shape_b->restitution;
            velocity_b->velocity -= collision_normal * glm::dot(velocity_b->velocity, collision_normal) * (1 + restitution);
            
        } else if (shape_b->is_static) {
            // Entity B is static, only move A
            transform_a->position += collision_normal * separation_distance;
            
            // Reflect A's velocity
            float restitution = shape_a->restitution;
            velocity_a->velocity -= collision_normal * glm::dot(velocity_a->velocity, collision_normal) * (1 + restitution);
        }
        
        // Mark transforms as dirty
        transform_a->dirty = true;
        transform_b->dirty = true;
        
        // Queue collision event
        pending_collision_events_.emplace_back(CollisionEvent{
            .entity_a = entity_a,
            .entity_b = entity_b,
            .collision_point = (transform_a->position + transform_b->position) * 0.5f,
            .collision_normal = collision_normal,
            .is_trigger = false
        });
    }
    
    void PublishPhysicsEvents(const FrameContext& context) {
        // Publish collision events
        for (const auto& collision : pending_collision_events_) {
            context.world->GetEventSystem().Publish(collision);
        }
        pending_collision_events_.clear();
        
        // Check for target reached events
        auto target_query = context.world->Query<TransformComponent, VelocityComponent, TargetComponent>();
        
        for (auto [entity, transform, velocity, target] : target_query) {
            if (target.type == TargetComponent::Type::Position && !target.target_reached) {
                float distance = glm::length(target.target_position - transform.position);
                
                if (distance < target.arrival_threshold) {
                    target.target_reached = true;
                    target.time_at_target = 0.0f;
                    
                    // Stop the entity
                    velocity.velocity = glm::vec3{0.0f};
                    
                    // Publish target reached event
                    context.world->GetEventSystem().Publish(EntityTargetReachedEvent{
                        .entity = entity,
                        .target_position = target.target_position,
                        .timestamp = context.current_time
                    });
                }
            }
        }
    }
    
    void ClearForces(const FrameContext& context) {
        auto force_query = context.world->Query<ForceComponent>();
        
        for (auto [entity, force] : force_query) {
            if (force.reset_each_frame) {
                force.accumulated_force = glm::vec3{0.0f};
                force.debug_forces.clear();
            }
        }
    }
    
    AABB CalculateAABB(const TransformComponent& transform, const ShapeComponent& shape) {
        glm::vec3 center = transform.position + glm::vec3{shape.offset, 0.0f};
        glm::vec3 half_size{shape.size * 0.5f, 0.0f};
        
        return AABB{
            .min = center - half_size,
            .max = center + half_size
        };
    }
    
    void UpdatePerformanceMetrics(double delta_time) {
        system_metrics_.frame_time = delta_time;
        system_metrics_.entities_processed = entities_processed_this_frame_;
        system_metrics_.collisions_detected = collision_pairs_.size();
        
        entities_processed_this_frame_ = 0;
    }
    
    // Configuration and state
    struct PhysicsConfig {
        glm::vec3 gravity{0.0f};
        float time_step = 1.0f / 60.0f;
        int position_iterations = 1;
        int velocity_iterations = 1;
        
        // World bounds for spatial grid
        AABB world_bounds{{-100.0f, -100.0f, 0.0f}, {100.0f, 100.0f, 0.0f}};
        float grid_cell_size = 5.0f;
    } config_;
    
    PhysicsConfig physics_config_;
    std::unique_ptr<SpatialGrid> spatial_grid_;
    std::set<std::pair<EntityId, EntityId>> collision_pairs_;
    std::vector<CollisionEvent> pending_collision_events_;
    SystemMetrics system_metrics_;
    size_t entities_processed_this_frame_ = 0;
    
    static constexpr bool PHYSICS_DEBUG_ENABLED = false;  // Compile-time debug flag
};
```

### Spatial Grid Implementation

```cpp
// Axis-Aligned Bounding Box structure
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    [[nodiscard]] bool Intersects(const AABB& other) const {
        return (min.x < other.max.x && max.x > other.min.x &&
                min.y < other.max.y && max.y > other.min.y);
    }
    
    [[nodiscard]] glm::vec3 Center() const {
        return (min + max) * 0.5f;
    }
    
    [[nodiscard]] glm::vec3 Size() const {
        return max - min;
    }
};

// Spatial grid for efficient collision detection
class SpatialGrid {
public:
    explicit SpatialGrid(const AABB& world_bounds, float cell_size)
        : world_bounds_(world_bounds)
        , cell_size_(cell_size)
        , inv_cell_size_(1.0f / cell_size) {
        
        // Calculate grid dimensions
        glm::vec3 world_size = world_bounds.Size();
        grid_width_ = static_cast<int>(std::ceil(world_size.x * inv_cell_size_));
        grid_height_ = static_cast<int>(std::ceil(world_size.y * inv_cell_size_));
        
        // Initialize grid cells
        grid_cells_.resize(grid_width_ * grid_height_);
    }
    
    void Clear() {
        for (auto& cell : grid_cells_) {
            cell.entities.clear();
        }
    }
    
    void Insert(EntityId entity, const AABB& bounds) {
        auto [min_cell, max_cell] = GetCellRange(bounds);
        
        for (int y = min_cell.y; y <= max_cell.y; ++y) {
            for (int x = min_cell.x; x <= max_cell.x; ++x) {
                if (IsValidCell(x, y)) {
                    GetCell(x, y).entities.push_back(entity);
                }
            }
        }
    }
    
    [[nodiscard]] std::vector<EntityId> Query(const AABB& bounds) const {
        std::set<EntityId> unique_entities;  // Avoid duplicates
        auto [min_cell, max_cell] = GetCellRange(bounds);
        
        for (int y = min_cell.y; y <= max_cell.y; ++y) {
            for (int x = min_cell.x; x <= max_cell.x; ++x) {
                if (IsValidCell(x, y)) {
                    const auto& cell = GetCell(x, y);
                    for (EntityId entity : cell.entities) {
                        unique_entities.insert(entity);
                    }
                }
            }
        }
        
        return std::vector<EntityId>(unique_entities.begin(), unique_entities.end());
    }
    
    [[nodiscard]] size_t GetEntityCount() const {
        size_t total = 0;
        for (const auto& cell : grid_cells_) {
            total += cell.entities.size();
        }
        return total;
    }

private:
    struct GridCell {
        std::vector<EntityId> entities;
    };
    
    struct CellCoord {
        int x, y;
    };
    
    [[nodiscard]] CellCoord WorldToCell(const glm::vec3& world_pos) const {
        glm::vec3 relative = world_pos - world_bounds_.min;
        return CellCoord{
            .x = static_cast<int>(relative.x * inv_cell_size_),
            .y = static_cast<int>(relative.y * inv_cell_size_)
        };
    }
    
    [[nodiscard]] std::pair<CellCoord, CellCoord> GetCellRange(const AABB& bounds) const {
        auto min_cell = WorldToCell(bounds.min);
        auto max_cell = WorldToCell(bounds.max);
        
        // Clamp to grid bounds
        min_cell.x = std::max(0, min_cell.x);
        min_cell.y = std::max(0, min_cell.y);
        max_cell.x = std::min(grid_width_ - 1, max_cell.x);
        max_cell.y = std::min(grid_height_ - 1, max_cell.y);
        
        return {min_cell, max_cell};
    }
    
    [[nodiscard]] bool IsValidCell(int x, int y) const {
        return x >= 0 && x < grid_width_ && y >= 0 && y < grid_height_;
    }
    
    [[nodiscard]] GridCell& GetCell(int x, int y) {
        return grid_cells_[y * grid_width_ + x];
    }
    
    [[nodiscard]] const GridCell& GetCell(int x, int y) const {
        return grid_cells_[y * grid_width_ + x];
    }
    
    AABB world_bounds_;
    float cell_size_;
    float inv_cell_size_;
    int grid_width_;
    int grid_height_;
    std::vector<GridCell> grid_cells_;
};
```

### Physics Events

```cpp
// Enhanced physics events for AI integration
struct CollisionEvent {
    EntityId entity_a;
    EntityId entity_b;
    glm::vec3 collision_point;
    glm::vec3 collision_normal;
    bool is_trigger = false;
    float collision_impulse = 0.0f;
};

struct EntityTargetReachedEvent {
    EntityId entity;
    glm::vec3 target_position;
    double timestamp;
    float time_to_reach;  // How long it took to reach the target
};

struct EntityStuckEvent {
    EntityId entity;
    glm::vec3 stuck_position;
    float stuck_duration;
    double timestamp;
};

struct PhysicsDebugEvent {
    EntityId entity;
    std::string debug_info;
    glm::vec3 world_position;
    std::vector<std::pair<std::string, glm::vec3>> force_vectors;
};
```

## Dependencies

### Internal Dependencies

- **Required Systems**: ECS Core system must be implemented first
- **Optional Systems**: Rendering system for debug visualization, AI systems for force application
- **Circular Dependencies**: None by design

### External Dependencies

- **Third-Party Libraries**:
    - `glm` for mathematics (already in project)
    - Standard library containers (`std::vector`, `std::set`)
- **Standard Library Features**: C++20 concepts, ranges, optional for component access
- **Platform APIs**: None directly

### Build System Dependencies

- **CMake Targets**: Engine ECS, math utilities
- **vcpkg Packages**: No additional packages required
- **Platform-Specific**: None
- **Module Dependencies**: Will be exposed through `engine.physics.cppm` module interface

### Asset Pipeline Dependencies

- **Asset Formats**: None directly
- **Configuration Files**: Physics parameters through JSON configuration
- **Resource Loading**: None directly

### Reference Implementation Examples

- **Existing Code Patterns**: Current collision detection in game code
- **Anti-Patterns**: Avoid O(n²) collision detection without spatial optimization
- **Integration Points**: AI systems will write to TargetComponent and ForceComponent

## Success Criteria

### Functional Requirements

- [ ] **Movement Integration**: Smooth acceleration-based movement for AI-driven entities
- [ ] **Collision Detection**: Accurate AABB collision detection with spatial optimization
- [ ] **Target Seeking**: AI entities reach targets with proper arrival behaviors
- [ ] **Force Application**: Multiple forces can be applied and accumulated properly
- [ ] **Event System**: Collision and target events are published for AI integration
- [ ] **Spatial Optimization**: O(n) collision detection performance through spatial grid

### Performance Requirements

- [ ] **Frame Rate**: Maintain 60+ FPS with 5,000 active entities
- [ ] **Collision Performance**: Handle 1,000+ moving entities with <5ms collision detection time
- [ ] **Memory Efficiency**: Spatial grid overhead under 10% of total physics memory
- [ ] **Force Calculation**: Apply forces to 1,000+ entities in <2ms
- [ ] **Integration Speed**: Position integration for all entities in <1ms

### Quality Requirements

- [ ] **Determinism**: Identical results across runs with same input (important for save/load)
- [ ] **Stability**: No entity tunneling or explosive behaviors under normal conditions
- [ ] **Debug Support**: Comprehensive debug visualization and metrics
- [ ] **Configurability**: Key physics parameters tunable through configuration files

### Acceptance Tests

```cpp
// Performance validation for large entity counts
TEST(PhysicsSystem, HandlesLargeEntityCounts) {
    World world;
    auto physics_system = std::make_unique<PhysicsSystem>();
    world.RegisterSystem(std::move(physics_system));
    
    // Create 5,000 entities with physics components
    std::vector<EntityId> entities;
    for (int i = 0; i < 5000; ++i) {
        auto entity = world.CreateEntity();
        world.AddComponent<TransformComponent>(entity, {
            .position = glm::vec3{
                static_cast<float>(i % 100), 
                static_cast<float>(i / 100), 
                0.0f
            }
        });
        world.AddComponent<VelocityComponent>(entity, {
            .velocity = glm::vec3{
                (rand() % 200 - 100) / 100.0f,
                (rand() % 200 - 100) / 100.0f,
                0.0f
            },
            .max_speed = 5.0f
        });
        world.AddComponent<ShapeComponent>(entity, {
            .shape_type = ShapeComponent::Type::Box,
            .size = glm::vec2{0.8f, 0.8f}
        });
        world.AddComponent<ForceComponent>(entity, {});
        entities.push_back(entity);
    }
    
    // Measure physics update performance
    FrameContext context{.world = &world, .current_time = 0.0};
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 60; ++frame) {
        world.Update(1.0/60.0, context);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto avg_frame_time = total_time.count() / 60.0;
    
    EXPECT_LT(avg_frame_time, 16.67);  // Must maintain 60 FPS
}

// Target seeking behavior validation
TEST(PhysicsSystem, TargetSeekingBehavior) {
    World world;
    auto physics_system = std::make_unique<PhysicsSystem>();
    world.RegisterSystem(std::move(physics_system));
    
    // Create entity with target
    auto entity = world.CreateEntity();
    glm::vec3 start_pos{0.0f, 0.0f, 0.0f};
    glm::vec3 target_pos{10.0f, 0.0f, 0.0f};
    
    world.AddComponent<TransformComponent>(entity, {.position = start_pos});
    world.AddComponent<VelocityComponent>(entity, {.max_speed = 5.0f});
    world.AddComponent<TargetComponent>(entity, {
        .target_position = target_pos,
        .type = TargetComponent::Type::Position,
        .arrival_threshold = 0.5f,
        .target_speed = 3.0f
    });
    world.AddComponent<ForceComponent>(entity, {});
    
    // Track target reached event
    bool target_reached = false;
    world.GetEventSystem().Subscribe<EntityTargetReachedEvent>(
        [&](const EntityTargetReachedEvent& event) {
            if (event.entity == entity) {
                target_reached = true;
            }
        }
    );
    
    // Simulate until target is reached
    FrameContext context{.world = &world, .current_time = 0.0};
    for (int frame = 0; frame < 300 && !target_reached; ++frame) {  // 5 seconds max
        world.Update(1.0/60.0, context);
        context.current_time += 1.0/60.0;
    }
    
    EXPECT_TRUE(target_reached);
    
    // Verify entity is close to target
    auto transform = world.GetComponent<TransformComponent>(entity);
    ASSERT_TRUE(transform);
    float distance = glm::length(transform->position - target_pos);
    EXPECT_LT(distance, 0.6f);  // Should be within arrival threshold
}

// Collision detection and response validation
TEST(PhysicsSystem, CollisionDetectionAndResponse) {
    World world;
    auto physics_system = std::make_unique<PhysicsSystem>();
    world.RegisterSystem(std::move(physics_system));
    
    // Create two entities on collision course
    auto entity_a = world.CreateEntity();
    auto entity_b = world.CreateEntity();
    
    world.AddComponent<TransformComponent>(entity_a, {.position = {-2.0f, 0.0f, 0.0f}});
    world.AddComponent<VelocityComponent>(entity_a, {.velocity = {2.0f, 0.0f, 0.0f}});
    world.AddComponent<ShapeComponent>(entity_a, {.size = {1.0f, 1.0f}});
    world.AddComponent<ForceComponent>(entity_a, {});
    
    world.AddComponent<TransformComponent>(entity_b, {.position = {2.0f, 0.0f, 0.0f}});
    world.AddComponent<VelocityComponent>(entity_b, {.velocity = {-2.0f, 0.0f, 0.0f}});
    world.AddComponent<ShapeComponent>(entity_b, {.size = {1.0f, 1.0f}});
    world.AddComponent<ForceComponent>(entity_b, {});
    
    // Track collision events
    bool collision_detected = false;
    world.GetEventSystem().Subscribe<CollisionEvent>(
        [&](const CollisionEvent& event) {
            collision_detected = true;
        }
    );
    
    // Simulate until collision occurs
    FrameContext context{.world = &world, .current_time = 0.0};
    for (int frame = 0; frame < 120 && !collision_detected; ++frame) {  // 2 seconds max
        world.Update(1.0/60.0, context);
    }
    
    EXPECT_TRUE(collision_detected);
    
    // Verify entities separated after collision
    auto transform_a = world.GetComponent<TransformComponent>(entity_a);
    auto transform_b = world.GetComponent<TransformComponent>(entity_b);
    ASSERT_TRUE(transform_a && transform_b);
    
    float distance = glm::length(transform_a->position - transform_b->position);
    EXPECT_GT(distance, 1.0f);  // Should be separated
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Physics Foundation (Estimated: 4 days)

- [ ] **Task 1.1**: Enhanced component definitions and traits (0.5 days)
- [ ] **Task 1.2**: Basic movement integration with force accumulation (1 day)
- [ ] **Task 1.3**: Target seeking force calculation (1 day)
- [ ] **Task 1.4**: Simple collision detection without spatial optimization (1 day)
- [ ] **Task 1.5**: Physics event system integration (0.5 days)
- [ ] **Deliverable**: Working physics system with basic movement and collision

#### Phase 2: Spatial Optimization (Estimated: 3 days)

- [ ] **Task 2.1**: Spatial grid implementation and testing (2 days)
- [ ] **Task 2.2**: Integration with collision detection system (0.5 days)
- [ ] **Task 2.3**: Performance optimization and tuning (0.5 days)
- [ ] **Deliverable**: Optimized physics system capable of handling thousands of entities

#### Phase 3: Advanced Features and Polish (Estimated: 3 days)

- [ ] **Task 3.1**: Separation forces for crowd avoidance (1 day)
- [ ] **Task 3.2**: Physics configuration system (0.5 days)
- [ ] **Task 3.3**: Debug visualization and metrics (1 day)
- [ ] **Task 3.4**: Integration testing with colony game (0.5 days)
- [ ] **Deliverable**: Production-ready physics system integrated with colony game

### File Structure

```
src/engine/physics/
├── include/
│   ├── PhysicsSystem.h
│   ├── SpatialGrid.h
│   ├── PhysicsComponents.h
│   ├── PhysicsEvents.h
│   ├── ForceGenerators.h
│   └── PhysicsTypes.h
├── src/
│   ├── PhysicsSystem.cpp
│   ├── SpatialGrid.cpp
│   ├── ForceGenerators.cpp
│   └── CollisionDetection.cpp
└── tests/
    ├── PhysicsSystemTests.cpp
    ├── SpatialGridTests.cpp
    ├── CollisionTests.cpp
    ├── ForceTests.cpp
    └── PerformanceTests.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::physics` for all physics-related classes
- **Header Guards**: Use `#pragma once` for all headers
- **Module Structure**: Public interfaces exposed through `engine.physics.cppm`
- **Build Integration**: Physics as separate CMake target `EnginePhysics` depending on `EngineECS`

### Testing Strategy

- **Unit Tests**: Individual testing for spatial grid, force generators, collision detection
- **Integration Tests**: Full physics system workflow with ECS integration
- **Performance Tests**: Benchmarking with large entity counts and complex scenarios
- **Behavior Tests**: Validation of physics behaviors like target seeking and collision response

## Risk Assessment

### Technical Risks

| Risk                         | Probability | Impact | Mitigation                                              |
|------------------------------|-------------|--------|---------------------------------------------------------|
| **Spatial Grid Performance** | Low         | High   | Extensive benchmarking, configurable cell sizes         |
| **Physics Stability**        | Medium      | High   | Conservative integration, stability testing             |
| **Memory Usage**             | Medium      | Medium | Memory profiling, component pooling                     |
| **Determinism Issues**       | Low         | Medium | Fixed-point arithmetic consideration, extensive testing |

### Integration Risks

- **ECS Coupling**: Risk of tight coupling with ECS implementation details
- **AI System Integration**: Risk of complex dependencies with AI force application
- **Performance Regression**: Risk of physics system becoming a bottleneck

### Resource Risks

- **Development Time**: Physics tuning may require extensive iteration
- **Complexity Creep**: Risk of over-engineering for simple game requirements
- **Testing Complexity**: Physics systems require complex scenario testing

### Contingency Plans

- **Performance Fallback**: Disable spatial optimization if it causes issues
- **Simplified Collision**: Fall back to simple distance-based collision detection
- **Force Simplification**: Remove complex force interactions if they cause instability

## Decision Rationale

### Architectural Decisions

#### Decision 1: Spatial Grid Over Other Optimizations

- **Options Considered**: Quadtree, spatial hash, uniform grid, no optimization
- **Selection Rationale**: Uniform grid provides predictable performance and simple implementation
- **Trade-offs**: Uses more memory than quadtree but provides consistent O(1) insertion/query
- **Future Impact**: Enables handling thousands of entities with minimal performance impact

#### Decision 2: Force-Based Movement Over Direct Position Updates

- **Options Considered**: Direct position manipulation, velocity-based, force-based, impulse-based
- **Selection Rationale**: Force-based provides natural-feeling movement and easy AI integration
- **Trade-offs**: Slightly more complex than direct manipulation but much more flexible
- **Future Impact**: Allows complex behaviors through force composition

#### Decision 3: AABB Collision Only (Initially)

- **Options Considered**: Circle collision, polygon collision, AABB, distance-based
- **Selection Rationale**: AABB provides good performance/accuracy balance for top-down games
- **Trade-offs**: Less accurate than circles for round objects but much faster
- **Future Impact**: Can be extended to support multiple collision shapes later

#### Decision 4: Single-Threaded Physics System

- **Options Considered**: Multi-threaded collision detection, parallel force application, single-threaded
- **Selection Rationale**: Spatial grid makes multi-threading complex, single-threaded is sufficient for target entity
  counts
- **Trade-offs**: Lower maximum performance ceiling but much simpler implementation
- **Future Impact**: Can be parallelized later if needed without changing interface

### Performance Decisions

- **Spatial Grid Cell Size**: Configurable but defaulting to 5.0 world units for good performance/accuracy balance
- **Force Accumulation**: Per-frame accumulation allows multiple AI systems to contribute forces
- **Event-Driven Communication**: Loose coupling with AI systems through physics events
- **Memory Pooling**: Component traits enable efficient memory management for physics components

## References

### Related Planning Documents

- `SYS_ECS_CORE_v1.md` - ECS foundation that physics system builds upon
- `ARCH_ENGINE_CORE_v1.md` - High-level engine architecture and performance targets

### External Resources

- [Real-Time Collision Detection](https://realtimecollisiondetection.net/) - Collision detection algorithms
- [Game Programming Patterns](https://gameprogrammingpatterns.com/) - Component and system design patterns
- [Spatial Data Structures](https://en.wikipedia.org/wiki/Spatial_database) - Spatial optimization techniques

### Existing Code References

- `src/pathfinding/Pathfinding.cpp` - Current movement and collision patterns
- `src/state/` - Existing entity management for integration reference
- `src/graphics/entity/` - Current entity rendering that will need physics integration

## Appendices

### A. Force Calculation Examples

Target seeking force calculation:

```
desired_velocity = normalize(target - position) * target_speed
steering_force = desired_velocity - current_velocity
clamped_force = clamp(steering_force, max_steering_force)
```

Separation force calculation:

```
for each nearby_entity:
    distance = length(other_position - position)
    if distance < separation_radius:
        force += normalize(position - other_position) * (separation_strength / distance²)
```

### B. Spatial Grid Performance Analysis

For 5,000 entities in a 100x100 world with 5.0 cell size:

- Grid dimensions: 20x20 = 400 cells
- Average entities per cell: 12.5
- Collision checks per entity: ~12 instead of 5,000
- Performance improvement: ~400x for collision detection

### C. Integration with Colony Game AI

```cpp
// Example AI system using physics
class WorkerAISystem {
public:
    void Update(double delta_time, World& world) {
        auto worker_query = world.Query<TransformComponent, WorkerComponent>();
        
        for (auto [entity, transform, worker] : worker_query) {
            if (worker.current_task == TaskType::Gather) {
                // Set physics target for gathering
                world.AddComponent<TargetComponent>(entity, {
                    .target_position = worker.resource_location,
                    .type = TargetComponent::Type::Position,
                    .target_speed = 2.0f
                });
            }
        }
        
        // React to physics events
        world.GetEventSystem().Subscribe<EntityTargetReachedEvent>(
            [&](const EntityTargetReachedEvent& event) {
                // Worker reached resource - start gathering
                HandleTargetReached(event.entity, world);
            }
        );
    }
};
```

---

**Document Status**: Draft  
**Last Updated**: June 30, 2025  
**Next Review**: July 7, 2025  
**Reviewers**: [To be assigned]
