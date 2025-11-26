module;

#include <memory>
#include <stdexcept>
#include <string>

module engine.physics;

namespace engine::physics {

    // Forward declarations of factory helper functions
    std::unique_ptr<IPhysicsBackend> CreateJoltBackend();
    std::unique_ptr<IPhysicsBackend> CreateBullet3Backend();
    std::unique_ptr<IPhysicsBackend> CreatePhysXBackend();

    // Factory function implementation
    std::unique_ptr<IPhysicsBackend> CreatePhysicsBackend(PhysicsEngineType engine) {
        switch (engine) {
            case PhysicsEngineType::JoltPhysics:
                return CreateJoltBackend();
            case PhysicsEngineType::Bullet3:
                return CreateBullet3Backend();
            case PhysicsEngineType::PhysX:
                return CreatePhysXBackend();
            case PhysicsEngineType::Havok:
                throw std::runtime_error("Havok backend requires commercial license - not implemented");
            default:
                throw std::runtime_error("Unknown physics engine type");
        }
    }

} // namespace engine::physics
