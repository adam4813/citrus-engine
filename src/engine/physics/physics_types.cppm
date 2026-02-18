module;

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

export module engine.physics:types;

import glm;

export namespace engine::physics {

// Entity identifier type (matches ECS entity type)
using EntityId = std::uint64_t;

// Enum for supported physics engines
enum class PhysicsEngineType { JoltPhysics, Bullet3, PhysX };

// Motion types for rigid bodies
enum class MotionType {
	Static, // Immovable object
	Kinematic, // Animated object (script-controlled)
	Dynamic // Physics-simulated object
};

// Shape types for colliders
enum class ShapeType {
	Box,
	Sphere,
	Capsule,
	Cylinder,
	ConvexHull,
	Mesh, // Static only
	Compound // Multiple shapes combined
};

// Collision layers for filtering
enum class CollisionLayer : std::uint8_t {
	Default = 0,
	Static = 1,
	Dynamic = 2,
	Kinematic = 3,
	Trigger = 4,
	Character = 5
};

// Constraint types for joints
enum class ConstraintType { Fixed, Hinge, Slider, Distance, Cone, PointToPoint, SixDOF };

// Physics world configuration
struct PhysicsConfig {
	glm::vec3 gravity{0.0F, -9.81F, 0.0F};
	float fixed_timestep{1.0F / 60.0F};
	int max_substeps{4};
	int collision_steps{1};
	bool enable_sleeping{true};
	float sleep_threshold{0.05F};
};

// Shape definition for colliders
struct ShapeConfig {
	ShapeType type{ShapeType::Box};

	// Box half-extents
	glm::vec3 box_half_extents{0.5F, 0.5F, 0.5F};

	// Sphere radius
	float sphere_radius{0.5F};

	// Capsule dimensions
	float capsule_radius{0.5F};
	float capsule_height{1.0F};

	// Cylinder dimensions
	float cylinder_radius{0.5F};
	float cylinder_height{1.0F};

	// Convex hull / mesh vertices (for ConvexHull and Mesh types)
	std::vector<glm::vec3> vertices{};
	std::vector<std::uint32_t> indices{}; // For mesh type

	// Compound shape children (for Compound type)
	std::vector<ShapeConfig> children{};
	std::vector<glm::vec3> child_positions{};
	std::vector<glm::quat> child_rotations{};

	// Local offset from entity center
	glm::vec3 offset{0.0F, 0.0F, 0.0F};
	glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
};

// Rigid body configuration
struct RigidBodyConfig {
	MotionType motion_type{MotionType::Dynamic};

	// Physical properties
	float mass{1.0F};
	float linear_damping{0.05F};
	float angular_damping{0.05F};
	float friction{0.5F};
	float restitution{0.3F}; // Bounciness

	// Initial velocities
	glm::vec3 linear_velocity{0.0F, 0.0F, 0.0F};
	glm::vec3 angular_velocity{0.0F, 0.0F, 0.0F};

	// CCD (Continuous Collision Detection)
	bool enable_ccd{false};
	float ccd_motion_threshold{0.01F};

	// Collision filtering
	CollisionLayer layer{CollisionLayer::Default};
	std::uint32_t collision_mask{0xFFFFFFFF}; // Collide with all layers by default

	// Sleeping
	bool allow_sleeping{true};
	bool start_asleep{false};

	// Gravity
	bool use_gravity{true};
	float gravity_scale{1.0F};
};

// Collider configuration (combines shape with physics material)
struct ColliderConfig {
	ShapeConfig shape{};
	bool is_trigger{false}; // Trigger doesn't generate collision response
	float friction{0.5F};
	float restitution{0.3F};
};

// Constraint configuration
struct ConstraintConfig {
	ConstraintType type{ConstraintType::Fixed};

	// Attachment points (local to each body)
	glm::vec3 anchor_a{0.0F, 0.0F, 0.0F};
	glm::vec3 anchor_b{0.0F, 0.0F, 0.0F};

	// Axis for hinge/slider constraints
	glm::vec3 axis{0.0F, 1.0F, 0.0F};

	// Limits for hinge constraints (radians)
	bool enable_limits{false};
	float min_limit{0.0F};
	float max_limit{0.0F};

	// Distance constraint
	float min_distance{0.0F};
	float max_distance{1.0F};

	// Spring properties
	bool enable_spring{false};
	float spring_stiffness{1000.0F};
	float spring_damping{100.0F};

	// Breaking force
	bool can_break{false};
	float break_force{1000000.0F};
	float break_torque{1000000.0F};
};

// Character controller configuration
struct CharacterControllerConfig {
	float height{1.8F};
	float radius{0.3F};
	float step_height{0.35F};
	float max_slope_angle{45.0F}; // Degrees
	float skin_width{0.02F};

	CollisionLayer layer{CollisionLayer::Character};
	std::uint32_t collision_mask{0xFFFFFFFF};
};

// Collision contact point
struct ContactPoint {
	glm::vec3 position{0.0F, 0.0F, 0.0F};
	glm::vec3 normal{0.0F, 1.0F, 0.0F};
	float penetration_depth{0.0F};
};

// Collision information
struct CollisionInfo {
	EntityId entity_a{0};
	EntityId entity_b{0};
	std::vector<ContactPoint> contacts{};
	glm::vec3 impulse{0.0F, 0.0F, 0.0F}; // Total collision impulse
	float separation_velocity{0.0F};

	[[nodiscard]] bool IsValid() const { return entity_a != 0 && entity_b != 0; }
};

// Ray definition
struct Ray {
	glm::vec3 origin{0.0F, 0.0F, 0.0F};
	glm::vec3 direction{0.0F, 0.0F, -1.0F};
	float max_distance{1000.0F};

	// Filtering
	CollisionLayer layer_filter{CollisionLayer::Default};
	std::uint32_t collision_mask{0xFFFFFFFF};
};

// Raycast result
struct RaycastResult {
	EntityId entity{0};
	glm::vec3 hit_point{0.0F, 0.0F, 0.0F};
	glm::vec3 hit_normal{0.0F, 1.0F, 0.0F};
	float distance{0.0F};

	[[nodiscard]] bool HasHit() const { return entity != 0; }
};

// Transform for physics bodies
struct PhysicsTransform {
	glm::vec3 position{0.0F, 0.0F, 0.0F};
	glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F}; // GLM quaternion order: (w, x, y, z)
	glm::vec3 scale{1.0F, 1.0F, 1.0F}; // Uniform scale (not used by physics but may be needed for compound shapes)

	[[nodiscard]] glm::mat4 GetMatrix() const {
		const glm::mat4 translation = glm::translate(glm::mat4(1.0F), position);
		const glm::mat4 rotation_x = glm::rotate(glm::mat4(1.0F), rotation.x, glm::vec3(1, 0, 0));
		const glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0F), rotation.y, glm::vec3(0, 1, 0));
		const glm::mat4 rotation_z = glm::rotate(glm::mat4(1.0F), rotation.z, glm::vec3(0, 0, 1));
		const glm::mat4 scale_mat = glm::scale(glm::mat4(1.0F), scale);
		return translation * rotation_z * rotation_y * rotation_x * scale_mat;
	}

	static PhysicsTransform FromMatrix(const glm::mat4& matrix) {
		PhysicsTransform transform;
		transform.position = glm::vec3(matrix[3]);
		transform.rotation = glm::normalize(glm::quat_cast(matrix));
		transform.scale = glm::vec3(
				glm::length(glm::vec3(matrix[0])),
				glm::length(glm::vec3(matrix[1])),
				glm::length(glm::vec3(matrix[2])));
		return transform;
	}
};

} // namespace engine::physics
