module;

#include <cstdint>
#include <vector>

export module engine.physics:components;

import :types;
import glm;

export namespace engine::physics {

// Core physics body definition
struct RigidBody {
	MotionType motion_type{MotionType::Dynamic};
	float mass{1.0F};
	float linear_damping{0.05F};
	float angular_damping{0.05F};
	float friction{0.5F};
	float restitution{0.3F};
	bool enable_ccd{false};
	bool use_gravity{true};
	float gravity_scale{1.0F};
	CollisionLayer layer{CollisionLayer::Default};
	uint32_t collision_mask{0xFFFFFFFF};
};

// Collision shape definition
struct CollisionShape {
	ShapeType type{ShapeType::Box};
	glm::vec3 box_half_extents{0.5F, 0.5F, 0.5F};
	float sphere_radius{0.5F};
	float capsule_radius{0.5F};
	float capsule_height{1.0F};
	float cylinder_radius{0.5F};
	float cylinder_height{1.0F};
	glm::vec3 offset{0.0F};
	glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
};

// Velocity tracked by physics (separate from engine::components::Velocity to avoid collision)
struct PhysicsVelocity {
	glm::vec3 linear{0.0F};
	glm::vec3 angular{0.0F};
};

// Force accumulator (cleared after physics step if clear_after_apply is true)
struct PhysicsForce {
	glm::vec3 force{0.0F};
	glm::vec3 torque{0.0F};
	bool clear_after_apply{true};
};

// Impulse (consumed immediately by physics step, then removed)
struct PhysicsImpulse {
	glm::vec3 impulse{0.0F};
	glm::vec3 point{0.0F}; // World-space application point
};

// Collision events for this entity (populated by physics module each frame)
struct CollisionEvents {
	std::vector<CollisionInfo> events; // CollisionInfo from physics_types.cppm
};

// Tag components
struct IsTrigger {}; // Trigger volume (no collision response)
struct IsSleeping {}; // Body is sleeping

// Singleton component for world-level physics config
struct PhysicsWorldConfig {
	glm::vec3 gravity{0.0F, -9.81F, 0.0F};
	float fixed_timestep{1.0F / 60.0F};
	int max_substeps{4};
	bool enable_sleeping{true};
	bool show_debug_physics{false};
};

// Constraint as relationship data (used with flecs relationships)
struct PhysicsConstraint {
	ConstraintType type{ConstraintType::Fixed};
	glm::vec3 anchor_a{0.0F};
	glm::vec3 anchor_b{0.0F};
	glm::vec3 axis{0.0F, 1.0F, 0.0F};
	bool enable_limits{false};
	float min_limit{0.0F};
	float max_limit{0.0F};
};

} // namespace engine::physics
