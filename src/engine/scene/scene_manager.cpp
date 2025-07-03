// Scene manager implementation for flecs-integrated architecture
module;

#include <memory>
#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>

module engine.scene;

import glm;
import engine.platform;
import engine.ecs.flecs;

namespace engine::scene {
    // Scene implementation
    struct Scene::Impl {
        SceneId id;
        std::string name;
        engine::ecs::ECSWorld &ecs_world;
        engine::ecs::Entity scene_root;

        bool active = false;
        bool loaded = false;
        engine::platform::fs::Path file_path;
        std::pair<Vec3, Vec3> world_bounds{{-1000.0f, -1000.0f, -1000.0f}, {1000.0f, 1000.0f, 1000.0f}};

        Impl(const std::string &scene_name, engine::ecs::ECSWorld &world)
            : name(scene_name), ecs_world(world) {
            static SceneId next_scene_id = 1;
            id = next_scene_id++;

            // Create scene root entity using flecs
            scene_root = ecs_world.CreateSceneRoot((name + "_Root").c_str());
        }
    };

    Scene::Scene(const std::string &name, engine::ecs::ECSWorld &ecs_world)
        : pimpl_(std::make_unique<Impl>(name, ecs_world)) {
    }

    Scene::~Scene() = default;

    SceneId Scene::GetId() const { return pimpl_->id; }
    const std::string &Scene::GetName() const { return pimpl_->name; }
    void Scene::SetName(const std::string &name) { pimpl_->name = name; }

    // === ENTITY MANAGEMENT USING FLECS ===

    engine::ecs::Entity Scene::CreateEntity(const std::string &name) {
        auto entity = pimpl_->ecs_world.CreateEntity(name.empty() ? nullptr : name.c_str());

        // Set scene root as parent to associate with this scene
        pimpl_->ecs_world.SetParent(entity, pimpl_->scene_root);

        return entity;
    }

    engine::ecs::Entity Scene::CreateEntity(const std::string &name, engine::ecs::Entity parent) {
        auto entity = CreateEntity(name);
        if (parent.is_valid()) {
            pimpl_->ecs_world.SetParent(entity, parent);
        }
        return entity;
    }

    void Scene::DestroyEntity(engine::ecs::Entity entity) {
        if (entity.is_valid()) {
            entity.destruct();
        }
    }

    std::vector<engine::ecs::Entity> Scene::GetAllEntities() const {
        return pimpl_->ecs_world.GetDescendants(pimpl_->scene_root);
    }

    engine::ecs::Entity Scene::FindEntityByName(const std::string &name) const {
        return pimpl_->ecs_world.FindEntityByName(name.c_str(), pimpl_->scene_root);
    }

    // === HIERARCHY MANAGEMENT ===

    engine::ecs::Entity Scene::GetSceneRoot() const {
        return pimpl_->scene_root;
    }

    void Scene::SetParent(engine::ecs::Entity child, engine::ecs::Entity parent) {
        pimpl_->ecs_world.SetParent(child, parent);
    }

    void Scene::RemoveParent(engine::ecs::Entity child) {
        pimpl_->ecs_world.RemoveParent(child);
        // Re-parent to scene root so it stays in the scene
        pimpl_->ecs_world.SetParent(child, pimpl_->scene_root);
    }

    engine::ecs::Entity Scene::GetParent(engine::ecs::Entity entity) const {
        return pimpl_->ecs_world.GetParent(entity);
    }

    std::vector<engine::ecs::Entity> Scene::GetChildren(engine::ecs::Entity parent) const {
        return pimpl_->ecs_world.GetChildren(parent);
    }

    std::vector<engine::ecs::Entity> Scene::GetDescendants(engine::ecs::Entity root) const {
        return pimpl_->ecs_world.GetDescendants(root);
    }

    // === SPATIAL QUERIES ===

    std::vector<engine::ecs::Entity> Scene::QueryPoint(const Vec3 &point, uint32_t layer_mask) const {
        auto all_results = pimpl_->ecs_world.QueryPoint(point, layer_mask);

        // Filter to only entities in this scene
        std::vector<engine::ecs::Entity> scene_results;
        for (auto entity: all_results) {
            if (pimpl_->ecs_world.IsDescendantOf(entity, pimpl_->scene_root)) {
                scene_results.push_back(entity);
            }
        }
        return scene_results;
    }

    std::vector<engine::ecs::Entity> Scene::QuerySphere(const Vec3 &center, float radius, uint32_t layer_mask) const {
        auto all_results = pimpl_->ecs_world.QuerySphere(center, radius, layer_mask);

        // Filter to only entities in this scene
        std::vector<engine::ecs::Entity> scene_results;
        for (auto entity: all_results) {
            if (pimpl_->ecs_world.IsDescendantOf(entity, pimpl_->scene_root)) {
                scene_results.push_back(entity);
            }
        }
        return scene_results;
    }

