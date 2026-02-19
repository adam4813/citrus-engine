# Physics Parenting

Physics bodies in Citrus Engine operate in **world space**, similar to Unity's approach. When a dynamic RigidBody is attached to an entity, the physics engine owns that entity's world-space position and rotation. The parent-child hierarchy is used for initial positioning only — once physics takes over, the body moves independently in world space.

Citrus Engine provides two approaches for multi-body setups:

1. **Independent dynamic bodies** — Each body moves independently in world space (default behavior for dynamic RigidBody entities)
2. **Compound shapes** — Multiple collision shapes move as a single rigid body

## How Physics Bodies Work in a Hierarchy

### The Rule: Physics Owns World Space

When an entity has a **dynamic RigidBody**, the physics sync-back system writes directly to **WorldTransform** (not Transform). This means:

- `Transform` always contains **local-space** values (the initial offset from parent)
- `WorldTransform.position` and `WorldTransform.rotation` contain **world-space** values (written directly by the physics sync system)
- `WorldTransform.matrix` is recomputed from those world-space values
- The transform propagation system skips recomputing WorldTransform from local Transform for physics entities

```
Parent (Static, at world 10,0,0)
  └─ Child (Dynamic, local offset 5,0,0 → starts at world 15,0,0)

After physics step:
  Child.Transform.position     = (5, 0, 0)       ← local-space (unchanged)
  Child.WorldTransform.position = (15, -0.1, 0)   ← world-space from physics
```

This prevents the double-transformation problem where the parent's transform would be applied twice.

### What This Means in Practice

- **Parent-child is for initialization only** — Setting a child entity's initial position as a local offset works. TransformPropagation computes the correct WorldTransform at setup. Once physics steps, it writes world-space values directly to WorldTransform
- **Transform stays local** — The Transform component is never modified by physics, preserving the original local offset
- **Moving a parent doesn't move physics children** — Dynamic bodies are independent. Use joints/constraints to link bodies
- **Static/Kinematic parents are useful** for organizing the scene hierarchy, but won't affect dynamic children's physics positions

### Example: Independent Bodies

```cpp
// Create static platform (parent)
auto platform = eng.ecs.CreateEntity("Platform");
platform.set<Transform>({{0.0f, 0.0f, 0.0f}});
platform.set<RigidBody>({.motion_type = MotionType::Static});
platform.set<CollisionShape>({.type = ShapeType::Box});

// Create dynamic ball (child) — starts at local offset, then physics takes over
auto ball = eng.ecs.CreateEntity("Ball");
ball.child_of(platform);
ball.set<Transform>({{0.0f, 10.0f, 0.0f}});  // Initial local offset
ball.set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0f});
ball.set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5f});

// After first physics step:
// ball.Transform.position            = (0, 10, 0)  ← local-space, unchanged
// ball.WorldTransform.position       = (0, 9.98, 0) ← world-space, falling under gravity
```

## Solution 2: Compound Shapes (Rigid Structures)

When you want **multiple collision shapes to move as a single rigid body**, use compound shapes. This creates one physics body with multiple sub-shapes at fixed offsets.

### How It Works

Instead of creating child entities with separate physics bodies, you create **one entity** with **one RigidBody** and define multiple shapes within it using the `Compound` shape type:

```cpp
ShapeConfig compound_config{
    .type = ShapeType::Compound,
    .children = {
        ShapeConfig{.type = ShapeType::Box, .box_half_extents = {1.0f, 0.2f, 2.0f}},  // Chassis
        ShapeConfig{.type = ShapeType::Sphere, .sphere_radius = 0.4f},                  // Wheel
        ShapeConfig{.type = ShapeType::Sphere, .sphere_radius = 0.4f},                  // Wheel
        // ... more shapes
    },
    .child_positions = {
        {0.0f, 0.5f, 0.0f},   // Chassis at center
        {-0.8f, 0.0f, 1.0f},  // Front-left wheel
        {0.8f, 0.0f, 1.0f},   // Front-right wheel
        // ... more positions
    },
    .child_rotations = {
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),  // Chassis (no rotation)
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),  // Wheel
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),  // Wheel
        // ... more rotations
    }
};
```

!!! note
    Compound shapes are currently created through the backend API, not directly via ECS components. The physics module translates `CollisionShape` components to backend `ShapeConfig` during body creation.

### When to Use

Use compound shapes when:

- Parts are **rigidly attached** (never move relative to each other)
- You want better **performance** (one body vs. many bodies + constraints)
- The object is a single logical entity (vehicle chassis, multi-part building)
- You don't need per-part physical properties (mass, friction)

### Example: Simple Vehicle Chassis

