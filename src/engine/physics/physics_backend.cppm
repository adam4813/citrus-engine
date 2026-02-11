module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module engine.physics:backend;

export import :types;
export import :components;

export namespace engine::physics {

// Result of syncing a body from the backend (position, rotation, velocity after step)
struct PhysicsSyncResult {
	glm::vec3 position{0.0F};
	glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
	glm::vec3 linear_velocity{0.0F};
	glm::vec3 angular_velocity{0.0F};
};

// Interface for physics backend implementations (Strategy Pattern)
class IPhysicsBackend {
public:
	virtual ~IPhysicsBackend() = default;

	// === Lifecycle ===
	virtual bool Initialize(const PhysicsConfig& config) = 0;
	virtual void Shutdown() = 0;

	// === World ===
	virtual void SetGravity(const glm::vec3& gravity) = 0;
	[[nodiscard]] virtual glm::vec3 GetGravity() const = 0;
	virtual void StepSimulation(float delta_time) = 0;

	// === Body Sync (ECS ↔ Backend) ===
	// Create or update a body in the backend from ECS component data
	virtual void SyncBodyToBackend(
			EntityId entity, const PhysicsTransform& transform, const RigidBody& body, const CollisionShape& shape) = 0;
	// Read back body state after simulation step
	[[nodiscard]] virtual PhysicsSyncResult SyncBodyFromBackend(EntityId entity) const = 0;
	// Remove a body from the backend
	virtual void RemoveBody(EntityId entity) = 0;
	// Check if backend has a body for this entity
	[[nodiscard]] virtual bool HasBody(EntityId entity) const = 0;

	// === Forces & Impulses ===
	virtual void ApplyForce(EntityId entity, const glm::vec3& force, const glm::vec3& torque) = 0;
	virtual void ApplyImpulse(EntityId entity, const glm::vec3& impulse, const glm::vec3& point) = 0;

	// === Collision Queries ===
	[[nodiscard]] virtual std::vector<CollisionInfo> GetCollisionEvents() const = 0;
	[[nodiscard]] virtual std::optional<RaycastResult> Raycast(const Ray& ray) const = 0;
	[[nodiscard]] virtual std::vector<RaycastResult> RaycastAll(const Ray& ray) const = 0;

	// === Constraints ===
	virtual bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& config) = 0;
	virtual void RemoveConstraint(EntityId entity_a, EntityId entity_b) = 0;

	// === Info ===
	[[nodiscard]] virtual std::string GetEngineName() const = 0;
};

// Factory function for pluggable backend creation (Factory Pattern)
[[nodiscard]] std::unique_ptr<IPhysicsBackend> CreatePhysicsBackend(PhysicsEngineType engine);

} // namespace engine::physics
