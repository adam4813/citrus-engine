module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

export module engine.physics;

import glm;

export import :backend;
export import :types;

export namespace engine::physics {

    // Main physics system that wraps the backend and provides a higher-level API
    // Similar to ScriptingSystem's design pattern
    class PhysicsSystem {
    private:
        std::unique_ptr<IPhysicsBackend> backend_;
        PhysicsEngineType engine_type_;
        PhysicsConfig config_;
        float accumulated_time_{0.0F};

    public:
        // Initialize with a specific physics engine
        explicit PhysicsSystem(PhysicsEngineType engine = PhysicsEngineType::JoltPhysics,
                              const PhysicsConfig& config = PhysicsConfig{})
            : engine_type_(engine), config_(config) {
            backend_ = CreatePhysicsBackend(engine);
            if (!backend_ || !backend_->Initialize(config)) {
                throw std::runtime_error("Failed to initialize physics backend");
            }
        }

        ~PhysicsSystem() {
            if (backend_) {
                backend_->Shutdown();
            }
        }

        // Disable copy
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;

        // Enable move
        PhysicsSystem(PhysicsSystem&&) = default;
        PhysicsSystem& operator=(PhysicsSystem&&) = default;

        // === Engine Configuration ===

        // Get the active physics engine type
        [[nodiscard]] PhysicsEngineType GetEngineType() const {
            return engine_type_;
        }

        // Get the engine name
        [[nodiscard]] std::string GetEngineName() const {
            if (backend_) {
                return backend_->GetEngineName();
            }
            return "None";
        }

        // Check if a feature is supported
        [[nodiscard]] bool SupportsFeature(const std::string& feature_name) const {
            if (backend_) {
                return backend_->SupportsFeature(feature_name);
            }
            return false;
        }

        // Change the physics engine at runtime
        // Note: All physics objects will be lost when switching engines
        bool SetEngine(PhysicsEngineType new_engine, const PhysicsConfig& config = PhysicsConfig{}) {
            if (new_engine == engine_type_) {
                return true; // Already using this engine
            }

            // Shutdown current backend
            if (backend_) {
                backend_->Shutdown();
            }

            // Create and initialize new backend
            try {
                backend_ = CreatePhysicsBackend(new_engine);
                if (!backend_ || !backend_->Initialize(config)) {
                    // Revert to previous engine on failure
                    backend_ = CreatePhysicsBackend(engine_type_);
                    if (backend_) {
                        backend_->Initialize(config_);
                    }
                    return false;
                }
                engine_type_ = new_engine;
                config_ = config;
                accumulated_time_ = 0.0F;
                return true;
            } catch (...) {
                // Revert to previous engine on exception
                backend_ = CreatePhysicsBackend(engine_type_);
                if (backend_) {
                    backend_->Initialize(config_);
                }
                return false;
            }
        }

        // === World Management ===

        // Set world gravity
        void SetGravity(const glm::vec3& gravity) {
            if (backend_) {
                backend_->SetGravity(gravity);
            }
        }

        // Get world gravity
        [[nodiscard]] glm::vec3 GetGravity() const {
            if (backend_) {
                return backend_->GetGravity();
            }
            return glm::vec3{0.0F, -9.81F, 0.0F};
        }

        // Update the physics simulation
        // Uses fixed timestep with accumulator for stable simulation
        void Update(float delta_time) {
            if (!backend_) {
                return;
            }

            accumulated_time_ += delta_time;

            // Step simulation in fixed increments
            int steps = 0;
            while (accumulated_time_ >= config_.fixed_timestep && steps < config_.max_substeps) {
                backend_->StepSimulation(config_.fixed_timestep);
                accumulated_time_ -= config_.fixed_timestep;
                ++steps;
            }

            // Clamp accumulated time to prevent spiral of death
            if (accumulated_time_ > config_.fixed_timestep * config_.max_substeps) {
                accumulated_time_ = config_.fixed_timestep * config_.max_substeps;
            }
        }

        // Step simulation by exactly one timestep (for deterministic simulation)
        void StepOnce() {
            if (backend_) {
                backend_->StepSimulation(config_.fixed_timestep);
            }
        }

        // === Rigid Body Management ===

        // Add a rigid body
        bool AddRigidBody(EntityId entity, const RigidBodyConfig& config = RigidBodyConfig{}) {
            if (backend_) {
                return backend_->AddRigidBody(entity, config);
            }
            return false;
        }

        // Remove a rigid body
        void RemoveRigidBody(EntityId entity) {
            if (backend_) {
                backend_->RemoveRigidBody(entity);
            }
        }

