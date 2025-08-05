﻿// Scene manager implementation for flecs-integrated architecture
module;

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <flecs.h>
#include <iostream>
#include <ranges>

module engine.scene;

import glm;
import engine.platform;
import engine.ecs.flecs;
import engine.assets;

namespace engine::scene {
    // Scene implementation
    struct Scene::Impl {
        SceneId id;
        std::string name;
        ecs::ECSWorld &ecs_world;
        ecs::Entity scene_root;

        bool active = false;
        bool loaded = false;
        platform::fs::Path file_path;
        std::pair<Vec3, Vec3> world_bounds{{-1000.0f, -1000.0f, -1000.0f}, {1000.0f, 1000.0f, 1000.0f}};

        // Asset management
        std::vector<std::string> required_assets; // Asset IDs or paths

        Impl(const std::string &scene_name, ecs::ECSWorld &world)
            : name(scene_name), ecs_world(world) {
            static SceneId next_scene_id = 1;
            id = next_scene_id++;

            // Create scene root entity using flecs
            scene_root = ecs_world.CreateSceneRoot((name + "_Root").c_str());
        }
    };

    Scene::Scene(const std::string &name, ecs::ECSWorld &ecs_world)
        : pimpl_(std::make_unique<Impl>(name, ecs_world)) {
    }

    Scene::~Scene() = default;

    SceneId Scene::GetId() const { return pimpl_->id; }
    const std::string &Scene::GetName() const { return pimpl_->name; }
    void Scene::SetName(const std::string &name) const { pimpl_->name = name; }

    // === ENTITY MANAGEMENT USING FLECS ===

    ecs::Entity Scene::CreateEntity(const std::string &name) const {
        const auto entity = pimpl_->ecs_world.CreateEntity(name.empty() ? nullptr : name.c_str());

        // Set scene root as parent to associate with this scene
        pimpl_->ecs_world.SetParent(entity, pimpl_->scene_root);

        return entity;
    }

    ecs::Entity Scene::CreateEntity(const std::string &name, const ecs::Entity &parent) const {
        const auto entity = CreateEntity(name);
        if (parent.is_valid()) {
            pimpl_->ecs_world.SetParent(entity, parent);
        }
        return entity;
    }

    void Scene::DestroyEntity(const ecs::Entity &entity) {
        if (entity.is_valid()) {
            entity.destruct();
        }
    }

    std::vector<ecs::Entity> Scene::GetAllEntities() const {
        return pimpl_->ecs_world.GetDescendants(pimpl_->scene_root);
    }

    ecs::Entity Scene::FindEntityByName(const std::string &name) const {
        return pimpl_->ecs_world.FindEntityByName(name.c_str(), pimpl_->scene_root);
    }

    // === HIERARCHY MANAGEMENT ===

    ecs::Entity Scene::GetSceneRoot() const {
        return pimpl_->scene_root;
    }

    void Scene::SetParent(const ecs::Entity &child, const ecs::Entity &parent) const {
        pimpl_->ecs_world.SetParent(child, parent);
    }

    void Scene::RemoveParent(const ecs::Entity &child) const {
        pimpl_->ecs_world.RemoveParent(child);
        // Re-parent to scene root so it stays in the scene
        pimpl_->ecs_world.SetParent(child, pimpl_->scene_root);
    }

    ecs::Entity Scene::GetParent(const ecs::Entity &entity) const {
        return pimpl_->ecs_world.GetParent(entity);
    }

    std::vector<ecs::Entity> Scene::GetChildren(const ecs::Entity &parent) const {
        return pimpl_->ecs_world.GetChildren(parent);
    }

    std::vector<ecs::Entity> Scene::GetDescendants(const ecs::Entity &root) const {
        return pimpl_->ecs_world.GetDescendants(root);
    }

    // === SPATIAL QUERIES ===

    std::vector<ecs::Entity> Scene::QueryPoint(const Vec3 &point, const uint32_t layer_mask) const {
        const auto all_results = pimpl_->ecs_world.QueryPoint(point, layer_mask);

        // Filter to only entities in this scene
        std::vector<ecs::Entity> scene_results;
        for (auto entity: all_results) {
            if (pimpl_->ecs_world.IsDescendantOf(entity, pimpl_->scene_root)) {
                scene_results.push_back(entity);
            }
        }
        return scene_results;
    }

    std::vector<ecs::Entity> Scene::QuerySphere(const Vec3 &center, const float radius,
                                                const uint32_t layer_mask) const {
        const auto all_results = pimpl_->ecs_world.QuerySphere(center, radius, layer_mask);

        // Filter to only entities in this scene
        std::vector<ecs::Entity> scene_results;
        for (auto entity: all_results) {
            if (pimpl_->ecs_world.IsDescendantOf(entity, pimpl_->scene_root)) {
                scene_results.push_back(entity);
            }
        }
        return scene_results;
    }

