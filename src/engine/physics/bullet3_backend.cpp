module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

// Bullet3 includes
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <btBulletDynamicsCommon.h>

module engine.physics;

import glm;

namespace engine::physics {

// Bullet3 backend implementation
class Bullet3Backend : public IPhysicsBackend {
private:
	PhysicsConfig config_{};
	bool initialized_{false};

	// Bullet world components
	std::unique_ptr<btDefaultCollisionConfiguration> collision_config_;
	std::unique_ptr<btCollisionDispatcher> dispatcher_;
	std::unique_ptr<btBroadphaseInterface> broadphase_;
	std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
	std::unique_ptr<btDiscreteDynamicsWorld> dynamics_world_;

	// Rigid body storage
	struct RigidBodyData {
		std::unique_ptr<btRigidBody> body;
		std::unique_ptr<btCollisionShape> shape;
		std::unique_ptr<btDefaultMotionState> motion_state;
		std::vector<std::unique_ptr<btTriangleMesh>> mesh_data; // Owned mesh data for btBvhTriangleMeshShape
		std::vector<std::unique_ptr<btCollisionShape>> child_shapes; // Owned child shapes for btCompoundShape
	};
	std::unordered_map<EntityId, RigidBodyData> rigid_bodies_;

	// Collision events storage
	std::vector<CollisionInfo> collision_events_;

	// Ghost pair callback (owned by broadphase, but we track it for documentation)
	btGhostPairCallback* ghost_pair_callback_{nullptr};

	// Result struct for shape creation (to properly manage memory)
	struct ShapeCreationResult {
		std::unique_ptr<btCollisionShape> shape;
		std::vector<std::unique_ptr<btTriangleMesh>> mesh_data; // Vector to handle multiple meshes in compound shapes
		std::vector<std::unique_ptr<btCollisionShape>> child_shapes;
	};

	// Helper to convert CollisionShape component to ShapeConfig
	static ShapeConfig ToShapeConfig(const CollisionShape& shape) {
		ShapeConfig config;
		config.type = shape.type;
		config.box_half_extents = shape.box_half_extents;
		config.sphere_radius = shape.sphere_radius;
		config.capsule_radius = shape.capsule_radius;
		config.capsule_height = shape.capsule_height;
		config.cylinder_radius = shape.cylinder_radius;
		config.cylinder_height = shape.cylinder_height;
		config.offset = shape.offset;
		config.rotation = shape.rotation;
		return config;
	}