        // Check if entity has a rigid body
        [[nodiscard]] bool HasRigidBody(EntityId entity) const {
            if (backend_) {
                return backend_->HasRigidBody(entity);
            }
            return false;
        }

        // Set rigid body transform
        // Note: GLM quaternion constructor uses (w, x, y, z) ordering
        void SetTransform(EntityId entity, const glm::vec3& position, const glm::quat& rotation = glm::quat{1.0F, 0.0F, 0.0F, 0.0F}) {
            if (backend_) {
                PhysicsTransform transform{position, rotation};
                backend_->SetRigidBodyTransform(entity, transform);
            }
        }

        // Get rigid body position
        [[nodiscard]] glm::vec3 GetPosition(EntityId entity) const {
            if (backend_) {
                return backend_->GetRigidBodyTransform(entity).position;
            }
            return glm::vec3{0.0F};
        }

        // Get rigid body rotation
        [[nodiscard]] glm::quat GetRotation(EntityId entity) const {
            if (backend_) {
                return backend_->GetRigidBodyTransform(entity).rotation;
            }
            return glm::quat{1.0F, 0.0F, 0.0F, 0.0F};
        }

        // Set linear velocity
        void SetLinearVelocity(EntityId entity, const glm::vec3& velocity) {
            if (backend_) {
                backend_->SetLinearVelocity(entity, velocity);
            }
        }

        // Get linear velocity
        [[nodiscard]] glm::vec3 GetLinearVelocity(EntityId entity) const {
            if (backend_) {
                return backend_->GetLinearVelocity(entity);
            }
            return glm::vec3{0.0F};
        }

        // Set angular velocity
        void SetAngularVelocity(EntityId entity, const glm::vec3& velocity) {
            if (backend_) {
                backend_->SetAngularVelocity(entity, velocity);
            }
        }

        // Get angular velocity
        [[nodiscard]] glm::vec3 GetAngularVelocity(EntityId entity) const {
            if (backend_) {
                return backend_->GetAngularVelocity(entity);
            }
            return glm::vec3{0.0F};
        }

        // Apply force at center of mass
        void ApplyForce(EntityId entity, const glm::vec3& force) {
            if (backend_) {
                backend_->ApplyForce(entity, force);
            }
        }

        // Apply force at position
        void ApplyForceAtPosition(EntityId entity, const glm::vec3& force, const glm::vec3& position) {
            if (backend_) {
                backend_->ApplyForceAtPosition(entity, force, position);
            }
        }

        // Apply impulse at center of mass
        void ApplyImpulse(EntityId entity, const glm::vec3& impulse) {
            if (backend_) {
                backend_->ApplyImpulse(entity, impulse);
            }
        }

        // Apply impulse at position
        void ApplyImpulseAtPosition(EntityId entity, const glm::vec3& impulse, const glm::vec3& position) {
            if (backend_) {
                backend_->ApplyImpulseAtPosition(entity, impulse, position);
            }
        }

        // Apply torque
        void ApplyTorque(EntityId entity, const glm::vec3& torque) {
            if (backend_) {
                backend_->ApplyTorque(entity, torque);
            }
        }

        // === Collider Management ===

        // Add a collider
        bool AddCollider(EntityId entity, const ColliderConfig& config = ColliderConfig{}) {
            if (backend_) {
                return backend_->AddCollider(entity, config);
            }
            return false;
        }

        // Remove a collider
        void RemoveCollider(EntityId entity) {
            if (backend_) {
                backend_->RemoveCollider(entity);
            }
        }

        // Check if entity has a collider
        [[nodiscard]] bool HasCollider(EntityId entity) const {
            if (backend_) {
                return backend_->HasCollider(entity);
            }
            return false;
        }

        // === CCD Control ===

        // Enable/disable CCD for an entity
        bool EnableCCD(EntityId entity, bool enable) {
            if (backend_) {
                return backend_->EnableCCD(entity, enable);
            }
            return false;
        }

        // Check if CCD is enabled
        [[nodiscard]] bool IsCCDEnabled(EntityId entity) const {
            if (backend_) {
                return backend_->IsCCDEnabled(entity);
            }
            return false;
        }

        // === Collision Queries ===

        // Check collision between two entities
        [[nodiscard]] std::optional<CollisionInfo> CheckCollision(EntityId entity_a, EntityId entity_b) const {
            if (backend_) {
                return backend_->CheckCollision(entity_a, entity_b);
            }
            return std::nullopt;
        }

        // Get all collisions for an entity
        [[nodiscard]] std::vector<CollisionInfo> GetAllCollisions(EntityId entity) const {
            if (backend_) {
                return backend_->GetAllCollisions(entity);
            }
            return {};
        }

