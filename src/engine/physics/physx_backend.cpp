module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

module engine.physics;

import glm;

namespace engine::physics {

// PhysX backend implementation (Adapter Pattern)
// PhysX requires NVIDIA GPU SDK and special setup - this is a placeholder implementation
// To use the real PhysX SDK:
// 1. Download PhysX SDK from NVIDIA
// 2. Add to vcpkg or as external dependency
// 3. Implement actual PhysX calls similar to Jolt/Bullet backends
class PhysXBackend : public IPhysicsBackend {
private:
	PhysicsConfig config_{};
	bool initialized_{false};
	glm::vec3 gravity_{0.0F, -9.81F, 0.0F};

	// Simulated rigid body storage (placeholder until real PhysX is integrated)
	struct RigidBodyData {
		PhysicsTransform transform{};
		glm::vec3 linear_velocity{0.0F, 0.0F, 0.0F};
		glm::vec3 angular_velocity{0.0F, 0.0F, 0.0F};
		float mass{1.0F};
		MotionType motion_type{MotionType::Dynamic};
		bool use_gravity{true};
		float gravity_scale{1.0F};
	};
	std::unordered_map<EntityId, RigidBodyData> rigid_bodies_{};

	// Collision events storage
	std::vector<CollisionInfo> collision_events_{};

public:
	// === Lifecycle ===

	bool Initialize(const PhysicsConfig& config) override {
		if (initialized_) {
			spdlog::warn("[PhysX] Already initialized");
			return true;
		}

		config_ = config;
		gravity_ = config.gravity;

		// In a real implementation, this would:
		// - Create PxFoundation
		// - Create PxPhysics
		// - Create PxCooking
		// - Create PxScene with PxSceneDesc
		// - Setup CUDA context manager for GPU acceleration

		spdlog::info("[PhysX] Initialized (stub - PhysX SDK not linked)");
		spdlog::warn("[PhysX] For real PhysX support, integrate the NVIDIA PhysX SDK");
		initialized_ = true;
		return true;
	}

	void Shutdown() override {
		if (!initialized_) {
			return;
		}

		rigid_bodies_.clear();
		collision_events_.clear();

		spdlog::info("[PhysX] Shutdown");
		initialized_ = false;
	}

	// === World Management ===

	void SetGravity(const glm::vec3& gravity) override { gravity_ = gravity; }

	[[nodiscard]] glm::vec3 GetGravity() const override { return gravity_; }

	void StepSimulation(const float delta_time) override {
		if (!initialized_) {
			return;
		}

		collision_events_.clear();

		// Simple stub simulation
		for (auto& body : rigid_bodies_ | std::views::values) {
			if (body.motion_type == MotionType::Dynamic && body.use_gravity) {
				body.linear_velocity += gravity_ * body.gravity_scale * delta_time;
				body.transform.position += body.linear_velocity * delta_time;
			}
		}
	}

	// === Body Sync (ECS ↔ Backend) ===

	void SyncBodyToBackend(
			const EntityId entity,
			const PhysicsTransform& transform,
			const RigidBody& body,
			const CollisionShape& /*shape*/) override {
		if (!initialized_) {
			spdlog::error("[PhysX] Cannot sync body - not initialized");
			return;
		}

		if (const auto it = rigid_bodies_.find(entity); it != rigid_bodies_.end()) {
			// Update existing body
			it->second.transform = transform;
			it->second.mass = body.mass;
			it->second.motion_type = body.motion_type;
			it->second.use_gravity = body.use_gravity;
			it->second.gravity_scale = body.gravity_scale;
		}
		else {
			// Create new body
			RigidBodyData data{};
			data.transform = transform;
			data.mass = body.mass;
			data.motion_type = body.motion_type;
			data.use_gravity = body.use_gravity;
			data.gravity_scale = body.gravity_scale;
			rigid_bodies_[entity] = data;
		}
	}

	[[nodiscard]] PhysicsSyncResult SyncBodyFromBackend(const EntityId entity) const override {
		PhysicsSyncResult result;

		if (const auto it = rigid_bodies_.find(entity); it != rigid_bodies_.end()) {
			result.position = it->second.transform.position;
			result.rotation = it->second.transform.rotation;
			result.linear_velocity = it->second.linear_velocity;
			result.angular_velocity = it->second.angular_velocity;
		}

		return result;
	}

	void RemoveBody(const EntityId entity) override { rigid_bodies_.erase(entity); }

	[[nodiscard]] bool HasBody(const EntityId entity) const override { return rigid_bodies_.contains(entity); }

	// === Forces & Impulses ===

	void ApplyForce(const EntityId entity, const glm::vec3& force, const glm::vec3& torque) override {
		if (const auto it = rigid_bodies_.find(entity);
			it != rigid_bodies_.end() && it->second.motion_type == MotionType::Dynamic) {
			const float inv_mass = 1.0F / it->second.mass;
			if (glm::length(force) > 0.0F) {
				it->second.linear_velocity += force * inv_mass;
			}
			if (glm::length(torque) > 0.0F) {
				it->second.angular_velocity += torque;
			}
		}
	}

	void ApplyImpulse(const EntityId entity, const glm::vec3& impulse, const glm::vec3& /*point*/) override {
		if (const auto it = rigid_bodies_.find(entity);
			it != rigid_bodies_.end() && it->second.motion_type == MotionType::Dynamic) {
			const float inv_mass = 1.0F / it->second.mass;
			it->second.linear_velocity += impulse * inv_mass;
		}
	}

	// === Collision Queries ===

	[[nodiscard]] std::vector<CollisionInfo> GetCollisionEvents() const override { return collision_events_; }

	// === Raycasting ===

	[[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& /*ray*/) const override {
		// Stub - no raycasting without real PhysX
		return std::nullopt;
	}

	[[nodiscard]] std::vector<RaycastResult> RaycastAll(const Ray& /*ray*/) const override {
		// Stub - no raycasting without real PhysX
		return {};
	}

	// === Constraints/Joints ===

	bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& /*config*/) override {
		spdlog::info("[PhysX] AddConstraint between {} and {} (stub)", entity_a, entity_b);
		return true;
	}

	void RemoveConstraint(EntityId /*entity_a*/, EntityId /*entity_b*/) override {}

	// === Engine Information ===

	[[nodiscard]] std::string GetEngineName() const override { return "PhysX (stub)"; }

	~PhysXBackend() override { Shutdown(); }
};

// Factory function - uses raw new because C++ modules don't support make_unique
// for types defined in module implementation units
std::unique_ptr<IPhysicsBackend> CreatePhysXBackend() { return std::unique_ptr<IPhysicsBackend>(new PhysXBackend()); }

} // namespace engine::physics