    // === SCENE STATE ===

    void Scene::SetWorldBounds(const Vec3 &min, const Vec3 &max) {
        pimpl_->world_bounds = {min, max};
    }

    std::pair<Vec3, Vec3> Scene::GetWorldBounds() const {
        return pimpl_->world_bounds;
    }

    void Scene::SetActive(bool active) { pimpl_->active = active; }
    bool Scene::IsActive() const { return pimpl_->active; }

    void Scene::SetLoaded(bool loaded) { pimpl_->loaded = loaded; }
    bool Scene::IsLoaded() const { return pimpl_->loaded; }

    void Scene::SetFilePath(const engine::platform::fs::Path &path) {
        pimpl_->file_path = path;
    }

    engine::platform::fs::Path Scene::GetFilePath() const {
        return pimpl_->file_path;
    }

    void Scene::Update(float delta_time) {
        // Scene-specific update logic can go here
        // The ECS world systems will handle entity updates automatically
    }

    // SceneManager implementation
    struct SceneManager::Impl {
        engine::ecs::ECSWorld &ecs_world;
        std::unordered_map<SceneId, std::unique_ptr<Scene> > scenes;
        SceneId active_scene = INVALID_SCENE;
        std::vector<SceneId> additional_active_scenes;

        Impl(engine::ecs::ECSWorld &world) : ecs_world(world) {
        }
    };

    SceneManager::SceneManager(engine::ecs::ECSWorld &ecs_world)
        : pimpl_(std::make_unique<Impl>(ecs_world)) {
    }

    SceneManager::~SceneManager() = default;

    SceneId SceneManager::CreateScene(const std::string &name) {
        auto scene = std::make_unique<Scene>(name, pimpl_->ecs_world);
        SceneId id = scene->GetId();
        pimpl_->scenes[id] = std::move(scene);
        return id;
    }

    SceneId SceneManager::LoadScene(const engine::platform::fs::Path &file_path) {
        // TODO: Load scene from file
        return INVALID_SCENE;
    }