	// Helper to create Bullet shape from config
	static ShapeCreationResult CreateShape(const ShapeConfig& config) {
		ShapeCreationResult result;

		switch (config.type) {
		case ShapeType::Box:
			result.shape = std::make_unique<btBoxShape>(
					btVector3(config.box_half_extents.x, config.box_half_extents.y, config.box_half_extents.z));
			return result;

		case ShapeType::Sphere: result.shape = std::make_unique<btSphereShape>(config.sphere_radius); return result;

		case ShapeType::Capsule:
			result.shape = std::make_unique<btCapsuleShape>(config.capsule_radius, config.capsule_height);
			return result;

		case ShapeType::Cylinder:
			result.shape = std::make_unique<btCylinderShape>(
					btVector3(config.cylinder_radius, config.cylinder_height * 0.5F, config.cylinder_radius));
			return result;

		case ShapeType::ConvexHull:
			if (!config.vertices.empty()) {
				auto hull = std::make_unique<btConvexHullShape>();
				for (const auto& v : config.vertices) {
					hull->addPoint(btVector3(v.x, v.y, v.z));
				}
				result.shape = std::move(hull);
				return result;
			}
			break;

		case ShapeType::Mesh:
			if (!config.vertices.empty() && !config.indices.empty()) {
				// Create triangle mesh - we own the mesh data
				auto meshData = std::make_unique<btTriangleMesh>();
				for (size_t i = 0; i + 2 < config.indices.size(); i += 3) {
					const auto& v0 = config.vertices[config.indices[i]];
					const auto& v1 = config.vertices[config.indices[i + 1]];
					const auto& v2 = config.vertices[config.indices[i + 2]];
					meshData->addTriangle(
							btVector3(v0.x, v0.y, v0.z), btVector3(v1.x, v1.y, v1.z), btVector3(v2.x, v2.y, v2.z));
				}
				// btBvhTriangleMeshShape does NOT take ownership - we keep mesh_data alive
				result.shape = std::make_unique<btBvhTriangleMeshShape>(meshData.get(), true);
				result.mesh_data.push_back(std::move(meshData));
				return result;
			}
			break;

		case ShapeType::Compound:
		{
			auto compound = std::make_unique<btCompoundShape>();
			for (size_t i = 0; i < config.children.size(); ++i) {
				auto childResult = CreateShape(config.children[i]);
				if (childResult.shape) {
					glm::vec3 pos = i < config.child_positions.size() ? config.child_positions[i] : glm::vec3(0.0F);
					glm::quat rot = i < config.child_rotations.size() ? config.child_rotations[i]
																	  : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);

					btTransform localTrans;
					localTrans.setOrigin(btVector3(pos.x, pos.y, pos.z));
					localTrans.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
					// btCompoundShape does NOT take ownership - we store child shapes
					compound->addChildShape(localTrans, childResult.shape.get());
					result.child_shapes.push_back(std::move(childResult.shape));
					// Also take ownership of any nested mesh data or child shapes
					for (auto& mesh : childResult.mesh_data) {
						result.mesh_data.push_back(std::move(mesh));
					}
					for (auto& nested : childResult.child_shapes) {
						result.child_shapes.push_back(std::move(nested));
					}
				}
			}
			result.shape = std::move(compound);
			return result;
		}

		default: break;
		}

		// Default to box
		spdlog::warn("[Bullet3] Unknown shape type, defaulting to box");
		result.shape = std::make_unique<btBoxShape>(btVector3(0.5F, 0.5F, 0.5F));
		return result;
	}

public:
	bool Initialize(const PhysicsConfig& config) override {
		if (initialized_) {
			spdlog::warn("[Bullet3] Already initialized");
			return true;
		}

		config_ = config;

		// Create Bullet world components
		collision_config_ = std::make_unique<btDefaultCollisionConfiguration>();
		dispatcher_ = std::make_unique<btCollisionDispatcher>(collision_config_.get());
		broadphase_ = std::make_unique<btDbvtBroadphase>();

		// Add ghost object support for character controllers
		// Note: The broadphase's overlapping pair cache takes ownership of this callback
		ghost_pair_callback_ = new btGhostPairCallback();
		broadphase_->getOverlappingPairCache()->setInternalGhostPairCallback(ghost_pair_callback_);

		solver_ = std::make_unique<btSequentialImpulseConstraintSolver>();

		dynamics_world_ = std::make_unique<btDiscreteDynamicsWorld>(
				dispatcher_.get(), broadphase_.get(), solver_.get(), collision_config_.get());

		dynamics_world_->setGravity(btVector3(config.gravity.x, config.gravity.y, config.gravity.z));

		spdlog::info("[Bullet3] Initialized");
		initialized_ = true;
		return true;
	}

	void Shutdown() override {
		if (!initialized_) {
			return;
		}

		// Remove all bodies from world first
		for (auto& data : rigid_bodies_ | std::views::values) {
			if (data.body) {
				dynamics_world_->removeRigidBody(data.body.get());
			}
		}
		rigid_bodies_.clear();
		collision_events_.clear();

		// Clean up Bullet world
		dynamics_world_.reset();
		solver_.reset();
		broadphase_.reset();
		dispatcher_.reset();
		collision_config_.reset();

		spdlog::info("[Bullet3] Shutdown");
		initialized_ = false;
	}

	void SetGravity(const glm::vec3& gravity) override {
		if (dynamics_world_) {
			dynamics_world_->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
		}
	}