        // Set collision callback
        void SetCollisionCallback(CollisionCallback callback) {
            if (backend_) {
                backend_->SetCollisionCallback(std::move(callback));
            }
        }

        // === Raycasting ===

        // Cast a ray and get the first hit
        [[nodiscard]] std::optional<RaycastResult> Raycast(const glm::vec3& origin, const glm::vec3& direction, float max_distance = 1000.0F) const {
            if (!backend_) {
                return std::nullopt;
            }
            Ray ray{origin, glm::normalize(direction), max_distance};
            return backend_->Raycast(ray);
        }

        // Cast a ray and get all hits
        [[nodiscard]] std::vector<RaycastResult> RaycastAll(const glm::vec3& origin, const glm::vec3& direction, float max_distance = 1000.0F) const {
            if (!backend_) {
                return {};
            }
            Ray ray{origin, glm::normalize(direction), max_distance};
            return backend_->RaycastAll(ray);
        }

        // === Constraints ===

        // Add a constraint between two entities
        bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& config = ConstraintConfig{}) {
            if (backend_) {
                return backend_->AddConstraint(entity_a, entity_b, config);
            }
            return false;
        }

        // Remove a constraint
        void RemoveConstraint(EntityId entity_a, EntityId entity_b) {
            if (backend_) {
                backend_->RemoveConstraint(entity_a, entity_b);
            }
        }

        // === Character Controller ===

        // Add a character controller
        bool AddCharacterController(EntityId entity, const CharacterControllerConfig& config = CharacterControllerConfig{}) {
            if (backend_) {
                return backend_->AddCharacterController(entity, config);
            }
            return false;
        }

        // Remove a character controller
        void RemoveCharacterController(EntityId entity) {
            if (backend_) {
                backend_->RemoveCharacterController(entity);
            }
        }

        // Check if entity has a character controller
        [[nodiscard]] bool HasCharacterController(EntityId entity) const {
            if (backend_) {
                return backend_->HasCharacterController(entity);
            }
            return false;
        }

        // Move character
        void MoveCharacter(EntityId entity, const glm::vec3& displacement, float delta_time) {
            if (backend_) {
                backend_->MoveCharacter(entity, displacement, delta_time);
            }
        }

        // Check if character is grounded
        [[nodiscard]] bool IsCharacterGrounded(EntityId entity) const {
            if (backend_) {
                return backend_->IsCharacterGrounded(entity);
            }
            return false;
        }

        // === Convenience Methods ===

        // Create a dynamic rigid body with a box collider
        bool CreateDynamicBox(EntityId entity, const glm::vec3& position, const glm::vec3& half_extents, float mass = 1.0F) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Dynamic;
            body_config.mass = mass;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Box;
            collider_config.shape.box_half_extents = half_extents;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Create a static rigid body with a box collider
        bool CreateStaticBox(EntityId entity, const glm::vec3& position, const glm::vec3& half_extents) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Static;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Box;
            collider_config.shape.box_half_extents = half_extents;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Create a dynamic rigid body with a sphere collider
        bool CreateDynamicSphere(EntityId entity, const glm::vec3& position, float radius, float mass = 1.0F) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Dynamic;
            body_config.mass = mass;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Sphere;
            collider_config.shape.sphere_radius = radius;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Create a static rigid body with a sphere collider
        bool CreateStaticSphere(EntityId entity, const glm::vec3& position, float radius) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Static;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Sphere;
            collider_config.shape.sphere_radius = radius;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Create a dynamic rigid body with a capsule collider
        bool CreateDynamicCapsule(EntityId entity, const glm::vec3& position, float radius, float height, float mass = 1.0F) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Dynamic;
            body_config.mass = mass;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Capsule;
            collider_config.shape.capsule_radius = radius;
            collider_config.shape.capsule_height = height;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Create a static rigid body with a capsule collider
        bool CreateStaticCapsule(EntityId entity, const glm::vec3& position, float radius, float height) {
            RigidBodyConfig body_config{};
            body_config.motion_type = MotionType::Static;

            ColliderConfig collider_config{};
            collider_config.shape.type = ShapeType::Capsule;
            collider_config.shape.capsule_radius = radius;
            collider_config.shape.capsule_height = height;

            if (!AddRigidBody(entity, body_config)) {
                return false;
            }

            if (!AddCollider(entity, collider_config)) {
                RemoveRigidBody(entity);
                return false;
            }

            SetTransform(entity, position);
            return true;
        }

        // Remove all physics from an entity
        void RemovePhysics(EntityId entity) {
            RemoveCharacterController(entity);
            RemoveCollider(entity);
            RemoveRigidBody(entity);
        }
    };

} // namespace engine::physics
