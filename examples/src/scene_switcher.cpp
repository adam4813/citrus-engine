#include "scene_switcher.h"
#include "scene_registry.h"
#include "engine_scene_adapter.h"

#include <imgui.h>

import engine;

namespace examples {

SceneSwitcher::SceneSwitcher()
    : active_scene_(nullptr)
    , active_scene_name_("")
    , active_adapter_(nullptr)
    , active_engine_scene_id_(0) {  // 0 is INVALID_SCENE
}

SceneSwitcher::~SceneSwitcher() = default;

void SceneSwitcher::Initialize(engine::Engine& engine, const std::string& default_scene_name) {
    // If a default scene is specified, try to activate it
    if (!default_scene_name.empty()) {
        SwitchToScene(engine, default_scene_name);
        return;
    }

    // Otherwise, activate the first available scene
    const auto& scenes = SceneRegistry::Instance().GetAllScenes();
    if (!scenes.empty()) {
        SwitchToScene(engine, scenes[0].name);
    }
}

void SceneSwitcher::Shutdown(engine::Engine& engine) {
    // Destroy the engine scene (will call shutdown callback)
    if (active_engine_scene_id_ != 0) {
        auto& scene_manager = engine::scene::GetSceneManager();
        scene_manager.DestroyScene(active_engine_scene_id_);
        active_engine_scene_id_ = 0;
    }
    
    // Clean up adapter (which owns the example scene)
    active_adapter_.reset();
    active_scene_ = nullptr;
    active_scene_name_.clear();
}

void SceneSwitcher::Update(engine::Engine& engine, float delta_time) {
    // Update is now handled by the engine's scene manager through callbacks
    // But we still call it here for backward compatibility
    if (active_scene_) {
        active_scene_->Update(engine, delta_time);
    }
}

void SceneSwitcher::Render(engine::Engine& engine) {
    // Render is now handled by the engine's scene manager through callbacks
    // But we still call it here for backward compatibility
    if (active_scene_) {
        active_scene_->Render(engine);
    }
}

void SceneSwitcher::RenderUI(engine::Engine& engine) {
    // Main menu bar with scene switcher
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Scenes")) {
            const auto& scenes = SceneRegistry::Instance().GetAllScenes();
            
            for (const auto& scene_info : scenes) {
                bool is_current = (scene_info.name == active_scene_name_);
                if (ImGui::MenuItem(scene_info.name.c_str(), nullptr, is_current)) {
                    if (!is_current) {
                        SwitchToScene(engine, scene_info.name);
                    }
                }
                
                // Show description as tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", scene_info.description.c_str());
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::EndMenu();
        }
        
        // Show active scene name in menu bar
        if (!active_scene_name_.empty()) {
            ImGui::Separator();
            ImGui::Text("Active: %s", active_scene_name_.c_str());
        }
        
        ImGui::EndMainMenuBar();
    }

    // Render active scene's UI
    if (active_scene_) {
        active_scene_->RenderUI(engine);
    }
}

bool SceneSwitcher::SwitchToScene(engine::Engine& engine, const std::string& scene_name) {
    // If already active, nothing to do
    if (scene_name == active_scene_name_ && active_adapter_) {
        return true;
    }

    auto& scene_manager = engine::scene::GetSceneManager();

    // Destroy current engine scene (will call shutdown callback)
    if (active_engine_scene_id_ != 0) {
        scene_manager.DestroyScene(active_engine_scene_id_);
        active_engine_scene_id_ = 0;
    }

    // Clean up adapter (which owns the example scene)
    active_adapter_.reset();
    active_scene_ = nullptr;

    // Create new example scene
    auto example_scene = SceneRegistry::Instance().CreateScene(scene_name);
    if (!example_scene) {
        active_scene_name_.clear();
        return false;
    }

    active_scene_name_ = scene_name;

    // Create engine scene for this example
    active_engine_scene_id_ = scene_manager.CreateScene(scene_name);
    auto& engine_scene = scene_manager.GetScene(active_engine_scene_id_);

    // Create adapter to bridge ExampleScene lifecycle with engine::scene::Scene
    active_adapter_ = std::make_shared<EngineSceneAdapter>(engine, std::move(example_scene));
    
    // Keep a raw pointer to the scene for direct access (UI rendering, etc.)
    active_scene_ = active_adapter_->GetScene();

    // Set up lifecycle callbacks with weak_ptr to prevent use-after-free
    // Capture a weak_ptr so callbacks can safely check if adapter still exists
    std::weak_ptr<EngineSceneAdapter> weak_adapter = active_adapter_;
    
    engine_scene.SetInitializeCallback([weak_adapter]() {
        if (auto adapter = weak_adapter.lock()) {
            adapter->OnInitialize();
        }
    });

    engine_scene.SetShutdownCallback([weak_adapter]() {
        if (auto adapter = weak_adapter.lock()) {
            adapter->OnShutdown();
        }
    });

    engine_scene.SetUpdateCallback([weak_adapter](float delta_time) {
        if (auto adapter = weak_adapter.lock()) {
            adapter->OnUpdate(delta_time);
        }
    });

    engine_scene.SetRenderCallback([weak_adapter]() {
        if (auto adapter = weak_adapter.lock()) {
            adapter->OnRender();
        }
    });

    // Activate the engine scene (will call initialize callback)
    scene_manager.SetActiveScene(active_engine_scene_id_);

    return true;
}

const std::string& SceneSwitcher::GetActiveSceneName() const {
    return active_scene_name_;
}

bool SceneSwitcher::HasActiveScene() const {
    return active_scene_ != nullptr;
}

} // namespace examples
