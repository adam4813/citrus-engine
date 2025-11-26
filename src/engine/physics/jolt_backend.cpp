module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>

module engine.physics;

namespace engine::physics {

    // JoltPhysics backend implementation (Adapter Pattern)
    // This is a stub implementation - actual Jolt integration requires the JoltPhysics library
    class JoltPhysicsBackend : public IPhysicsBackend {
    private:
        PhysicsConfig config_{};
        bool initialized_{false};
        glm::vec3 gravity_{0.0F, -9.81F, 0.0F};

        // Simulated rigid body storage
        struct RigidBodyData {
            RigidBodyConfig config{};
            PhysicsTransform transform{};
            glm::vec3 linear_velocity{0.0F, 0.0F, 0.0F};
            glm::vec3 angular_velocity{0.0F, 0.0F, 0.0F};
            bool ccd_enabled{false};
        };
        std::unordered_map<EntityId, RigidBodyData> rigid_bodies_{};

        // Simulated collider storage
        struct ColliderData {
            ColliderConfig config{};
        };
        std::unordered_map<EntityId, ColliderData> colliders_{};

        // Simulated character controller storage
        struct CharacterData {
            CharacterControllerConfig config{};
            PhysicsTransform transform{};
            bool grounded{false};
        };
        std::unordered_map<EntityId, CharacterData> characters_{};

        // Collision callback
        CollisionCallback collision_callback_{};

    public:
        // === Lifecycle ===

        bool Initialize(const PhysicsConfig& config) override {
            if (initialized_) {
                spdlog::warn("[JoltPhysics] Already initialized");
                return true;
            }

            config_ = config;
            gravity_ = config.gravity;

            // In a real implementation, this would:
            // - Call JPH::RegisterDefaultAllocator()
            // - Create JPH::Factory
            // - Call JPH::RegisterTypes()
            // - Create TempAllocator
            // - Create JobSystem
            // - Create BroadPhaseLayerInterface
            // - Create PhysicsSystem

            spdlog::info("[JoltPhysics] Initialized (stub implementation)");
            initialized_ = true;
            return true;
        }

        void Shutdown() override {
            if (!initialized_) {
                return;
            }

            rigid_bodies_.clear();
            colliders_.clear();
            characters_.clear();

            // In a real implementation, this would clean up Jolt resources

            spdlog::info("[JoltPhysics] Shutdown");
            initialized_ = false;
        }

        // === World Management ===

        void SetGravity(const glm::vec3& gravity) override {
            gravity_ = gravity;
            // In real impl: physics_system_->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
        }

        [[nodiscard]] glm::vec3 GetGravity() const override {
            return gravity_;
        }

        void StepSimulation(float delta_time) override {
            if (!initialized_) {
                return;
            }

            // In a real implementation, this would call:
            // physics_system_->Update(delta_time, collision_steps_, &temp_allocator_, &job_system_);

            // Simple stub simulation: apply gravity to dynamic bodies
            for (auto& [entity, body] : rigid_bodies_) {
                if (body.config.motion_type == MotionType::Dynamic && body.config.use_gravity) {
                    body.linear_velocity += gravity_ * body.config.gravity_scale * delta_time;
                    body.transform.position += body.linear_velocity * delta_time;
                }
            }
        }

        // === Rigid Body Management ===

        bool AddRigidBody(EntityId entity, const RigidBodyConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add rigid body - not initialized");
                return false;
            }

            if (rigid_bodies_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a rigid body", entity);
                return false;
            }

            RigidBodyData data{};
            data.config = config;
            data.linear_velocity = config.linear_velocity;
            data.angular_velocity = config.angular_velocity;
            data.ccd_enabled = config.enable_ccd;

            rigid_bodies_[entity] = data;
            return true;
        }

        void RemoveRigidBody(EntityId entity) override {
            rigid_bodies_.erase(entity);
        }

        [[nodiscard]] bool HasRigidBody(EntityId entity) const override {
            return rigid_bodies_.contains(entity);
        }