    void SceneManager::UnloadScene(SceneId scene_id) {
        auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            it->second->SetLoaded(false);
        }
    }

    void SceneManager::DestroyScene(SceneId scene_id) {
        auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            // Destroy all entities in the scene
            auto entities = it->second->GetAllEntities();
            for (auto entity: entities) {
                entity.destruct();
            }

            // Destroy scene root
            it->second->GetSceneRoot().destruct();

            // Remove from maps
            pimpl_->scenes.erase(it);
        }

        // Clean up active scene references
        if (pimpl_->active_scene == scene_id) {
            pimpl_->active_scene = INVALID_SCENE;
        }

        auto active_it = std::find(pimpl_->additional_active_scenes.begin(),
                                   pimpl_->additional_active_scenes.end(),
                                   scene_id);
        if (active_it != pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.erase(active_it);
        }
    }

    Scene &SceneManager::GetScene(SceneId scene_id) {
        auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            return *it->second;
        }
        static auto invalid_scene = std::make_unique<Scene>("Invalid", pimpl_->ecs_world);
        return *invalid_scene;
    }

    const Scene &SceneManager::GetScene(SceneId scene_id) const {
        auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            return *it->second;
        }
        static auto invalid_scene = std::make_unique<Scene>("Invalid",
                                                            const_cast<engine::ecs::ECSWorld &>(pimpl_->ecs_world));
        return *invalid_scene;
    }

    Scene *SceneManager::TryGetScene(SceneId scene_id) {
        auto it = pimpl_->scenes.find(scene_id);
        return it != pimpl_->scenes.end() ? it->second.get() : nullptr;
    }

    const Scene *SceneManager::TryGetScene(SceneId scene_id) const {
        auto it = pimpl_->scenes.find(scene_id);
        return it != pimpl_->scenes.end() ? it->second.get() : nullptr;
    }

    SceneId SceneManager::FindSceneByName(const std::string &name) const {
        for (const auto &[id, scene]: pimpl_->scenes) {
            if (scene->GetName() == name) {
                return id;
            }
        }
        return INVALID_SCENE;
    }

    std::vector<SceneId> SceneManager::GetAllScenes() const {
        std::vector<SceneId> result;
        for (const auto &[id, scene]: pimpl_->scenes) {
            result.push_back(id);
        }
        return result;
    }

    std::vector<SceneId> SceneManager::GetActiveScenes() const {
        std::vector<SceneId> result;
        if (pimpl_->active_scene != INVALID_SCENE) {
            result.push_back(pimpl_->active_scene);
        }
        result.insert(result.end(), pimpl_->additional_active_scenes.begin(),
                      pimpl_->additional_active_scenes.end());
        return result;
    }

    void SceneManager::SetActiveScene(SceneId scene_id) {
        if (pimpl_->active_scene != INVALID_SCENE) {
            auto old_scene = TryGetScene(pimpl_->active_scene);
            if (old_scene) old_scene->SetActive(false);
        }

        pimpl_->active_scene = scene_id;
        auto new_scene = TryGetScene(scene_id);
        if (new_scene) new_scene->SetActive(true);
    }

    SceneId SceneManager::GetActiveScene() const {
        return pimpl_->active_scene;
    }

    void SceneManager::ActivateScene(SceneId scene_id) {
        SetActiveScene(scene_id);
    }

    void SceneManager::DeactivateScene(SceneId scene_id) {
        if (pimpl_->active_scene == scene_id) {
            auto scene = TryGetScene(scene_id);
            if (scene) scene->SetActive(false);
            pimpl_->active_scene = INVALID_SCENE;
        }
    }

    void SceneManager::ActivateAdditionalScene(SceneId scene_id) {
        auto it = std::find(pimpl_->additional_active_scenes.begin(),
                            pimpl_->additional_active_scenes.end(),
                            scene_id);
        if (it == pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.push_back(scene_id);
            auto scene = TryGetScene(scene_id);
            if (scene) scene->SetActive(true);
        }
    }

    void SceneManager::DeactivateAdditionalScene(SceneId scene_id) {
        auto it = std::find(pimpl_->additional_active_scenes.begin(),
                            pimpl_->additional_active_scenes.end(),
                            scene_id);
        if (it != pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.erase(it);
            auto scene = TryGetScene(scene_id);
            if (scene) scene->SetActive(false);
        }
    }

    void SceneManager::TransitionToScene(SceneId new_scene, float transition_time) {
        // TODO: Implement scene transition with timing
        SetActiveScene(new_scene);
    }

    bool SceneManager::IsTransitioning() const {
        return false; // TODO: Implement transition state tracking
    }

    float SceneManager::GetTransitionProgress() const {
        return 1.0f; // TODO: Return actual transition progress
    }

    bool SceneManager::SaveScene(SceneId scene_id, const engine::platform::fs::Path &file_path) {
        // TODO: Serialize scene to file using flecs reflection/serialization
        return false;
    }

    SceneId SceneManager::LoadSceneFromFile(const engine::platform::fs::Path &file_path) {
        // TODO: Deserialize scene from file
        return INVALID_SCENE;
    }

    std::vector<engine::ecs::Entity> SceneManager::QueryPoint(const Vec3 &point, uint32_t layer_mask) const {
        return pimpl_->ecs_world.QueryPoint(point, layer_mask);
    }

    std::vector<engine::ecs::Entity> SceneManager::QuerySphere(const Vec3 &center, float radius,
                                                               uint32_t layer_mask) const {
        return pimpl_->ecs_world.QuerySphere(center, radius, layer_mask);
    }

    void SceneManager::Update(float delta_time) {
        for (SceneId scene_id: GetActiveScenes()) {
            auto scene = TryGetScene(scene_id);
            if (scene) {
                scene->Update(delta_time);
            }
        }
    }

    size_t SceneManager::GetSceneCount() const {
        return pimpl_->scenes.size();
    }

    size_t SceneManager::GetActiveSceneCount() const {
        return GetActiveScenes().size();
    }

    size_t SceneManager::GetTotalEntityCount() const {
        size_t total = 0;
        for (const auto &[id, scene]: pimpl_->scenes) {
            total += scene->GetAllEntities().size();
        }
        return total;
    }

    void SceneManager::Clear() {
        // Destroy all scenes and their entities
        for (auto &[id, scene]: pimpl_->scenes) {
            auto entities = scene->GetAllEntities();
            for (auto entity: entities) {
                entity.destruct();
            }
            scene->GetSceneRoot().destruct();
        }

        pimpl_->scenes.clear();
        pimpl_->active_scene = INVALID_SCENE;
        pimpl_->additional_active_scenes.clear();
    }

    // Global scene manager
    static std::unique_ptr<SceneManager> g_scene_manager;

    SceneManager &GetSceneManager() {
        if (!g_scene_manager) {
            throw std::runtime_error("Scene system not initialized! Call InitializeSceneSystem first.");
        }
        return *g_scene_manager;
    }

    void InitializeSceneSystem(engine::ecs::ECSWorld &ecs_world) {
        g_scene_manager = std::make_unique<SceneManager>(ecs_world);
    }

    void ShutdownSceneSystem() {
        g_scene_manager.reset();
    }
} // namespace engine::scene
