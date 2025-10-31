#include "scene_switcher.h"
#include "scene_registry.h"

#ifndef __EMSCRIPTEN__
#include <imgui.h>
#else
#include <imgui.h>
#endif

import engine;

namespace examples {

SceneSwitcher::SceneSwitcher()
    : active_scene_(nullptr)
    , active_scene_name_("")
    , show_scene_menu_(true) {
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
    if (active_scene_) {
        active_scene_->Shutdown(engine);
        active_scene_.reset();
        active_scene_name_.clear();
    }
}

void SceneSwitcher::Update(engine::Engine& engine, float delta_time) {
    if (active_scene_) {
        active_scene_->Update(engine, delta_time);
    }
}

void SceneSwitcher::Render(engine::Engine& engine) {
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
    if (scene_name == active_scene_name_ && active_scene_) {
        return true;
    }

    // Shutdown current scene
    if (active_scene_) {
        active_scene_->Shutdown(engine);
        active_scene_.reset();
    }

    // Create and initialize new scene
    active_scene_ = SceneRegistry::Instance().CreateScene(scene_name);
    if (!active_scene_) {
        active_scene_name_.clear();
        return false;
    }

    active_scene_name_ = scene_name;
    active_scene_->Initialize(engine);
    return true;
}

const std::string& SceneSwitcher::GetActiveSceneName() const {
    return active_scene_name_;
}

bool SceneSwitcher::HasActiveScene() const {
    return active_scene_ != nullptr;
}

} // namespace examples