        void SetRigidBodyTransform(EntityId entity, const PhysicsTransform& transform) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                it->second.transform = transform;
            }
        }

        [[nodiscard]] PhysicsTransform GetRigidBodyTransform(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                return it->second.transform;
            }
            return PhysicsTransform{};
        }

        void SetLinearVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                it->second.linear_velocity = velocity;
            }
        }

        [[nodiscard]] glm::vec3 GetLinearVelocity(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                return it->second.linear_velocity;
            }
            return glm::vec3{0.0F};
        }

        void SetAngularVelocity(EntityId entity, const glm::vec3& velocity) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                it->second.angular_velocity = velocity;
            }
        }

        [[nodiscard]] glm::vec3 GetAngularVelocity(EntityId entity) const override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                return it->second.angular_velocity;
            }
            return glm::vec3{0.0F};
        }

        void ApplyForce(EntityId entity, const glm::vec3& force) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.config.motion_type == MotionType::Dynamic) {
                // F = ma, so a = F/m
                float inv_mass = 1.0F / it->second.config.mass;
                it->second.linear_velocity += force * inv_mass;
            }
        }

        void ApplyForceAtPosition(EntityId entity, const glm::vec3& force, const glm::vec3& /*position*/) override {
            // Simplified: just apply force at center
            ApplyForce(entity, force);
        }

        void ApplyImpulse(EntityId entity, const glm::vec3& impulse) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.config.motion_type == MotionType::Dynamic) {
                // Impulse = m * delta_v, so delta_v = impulse / m
                float inv_mass = 1.0F / it->second.config.mass;
                it->second.linear_velocity += impulse * inv_mass;
            }
        }

        void ApplyImpulseAtPosition(EntityId entity, const glm::vec3& impulse, const glm::vec3& /*position*/) override {
            // Simplified: just apply impulse at center
            ApplyImpulse(entity, impulse);
        }

        void ApplyTorque(EntityId entity, const glm::vec3& torque) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end() && it->second.config.motion_type == MotionType::Dynamic) {
                it->second.angular_velocity += torque;
            }
        }

        // === Collider/Shape Management ===

        bool AddCollider(EntityId entity, const ColliderConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add collider - not initialized");
                return false;
            }

            if (colliders_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a collider", entity);
                return false;
            }

            ColliderData data{};
            data.config = config;
            colliders_[entity] = data;
            return true;
        }

        void RemoveCollider(EntityId entity) override {
            colliders_.erase(entity);
        }

        [[nodiscard]] bool HasCollider(EntityId entity) const override {
            return colliders_.contains(entity);
        }

        // === CCD Control ===

        bool EnableCCD(EntityId entity, bool enable) override {
            auto it = rigid_bodies_.find(entity);
            if (it != rigid_bodies_.end()) {
                it->second.ccd_enabled = enable;
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

        // === Collision Queries ===

        [[nodiscard]] std::optional<CollisionInfo> CheckCollision(EntityId /*entity_a*/, EntityId /*entity_b*/) const override {
            // Stub: No collision detection implemented
            return std::nullopt;
        }

        [[nodiscard]] std::vector<CollisionInfo> GetAllCollisions(EntityId /*entity*/) const override {
            // Stub: No collision detection implemented
            return {};
        }

        void SetCollisionCallback(CollisionCallback callback) override {
            collision_callback_ = std::move(callback);
        }

        // === Raycasting ===

        [[nodiscard]] std::optional<RaycastResult> Raycast(const Ray& /*ray*/) const override {
            // Stub: No raycasting implemented
            return std::nullopt;
        }

        [[nodiscard]] std::vector<RaycastResult> RaycastAll(const Ray& /*ray*/) const override {
            // Stub: No raycasting implemented
            return {};
        }

        // === Constraints/Joints ===

        bool AddConstraint(EntityId /*entity_a*/, EntityId /*entity_b*/, const ConstraintConfig& /*config*/) override {
            // Stub: No constraint system implemented
            spdlog::info("[JoltPhysics] AddConstraint (stub)");
            return true;
        }

        void RemoveConstraint(EntityId /*entity_a*/, EntityId /*entity_b*/) override {
            // Stub: No constraint system implemented
        }

        // === Character Controller ===

        bool AddCharacterController(EntityId entity, const CharacterControllerConfig& config) override {
            if (!initialized_) {
                spdlog::error("[JoltPhysics] Cannot add character controller - not initialized");
                return false;
            }

            if (characters_.contains(entity)) {
                spdlog::warn("[JoltPhysics] Entity {} already has a character controller", entity);
                return false;
            }

            CharacterData data{};
            data.config = config;
            characters_[entity] = data;
            return true;
        }

        void RemoveCharacterController(EntityId entity) override {
            characters_.erase(entity);
        }

        [[nodiscard]] bool HasCharacterController(EntityId entity) const override {
            return characters_.contains(entity);
        }

        void MoveCharacter(EntityId entity, const glm::vec3& displacement, float /*delta_time*/) override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                it->second.transform.position += displacement;
            }
        }

        [[nodiscard]] bool IsCharacterGrounded(EntityId entity) const override {
            auto it = characters_.find(entity);
            if (it != characters_.end()) {
                return it->second.grounded;
            }
            return false;
        }

        // === Engine Information ===

        [[nodiscard]] PhysicsEngineType GetEngineType() const override {
            return PhysicsEngineType::JoltPhysics;
        }

        [[nodiscard]] std::string GetEngineName() const override {
            return "JoltPhysics (stub)";
        }

        [[nodiscard]] bool SupportsFeature(const std::string& feature_name) const override {
            // JoltPhysics supports these features
            if (feature_name == "ccd") return true;
            if (feature_name == "convex_hull") return true;
            if (feature_name == "compound_shapes") return true;
            if (feature_name == "character_controller") return true;
            if (feature_name == "constraints") return true;
            if (feature_name == "multithreading") return true;
            if (feature_name == "static_mesh") return true;
            if (feature_name == "sleeping") return true;
            // Jolt does NOT support CCD for concave meshes
            if (feature_name == "ccd_concave_mesh") return false;
            if (feature_name == "gpu_acceleration") return false;
            return false;
        }

        ~JoltPhysicsBackend() override {
            Shutdown();
        }
    };

    // Factory helper function
    std::unique_ptr<IPhysicsBackend> CreateJoltBackend() {
        return std::make_unique<JoltPhysicsBackend>();
    }

} // namespace engine::physics
