module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

// Bullet3 includes
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

module engine.physics;

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
            std::vector<std::unique_ptr<btTriangleMesh>> mesh_data;  // Owned mesh data for btBvhTriangleMeshShape
            std::vector<std::unique_ptr<btCollisionShape>> child_shapes;  // Owned child shapes for btCompoundShape
            RigidBodyConfig config;
            bool ccd_enabled{false};
        };
        std::unordered_map<EntityId, RigidBodyData> rigid_bodies_;

        // Collider storage
        std::unordered_map<EntityId, ColliderConfig> colliders_;

        // Character controller storage
        struct CharacterData {
            std::unique_ptr<btPairCachingGhostObject> ghost_object;
            std::unique_ptr<btConvexShape> shape;
            std::unique_ptr<btKinematicCharacterController> controller;
            CharacterControllerConfig config;
        };
        std::unordered_map<EntityId, CharacterData> characters_;

        // Ghost pair callback (owned by broadphase, but we track it for documentation)
        btGhostPairCallback* ghost_pair_callback_{nullptr};

        // Collision callback
        CollisionCallback collision_callback_;

        // Result struct for shape creation (to properly manage memory)
        struct ShapeCreationResult {
            std::unique_ptr<btCollisionShape> shape;
            std::vector<std::unique_ptr<btTriangleMesh>> mesh_data;  // Vector to handle multiple meshes in compound shapes
            std::vector<std::unique_ptr<btCollisionShape>> child_shapes;
        };

        // Helper to create Bullet shape from config
        ShapeCreationResult CreateShape(const ShapeConfig& config) {
            ShapeCreationResult result;
            
            switch (config.type) {
                case ShapeType::Box:
                    result.shape = std::make_unique<btBoxShape>(btVector3(
                        config.box_half_extents.x,
                        config.box_half_extents.y,
                        config.box_half_extents.z
                    ));
                    return result;

                case ShapeType::Sphere:
                    result.shape = std::make_unique<btSphereShape>(config.sphere_radius);
                    return result;

                case ShapeType::Capsule:
                    result.shape = std::make_unique<btCapsuleShape>(config.capsule_radius, config.capsule_height);
                    return result;

                case ShapeType::Cylinder:
                    result.shape = std::make_unique<btCylinderShape>(btVector3(
                        config.cylinder_radius,
                        config.cylinder_height * 0.5F,
                        config.cylinder_radius
                    ));
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
                                btVector3(v0.x, v0.y, v0.z),
                                btVector3(v1.x, v1.y, v1.z),
                                btVector3(v2.x, v2.y, v2.z)
                            );
                        }
                        // btBvhTriangleMeshShape does NOT take ownership - we keep mesh_data alive
                        result.shape = std::make_unique<btBvhTriangleMeshShape>(meshData.get(), true);
                        result.mesh_data.push_back(std::move(meshData));
                        return result;
                    }
                    break;

                case ShapeType::Compound: {
                    auto compound = std::make_unique<btCompoundShape>();
                    for (size_t i = 0; i < config.children.size(); ++i) {
                        auto childResult = CreateShape(config.children[i]);
                        if (childResult.shape) {
                            glm::vec3 pos = i < config.child_positions.size() ?
                                           config.child_positions[i] : glm::vec3(0.0F);
                            glm::quat rot = i < config.child_rotations.size() ?
                                           config.child_rotations[i] : glm::quat(1.0F, 0.0F, 0.0F, 0.0F);

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

                default:
                    break;
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
                dispatcher_.get(),
                broadphase_.get(),
                solver_.get(),
                collision_config_.get()
            );

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
            for (auto& [entity, data] : rigid_bodies_) {
                if (data.body) {
                    dynamics_world_->removeRigidBody(data.body.get());
                }
            }
            rigid_bodies_.clear();

            // Remove character controllers
            for (auto& [entity, data] : characters_) {
                if (data.controller) {
                    dynamics_world_->removeAction(data.controller.get());
                }
                if (data.ghost_object) {
                    dynamics_world_->removeCollisionObject(data.ghost_object.get());
                }
            }
            characters_.clear();
            colliders_.clear();

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

            dynamics_world_->stepSimulation(delta_time, config_.max_substeps, config_.fixed_timestep);

            // Process collision callbacks
            if (collision_callback_) {
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

                        collision_callback_(info);
                    }
                }
            }
        }

        bool AddRigidBody(EntityId entity, const RigidBodyConfig& config) override {
            if (!initialized_) {
                spdlog::error("[Bullet3] Cannot add rigid body - not initialized");
                return false;
            }

            if (rigid_bodies_.contains(entity)) {
                spdlog::warn("[Bullet3] Entity {} already has a rigid body", entity);
                return false;
            }

            RigidBodyData data;
            data.config = config;
            data.ccd_enabled = config.enable_ccd;

            // Create shape
            if (colliders_.contains(entity)) {
                auto shapeResult = CreateShape(colliders_[entity].shape);
                data.shape = std::move(shapeResult.shape);
                data.mesh_data = std::move(shapeResult.mesh_data);
                data.child_shapes = std::move(shapeResult.child_shapes);
            } else {
                data.shape = std::make_unique<btBoxShape>(btVector3(0.5F, 0.5F, 0.5F));
            }

            // Calculate mass and inertia
            btScalar mass = config.motion_type == MotionType::Dynamic ? config.mass : 0.0F;
            btVector3 inertia(0, 0, 0);
            if (mass > 0) {
                data.shape->calculateLocalInertia(mass, inertia);
            }

            // Create motion state
            btTransform transform;
            transform.setIdentity();
            data.motion_state = std::make_unique<btDefaultMotionState>(transform);

            // Create rigid body
            btRigidBody::btRigidBodyConstructionInfo info(mass, data.motion_state.get(), data.shape.get(), inertia);
            info.m_friction = config.friction;
            info.m_restitution = config.restitution;
            info.m_linearDamping = config.linear_damping;
            info.m_angularDamping = config.angular_damping;

            data.body = std::make_unique<btRigidBody>(info);
            data.body->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

            // Set kinematic if needed
            if (config.motion_type == MotionType::Kinematic) {
                data.body->setCollisionFlags(data.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                data.body->setActivationState(DISABLE_DEACTIVATION);
            }

            // Set CCD
            if (config.enable_ccd && config.motion_type == MotionType::Dynamic) {
                data.body->setCcdMotionThreshold(config.ccd_motion_threshold);
                data.body->setCcdSweptSphereRadius(0.2F); // Approximate
            }

            // Gravity settings
            if (!config.use_gravity || config.gravity_scale == 0.0F) {
                data.body->setGravity(btVector3(0, 0, 0));
            } else if (config.gravity_scale != 1.0F) {
                auto g = dynamics_world_->getGravity() * config.gravity_scale;
                data.body->setGravity(g);
            }

            // Sleeping settings
            if (!config.allow_sleeping) {
                data.body->setActivationState(DISABLE_DEACTIVATION);
            }

            // Set initial velocities
            data.body->setLinearVelocity(btVector3(
                config.linear_velocity.x, config.linear_velocity.y, config.linear_velocity.z));
            data.body->setAngularVelocity(btVector3(
                config.angular_velocity.x, config.angular_velocity.y, config.angular_velocity.z));

            // Add to world
            dynamics_world_->addRigidBody(data.body.get());

            rigid_bodies_[entity] = std::move(data);
            return true;
        }

        void RemoveRigidBody(EntityId entity) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                if (it->second.body) {
                    dynamics_world_->removeRigidBody(it->second.body.get());
                }
                rigid_bodies_.erase(it);
            }
        }

        [[nodiscard]] bool HasRigidBody(EntityId entity) const override {
            return rigid_bodies_.contains(entity);
        }

        void SetRigidBodyTransform(EntityId entity, const PhysicsTransform& transform) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                btTransform btTrans;
                btTrans.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));
                btTrans.setRotation(btQuaternion(transform.rotation.x, transform.rotation.y,
                                                  transform.rotation.z, transform.rotation.w));
                it->second.body->setWorldTransform(btTrans);
                it->second.body->getMotionState()->setWorldTransform(btTrans);
                it->second.body->activate();
            }
        }

        [[nodiscard]] PhysicsTransform GetRigidBodyTransform(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                btTransform trans;
                it->second.body->getMotionState()->getWorldTransform(trans);
                auto pos = trans.getOrigin();
                auto rot = trans.getRotation();
                return PhysicsTransform{
                    glm::vec3(pos.x(), pos.y(), pos.z()),
                    glm::quat(rot.w(), rot.x(), rot.y(), rot.z())
                };
            }
            return PhysicsTransform{};
        }

        void SetLinearVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
                it->second.body->activate();
            }
        }

        [[nodiscard]] glm::vec3 GetLinearVelocity(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                auto v = it->second.body->getLinearVelocity();
                return glm::vec3(v.x(), v.y(), v.z());
            }
            return glm::vec3{0.0F};
        }

        void SetAngularVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
                it->second.body->activate();
            }
        }

        [[nodiscard]] glm::vec3 GetAngularVelocity(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                auto v = it->second.body->getAngularVelocity();
                return glm::vec3(v.x(), v.y(), v.z());
            }
            return glm::vec3{0.0F};
        }

        void ApplyForce(EntityId entity, const glm::vec3& force) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->applyCentralForce(btVector3(force.x, force.y, force.z));
                it->second.body->activate();
            }
        }

        void ApplyForceAtPosition(EntityId entity, const glm::vec3& force, const glm::vec3& position) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->applyForce(
                    btVector3(force.x, force.y, force.z),
                    btVector3(position.x, position.y, position.z)
                );
                it->second.body->activate();
            }
        }

        void ApplyImpulse(EntityId entity, const glm::vec3& impulse) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
                it->second.body->activate();
            }
        }

        void ApplyImpulseAtPosition(EntityId entity, const glm::vec3& impulse, const glm::vec3& position) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->applyImpulse(
                    btVector3(impulse.x, impulse.y, impulse.z),
                    btVector3(position.x, position.y, position.z)
                );
                it->second.body->activate();
            }
        }

        void ApplyTorque(EntityId entity, const glm::vec3& torque) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.body->applyTorque(btVector3(torque.x, torque.y, torque.z));
                it->second.body->activate();
            }
        }

        bool AddCollider(EntityId entity, const ColliderConfig& config) override {
            if (!initialized_) {
                spdlog::error("[Bullet3] Cannot add collider - not initialized");
                return false;
            }

            if (colliders_.contains(entity)) {
                spdlog::warn("[Bullet3] Entity {} already has a collider", entity);
                return false;
            }

            colliders_[entity] = config;

            // Update existing body's shape if one exists
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                auto shapeResult = CreateShape(config.shape);
                if (shapeResult.shape) {
                    // Need to remove and re-add body with new shape
                    dynamics_world_->removeRigidBody(it->second.body.get());

                    btScalar mass = it->second.config.motion_type == MotionType::Dynamic ?
                                   it->second.config.mass : 0.0F;
                    btVector3 inertia(0, 0, 0);
                    if (mass > 0) {
                        shapeResult.shape->calculateLocalInertia(mass, inertia);
                    }

                    it->second.body->setCollisionShape(shapeResult.shape.get());
                    it->second.body->setMassProps(mass, inertia);
                    it->second.shape = std::move(shapeResult.shape);
                    it->second.mesh_data = std::move(shapeResult.mesh_data);
                    it->second.child_shapes = std::move(shapeResult.child_shapes);

                    dynamics_world_->addRigidBody(it->second.body.get());
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
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.body) {
                it->second.ccd_enabled = enable;
                if (enable) {
                    it->second.body->setCcdMotionThreshold(it->second.config.ccd_motion_threshold);
                    it->second.body->setCcdSweptSphereRadius(0.2F);
                } else {
                    it->second.body->setCcdMotionThreshold(0);
                    it->second.body->setCcdSweptSphereRadius(0);
                }
                return true;
            }
            return false;
        }

        [[nodiscard]] bool IsCCDEnabled(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                return it->second.ccd_enabled;
            }
            return false;
        }

        [[nodiscard]] std::optional<CollisionInfo> CheckCollision(EntityId entity_a, EntityId entity_b) const override {
            auto it_a = rigid_bodies_.find(entity_a);
            auto it_b = rigid_bodies_.find(entity_b);

            if (it_a == rigid_bodies_.end() || it_b == rigid_bodies_.end()) {
                return std::nullopt;
            }

            // Check if there's an existing contact between these bodies
            int numManifolds = dispatcher_->getNumManifolds();
            for (int i = 0; i < numManifolds; ++i) {
                btPersistentManifold* manifold = dispatcher_->getManifoldByIndexInternal(i);
                const btCollisionObject* objA = manifold->getBody0();
                const btCollisionObject* objB = manifold->getBody1();

                EntityId idA = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objA->getUserPointer()));
                EntityId idB = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objB->getUserPointer()));

                if ((idA == entity_a && idB == entity_b) || (idA == entity_b && idB == entity_a)) {
                    if (manifold->getNumContacts() > 0) {
                        CollisionInfo info;
                        info.entity_a = entity_a;
                        info.entity_b = entity_b;

                        for (int j = 0; j < manifold->getNumContacts(); ++j) {
                            btManifoldPoint& pt = manifold->getContactPoint(j);
                            ContactPoint contact;
                            btVector3 pos = pt.getPositionWorldOnA();
                            btVector3 normal = pt.m_normalWorldOnB;
                            contact.position = glm::vec3(pos.x(), pos.y(), pos.z());
                            contact.normal = glm::vec3(normal.x(), normal.y(), normal.z());
                            contact.penetration_depth = -pt.getDistance();
                            info.contacts.push_back(contact);
                        }

                        return info;
                    }
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<CollisionInfo> GetAllCollisions(EntityId entity) const override {
            std::vector<CollisionInfo> results;

            int numManifolds = dispatcher_->getNumManifolds();
            for (int i = 0; i < numManifolds; ++i) {
                btPersistentManifold* manifold = dispatcher_->getManifoldByIndexInternal(i);
                const btCollisionObject* objA = manifold->getBody0();
                const btCollisionObject* objB = manifold->getBody1();

                EntityId idA = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objA->getUserPointer()));
                EntityId idB = static_cast<EntityId>(reinterpret_cast<uintptr_t>(objB->getUserPointer()));

                if (idA == entity || idB == entity) {
                    if (manifold->getNumContacts() > 0) {
                        CollisionInfo info;
                        info.entity_a = idA;
                        info.entity_b = idB;

                        for (int j = 0; j < manifold->getNumContacts(); ++j) {
                            btManifoldPoint& pt = manifold->getContactPoint(j);
                            ContactPoint contact;
                            btVector3 pos = pt.getPositionWorldOnA();
                            btVector3 normal = pt.m_normalWorldOnB;
                            contact.position = glm::vec3(pos.x(), pos.y(), pos.z());
                            contact.normal = glm::vec3(normal.x(), normal.y(), normal.z());
                            contact.penetration_depth = -pt.getDistance();
                            info.contacts.push_back(contact);
                        }

                        results.push_back(info);
                    }
                }
            }

            return results;
        }

        void SetCollisionCallback(CollisionCallback callback) override {
            collision_callback_ = std::move(callback);
        }

        [[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& ray) const override {
            if (!dynamics_world_) {
                return std::nullopt;
            }

            btVector3 from(ray.origin.x, ray.origin.y, ray.origin.z);
            btVector3 to(
                ray.origin.x + ray.direction.x * ray.max_distance,
                ray.origin.y + ray.direction.y * ray.max_distance,
                ray.origin.z + ray.direction.z * ray.max_distance
            );

            btCollisionWorld::ClosestRayResultCallback callback(from, to);
            dynamics_world_->rayTest(from, to, callback);

            if (callback.hasHit()) {
                RaycastResult result;
                result.entity = static_cast<EntityId>(
                    reinterpret_cast<uintptr_t>(callback.m_collisionObject->getUserPointer()));
                result.hit_point = glm::vec3(
                    callback.m_hitPointWorld.x(),
                    callback.m_hitPointWorld.y(),
                    callback.m_hitPointWorld.z()
                );
                result.hit_normal = glm::vec3(
                    callback.m_hitNormalWorld.x(),
                    callback.m_hitNormalWorld.y(),
                    callback.m_hitNormalWorld.z()
                );
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
                ray.origin.z + ray.direction.z * ray.max_distance
            );

            btCollisionWorld::AllHitsRayResultCallback callback(from, to);
            dynamics_world_->rayTest(from, to, callback);

            std::vector<RaycastResult> results;
            for (int i = 0; i < callback.m_collisionObjects.size(); ++i) {
                RaycastResult result;
                result.entity = static_cast<EntityId>(
                    reinterpret_cast<uintptr_t>(callback.m_collisionObjects[i]->getUserPointer()));
                result.hit_point = glm::vec3(
                    callback.m_hitPointWorld[i].x(),
                    callback.m_hitPointWorld[i].y(),
                    callback.m_hitPointWorld[i].z()
                );
                result.hit_normal = glm::vec3(
                    callback.m_hitNormalWorld[i].x(),
                    callback.m_hitNormalWorld[i].y(),
                    callback.m_hitNormalWorld[i].z()
                );
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

        bool AddCharacterController(EntityId entity, const CharacterControllerConfig& config) override {
            if (!initialized_) {
                spdlog::error("[Bullet3] Cannot add character controller - not initialized");
                return false;
            }

            if (characters_.contains(entity)) {
                spdlog::warn("[Bullet3] Entity {} already has a character controller", entity);
                return false;
            }

            CharacterData data;
            data.config = config;

            // Create capsule shape
            data.shape = std::make_unique<btCapsuleShape>(config.radius, config.height - 2 * config.radius);

            // Create ghost object
            data.ghost_object = std::make_unique<btPairCachingGhostObject>();
            data.ghost_object->setWorldTransform(btTransform::getIdentity());
            data.ghost_object->setCollisionShape(data.shape.get());
            data.ghost_object->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
            data.ghost_object->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entity)));

            // Create character controller
            data.controller = std::make_unique<btKinematicCharacterController>(
                data.ghost_object.get(),
                data.shape.get(),
                config.step_height
            );
            data.controller->setMaxSlope(btRadians(config.max_slope_angle));

            // Add to world
            dynamics_world_->addCollisionObject(data.ghost_object.get(),
                btBroadphaseProxy::CharacterFilter,
                btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
            dynamics_world_->addAction(data.controller.get());

            characters_[entity] = std::move(data);
            return true;
        }

        void RemoveCharacterController(EntityId entity) override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                if (it->second.controller) {
                    dynamics_world_->removeAction(it->second.controller.get());
                }
                if (it->second.ghost_object) {
                    dynamics_world_->removeCollisionObject(it->second.ghost_object.get());
                }
                characters_.erase(it);
            }
        }

        [[nodiscard]] bool HasCharacterController(EntityId entity) const override {
            return characters_.contains(entity);
        }

        void MoveCharacter(EntityId entity, const glm::vec3& displacement, float delta_time) override {
            auto it = characters_.find(entity);
            if (it != characters_.end() && it->second.controller) {
                it->second.controller->setWalkDirection(btVector3(
                    displacement.x / delta_time * config_.fixed_timestep,
                    displacement.y / delta_time * config_.fixed_timestep,
                    displacement.z / delta_time * config_.fixed_timestep
                ));
            }
        }

        [[nodiscard]] bool IsCharacterGrounded(EntityId entity) const override {
            auto it = characters_.find(entity);
            if (it != characters_.end() && it->second.controller) {
                return it->second.controller->onGround();
            }
            return false;
        }

        [[nodiscard]] PhysicsEngineType GetEngineType() const override {
            return PhysicsEngineType::Bullet3;
        }

        [[nodiscard]] std::string GetEngineName() const override {
            return "Bullet3";
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

        ~Bullet3Backend() override {
            Shutdown();
        }
    };

    // Factory function - uses raw new because C++ modules don't support make_unique
    // for types defined in module implementation units
    std::unique_ptr<IPhysicsBackend> CreateBullet3Backend() {
        return std::unique_ptr<IPhysicsBackend>(new Bullet3Backend());
    }

} // namespace engine::physics
