module;

// Standard library includes for scene management
#include <memory>
#include <string>
#include <functional>
#include <optional>
#include <cstdint>

export module engine.scene;

import glm;
import engine.ecs.flecs;
import engine.platform;
import engine.rendering;

export namespace engine::scene {
    // =============================================================================
    // Core Scene Types
    // =============================================================================

    using Vec3 = glm::vec3;
    using SceneId = uint32_t;

    inline constexpr SceneId INVALID_SCENE = 0;

    // =============================================================================
    // Flecs-Integrated Scene Management
    // =============================================================================

    class Scene {
    public:
        Scene(const std::string &name, engine::ecs::ECSWorld &ecs_world);

        ~Scene();

        // Scene properties
        SceneId GetId() const;

        const std::string &GetName() const;

        void SetName(const std::string &name);

        // === ENTITY MANAGEMENT USING FLECS ===

        // Create entity and add to scene
        engine::ecs::Entity CreateEntity(const std::string &name = "");

        // Create entity with parent (hierarchy)
        engine::ecs::Entity CreateEntity(const std::string &name, engine::ecs::Entity parent);

        // Destroy entity from scene
        void DestroyEntity(engine::ecs::Entity entity);

        // Get all entities in this scene
        std::vector<engine::ecs::Entity> GetAllEntities() const;

        // Find entity by name within this scene
        engine::ecs::Entity FindEntityByName(const std::string &name) const;

        // === HIERARCHY MANAGEMENT ===

        // Get the scene root entity
        engine::ecs::Entity GetSceneRoot() const;

        // Create hierarchical structures
        void SetParent(engine::ecs::Entity child, engine::ecs::Entity parent);

        void RemoveParent(engine::ecs::Entity child);

        engine::ecs::Entity GetParent(engine::ecs::Entity entity) const;

        std::vector<engine::ecs::Entity> GetChildren(engine::ecs::Entity parent) const;

        std::vector<engine::ecs::Entity> GetDescendants(engine::ecs::Entity root) const;

        // === SPATIAL QUERIES ===

        // Find entities at a point within this scene
        std::vector<engine::ecs::Entity> QueryPoint(const Vec3 &point, uint32_t layer_mask = 0xFFFFFFFF) const;

        // Find entities in sphere within this scene
        std::vector<engine::ecs::Entity> QuerySphere(const Vec3 &center, float radius,
                                                     uint32_t layer_mask = 0xFFFFFFFF) const;

        // === SCENE STATE ===

        // Scene bounds
        void SetWorldBounds(const Vec3 &min, const Vec3 &max);

        std::pair<Vec3, Vec3> GetWorldBounds() const;

        // Scene state
        void SetActive(bool active);

        bool IsActive() const;

        void SetLoaded(bool loaded);

        bool IsLoaded() const;

        // Asset management
        void SetFilePath(const engine::platform::fs::Path &path);

        engine::platform::fs::Path GetFilePath() const;

        // Update scene (called each frame)
        void Update(float delta_time);

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    // =============================================================================
    // Scene Manager
    // =============================================================================

    class SceneManager {
    public:
        SceneManager(engine::ecs::ECSWorld &ecs_world);

        ~SceneManager();

        // Scene creation and management
        SceneId CreateScene(const std::string &name);

        SceneId LoadScene(const engine::platform::fs::Path &file_path);

        void UnloadScene(SceneId scene_id);

        void DestroyScene(SceneId scene_id);

        // Scene access
        Scene &GetScene(SceneId scene_id);

        const Scene &GetScene(SceneId scene_id) const;

        Scene *TryGetScene(SceneId scene_id);

        const Scene *TryGetScene(SceneId scene_id) const;

        SceneId FindSceneByName(const std::string &name) const;

        std::vector<SceneId> GetAllScenes() const;

        std::vector<SceneId> GetActiveScenes() const;

        // Scene activation
        void SetActiveScene(SceneId scene_id);

        SceneId GetActiveScene() const;

        void ActivateScene(SceneId scene_id);

        void DeactivateScene(SceneId scene_id);

        // Multi-scene support
        void ActivateAdditionalScene(SceneId scene_id);

        void DeactivateAdditionalScene(SceneId scene_id);

        // Scene transitions
        void TransitionToScene(SceneId new_scene, float transition_time = 0.0f);

        bool IsTransitioning() const;

        float GetTransitionProgress() const;

        // Scene serialization
        bool SaveScene(SceneId scene_id, const engine::platform::fs::Path &file_path);

        SceneId LoadSceneFromFile(const engine::platform::fs::Path &file_path);

        // Global spatial queries (across all active scenes)
        std::vector<engine::ecs::Entity> QueryPoint(const Vec3 &point, uint32_t layer_mask = 0xFFFFFFFF) const;

        std::vector<engine::ecs::Entity> QuerySphere(const Vec3 &center, float radius,
                                                     uint32_t layer_mask = 0xFFFFFFFF) const;

        // Update all active scenes
        void Update(float delta_time);

        // Statistics
        size_t GetSceneCount() const;

        size_t GetActiveSceneCount() const;

        size_t GetTotalEntityCount() const;

        // Clear all scenes
        void Clear();

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    // =============================================================================
    // Utility Functions
    // =============================================================================

    // Global scene manager access (initialized with ECS world)
    SceneManager &GetSceneManager();

    // Initialize/shutdown scene management
    void InitializeSceneSystem(engine::ecs::ECSWorld &ecs_world);

    void ShutdownSceneSystem();
} // namespace engine::scene
