#pragma once

#include "example_scene.h"

import engine;

namespace examples {

/**
 * Adapter that bridges ExampleScene lifecycle with engine::scene::Scene.
 * 
 * This allows ExampleScenes to be integrated with the engine's scene management
 * system while maintaining their familiar lifecycle interface (Initialize, Update, 
 * Render, RenderUI, Shutdown).
 * 
 * Usage:
 *   auto adapter = std::make_unique<EngineSceneAdapter>(
 *       engine, std::move(example_scene));
 *   scene.SetInitializeCallback([adapter]() { adapter->OnInitialize(); });
 *   // etc.
 */
class EngineSceneAdapter {
public:
    EngineSceneAdapter(engine::Engine& engine, std::unique_ptr<ExampleScene> scene)
        : engine_(engine)
        , scene_(std::move(scene)) {
    }

    ~EngineSceneAdapter() {
        if (scene_) {
            scene_->Shutdown(engine_);
        }
    };

    void OnInitialize() {
        if (scene_) {
            scene_->Initialize(engine_);
        }
    }

    void OnShutdown() {
        if (scene_) {
            scene_->Shutdown(engine_);
        }
    }

    void OnUpdate(float delta_time) {
        if (scene_) {
            scene_->Update(engine_, delta_time);
        }
    }

    void OnRender() {
        if (scene_) {
            scene_->Render(engine_);
        }
    }

    void OnRenderUI() {
        if (scene_) {
            scene_->RenderUI(engine_);
        }
    }

    ExampleScene* GetScene() const {
        return scene_.get();
    }

private:
    engine::Engine& engine_;
    std::unique_ptr<ExampleScene> scene_;
};

} // namespace examples