    // === SCENE STATE ===

    void Scene::SetWorldBounds(const Vec3 &min, const Vec3 &max) const {
        pimpl_->world_bounds = {min, max};
    }

    std::pair<Vec3, Vec3> Scene::GetWorldBounds() const {
        return pimpl_->world_bounds;
    }

    void Scene::SetActive(const bool active) const { pimpl_->active = active; }
    bool Scene::IsActive() const { return pimpl_->active; }

    void Scene::SetLoaded(const bool loaded) const { pimpl_->loaded = loaded; }
    bool Scene::IsLoaded() const { return pimpl_->loaded; }

    void Scene::SetFilePath(const platform::fs::Path &path) const {
        pimpl_->file_path = path;
    }

    platform::fs::Path Scene::GetFilePath() const {
        return pimpl_->file_path;
    }

    void Scene::Update(float delta_time) {
        // Scene-specific update logic can go here
        // The ECS world systems will handle entity updates automatically
    }

    // === ASSET MANAGEMENT ===
    void Scene::AddRequiredAsset(const std::string &asset_id) const {
        if (std::ranges::find(pimpl_->required_assets, asset_id) == pimpl_->
            required_assets.end()) {
            pimpl_->required_assets.push_back(asset_id);
        }
    }

    void Scene::RemoveRequiredAsset(const std::string &asset_id) const {
        auto &assets = pimpl_->required_assets;
        std::erase(assets, asset_id);
    }

    const std::vector<std::string> &Scene::GetRequiredAssets() const {
        return pimpl_->required_assets;
    }

    bool Scene::LoadAssets() const {
        // TODO: Integrate with asset manager to load assets
        for (const auto &asset: pimpl_->required_assets) {
            // Get file extension for asset type by splitting on the last dot
            const auto ext_pos = asset.find_last_of('.');
            if (ext_pos == std::string::npos) {
                continue; // Invalid asset reference
            }
            // TODO: Come back when asset manager can preload assets
            const std::string ext = asset.substr(ext_pos + 1);
            if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga") {
                // assets::AssetManager::Instance().LoadImage(asset);
            } else if (ext == "vert" || ext == "frag" || ext == "glsl") {
                // assets::AssetManager::Instance().LoadTextFile(asset);
            }
        }