	[[nodiscard]] glm::vec3 GetGravity() const override {
		if (dynamics_world_) {
			auto g = dynamics_world_->getGravity();
			return glm::vec3(g.x(), g.y(), g.z());
		}
		return config_.gravity;
	}

	void StepSimulation(float delta_time) override {
		if (!initialized_ || !dynamics_world_) {
			return;
		}

		// Clear previous collision events
		collision_events_.clear();

		dynamics_world_->stepSimulation(delta_time, config_.max_substeps, config_.fixed_timestep);

		// Collect collision events
		int numManifolds = dispatcher_->getNumManifolds();
		for (int i = 0; i < numManifolds; ++i) {
			btPersistentManifold* manifold = dispatcher_->getManifoldByIndexInternal(i);
			const btCollisionObject* objA = manifold->getBody0();
			const btCollisionObject* objB = manifold->getBody1();

			int numContacts = manifold->getNumContacts();
			if (numContacts > 0) {
				CollisionInfo info;
				info.entity_a = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objA->getUserPointer()));
				info.entity_b = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objB->getUserPointer()));

				for (int j = 0; j < numContacts; ++j) {
					btManifoldPoint& pt = manifold->getContactPoint(j);
					ContactPoint contact;
					btVector3 pos = pt.getPositionWorldOnA();
					btVector3 normal = pt.m_normalWorldOnB;
					contact.position = glm::vec3(pos.x(), pos.y(), pos.z());
					contact.normal = glm::vec3(normal.x(), normal.y(), normal.z());
					contact.penetration_depth = -pt.getDistance();
					info.contacts.push_back(contact);
				}

				collision_events_.push_back(info);
			}
		}
	}

	void SyncBodyToBackend(
			EntityId entity,
			const PhysicsTransform& transform,
			const RigidBody& body,
			const CollisionShape& shape) override {
		if (!initialized_) {
			spdlog::error("[Bullet3] Cannot sync body - not initialized");
			return;
		}

		auto it = rigid_bodies_.find(entity);
		ShapeConfig shapeConfig = ToShapeConfig(shape);
		auto shapeResult = CreateShape(shapeConfig);

		if (!shapeResult.shape) {
			spdlog::error("[Bullet3] Failed to create shape for entity {}", entity);
			return;
		}

		if (it != rigid_bodies_.end()) {
			// Update existing body
			RigidBodyData& data = it->second;

			// Update shape if different
			data.shape = std::move(shapeResult.shape);
			data.mesh_data = std::move(shapeResult.mesh_data);
			data.child_shapes = std::move(shapeResult.child_shapes);

			// Update mass and inertia
			btScalar mass = body.motion_type == MotionType::Dynamic ? body.mass : 0.0F;
			btVector3 inertia(0, 0, 0);
			if (mass > 0) {
				data.shape->calculateLocalInertia(mass, inertia);
			}
			data.body->setCollisionShape(data.shape.get());
			data.body->setMassProps(mass, inertia);

			// Update transform
			btTransform btTrans;
			btTrans.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
			btTrans.setRotation(btQuaternion(
					transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));
			data.body->setWorldTransform(btTrans);
			data.body->getMotionState()->setWorldTransform(btTrans);

			// Update physics properties
			data.body->setFriction(body.friction);
			data.body->setRestitution(body.restitution);
			data.body->setDamping(body.linear_damping, body.angular_damping);

			// Update CCD
			if (body.enable_ccd && body.motion_type == MotionType::Dynamic) {
				data.body->setCcdMotionThreshold(0.01F);
				data.body->setCcdSweptSphereRadius(0.2F);
			}
			else {
				data.body->setCcdMotionThreshold(0.0F);
			}

			// Update gravity
			if (!body.use_gravity || body.gravity_scale == 0.0F) {
				data.body->setGravity(btVector3(0, 0, 0));
			}
			else if (body.gravity_scale != 1.0F) {
				auto g = dynamics_world_->getGravity() * body.gravity_scale;
				data.body->setGravity(g);
			}
			else {
				data.body->setGravity(dynamics_world_->getGravity());
			}

			data.body->activate();
		}
		else {
			// Create new body
			RigidBodyData data;
			data.shape = std::move(shapeResult.shape);
			data.mesh_data = std::move(shapeResult.mesh_data);
			data.child_shapes = std::move(shapeResult.child_shapes);

			// Calculate mass and inertia
			btScalar mass = body.motion_type == MotionType::Dynamic ? body.mass : 0.0F;
			btVector3 inertia(0, 0, 0);
			if (mass > 0) {
				data.shape->calculateLocalInertia(mass, inertia);
			}

			// Create motion state with transform
			btTransform btTrans;
			btTrans.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
			btTrans.setRotation(btQuaternion(
					transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w));
			data.motion_state = std::make_unique<btDefaultMotionState>(btTrans);

			// Create rigid body
			btRigidBody::btRigidBodyConstructionInfo info(mass, data.motion_state.get(), data.shape.get(), inertia);
			info.m_friction = body.friction;
			info.m_restitution = body.restitution;
			info.m_linearDamping = body.linear_damping;
			info.m_angularDamping = body.angular_damping;

			data.body = std::make_unique<btRigidBody>(info);
			// FIXME: Entity IDs are stored in btCollisionObject::setUserPointer() by casting a 64-bit EntityId it will truncate on wasm32
			data.body->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

			// Set kinematic if needed
			if (body.motion_type == MotionType::Kinematic) {
				data.body->setCollisionFlags(data.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				data.body->setActivationState(DISABLE_DEACTIVATION);
			}

			// Set CCD
			if (body.enable_ccd && body.motion_type == MotionType::Dynamic) {
				data.body->setCcdMotionThreshold(0.01F);
				data.body->setCcdSweptSphereRadius(0.2F);
			}

			// Gravity settings
			if (!body.use_gravity || body.gravity_scale == 0.0F) {
				data.body->setGravity(btVector3(0, 0, 0));
			}
			else if (body.gravity_scale != 1.0F) {
				auto g = dynamics_world_->getGravity() * body.gravity_scale;
				data.body->setGravity(g);
			}

			// Add to world
			dynamics_world_->addRigidBody(data.body.get());

			rigid_bodies_[entity] = std::move(data);
		}
	}

	[[nodiscard]] PhysicsSyncResult SyncBodyFromBackend(EntityId entity) const override {
		PhysicsSyncResult result;

		auto it = rigid_bodies_.find(entity);
		if (it != rigid_bodies_.end() && it->second.body) {
			btTransform trans;
			it->second.body->getMotionState()->getWorldTransform(trans);

			btVector3 pos = trans.getOrigin();
			btQuaternion rot = trans.getRotation();
			result.position = glm::vec3(pos.x(), pos.y(), pos.z());
			result.rotation = glm::quat(rot.w(), rot.x(), rot.y(), rot.z());

			btVector3 linVel = it->second.body->getLinearVelocity();
			result.linear_velocity = glm::vec3(linVel.x(), linVel.y(), linVel.z());

			btVector3 angVel = it->second.body->getAngularVelocity();
			result.angular_velocity = glm::vec3(angVel.x(), angVel.y(), angVel.z());
		}

		return result;
	}

	void RemoveBody(EntityId entity) override {
		auto it = rigid_bodies_.find(entity);
		if (it != rigid_bodies_.end()) {
			if (it->second.body) {
				dynamics_world_->removeRigidBody(it->second.body.get());
			}
			rigid_bodies_.erase(it);
		}
	}

	[[nodiscard]] bool HasBody(EntityId entity) const override { return rigid_bodies_.contains(entity); }

	void ApplyForce(EntityId entity, const glm::vec3& force, const glm::vec3& torque) override {
		auto it = rigid_bodies_.find(entity);
		if (it != rigid_bodies_.end() && it->second.body) {
			if (glm::length(force) > 0.0F) {
				it->second.body->applyCentralForce(btVector3(force.x, force.y, force.z));
			}
			if (glm::length(torque) > 0.0F) {
				it->second.body->applyTorque(btVector3(torque.x, torque.y, torque.z));
			}
			it->second.body->activate();
		}
	}

	void ApplyImpulse(EntityId entity, const glm::vec3& impulse, const glm::vec3& point) override {
		auto it = rigid_bodies_.find(entity);
		if (it != rigid_bodies_.end() && it->second.body) {
			if (glm::length(point) > 0.0F) {
				// Apply at specific point (relative to center of mass)
				btVector3 relPos = btVector3(point.x, point.y, point.z) - it->second.body->getCenterOfMassPosition();
				it->second.body->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), relPos);
			}
			else {
				// Apply at center of mass
				it->second.body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
			}
			it->second.body->activate();
		}
	}

	[[nodiscard]] std::vector<CollisionInfo> GetCollisionEvents() const override { return collision_events_; }

	[[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& ray) const override {
		if (!dynamics_world_) {
			return std::nullopt;
		}

		btVector3 from(ray.origin.x, ray.origin.y, ray.origin.z);
		btVector3 to(
				ray.origin.x + ray.direction.x * ray.max_distance,
				ray.origin.y + ray.direction.y * ray.max_distance,
				ray.origin.z + ray.direction.z * ray.max_distance);

		btCollisionWorld::ClosestRayResultCallback callback(from, to);
		dynamics_world_->rayTest(from, to, callback);

		if (callback.hasHit()) {
			RaycastResult result;
			result.entity =
					static_cast<EntityId>(reinterpret_cast<uintptr_t>(callback.m_collisionObject->getUserPointer()));
			result.hit_point =
					glm::vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
			result.hit_normal = glm::vec3(
					callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(), callback.m_hitNormalWorld.z());
			result.distance = callback.m_closestHitFraction * ray.max_distance;
			return result;
		}

		return std::nullopt;
	}

	[[nodiscard]] std::vector<RaycastResult> RaycastAll(const Ray& ray) const override {
		if (!dynamics_world_) {
			return {};
		}

		btVector3 from(ray.origin.x, ray.origin.y, ray.origin.z);
		btVector3 to(
				ray.origin.x + ray.direction.x * ray.max_distance,
				ray.origin.y + ray.direction.y * ray.max_distance,
				ray.origin.z + ray.direction.z * ray.max_distance);

		btCollisionWorld::AllHitsRayResultCallback callback(from, to);
		dynamics_world_->rayTest(from, to, callback);

		std::vector<RaycastResult> results;
		for (int i = 0; i < callback.m_collisionObjects.size(); ++i) {
			RaycastResult result;
			result.entity = static_cast<EntityId>(
					reinterpret_cast<uintptr_t>(callback.m_collisionObjects[i]->getUserPointer()));
			result.hit_point = glm::vec3(
					callback.m_hitPointWorld[i].x(), callback.m_hitPointWorld[i].y(), callback.m_hitPointWorld[i].z());
			result.hit_normal = glm::vec3(
					callback.m_hitNormalWorld[i].x(),
					callback.m_hitNormalWorld[i].y(),
					callback.m_hitNormalWorld[i].z());
			result.distance = callback.m_hitFractions[i] * ray.max_distance;
			results.push_back(result);
		}

		return results;
	}

	bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& /*config*/) override {
		spdlog::info("[Bullet3] AddConstraint between {} and {}", entity_a, entity_b);
		return true;
	}

	void RemoveConstraint(EntityId /*entity_a*/, EntityId /*entity_b*/) override {}

	[[nodiscard]] std::string GetEngineName() const override { return "Bullet3"; }

	~Bullet3Backend() override { Shutdown(); }
};

// Factory function - uses raw new because C++ modules don't support make_unique
// for types defined in module implementation units
std::unique_ptr<IPhysicsBackend> CreateBullet3Backend() {
	return std::unique_ptr<IPhysicsBackend>(new Bullet3Backend());
}

} // namespace engine::physics
