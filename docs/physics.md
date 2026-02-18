# Physics API

Citrus Engine provides a flexible physics system with support for multiple backends. This document covers how to use physics in your game.

## Overview

The physics system is backend-agnostic, allowing you to choose between:

- **Jolt Physics** - High-performance, modern physics engine
- **Bullet3** - Widely-used open-source physics
- **PhysX** (future) - Industry-standard physics from NVIDIA

All backends use the same ECS component interface, so switching backends doesn't require code changes.

## Choosing a Physics Backend

### Import a Physics Module

Select one physics backend by importing its module:

```cpp
import engine;

// Initialize engine
engine::Engine eng;
eng.Init(1280, 720);

// Import Jolt Physics backend
eng.ecs.GetWorld().import<engine::physics::JoltPhysicsModule>();

// OR import Bullet3 backend
// eng.ecs.GetWorld().import<engine::physics::Bullet3PhysicsModule>();
```

!!! note
    Only import **one** physics module per world. Importing multiple backends will cause conflicts.

## Physics Components

### RigidBody

The `RigidBody` component defines the physical properties of an entity:

```cpp
struct RigidBody {
    MotionType motion_type{MotionType::Dynamic};
    float mass{1.0f};
    float linear_damping{0.05f};
    float angular_damping{0.05f};
    float friction{0.5f};
    float restitution{0.3f};
    bool enable_ccd{false};           // Continuous collision detection
    bool use_gravity{true};
    float gravity_scale{1.0f};
    CollisionLayer layer{CollisionLayer::Default};
    uint32_t collision_mask{0xFFFFFFFF};
};
```

**Motion Types:**

- `MotionType::Static` - Doesn't move (terrain, walls)
- `MotionType::Dynamic` - Fully simulated (players, projectiles)
- `MotionType::Kinematic` - Moved by user, affects others (moving platforms)

**Example:**

```cpp
auto entity = eng.ecs.CreateEntity("Box");
entity.set<engine::components::Transform>({{0.0f, 5.0f, 0.0f}});
entity.set<engine::physics::RigidBody>({
    .motion_type = engine::physics::MotionType::Dynamic,
    .mass = 10.0f,
    .friction = 0.6f,
    .restitution = 0.4f  // Bounciness
});
```

### CollisionShape

The `CollisionShape` component defines the collision geometry:

```cpp
struct CollisionShape {
    ShapeType type{ShapeType::Box};
    
    // Box parameters
    glm::vec3 box_half_extents{0.5f, 0.5f, 0.5f};
    
    // Sphere parameters
    float sphere_radius{0.5f};
    
    // Capsule parameters
    float capsule_radius{0.5f};
    float capsule_height{1.0f};
    
    // Cylinder parameters
    float cylinder_radius{0.5f};
    float cylinder_height{1.0f};
    
    // Offset from entity transform
    glm::vec3 offset{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};
```

**Shape Types:**

- `ShapeType::Box` - Rectangular box
- `ShapeType::Sphere` - Sphere
- `ShapeType::Capsule` - Capsule (pill shape)
- `ShapeType::Cylinder` - Cylinder

**Example:**

```cpp
// Box shape
entity.set<engine::physics::CollisionShape>({
    .type = engine::physics::ShapeType::Box,
    .box_half_extents = {1.0f, 1.0f, 1.0f}  // 2x2x2 box
});

// Sphere shape
entity.set<engine::physics::CollisionShape>({
    .type = engine::physics::ShapeType::Sphere,
    .sphere_radius = 0.5f
});

// Capsule for characters
entity.set<engine::physics::CollisionShape>({
    .type = engine::physics::ShapeType::Capsule,
    .capsule_radius = 0.5f,
    .capsule_height = 2.0f,
    .offset = {0.0f, 1.0f, 0.0f}  // Offset up
});
```

### PhysicsVelocity

Stores the current velocity of a physics body:

```cpp
struct PhysicsVelocity {
    glm::vec3 linear{0.0f};   // Linear velocity (m/s)
    glm::vec3 angular{0.0f};  // Angular velocity (rad/s)
};
```

**Example:**

```cpp
// Set initial velocity
entity.set<engine::physics::PhysicsVelocity>({
    .linear = {5.0f, 0.0f, 0.0f}  // Move right at 5 m/s
});

// Read velocity
auto velocity = entity.get<engine::physics::PhysicsVelocity>();
std::cout << "Speed: " << glm::length(velocity->linear) << " m/s" << std::endl;
```

### PhysicsForce

Apply forces and torques to bodies:

```cpp
struct PhysicsForce {
    glm::vec3 force{0.0f};
    glm::vec3 torque{0.0f};
    bool clear_after_apply{true};  // Reset after physics step
};
```

**Example:**