        return true;
    }

    void Scene::UnloadAssets() const {
        // TODO: Integrate with asset manager to unload assets
        for ([[maybe_unused]] const auto &asset: pimpl_->required_assets) {
            // AssetManager::Get().Unload(asset);
        }
    }

    // SceneManager implementation
    struct SceneManager::Impl {
        ecs::ECSWorld &ecs_world;
        std::unordered_map<SceneId, std::unique_ptr<Scene> > scenes;
        SceneId active_scene = INVALID_SCENE;
        std::vector<SceneId> additional_active_scenes;

        Impl(ecs::ECSWorld &world) : ecs_world(world) {
        }
    };

    SceneManager::SceneManager(ecs::ECSWorld &ecs_world)
        : pimpl_(std::make_unique<Impl>(ecs_world)) {
    }

    SceneManager::~SceneManager() = default;

    SceneId SceneManager::CreateScene(const std::string &name) const {
        auto scene = std::make_unique<Scene>(name, pimpl_->ecs_world);
        const SceneId id = scene->GetId();
        pimpl_->scenes[id] = std::move(scene);
        return id;
    }

    SceneId SceneManager::LoadScene(const platform::fs::Path &file_path) {
        // TODO: Load scene from file
        return INVALID_SCENE;
    }

    void SceneManager::UnloadScene(const SceneId scene_id) const {
        const auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            it->second->SetLoaded(false);
        }
    }

    void SceneManager::DestroyScene(const SceneId scene_id) const {
        const auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            // Unload assets before destroying scene
            it->second->UnloadAssets();
            // Destroy all entities in the scene
            const auto entities = it->second->GetAllEntities();
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

        const auto active_it = std::ranges::find(pimpl_->additional_active_scenes,
                                                 scene_id);
        if (active_it != pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.erase(active_it);
        }
    }

    Scene &SceneManager::GetScene(const SceneId scene_id) {
        const auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            return *it->second;
        }
        static auto invalid_scene = std::make_unique<Scene>("Invalid", pimpl_->ecs_world);
        return *invalid_scene;
    }

    const Scene &SceneManager::GetScene(const SceneId scene_id) const {
        const auto it = pimpl_->scenes.find(scene_id);
        if (it != pimpl_->scenes.end()) {
            return *it->second;
        }
        static auto invalid_scene = std::make_unique<Scene>("Invalid",
                                                            const_cast<ecs::ECSWorld &>(pimpl_->ecs_world));
        return *invalid_scene;
    }

    Scene *SceneManager::TryGetScene(const SceneId scene_id) {
        const auto it = pimpl_->scenes.find(scene_id);
        return it != pimpl_->scenes.end() ? it->second.get() : nullptr;
    }

    const Scene *SceneManager::TryGetScene(const SceneId scene_id) const {
        const auto it = pimpl_->scenes.find(scene_id);
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
        for (const auto &id: pimpl_->scenes | std::views::keys) {
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

    void SceneManager::SetActiveScene(const SceneId scene_id) const {
        // Deactivate previous active scene
        if (pimpl_->active_scene != INVALID_SCENE) {
            if (const auto prev_scene = pimpl_->scenes.find(pimpl_->active_scene); prev_scene != pimpl_->scenes.end()) {
                prev_scene->second->SetActive(false);
                prev_scene->second->UnloadAssets(); // Unload assets of previous scene
            }
        }
        // Activate new scene
        if (const auto it = pimpl_->scenes.find(scene_id); it != pimpl_->scenes.end()) {
            it->second->SetActive(true);
            if (!it->second->LoadAssets()) {
                // Failed to load assets, log error and do not activate
                std::cerr << "Failed to load assets for scene: " << it->second->GetName() << '\n';
            }
            pimpl_->active_scene = scene_id;
        }
    }

    SceneId SceneManager::GetActiveScene() const {
        return pimpl_->active_scene;
    }

    void SceneManager::ActivateScene(const SceneId scene_id) const {
        SetActiveScene(scene_id);
    }

    void SceneManager::DeactivateScene(const SceneId scene_id) {
        if (pimpl_->active_scene == scene_id) {
            if (const auto scene = TryGetScene(scene_id)) {
                scene->SetActive(false);
                scene->UnloadAssets(); // Unload assets when deactivating
            }
            pimpl_->active_scene = INVALID_SCENE;
        }
    }

    void SceneManager::ActivateAdditionalScene(const SceneId scene_id) {
        const auto it = std::ranges::find(pimpl_->additional_active_scenes,
                                          scene_id);
        if (it == pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.push_back(scene_id);
            if (const auto scene = TryGetScene(scene_id)) { scene->SetActive(true); }
        }
    }

    void SceneManager::DeactivateAdditionalScene(const SceneId scene_id) {
        const auto it = std::ranges::find(pimpl_->additional_active_scenes,
                                          scene_id);
        if (it != pimpl_->additional_active_scenes.end()) {
            pimpl_->additional_active_scenes.erase(it);
            if (const auto scene = TryGetScene(scene_id)) {
                scene->SetActive(false);
                scene->UnloadAssets(); // Unload assets for additional scene
            }
        }
    }

    void SceneManager::TransitionToScene(const SceneId new_scene, float transition_time) const {
        // TODO: Implement scene transition with timing
        SetActiveScene(new_scene);
    }

    bool SceneManager::IsTransitioning() const {
        return false; // TODO: Implement transition state tracking
    }

    float SceneManager::GetTransitionProgress() const {
        return 1.0f; // TODO: Return actual transition progress
    }

    bool SceneManager::SaveScene(SceneId scene_id, const platform::fs::Path &file_path) {
        // TODO: Serialize scene to file using flecs reflection/serialization
        return false;
    }

    SceneId SceneManager::LoadSceneFromFile(const platform::fs::Path &file_path) {
        // TODO: Deserialize scene from file
        return INVALID_SCENE;
    }

    std::vector<ecs::Entity> SceneManager::QueryPoint(const Vec3 &point, const uint32_t layer_mask) const {
        return pimpl_->ecs_world.QueryPoint(point, layer_mask);
    }

    std::vector<ecs::Entity> SceneManager::QuerySphere(const Vec3 &center, const float radius,
                                                       const uint32_t layer_mask) const {
        return pimpl_->ecs_world.QuerySphere(center, radius, layer_mask);
    }

    void SceneManager::Update(const float delta_time) {
        for (const SceneId scene_id: GetActiveScenes()) {
            if (const auto scene = TryGetScene(scene_id)) {
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
        for (const auto &scene: pimpl_->scenes | std::views::values) {
            total += scene->GetAllEntities().size();
        }
        return total;
    }

    void SceneManager::Clear() const {
        // Destroy all scenes and their entities
        for (const auto &scene: pimpl_->scenes | std::views::values) {
            for (auto entities = scene->GetAllEntities(); auto entity: entities) {
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

    void InitializeSceneSystem(ecs::ECSWorld &ecs_world) {
        g_scene_manager = std::make_unique<SceneManager>(ecs_world);
    }

    void ShutdownSceneSystem() {
        g_scene_manager.reset();
    }
} // namespace engine::scene
