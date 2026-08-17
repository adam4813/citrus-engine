module;

#include <cstdarg>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

// Jolt Physics includes. Order matters here.
// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Core/Color.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif
// clang-format on

module engine.physics;

import glm;

// Jolt callback for trace messages
static void JoltTraceImpl(const char* inFMT, ...) {
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);
	spdlog::trace("[JoltPhysics] {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
// Jolt callback for asserts
static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine) {
	spdlog::error(
		"[JoltPhysics] Assert failed: {} - {} ({}:{})",
		inExpression,
		inMessage ? inMessage : "",
		inFile,
		inLine
	);
	return true; // Break into debugger
}
#endif

namespace engine::physics {

// Layer definitions for Jolt
namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

// BroadPhase layer definitions
namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS = 2;
} // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
	BPLayerInterfaceImpl() {
		object_to_broad_phase_[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		object_to_broad_phase_[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	[[nodiscard]] uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

	[[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return object_to_broad_phase_[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	[[nodiscard]] const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
		switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING): return "NON_MOVING";
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING): return "MOVING";
		default: JPH_ASSERT(false); return "INVALID";
		}
	}
#endif

private:
	JPH::BroadPhaseLayer object_to_broad_phase_[Layers::NUM_LAYERS]{};
};

// ObjectVsBroadPhaseLayerFilter implementation
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
	[[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
		switch (inLayer1) {
		case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING: return true;
		default: JPH_ASSERT(false); return false;
		}
	}
};

// ObjectLayerPairFilter implementation
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
	[[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
		switch (inObject1) {
		case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
		case Layers::MOVING: return true;
		default: JPH_ASSERT(false); return false;
		}
	}
};

// Contact listener for collision events
class ContactListenerImpl : public JPH::ContactListener {
public:
	void ClearEvents() { collision_events_.clear(); }

	[[nodiscard]] const std::vector<CollisionInfo>& GetEvents() const { return collision_events_; }

	JPH::ValidateResult OnContactValidate(
		const JPH::Body& /*inBody1*/,
		const JPH::Body& /*inBody2*/,
		JPH::RVec3Arg /*inBaseOffset*/,
		const JPH::CollideShapeResult& /*inCollisionResult*/
	) override {
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void OnContactAdded(
		const JPH::Body& inBody1,
		const JPH::Body& inBody2,
		const JPH::ContactManifold& inManifold,
		JPH::ContactSettings& /*ioSettings*/
	) override {
		CollisionInfo info;
		info.entity_a = inBody1.GetUserData();
		info.entity_b = inBody2.GetUserData();

		// Extract contact points
		for (uint32_t i = 0; i < inManifold.mRelativeContactPointsOn1.size(); ++i) {
			ContactPoint point;
			JPH::Vec3 const world_point = inManifold.GetWorldSpaceContactPointOn1(i);
			point.position = glm::vec3(world_point.GetX(), world_point.GetY(), world_point.GetZ());
			point.normal = glm::vec3(
				inManifold.mWorldSpaceNormal.GetX(),
				inManifold.mWorldSpaceNormal.GetY(),
				inManifold.mWorldSpaceNormal.GetZ()
			);
			point.penetration_depth = inManifold.mPenetrationDepth;
			info.contacts.push_back(point);
		}

		collision_events_.push_back(info);
	}

private:
	std::vector<CollisionInfo> collision_events_;
};

// JoltPhysics backend implementation
class JoltPhysicsBackend : public IPhysicsBackend {
private:
	PhysicsConfig config_{};
	bool initialized_{false};

	// Jolt objects
	std::unique_ptr<JPH::TempAllocatorImpl> temp_allocator_;
	std::unique_ptr<JPH::JobSystemThreadPool> job_system_;
	std::unique_ptr<BPLayerInterfaceImpl> broad_phase_layer_interface_;
	std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> object_vs_broad_phase_layer_filter_;
	std::unique_ptr<ObjectLayerPairFilterImpl> object_layer_pair_filter_;
	std::unique_ptr<JPH::PhysicsSystem> physics_system_;
	std::unique_ptr<ContactListenerImpl> contact_listener_;

	// Entity to body mapping
	std::unordered_map<EntityId, JPH::BodyID> entity_to_body_;

	// Helper to convert motion type
	static JPH::EMotionType ToJoltMotionType(MotionType type) {
		switch (type) {
		case MotionType::Static: return JPH::EMotionType::Static;
		case MotionType::Kinematic: return JPH::EMotionType::Kinematic;
		case MotionType::Dynamic: return JPH::EMotionType::Dynamic;
		default: return JPH::EMotionType::Dynamic;
		}
	}

	// Helper to get object layer from motion type
	static JPH::ObjectLayer GetObjectLayer(MotionType type) {
		return type == MotionType::Static ? Layers::NON_MOVING : Layers::MOVING;
	}

	// Helper to convert CollisionShape component to ShapeConfig
	static ShapeConfig ToShapeConfig(const CollisionShape& shape, const PhysicsTransform& transform) {
		// For non-uniform scaling, we take the maximum scale component to ensure the shape fully encompasses the scaled dimensions.
		// TODO: Update non-uniform scaling for non-boxes
		const auto scale = glm::length(transform.scale) > 0.0F ? transform.scale : glm::vec3(1.0F); // Avoid zero scale
		const auto max_scale = std::max({scale.x, scale.y, scale.z});
		ShapeConfig config;
		config.type = shape.type;
		config.box_half_extents = shape.box_half_extents * scale;   // Apply world scale to box half-extents
		config.sphere_radius = shape.sphere_radius * max_scale;     // Use max scale for sphere radius
		config.capsule_radius = shape.capsule_radius * max_scale;   // Use max scale for capsule radius
		config.capsule_height = shape.capsule_height * max_scale;   // Use max scale for capsule height
		config.cylinder_radius = shape.cylinder_radius * max_scale; // Use max scale for cylinder radius
		config.cylinder_height = shape.cylinder_height * max_scale; // Use max scale for cylinder height
		config.offset = shape.offset;
		config.rotation = shape.rotation;
		// Convert compound children from CollisionShape to ShapeConfig format
		for (const auto& child : shape.compound_children) {
			ShapeConfig child_config;
			child_config.type = child.type;
			child_config.box_half_extents = child.box_half_extents * scale;
			child_config.sphere_radius = child.sphere_radius * max_scale;
			child_config.capsule_radius = child.capsule_radius * max_scale;
			child_config.capsule_height = child.capsule_height * max_scale;
			child_config.cylinder_radius = child.cylinder_radius * max_scale;
			child_config.cylinder_height = child.cylinder_height * max_scale;
			config.children.push_back(child_config);
			config.child_positions.push_back(child.position);
			config.child_rotations.push_back(child.rotation);
		}
		return config;
	}

	// Helper to create Jolt shape from config
	static JPH::ShapeRefC CreateShape(const ShapeConfig& config) {
		JPH::ShapeSettings::ShapeResult result;

		switch (config.type) {
		case ShapeType::Box:
		{
			JPH::BoxShapeSettings const settings(
				JPH::Vec3(config.box_half_extents.x, config.box_half_extents.y, config.box_half_extents.z)
			);
			result = settings.Create();
			break;
		}
		case ShapeType::Sphere:
		{
			JPH::SphereShapeSettings const settings(config.sphere_radius);
			result = settings.Create();
			break;
		}
		case ShapeType::Capsule:
		{
			// Jolt capsule uses half-height (height of cylindrical portion / 2)
			JPH::CapsuleShapeSettings const settings(config.capsule_height * 0.5F, config.capsule_radius);
			result = settings.Create();
			break;
		}
		case ShapeType::Cylinder:
		{
			JPH::CylinderShapeSettings const settings(config.cylinder_height * 0.5F, config.cylinder_radius);
			result = settings.Create();
			break;
		}
		case ShapeType::ConvexHull:
		{
			if (!config.vertices.empty()) {
				std::vector<JPH::Vec3> jolt_verts;
				jolt_verts.reserve(config.vertices.size());
				for (const auto& v : config.vertices) {
					jolt_verts.emplace_back(v.x, v.y, v.z);
				}
				JPH::ConvexHullShapeSettings const settings(jolt_verts.data(), static_cast<int>(jolt_verts.size()));
				result = settings.Create();
			}
			break;
		}
		case ShapeType::Mesh:
		{
			if (!config.vertices.empty() && !config.indices.empty()) {
				JPH::TriangleList triangles;
				for (size_t i = 0; i + 2 < config.indices.size(); i += 3) {
					const auto& v0 = config.vertices[config.indices[i]];
					const auto& v1 = config.vertices[config.indices[i + 1]];
					const auto& v2 = config.vertices[config.indices[i + 2]];
					triangles.push_back(
						JPH::Triangle(
							JPH::Float3(v0.x, v0.y, v0.z),
							JPH::Float3(v1.x, v1.y, v1.z),
							JPH::Float3(v2.x, v2.y, v2.z)
						)
					);
				}
				JPH::MeshShapeSettings const settings(triangles);
				result = settings.Create();
			}
			break;
		}
		case ShapeType::Compound:
		{
			JPH::StaticCompoundShapeSettings settings;
			for (size_t i = 0; i < config.children.size(); ++i) {
				if (auto child_shape = CreateShape(config.children[i])) {
					glm::vec3 pos = i < config.child_positions.size() ? config.child_positions[i] : glm::vec3(0.0F);
					glm::quat rot = i < config.child_rotations.size() ? config.child_rotations[i]
																	  : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
					settings.AddShape(
						JPH::Vec3(pos.x, pos.y, pos.z),
						JPH::Quat(rot.x, rot.y, rot.z, rot.w),
						child_shape
					);
				}
			}
			result = settings.Create();
			break;
		}
		default:
			spdlog::warn("[JoltPhysics] Unknown shape type, defaulting to box");
			JPH::BoxShapeSettings const settings(JPH::Vec3(0.5F, 0.5F, 0.5F));
			result = settings.Create();
			break;
		}

		if (result.HasError()) {
			spdlog::error("[JoltPhysics] Failed to create shape: {}", result.GetError().c_str());
			return nullptr;
		}

		return result.Get();
	}

public:
	bool Initialize(const PhysicsConfig& config) override {
		if (initialized_) {
			spdlog::warn("[JoltPhysics] Already initialized");
			return true;
		}

		config_ = config;

		// Register Jolt allocators and trace
		JPH::RegisterDefaultAllocator();
		JPH::Trace = JoltTraceImpl;
#ifdef JPH_ENABLE_ASSERTS
		JPH::AssertFailed = JoltAssertFailedImpl;
#endif

		// Create factory - Jolt uses a global singleton pattern
		// We allocate here and delete in Shutdown() to match Jolt's expected lifecycle
		JPH::Factory::sInstance = new JPH::Factory();

		// Register all physics types
		JPH::RegisterTypes();

		// Create temp allocator (10MB)
		temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

		// Create job system (use hardware thread count - 1, min 1)
		auto num_threads = std::max(1U, std::thread::hardware_concurrency() - 1);
		job_system_ = std::make_unique<JPH::JobSystemThreadPool>(
			JPH::cMaxPhysicsJobs,
			JPH::cMaxPhysicsBarriers,
			static_cast<int>(num_threads)
		);

		// Create broad phase layer interface
		broad_phase_layer_interface_ = std::make_unique<BPLayerInterfaceImpl>();
		object_vs_broad_phase_layer_filter_ = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
		object_layer_pair_filter_ = std::make_unique<ObjectLayerPairFilterImpl>();

		// Create physics system
		constexpr uint32_t c_max_bodies = 65536;
		constexpr uint32_t c_num_body_mutexes = 0; // Auto-detect
		constexpr uint32_t c_max_body_pairs = 65536;
		constexpr uint32_t c_max_contact_constraints = 10240;

		physics_system_ = std::make_unique<JPH::PhysicsSystem>();
		physics_system_->Init(
			c_max_bodies,
			c_num_body_mutexes,
			c_max_body_pairs,
			c_max_contact_constraints,
			*broad_phase_layer_interface_,
			*object_vs_broad_phase_layer_filter_,
			*object_layer_pair_filter_
		);

		// Set gravity
		physics_system_->SetGravity(JPH::Vec3(config.gravity.x, config.gravity.y, config.gravity.z));

		// Create and set contact listener
		contact_listener_ = std::make_unique<ContactListenerImpl>();
		physics_system_->SetContactListener(contact_listener_.get());

		spdlog::info("[JoltPhysics] Initialized with {} worker threads", num_threads);
		initialized_ = true;
		return true;
	}

	void Shutdown() override {
		if (!initialized_) {
			return;
		}

		// Remove all bodies
		auto& body_interface = physics_system_->GetBodyInterface();
		for (auto& body_id : entity_to_body_ | std::views::values) {
			body_interface.RemoveBody(body_id);
			body_interface.DestroyBody(body_id);
		}
		entity_to_body_.clear();

		// Clean up Jolt
		physics_system_.reset();
		contact_listener_.reset();
		object_layer_pair_filter_.reset();
		object_vs_broad_phase_layer_filter_.reset();
		broad_phase_layer_interface_.reset();
		job_system_.reset();
		temp_allocator_.reset();

		// Unregister types and destroy factory
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		spdlog::info("[JoltPhysics] Shutdown");
		initialized_ = false;
	}

	void SetGravity(const glm::vec3& gravity) override {
		if (physics_system_) {
			physics_system_->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
		}
	}

	[[nodiscard]] glm::vec3 GetGravity() const override {
		if (physics_system_) {
			auto g = physics_system_->GetGravity();
			return {g.GetX(), g.GetY(), g.GetZ()};
		}
		return config_.gravity;
	}

	void StepSimulation(float delta_time) override {
		if (!initialized_ || !physics_system_) {
			return;
		}

		// Clear collision events from previous frame
		if (contact_listener_) {
			contact_listener_->ClearEvents();
		}

		physics_system_->Update(delta_time, config_.collision_steps, temp_allocator_.get(), job_system_.get());
	}

	void SyncBodyToBackend(
		EntityId entity,
		const PhysicsTransform& transform,
		const RigidBody& body,
		const CollisionShape& shape
	) override {
		if (!initialized_) {
			spdlog::error("[JoltPhysics] Cannot sync body - not initialized");
			return;
		}

		auto& body_interface = physics_system_->GetBodyInterface();
		auto it = entity_to_body_.find(entity);

		// Create shape from CollisionShape component
		ShapeConfig const shape_config = ToShapeConfig(shape, transform);
		JPH::ShapeRefC const jolt_shape = CreateShape(shape_config);
		if (!jolt_shape) {
			spdlog::error("[JoltPhysics] Failed to create shape for entity {}", entity);
			return;
		}

		if (it != entity_to_body_.end()) {
			// Body exists - update it
			JPH::BodyID const body_id = it->second;

			// Update shape
			body_interface.SetShape(body_id, jolt_shape, true, JPH::EActivation::Activate);

			// Update transform
			body_interface.SetPositionAndRotation(
				body_id,
				JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
				JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w),
				JPH::EActivation::Activate
			);

			// Update physics properties
			body_interface.SetFriction(body_id, body.friction);
			body_interface.SetRestitution(body_id, body.restitution);
			body_interface.SetGravityFactor(body_id, body.use_gravity ? body.gravity_scale : 0.0F);

			// Update CCD
			body_interface.SetMotionQuality(
				body_id,
				body.enable_ccd ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete
			);
		}
		else {
			// Create new body
			JPH::BodyCreationSettings body_settings(
				jolt_shape,
				JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
				JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w),
				ToJoltMotionType(body.motion_type),
				GetObjectLayer(body.motion_type)
			);

			// Set physics properties
			body_settings.mFriction = body.friction;
			body_settings.mRestitution = body.restitution;
			body_settings.mLinearDamping = body.linear_damping;
			body_settings.mAngularDamping = body.angular_damping;
			body_settings.mGravityFactor = body.use_gravity ? body.gravity_scale : 0.0F;
			body_settings.mAllowSleeping = true;

			if (body.motion_type == MotionType::Dynamic) {
				body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
				body_settings.mMassPropertiesOverride.mMass = body.mass;
			}

			// CCD settings
			if (body.enable_ccd) {
				body_settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
			}

			// Store entity ID in user data
			body_settings.mUserData = entity;

			// Create body
			JPH::BodyID const body_id = body_interface.CreateAndAddBody(body_settings, JPH::EActivation::Activate);

			if (body_id.IsInvalid()) {
				spdlog::error("[JoltPhysics] Failed to create body for entity {}", entity);
				return;
			}

			entity_to_body_[entity] = body_id;
		}
	}

	[[nodiscard]] PhysicsSyncResult SyncBodyFromBackend(EntityId entity) const override {
		PhysicsSyncResult result;

		auto it = entity_to_body_.find(entity);
		if (it != entity_to_body_.end()) {
			auto& body_interface = physics_system_->GetBodyInterface();
			JPH::RVec3 pos;
			JPH::Quat rot{};
			body_interface.GetPositionAndRotation(it->second, pos, rot);

			result.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
			result.rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());

			auto linear_vel = body_interface.GetLinearVelocity(it->second);
			result.linear_velocity = glm::vec3(linear_vel.GetX(), linear_vel.GetY(), linear_vel.GetZ());

			auto angular_vel = body_interface.GetAngularVelocity(it->second);
			result.angular_velocity = glm::vec3(angular_vel.GetX(), angular_vel.GetY(), angular_vel.GetZ());
		}

		return result;
	}

	void RemoveBody(EntityId entity) override {
		auto it = entity_to_body_.find(entity);
		if (it != entity_to_body_.end()) {
			auto& body_interface = physics_system_->GetBodyInterface();
			body_interface.RemoveBody(it->second);
			body_interface.DestroyBody(it->second);
			entity_to_body_.erase(it);
		}
	}

	[[nodiscard]] bool HasBody(EntityId entity) const override { return entity_to_body_.contains(entity); }

	void ApplyForce(EntityId entity, const glm::vec3& force, const glm::vec3& torque) override {
		auto it = entity_to_body_.find(entity);
		if (it != entity_to_body_.end()) {
			auto& body_interface = physics_system_->GetBodyInterface();
			if (glm::length(force) > 0.0F) {
				body_interface.AddForce(it->second, JPH::Vec3(force.x, force.y, force.z));
			}
			if (glm::length(torque) > 0.0F) {
				body_interface.AddTorque(it->second, JPH::Vec3(torque.x, torque.y, torque.z));
			}
		}
	}

	void ApplyImpulse(EntityId entity, const glm::vec3& impulse, const glm::vec3& point) override {
		auto it = entity_to_body_.find(entity);
		if (it != entity_to_body_.end()) {
			auto& body_interface = physics_system_->GetBodyInterface();
			if (glm::length(point) > 0.0F) {
				// Apply at specific point
				body_interface.AddImpulse(
					it->second,
					JPH::Vec3(impulse.x, impulse.y, impulse.z),
					JPH::RVec3(point.x, point.y, point.z)
				);
			}
			else {
				// Apply at center of mass
				body_interface.AddImpulse(it->second, JPH::Vec3(impulse.x, impulse.y, impulse.z));
			}
		}
	}

	[[nodiscard]] std::vector<CollisionInfo> GetCollisionEvents() const override {
		if (contact_listener_) {
			return contact_listener_->GetEvents();
		}
		return {};
	}

	[[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& ray) const override {
		if (!physics_system_) {
			return std::nullopt;
		}

		JPH::RRayCast const jolt_ray(
			JPH::RVec3(ray.origin.x, ray.origin.y, ray.origin.z),
			JPH::Vec3(
				ray.direction.x * ray.max_distance,
				ray.direction.y * ray.max_distance,
				ray.direction.z * ray.max_distance
			)
		);

		if (JPH::RayCastResult hit; physics_system_->GetNarrowPhaseQuery().CastRay(jolt_ray, hit)) {
			RaycastResult result;
			const auto hit_point = jolt_ray.GetPointOnRay(hit.mFraction);
			result.hit_point = glm::vec3(hit_point.GetX(), hit_point.GetY(), hit_point.GetZ());
			result.distance = hit.mFraction * ray.max_distance;

			// Get entity from body
			if (const auto* body = physics_system_->GetBodyLockInterface().TryGetBody(hit.mBodyID)) {
				result.entity = body->GetUserData();
			}

			return result;
		}

		return std::nullopt;
	}

	[[nodiscard]] std::vector<RaycastResult> RaycastAll(const Ray& ray) const override {
		// Jolt doesn't have a built-in "raycast all" - would need custom collector
		// For now, just return single hit
		if (auto hit = Raycast(ray)) {
			return {*hit};
		}
		return {};
	}

	bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& /*config*/) override {
		// Constraint implementation would require more complex Jolt constraint setup
		spdlog::info("[JoltPhysics] AddConstraint between {} and {} (basic implementation)", entity_a, entity_b);
		return true;
	}

	void RemoveConstraint(EntityId /*entity_a*/, EntityId /*entity_b*/) override {
		// Constraint removal would need constraint tracking
	}

	[[nodiscard]] std::string GetEngineName() const override { return "JoltPhysics"; }

