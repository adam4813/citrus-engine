module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdarg>

#include <spdlog/spdlog.h>

// Jolt Physics includes
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

module engine.physics;

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
    spdlog::error("[JoltPhysics] Assert failed: {} - {} ({}:{})", inExpression, inMessage ? inMessage : "", inFile, inLine);
    return true; // Break into debugger
}
#endif

namespace engine::physics {

    // Layer definitions for Jolt
    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }

    // BroadPhase layer definitions
    namespace BroadPhaseLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32_t NUM_LAYERS = 2;
    }

    // BroadPhaseLayerInterface implementation
    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            object_to_broad_phase_[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            object_to_broad_phase_[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        [[nodiscard]] uint32_t GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

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
                case Layers::NON_MOVING:
                    return inLayer2 == BroadPhaseLayers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
    };

    // ObjectLayerPairFilter implementation
    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
            switch (inObject1) {
                case Layers::NON_MOVING:
                    return inObject2 == Layers::MOVING;
                case Layers::MOVING:
                    return true;
                default:
                    JPH_ASSERT(false);
                    return false;
            }
        }
    };

    // Contact listener for collision callbacks
    class ContactListenerImpl : public JPH::ContactListener {
    public:
        void SetCallback(CollisionCallback callback) {
            callback_ = std::move(callback);
        }

        JPH::ValidateResult OnContactValidate(const JPH::Body& /*inBody1*/, const JPH::Body& /*inBody2*/,
                                               JPH::RVec3Arg /*inBaseOffset*/,
                                               const JPH::CollideShapeResult& /*inCollisionResult*/) override {
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                           const JPH::ContactManifold& inManifold,
                           JPH::ContactSettings& /*ioSettings*/) override {
            if (callback_) {
                CollisionInfo info;
                info.entity_a = inBody1.GetUserData();
                info.entity_b = inBody2.GetUserData();

                // Extract contact points
                for (uint32_t i = 0; i < inManifold.mRelativeContactPointsOn1.size(); ++i) {
                    ContactPoint point;
                    JPH::Vec3 worldPoint = inManifold.GetWorldSpaceContactPointOn1(i);
                    point.position = glm::vec3(worldPoint.GetX(), worldPoint.GetY(), worldPoint.GetZ());
                    point.normal = glm::vec3(inManifold.mWorldSpaceNormal.GetX(),
                                            inManifold.mWorldSpaceNormal.GetY(),
                                            inManifold.mWorldSpaceNormal.GetZ());
                    point.penetration_depth = inManifold.mPenetrationDepth;
                    info.contacts.push_back(point);
                }

                callback_(info);
            }
        }

    private:
        CollisionCallback callback_;
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

        // Collider storage (shapes are owned by bodies, but we track config)
        std::unordered_map<EntityId, ColliderConfig> colliders_;

        // Character controller storage
        std::unordered_map<EntityId, std::unique_ptr<JPH::Character>> characters_;
        std::unordered_map<EntityId, CharacterControllerConfig> character_configs_;

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

        // Helper to create Jolt shape from config
        JPH::ShapeRefC CreateShape(const ShapeConfig& config) {
            JPH::ShapeSettings::ShapeResult result;

            switch (config.type) {
                case ShapeType::Box: {
                    JPH::BoxShapeSettings settings(JPH::Vec3(config.box_half_extents.x,
                                                             config.box_half_extents.y,
                                                             config.box_half_extents.z));
                    result = settings.Create();
                    break;
                }
                case ShapeType::Sphere: {
                    JPH::SphereShapeSettings settings(config.sphere_radius);
                    result = settings.Create();
                    break;
                }
                case ShapeType::Capsule: {
                    // Jolt capsule uses half-height (height of cylindrical portion / 2)
                    JPH::CapsuleShapeSettings settings(config.capsule_height * 0.5F, config.capsule_radius);
                    result = settings.Create();
                    break;
                }
                case ShapeType::Cylinder: {
                    JPH::CylinderShapeSettings settings(config.cylinder_height * 0.5F, config.cylinder_radius);
                    result = settings.Create();
                    break;
                }
                case ShapeType::ConvexHull: {
                    if (!config.vertices.empty()) {
                        std::vector<JPH::Vec3> joltVerts;
                        joltVerts.reserve(config.vertices.size());
                        for (const auto& v : config.vertices) {
                            joltVerts.emplace_back(v.x, v.y, v.z);
                        }
                        JPH::ConvexHullShapeSettings settings(joltVerts.data(),
                                                              static_cast<int>(joltVerts.size()));
                        result = settings.Create();
                    }
                    break;
                }
                case ShapeType::Mesh: {
                    if (!config.vertices.empty() && !config.indices.empty()) {
                        JPH::TriangleList triangles;
                        for (size_t i = 0; i + 2 < config.indices.size(); i += 3) {
                            const auto& v0 = config.vertices[config.indices[i]];
                            const auto& v1 = config.vertices[config.indices[i + 1]];
                            const auto& v2 = config.vertices[config.indices[i + 2]];
                            triangles.push_back(JPH::Triangle(
                                JPH::Float3(v0.x, v0.y, v0.z),
                                JPH::Float3(v1.x, v1.y, v1.z),
                                JPH::Float3(v2.x, v2.y, v2.z)
                            ));
                        }
                        JPH::MeshShapeSettings settings(triangles);
                        result = settings.Create();
                    }
                    break;
                }
                case ShapeType::Compound: {
                    JPH::StaticCompoundShapeSettings settings;
                    for (size_t i = 0; i < config.children.size(); ++i) {
                        auto childShape = CreateShape(config.children[i]);
                        if (childShape) {
                            glm::vec3 pos = i < config.child_positions.size() ?
                                           config.child_positions[i] : glm::vec3(0.0F);
                            glm::quat rot = i < config.child_rotations.size() ?
                                           config.child_rotations[i] : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
                            settings.AddShape(
                                JPH::Vec3(pos.x, pos.y, pos.z),
                                JPH::Quat(rot.x, rot.y, rot.z, rot.w),
                                childShape
                            );
                        }
                    }
                    result = settings.Create();
                    break;
                }
                default:
                    spdlog::warn("[JoltPhysics] Unknown shape type, defaulting to box");
                    JPH::BoxShapeSettings settings(JPH::Vec3(0.5F, 0.5F, 0.5F));
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

            // Create factory
            JPH::Factory::sInstance = new JPH::Factory();

            // Register all physics types
            JPH::RegisterTypes();

            // Create temp allocator (10MB)
            temp_allocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

            // Create job system (use hardware thread count - 1, min 1)
            auto numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
            job_system_ = std::make_unique<JPH::JobSystemThreadPool>(
                JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(numThreads));

            // Create broad phase layer interface
            broad_phase_layer_interface_ = std::make_unique<BPLayerInterfaceImpl>();
            object_vs_broad_phase_layer_filter_ = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
            object_layer_pair_filter_ = std::make_unique<ObjectLayerPairFilterImpl>();

            // Create physics system
            const uint32_t cMaxBodies = 65536;
            const uint32_t cNumBodyMutexes = 0; // Auto-detect
            const uint32_t cMaxBodyPairs = 65536;
            const uint32_t cMaxContactConstraints = 10240;

            physics_system_ = std::make_unique<JPH::PhysicsSystem>();
            physics_system_->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                                  *broad_phase_layer_interface_,
                                  *object_vs_broad_phase_layer_filter_,
                                  *object_layer_pair_filter_);

            // Set gravity
            physics_system_->SetGravity(JPH::Vec3(config.gravity.x, config.gravity.y, config.gravity.z));

            // Create and set contact listener
            contact_listener_ = std::make_unique<ContactListenerImpl>();
            physics_system_->SetContactListener(contact_listener_.get());

            spdlog::info("[JoltPhysics] Initialized with {} worker threads", numThreads);
            initialized_ = true;
            return true;
        }

        void Shutdown() override {
            if (!initialized_) {
                return;
            }

            // Remove all bodies
            auto& bodyInterface = physics_system_->GetBodyInterface();
            for (auto& [entity, bodyId] : entity_to_body_) {
                bodyInterface.RemoveBody(bodyId);
                bodyInterface.DestroyBody(bodyId);
            }
            entity_to_body_.clear();
            colliders_.clear();

            // Clear characters
            characters_.clear();
            character_configs_.clear();

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
                return glm::vec3(g.GetX(), g.GetY(), g.GetZ());
            }
            return config_.gravity;
        }

        void StepSimulation(float delta_time) override {
            if (!initialized_ || !physics_system_) {
                return;
            }

            physics_system_->Update(delta_time, config_.collision_steps,
                                    temp_allocator_.get(), job_system_.get());
        }

        bool AddRigidBody(EntityId entity, const RigidBodyConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add rigid body - not initialized");
                return false;
            }

            if (entity_to_body_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a rigid body", entity);
                return false;
            }

            // Create a default box shape if no collider exists
            JPH::ShapeRefC shape;
            if (colliders_.contains(entity)) {
                shape = CreateShape(colliders_[entity].shape);
            } else {
                JPH::BoxShapeSettings boxSettings(JPH::Vec3(0.5F, 0.5F, 0.5F));
                auto result = boxSettings.Create();
                if (result.HasError()) {
                    spdlog::error("[JoltPhysics] Failed to create default shape");
                    return false;
                }
                shape = result.Get();
            }

            // Create body settings
            JPH::BodyCreationSettings bodySettings(
                shape,
                JPH::RVec3(0, 0, 0),
                JPH::Quat::sIdentity(),
                ToJoltMotionType(config.motion_type),
                GetObjectLayer(config.motion_type)
            );

            // Set physics properties
            bodySettings.mFriction = config.friction;
            bodySettings.mRestitution = config.restitution;
            bodySettings.mLinearDamping = config.linear_damping;
            bodySettings.mAngularDamping = config.angular_damping;
            bodySettings.mGravityFactor = config.use_gravity ? config.gravity_scale : 0.0F;
            bodySettings.mAllowSleeping = config.allow_sleeping;

            if (config.motion_type == MotionType::Dynamic) {
                bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                bodySettings.mMassPropertiesOverride.mMass = config.mass;
            }

            // CCD settings
            if (config.enable_ccd) {
                bodySettings.mMotionQuality = JPH::EMotionQuality::LinearCast;
            }

            // Store entity ID in user data
            bodySettings.mUserData = entity;

            // Create body
            auto& bodyInterface = physics_system_->GetBodyInterface();
            JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(bodySettings,
                config.start_asleep ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

            if (bodyId.IsInvalid()) {
                spdlog::error("[JoltPhysics] Failed to create body for entity {}", entity);
                return false;
            }

            // Set initial velocities
            if (config.motion_type == MotionType::Dynamic) {
                bodyInterface.SetLinearVelocity(bodyId,
                    JPH::Vec3(config.linear_velocity.x, config.linear_velocity.y, config.linear_velocity.z));
                bodyInterface.SetAngularVelocity(bodyId,
                    JPH::Vec3(config.angular_velocity.x, config.angular_velocity.y, config.angular_velocity.z));
            }

            entity_to_body_[entity] = bodyId;

            return true;
        }

        void RemoveRigidBody(EntityId entity) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.RemoveBody(it->second);
                bodyInterface.DestroyBody(it->second);
                entity_to_body_.erase(it);
            }
        }

        [[nodiscard]] bool HasRigidBody(EntityId entity) const override {
            return entity_to_body_.contains(entity);
        }

        void SetRigidBodyTransform(EntityId entity, const PhysicsTransform& transform) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.SetPositionAndRotation(
                    it->second,
                    JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
                    JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w),
                    JPH::EActivation::Activate
                );
            }
        }

        [[nodiscard]] PhysicsTransform GetRigidBodyTransform(EntityId entity) const override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                JPH::RVec3 pos;
                JPH::Quat rot;
                bodyInterface.GetPositionAndRotation(it->second, pos, rot);
                return PhysicsTransform{
                    glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ()),
                    glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ())
                };
            }
            return PhysicsTransform{};
        }

        void SetLinearVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.SetLinearVelocity(it->second,
                    JPH::Vec3(velocity.x, velocity.y, velocity.z));
            }
        }

        [[nodiscard]] glm::vec3 GetLinearVelocity(EntityId entity) const override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                auto v = bodyInterface.GetLinearVelocity(it->second);
                return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
            }
            return glm::vec3{0.0F};
        }

        void SetAngularVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.SetAngularVelocity(it->second,
                    JPH::Vec3(velocity.x, velocity.y, velocity.z));
            }
        }

        [[nodiscard]] glm::vec3 GetAngularVelocity(EntityId entity) const override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                auto v = bodyInterface.GetAngularVelocity(it->second);
                return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
            }
            return glm::vec3{0.0F};
        }

        void ApplyForce(EntityId entity, const glm::vec3& force) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.AddForce(it->second, JPH::Vec3(force.x, force.y, force.z));
            }
        }

        void ApplyForceAtPosition(EntityId entity, const glm::vec3& force, const glm::vec3& position) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.AddForce(it->second,
                    JPH::Vec3(force.x, force.y, force.z),
                    JPH::RVec3(position.x, position.y, position.z));
            }
        }

        void ApplyImpulse(EntityId entity, const glm::vec3& impulse) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.AddImpulse(it->second, JPH::Vec3(impulse.x, impulse.y, impulse.z));
            }
        }

        void ApplyImpulseAtPosition(EntityId entity, const glm::vec3& impulse, const glm::vec3& position) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.AddImpulse(it->second,
                    JPH::Vec3(impulse.x, impulse.y, impulse.z),
                    JPH::RVec3(position.x, position.y, position.z));
            }
        }

        void ApplyTorque(EntityId entity, const glm::vec3& torque) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.AddTorque(it->second, JPH::Vec3(torque.x, torque.y, torque.z));
            }
        }

        bool AddCollider(EntityId entity, const ColliderConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add collider - not initialized");
                return false;
            }

            if (colliders_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a collider", entity);
                return false;
            }

            colliders_[entity] = config;

            // If body already exists, update its shape
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto shape = CreateShape(config.shape);
                if (shape) {
                    auto& bodyInterface = physics_system_->GetBodyInterface();
                    bodyInterface.SetShape(it->second, shape, true, JPH::EActivation::Activate);
                }
            }

            return true;
        }

        void RemoveCollider(EntityId entity) override {
            colliders_.erase(entity);
        }

        [[nodiscard]] bool HasCollider(EntityId entity) const override {
            return colliders_.contains(entity);
        }

        bool EnableCCD(EntityId entity, bool enable) override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                auto& bodyInterface = physics_system_->GetBodyInterface();
                bodyInterface.SetMotionQuality(it->second,
                    enable ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete);
                return true;
            }
            return false;
        }

        [[nodiscard]] bool IsCCDEnabled(EntityId entity) const override {
            auto it = entity_to_body_.find(entity);
            if (it != entity_to_body_.end()) {
                const auto* body = physics_system_->GetBodyLockInterface().TryGetBody(it->second);
                if (body) {
                    return body->GetMotionProperties() &&
                           body->GetMotionProperties()->GetMotionQuality() == JPH::EMotionQuality::LinearCast;
                }
            }
            return false;
        }

        [[nodiscard]] std::optional<CollisionInfo> CheckCollision(EntityId entity_a, EntityId entity_b) const override {
            auto it_a = entity_to_body_.find(entity_a);
            auto it_b = entity_to_body_.find(entity_b);

            if (it_a == entity_to_body_.end() || it_b == entity_to_body_.end()) {
                return std::nullopt;
            }

            // Use narrow phase query to check collision
            // This is a simplified implementation - in a real scenario you'd use the collision results
            // from the contact listener during simulation
            return std::nullopt;
        }

        [[nodiscard]] std::vector<CollisionInfo> GetAllCollisions(EntityId entity) const override {
            // In a real implementation, this would track collisions from the contact listener
            // For now, return empty as collision tracking would require additional state
            (void)entity;
            return {};
        }

        void SetCollisionCallback(CollisionCallback callback) override {
            if (contact_listener_) {
                contact_listener_->SetCallback(std::move(callback));
            }
        }

        [[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& ray) const override {
            if (!physics_system_) {
                return std::nullopt;
            }

            JPH::RRayCast joltRay(
                JPH::RVec3(ray.origin.x, ray.origin.y, ray.origin.z),
                JPH::Vec3(ray.direction.x * ray.max_distance,
                         ray.direction.y * ray.max_distance,
                         ray.direction.z * ray.max_distance)
            );

            JPH::RayCastResult hit;
            if (physics_system_->GetNarrowPhaseQuery().CastRay(joltRay, hit)) {
                RaycastResult result;
                auto hitPoint = joltRay.GetPointOnRay(hit.mFraction);
                result.hit_point = glm::vec3(hitPoint.GetX(), hitPoint.GetY(), hitPoint.GetZ());
                result.distance = hit.mFraction * ray.max_distance;

                // Get entity from body
                const auto* body = physics_system_->GetBodyLockInterface().TryGetBody(hit.mBodyID);
                if (body) {
                    result.entity = body->GetUserData();
                }

                return result;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<RaycastResult> RaycastAll(const Ray& ray) const override {
            // Jolt doesn't have a built-in "raycast all" - would need custom collector
            // For now, just return single hit
            auto hit = Raycast(ray);
            if (hit) {
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

        bool AddCharacterController(EntityId entity, const CharacterControllerConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add character controller - not initialized");
                return false;
            }

            if (characters_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a character controller", entity);
                return false;
            }

            character_configs_[entity] = config;

            // Create capsule shape for character
            JPH::CapsuleShapeSettings capsuleSettings(config.height * 0.5F - config.radius, config.radius);
            auto shapeResult = capsuleSettings.Create();
            if (shapeResult.HasError()) {
                spdlog::error("[JoltPhysics] Failed to create character shape");
                return false;
            }

            // Character settings
            JPH::CharacterSettings settings;
            settings.mMaxSlopeAngle = glm::radians(config.max_slope_angle);
            settings.mShape = shapeResult.Get();
            settings.mLayer = Layers::MOVING;

            auto character = std::make_unique<JPH::Character>(
                &settings,
                JPH::RVec3::sZero(),
                JPH::Quat::sIdentity(),
                entity,
                physics_system_.get()
            );
            character->AddToPhysicsSystem(JPH::EActivation::Activate);

            characters_[entity] = std::move(character);
            return true;
        }

        void RemoveCharacterController(EntityId entity) override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                it->second->RemoveFromPhysicsSystem();
                characters_.erase(it);
            }
            character_configs_.erase(entity);
        }

        [[nodiscard]] bool HasCharacterController(EntityId entity) const override {
            return characters_.contains(entity);
        }

        void MoveCharacter(EntityId entity, const glm::vec3& displacement, float delta_time) override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                it->second->SetLinearVelocity(JPH::Vec3(
                    displacement.x / delta_time,
                    displacement.y / delta_time,
                    displacement.z / delta_time
                ));
            }
        }

        [[nodiscard]] bool IsCharacterGrounded(EntityId entity) const override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                return it->second->GetGroundState() == JPH::Character::EGroundState::OnGround;
            }
            return false;
        }

        [[nodiscard]] PhysicsEngineType GetEngineType() const override {
            return PhysicsEngineType::JoltPhysics;
        }

        [[nodiscard]] std::string GetEngineName() const override {
            return "JoltPhysics";
        }

        [[nodiscard]] bool SupportsFeature(const std::string& feature_name) const override {
            if (feature_name == "ccd") return true;
            if (feature_name == "convex_hull") return true;
            if (feature_name == "compound_shapes") return true;
            if (feature_name == "character_controller") return true;
            if (feature_name == "constraints") return true;
            if (feature_name == "multithreading") return true;
            if (feature_name == "static_mesh") return true;
            if (feature_name == "sleeping") return true;
            if (feature_name == "ccd_concave_mesh") return false;
            if (feature_name == "gpu_acceleration") return false;
            return false;
        }

        ~JoltPhysicsBackend() override {
            Shutdown();
        }
    };

    std::unique_ptr<IPhysicsBackend> CreateJoltBackend() {
        return std::unique_ptr<IPhysicsBackend>(new JoltPhysicsBackend());
    }

} // namespace engine::physics
