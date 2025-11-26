module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module engine.physics:backend;

export import :types;

export namespace engine::physics {

    // Callback type for collision events
    using CollisionCallback = std::function<void(const CollisionInfo&)>;

    // Interface for physics backend implementations (Strategy Pattern)
    class IPhysicsBackend {
    public:
        virtual ~IPhysicsBackend() = default;

        // === Lifecycle ===

        // Initialize the physics backend
        virtual bool Initialize(const PhysicsConfig& config) = 0;

        // Shutdown and cleanup resources
        virtual void Shutdown() = 0;

        // === World Management ===

        // Set world gravity
        virtual void SetGravity(const glm::vec3& gravity) = 0;

        // Get current gravity
        [[nodiscard]] virtual glm::vec3 GetGravity() const = 0;

        // Step the physics simulation by delta_time
        virtual void StepSimulation(float delta_time) = 0;

        // === Rigid Body Management ===

        // Add a rigid body to an entity
        virtual bool AddRigidBody(EntityId entity, const RigidBodyConfig& config) = 0;

        // Remove a rigid body from an entity
        virtual void RemoveRigidBody(EntityId entity) = 0;

        // Check if an entity has a rigid body
        [[nodiscard]] virtual bool HasRigidBody(EntityId entity) const = 0;

        // Set the transform of a rigid body
        virtual void SetRigidBodyTransform(EntityId entity, const PhysicsTransform& transform) = 0;

        // Get the transform of a rigid body
        [[nodiscard]] virtual PhysicsTransform GetRigidBodyTransform(EntityId entity) const = 0;

        // Set linear velocity
        virtual void SetLinearVelocity(EntityId entity, const glm::vec3& velocity) = 0;

        // Get linear velocity
        [[nodiscard]] virtual glm::vec3 GetLinearVelocity(EntityId entity) const = 0;

        // Set angular velocity
        virtual void SetAngularVelocity(EntityId entity, const glm::vec3& velocity) = 0;

        // Get angular velocity
        [[nodiscard]] virtual glm::vec3 GetAngularVelocity(EntityId entity) const = 0;

        // Apply force at center of mass
        virtual void ApplyForce(EntityId entity, const glm::vec3& force) = 0;

        // Apply force at a world position
        virtual void ApplyForceAtPosition(EntityId entity, const glm::vec3& force, const glm::vec3& position) = 0;

        // Apply impulse at center of mass
        virtual void ApplyImpulse(EntityId entity, const glm::vec3& impulse) = 0;

        // Apply impulse at a world position
        virtual void ApplyImpulseAtPosition(EntityId entity, const glm::vec3& impulse, const glm::vec3& position) = 0;

        // Apply torque
        virtual void ApplyTorque(EntityId entity, const glm::vec3& torque) = 0;

        // === Collider/Shape Management ===

        // Add a collider to an entity
        virtual bool AddCollider(EntityId entity, const ColliderConfig& config) = 0;

        // Remove a collider from an entity
        virtual void RemoveCollider(EntityId entity) = 0;

        // Check if an entity has a collider
        [[nodiscard]] virtual bool HasCollider(EntityId entity) const = 0;

        // === CCD Control ===

        // Enable/disable CCD for an entity
        virtual bool EnableCCD(EntityId entity, bool enable) = 0;

        // Check if CCD is enabled for an entity
        [[nodiscard]] virtual bool IsCCDEnabled(EntityId entity) const = 0;

        // === Collision Queries ===

        // Check for collision between two entities
        [[nodiscard]] virtual std::optional<CollisionInfo> CheckCollision(EntityId entity_a, EntityId entity_b) const = 0;

        // Get all collisions for an entity
        [[nodiscard]] virtual std::vector<CollisionInfo> GetAllCollisions(EntityId entity) const = 0;

        // Set collision callback
        virtual void SetCollisionCallback(CollisionCallback callback) = 0;

        // === Raycasting ===

        // Cast a ray and get the first hit
        [[nodiscard]] virtual std::optional<RaycastResult> Raycast(const Ray& ray) const = 0;

        // Cast a ray and get all hits
        [[nodiscard]] virtual std::vector<RaycastResult> RaycastAll(const Ray& ray) const = 0;

        // === Constraints/Joints ===

        // Add a constraint between two entities
        virtual bool AddConstraint(EntityId entity_a, EntityId entity_b, const ConstraintConfig& config) = 0;

        // Remove a constraint between two entities
        virtual void RemoveConstraint(EntityId entity_a, EntityId entity_b) = 0;

        // === Character Controller ===

        // Add a character controller to an entity
        virtual bool AddCharacterController(EntityId entity, const CharacterControllerConfig& config) = 0;

        // Remove a character controller from an entity
        virtual void RemoveCharacterController(EntityId entity) = 0;

        // Check if an entity has a character controller
        [[nodiscard]] virtual bool HasCharacterController(EntityId entity) const = 0;

        // Move a character controller
        virtual void MoveCharacter(EntityId entity, const glm::vec3& displacement, float delta_time) = 0;

        // Get ground state for character
        [[nodiscard]] virtual bool IsCharacterGrounded(EntityId entity) const = 0;

        // === Engine Information ===

        // Get the physics engine type
        [[nodiscard]] virtual PhysicsEngineType GetEngineType() const = 0;

        // Get engine name as string
        [[nodiscard]] virtual std::string GetEngineName() const = 0;

        // Check if the backend supports a specific feature
        [[nodiscard]] virtual bool SupportsFeature(const std::string& feature_name) const = 0;
    };

    // Factory function for pluggable backend creation (Factory Pattern)
    [[nodiscard]] std::unique_ptr<IPhysicsBackend> CreatePhysicsBackend(PhysicsEngineType engine);

} // namespace engine::physics