#ifdef JPH_DEBUG_RENDERER
	void DebugDraw(IPhysicsDebugRenderer& renderer) override {
		if (!initialized_ || !physics_system_) return;

		// Inline adapter: bridges Jolt's DebugRendererSimple to our IPhysicsDebugRenderer
		class Adapter : public JPH::DebugRendererSimple {
		public:
			explicit Adapter(IPhysicsDebugRenderer& r) : target_(r) { Initialize(); }

			void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg c) override {
				target_.DrawLine(
					{from.GetX(), from.GetY(), from.GetZ()},
					{to.GetX(), to.GetY(), to.GetZ()},
					{c.r / 255.0F, c.g / 255.0F, c.b / 255.0F}
				);
			}

			void DrawTriangle(
				JPH::RVec3Arg v1,
				JPH::RVec3Arg v2,
				JPH::RVec3Arg v3,
				JPH::ColorArg c,
				ECastShadow /*inCastShadow*/
			) override {
				const glm::vec3 color{c.r / 255.0F, c.g / 255.0F, c.b / 255.0F};
				target_.DrawTriangle(
					{v1.GetX(), v1.GetY(), v1.GetZ()},
					{v2.GetX(), v2.GetY(), v2.GetZ()},
					{v3.GetX(), v3.GetY(), v3.GetZ()},
					color,
					c.a / 255.0F
				);
			}

			void DrawText3D(
				JPH::RVec3Arg pos,
				const std::string_view& text,
				JPH::ColorArg /*inColor*/,
				float /*inHeight*/
			) override {
				target_.DrawText({pos.GetX(), pos.GetY(), pos.GetZ()}, std::string(text));
			}

		private:
			IPhysicsDebugRenderer& target_;
		};

		Adapter adapter(renderer);

		JPH::BodyManager::DrawSettings settings;
		settings.mDrawShape = true;
		settings.mDrawShapeWireframe = true;
		settings.mDrawBoundingBox = false;
		settings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;

		physics_system_->DrawBodies(settings, &adapter);
	}
#endif

	~JoltPhysicsBackend() override { Shutdown(); }
};

// Factory function - uses raw new because C++ modules don't support make_unique
// for types defined in module implementation units
std::unique_ptr<IPhysicsBackend> CreateJoltBackend() {
	return std::unique_ptr<IPhysicsBackend>(new JoltPhysicsBackend());
}

} // namespace engine::physics
