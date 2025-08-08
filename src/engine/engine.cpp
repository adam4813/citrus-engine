module;

#include <cstdint>
#include <flecs.h>
#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#endif
#include <GLFW/glfw3.h>

module engine;

namespace engine {
    // Implementation
    Engine::Engine() {
        // Constructor: can initialize subsystems if needed
    }

    Engine::~Engine() {
        Shutdown();
    }

    bool Engine::Init(const uint32_t window_width, const uint32_t window_height) {
        // Initialize OS/platform system
        if (!os::OS::Initialize()) {
            return false;
        }
        window = os::OS::CreateWindow(window_width, window_height, "Game");
        if (!window) {
            return false;
        }
        // Initialize input system
        if (!input::Input::Initialize()) {
            return false;
        }
        renderer = &rendering::GetRenderer();
        // Initialize renderer (if needed)
        if (!renderer->Initialize(window_width, window_height)) {
            return false;
        }

        if (!ui_renderer.Initialize()) {
            return false;
        }

        // ECSWorld is constructed automatically
        scene::InitializeSceneSystem(ecs);
        return true;
    }

    void Engine::Update(const float dt) {
        // Poll input events
        input::Input::PollEvents();
        // Progress ECS world (run systems)
        ecs.Progress(dt);
        // Render scene
        ecs.SubmitRenderCommands(*renderer);
        // ...update other subsystems as needed...
    }

    void Engine::Shutdown() {
        // Shutdown input system
        input::Input::Shutdown();
        // Shutdown renderer
        if (renderer) { renderer->Shutdown(); }
        // Shutdown OS/platform system
        if (window) {
            os::OS::Shutdown(window);
            window = nullptr;
        }
        // ...shutdown other subsystems as needed...
    }
}