```cpp
import engine;
import glm;

// Create vehicle entity
auto vehicle = eng.ecs.CreateEntity("Vehicle");
vehicle.set<engine::components::Transform>({{0.0f, 2.0f, 0.0f}});

// Define compound shape via backend API
// (In the future, this may be exposed as a component)
auto backend_ptr = eng.ecs.GetWorld().get<engine::physics::PhysicsBackendPtr>();
if (backend_ptr && backend_ptr->backend) {
    engine::physics::RigidBodyConfig body_config{
        .motion_type = engine::physics::MotionType::Dynamic,
        .mass = 1200.0f  // Total mass for entire compound
    };
    
    engine::physics::ShapeConfig shape_config{
        .type = engine::physics::ShapeType::Compound,
        .children = {
            // Chassis (main body)
            engine::physics::ShapeConfig{
                .type = engine::physics::ShapeType::Box,
                .box_half_extents = {1.0f, 0.4f, 2.0f}
            },
            // Front bumper
            engine::physics::ShapeConfig{
                .type = engine::physics::ShapeType::Box,
                .box_half_extents = {1.2f, 0.2f, 0.3f}
            },
        },
        .child_positions = {
            {0.0f, 0.5f, 0.0f},   // Chassis center
            {0.0f, 0.2f, 2.2f},   // Bumper at front
        },
        .child_rotations = {
            glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
            glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        }
    };
    
    engine::physics::PhysicsTransform transform{
        .position = {0.0f, 2.0f, 0.0f},
        .rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        .scale = {1.0f, 1.0f, 1.0f}
    };
    
    backend_ptr->backend->CreateBody(
        vehicle.id(),
        body_config,
        shape_config,
        transform
    );
}

// Set RigidBody component for ECS tracking
vehicle.set<engine::physics::RigidBody>({
    .motion_type = engine::physics::MotionType::Dynamic,
    .mass = 1200.0f
});
```

**Result:** One rigid body with multiple collision shapes. The entire compound moves as a single unit, with better performance than separate bodies.

### Compound vs. Parent-Child

| Aspect | Compound Shape | Independent Bodies |
|--------|----------------|-------------------|
| **Number of physics bodies** | 1 | Multiple (parent + children) |
| **Relative motion** | Fixed (rigid) | Can move independently |
| **Performance** | Better (fewer bodies) | Slower (more bodies + constraints) |
| **Use case** | Rigid structures | Articulated objects |
| **Mass distribution** | Single total mass | Per-part masses |
| **Joints/Constraints** | Not needed | Required for connection |

## Best Practices

### 1. Choose the Right Approach

- **Independent bodies**: Ragdolls, hinged doors, rope segments — use constraints/joints to connect them
- **Compound shapes**: Vehicles, furniture, buildings — all parts are rigidly attached

### 2. Understand That Physics Bodies Are Independent

Unlike non-physics entities, dynamic bodies don't follow their parent when the parent moves:

```cpp
// Non-physics child: follows parent
Parent.Transform.position = (10, 0, 0)
  └─ Child.Transform.position = (0, 5, 0)
     Child.WorldTransform = (10, 5, 0)  // Parent + local

// Physics child: independent in world space
Parent.Transform.position = (10, 0, 0)
  └─ Child.Transform.position = (15, 3, 0)  // ← This is WORLD space
     Child.WorldTransform = (15, 3, 0)  // Physics owns this directly
```

### 3. Use Constraints for Linked Motion

If you want a child body to follow its parent, use physics constraints (joints):

```cpp
// Example: Door hinge (constraint between static frame and dynamic door)
auto frame = eng.ecs.CreateEntity("DoorFrame");
frame.set<RigidBody>({.motion_type = MotionType::Static});

auto door = eng.ecs.CreateEntity("Door");
door.set<RigidBody>({.motion_type = MotionType::Dynamic});

// Add hinge constraint — the physics engine handles the connection
```

### 4. Profile Performance

If you have many bodies, compound shapes are faster (one body vs. many):

```cpp
// Many separate bodies (slower)
for (int i = 0; i < 100; i++) {
    auto part = eng.ecs.CreateEntity("Part" + std::to_string(i));
    part.set<RigidBody>({...});
    part.set<CollisionShape>({...});
}

// One compound body (faster)
ShapeConfig compound{
    .type = ShapeType::Compound,
    .children = /* 100 shapes */
};
```

## Performance Considerations

### Independent Bodies

- **Pros**: Flexible, realistic articulation, supports joints
- **Cons**: More bodies = slower simulation
- **Note**: No per-frame overhead for hierarchy — physics bodies skip parent transform entirely

### Compound Shapes

- **Pros**: Single body simulation, fastest option
- **Cons**: No relative motion, all-or-nothing
- **Cost**: O(1) - one body regardless of shape count

### Recommendation

For **rigid structures**: Use compound shapes
For **articulated objects**: Use independent bodies with constraints

## Debugging Tips

### Visualize Physics Shapes

Enable debug rendering to see actual collision shapes:

```cpp
auto config = eng.ecs.GetWorld().get<engine::physics::PhysicsWorldConfig>();
config->show_debug_physics = true;
```

Debug rendering uses the native physics backend's debug renderer (Jolt's DebugRenderer or Bullet3's btIDebugDraw), so all shape types are rendered accurately with proper rotation.

### Verify Transform Values

Remember that for dynamic bodies, `Transform.position` is **world-space**:

```cpp
eng.ecs.GetWorld().each([](flecs::entity e, 
                           const Transform& t,
                           const WorldTransform& wt) {
    if (e.has<RigidBody>()) {
        // For dynamic bodies: t.position IS world position
        // WorldTransform should match (no parent applied)
    }
});
```

## Further Reading

- **[Physics API](physics.md)** - Core physics components and setup
- **[Scene Management](scenes.md)** - Entity hierarchies and prefabs
- **[Architecture Overview](architecture.md)** - ECS fundamentals

## Summary

| Entity Type | Transform.position | WorldTransform | Parent affects? |
|-------------|-------------------|----------------|-----------------|
| No RigidBody | Local space | parent × local | ✅ Yes |
| Static/Kinematic RigidBody | Local space | parent × local | ✅ Yes |
| **Dynamic RigidBody** | **World space** | **= Transform** | **❌ No** |

- Dynamic bodies operate in world space (like Unity)
- Parent-child is for initial positioning and scene organization
- Use compound shapes for rigid multi-part objects
- Use constraints/joints for connected but movable parts