```cpp
// Apply upward force (jump)
entity.set<engine::physics::PhysicsForce>({
    .force = {0.0f, 500.0f, 0.0f},
    .clear_after_apply = true
});

// Continuous force (thruster)
entity.set<engine::physics::PhysicsForce>({
    .force = {0.0f, 10.0f, 0.0f},
    .clear_after_apply = false  // Keep applying
});
```

### PhysicsImpulse

Apply instantaneous impulses (consumed immediately):

```cpp
struct PhysicsImpulse {
    glm::vec3 impulse{0.0f};
    glm::vec3 point{0.0f};  // World-space application point
};
```

**Example:**

```cpp
// Apply impulse at center of mass
entity.set<engine::physics::PhysicsImpulse>({
    .impulse = {100.0f, 0.0f, 0.0f}
});

// Apply impulse at specific point (creates torque)
entity.set<engine::physics::PhysicsImpulse>({
    .impulse = {0.0f, 50.0f, 0.0f},
    .point = {1.0f, 0.0f, 0.0f}  // Apply to right side
});
```

## Tag Components

### IsTrigger

Mark a body as a trigger volume (no physical collision response):

```cpp
entity.add<engine::physics::IsTrigger>();
```

Triggers generate collision events but don't affect motion.

### IsSleeping

Tag indicating a body is currently sleeping (performance optimization):

```cpp
// Check if sleeping
if (entity.has<engine::physics::IsSleeping>()) {
    std::cout << "Body is sleeping" << std::endl;
}
```

Physics engines automatically sleep inactive bodies.

## Collision Events

### CollisionEvents Component

Collision information is stored in the `CollisionEvents` component:

```cpp
struct CollisionEvents {
    std::vector<CollisionInfo> events;
};
```

**CollisionInfo:**

```cpp
struct CollisionInfo {
    flecs::entity other_entity;    // The other entity in collision
    glm::vec3 contact_point;        // World-space contact point
    glm::vec3 contact_normal;       // Normal pointing from this to other
    float penetration_depth;        // How deep the overlap is
    bool is_trigger;                // True if either body is a trigger
};
```

**Example:**

```cpp
// Query entities with collision events
eng.ecs.GetWorld().each([](flecs::entity e, 
                            const engine::physics::CollisionEvents& events) {
    for (const auto& collision : events.events) {
        std::cout << "Entity " << e.name() 
                  << " collided with " << collision.other_entity.name() 
                  << std::endl;
        
        if (collision.is_trigger) {
            std::cout << "  Trigger collision!" << std::endl;
        }
    }
});
```

## Physics World Configuration

Configure global physics settings via the `PhysicsWorldConfig` singleton:

```cpp
struct PhysicsWorldConfig {
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    float fixed_timestep{1.0f / 60.0f};  // 60 Hz simulation
    int max_substeps{4};
    bool enable_sleeping{true};
    bool show_debug_physics{false};      // Render collision shapes
};
```

**Example:**

```cpp
// Get or create singleton
auto config_entity = eng.ecs.GetWorld().entity("PhysicsConfig");
config_entity.set<engine::physics::PhysicsWorldConfig>({
    .gravity = {0.0f, -20.0f, 0.0f},     // Stronger gravity
    .fixed_timestep = 1.0f / 120.0f,     // 120 Hz simulation
    .show_debug_physics = true            // Show debug rendering
});
```

## Raycasting

Perform raycasts to detect objects along a line:

```cpp
// Get physics backend
auto backend_ptr = eng.ecs.GetWorld()
    .get<engine::physics::PhysicsBackendPtr>();

if (backend_ptr && backend_ptr->backend) {
    // Cast ray
    engine::physics::Ray ray{
        .origin = {0.0f, 0.0f, 0.0f},
        .direction = {0.0f, -1.0f, 0.0f},  // Down
        .max_distance = 100.0f
    };
    
    auto result = backend_ptr->backend->Raycast(ray);
    
    if (result.hit) {
        std::cout << "Hit entity: " << result.entity.name() << std::endl;
        std::cout << "Distance: " << result.distance << std::endl;
        std::cout << "Hit point: " << glm::to_string(result.hit_point) << std::endl;
    }
}
```

**RaycastResult:**

```cpp
struct RaycastResult {
    bool hit{false};
    flecs::entity entity;        // Entity that was hit
    glm::vec3 hit_point{0.0f};
    glm::vec3 hit_normal{0.0f};
    float distance{0.0f};
};
```

## Complete Example

Here's a complete example setting up a simple physics scene:

```cpp
import engine;
import glm;

void SetupPhysicsScene(engine::Engine& eng) {
    // Import physics backend
    eng.ecs.GetWorld().import<engine::physics::JoltPhysicsModule>();
    
    // Configure physics world
    auto config = eng.ecs.GetWorld().entity("PhysicsConfig");
    config.set<engine::physics::PhysicsWorldConfig>({
        .gravity = {0.0f, -9.81f, 0.0f},
        .enable_sleeping = true
    });
    
    // Create static ground plane
    auto ground = eng.ecs.CreateEntity("Ground");
    ground.set<engine::components::Transform>({{0.0f, -1.0f, 0.0f}});
    ground.set<engine::physics::RigidBody>({
        .motion_type = engine::physics::MotionType::Static
    });
    ground.set<engine::physics::CollisionShape>({
        .type = engine::physics::ShapeType::Box,
        .box_half_extents = {50.0f, 0.5f, 50.0f}  // Large flat box
    });
    
    // Create dynamic box
    auto box = eng.ecs.CreateEntity("Box");
    box.set<engine::components::Transform>({{0.0f, 10.0f, 0.0f}});
    box.set<engine::physics::RigidBody>({
        .motion_type = engine::physics::MotionType::Dynamic,
        .mass = 10.0f,
        .friction = 0.5f,
        .restitution = 0.3f
    });
    box.set<engine::physics::CollisionShape>({
        .type = engine::physics::ShapeType::Box,
        .box_half_extents = {1.0f, 1.0f, 1.0f}
    });
    
    // Create character with capsule
    auto character = eng.ecs.CreateEntity("Character");
    character.set<engine::components::Transform>({{5.0f, 5.0f, 0.0f}});
    character.set<engine::physics::RigidBody>({
        .motion_type = engine::physics::MotionType::Dynamic,
        .mass = 70.0f,  // kg
        .friction = 0.8f,
        .use_gravity = true
    });
    character.set<engine::physics::CollisionShape>({
        .type = engine::physics::ShapeType::Capsule,
        .capsule_radius = 0.5f,
        .capsule_height = 2.0f,
        .offset = {0.0f, 1.0f, 0.0f}
    });
    
    // Create trigger zone
    auto trigger = eng.ecs.CreateEntity("TriggerZone");
    trigger.set<engine::components::Transform>({{0.0f, 2.0f, 0.0f}});
    trigger.set<engine::physics::RigidBody>({
        .motion_type = engine::physics::MotionType::Static
    });
    trigger.set<engine::physics::CollisionShape>({
        .type = engine::physics::ShapeType::Box,
        .box_half_extents = {5.0f, 5.0f, 5.0f}
    });
    trigger.add<engine::physics::IsTrigger>();
}
```

## Character Controller Pattern

For character movement, combine physics with custom control:

```cpp
void UpdateCharacter(engine::Engine& eng, float delta_time) {
    eng.ecs.GetWorld().each([delta_time](
        flecs::entity e,
        engine::physics::PhysicsVelocity& velocity,
        const engine::physics::RigidBody& body
    ) {
        if (e.name() != "Character") return;
        
        // Get input
        glm::vec3 move_dir{0.0f};
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::W)) {
            move_dir.z -= 1.0f;
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::S)) {
            move_dir.z += 1.0f;
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::A)) {
            move_dir.x -= 1.0f;
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::D)) {
            move_dir.x += 1.0f;
        }
        
        // Normalize and scale
        if (glm::length(move_dir) > 0.0f) {
            move_dir = glm::normalize(move_dir) * 5.0f;  // 5 m/s
        }
        
        // Apply to horizontal velocity only (preserve vertical)
        velocity.linear.x = move_dir.x;
        velocity.linear.z = move_dir.z;
        
        // Jump
        if (engine::input::Input::IsKeyJustPressed(engine::input::KeyCode::SPACE)) {
            e.set<engine::physics::PhysicsImpulse>({
                .impulse = {0.0f, 300.0f, 0.0f}
            });
        }
    });
}
```

## Best Practices

1. **Use appropriate motion types**: Static for terrain, Dynamic for interactive objects
2. **Enable CCD for fast objects**: Set `enable_ccd = true` for bullets, fast projectiles
3. **Tune damping values**: Prevent bouncing with higher damping
4. **Use collision layers**: Filter what objects collide with each other
5. **Prefer impulses for instant changes**: Use forces for continuous application
6. **Check collision events**: React to collisions via `CollisionEvents` component
7. **Scale masses realistically**: Human ~70kg, car ~1000kg

## Performance Tips

- **Use sleeping**: Enable sleeping for better performance with many static bodies
- **Simplify collision shapes**: Use boxes/spheres instead of complex meshes
- **Fixed timestep**: Keep `fixed_timestep` at 1/60 or 1/120 for stability
- **Limit max substeps**: Prevents simulation spiral of death

## Further Reading

- **[Architecture Overview](architecture.md)** - How physics integrates with ECS
- **[Getting Started](getting-started.md)** - Set up your project
- **[Scene Management](scenes.md)** - Save/load physics setups
