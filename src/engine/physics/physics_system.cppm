module;

#include <flecs.h>

export module engine.physics;

export import :backend;
export import :components;
export import :debug_renderer;
export import :types;

export namespace engine::physics {

// Flecs physics modules — import one to activate a physics backend.
// Usage: world.import<JoltPhysicsModule>() or world.import<Bullet3PhysicsModule>()
// Application code only touches ECS components (RigidBody, CollisionShape, etc.)
// and the module manages the backend internally.
struct JoltPhysicsModule {
	JoltPhysicsModule(const flecs::world& world);
};

struct Bullet3PhysicsModule {
	Bullet3PhysicsModule(const flecs::world& world);
};

} // namespace engine::physics
