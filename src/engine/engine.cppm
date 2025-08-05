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

export module engine;

// Re-export all engine subsystems for convenient access
export import engine.assets;
export import engine.platform;
export import engine.ecs.flecs;
export import engine.scene;
export import engine.rendering;
export import engine.os;
export import engine.input;

// Main engine namespace
export namespace engine {
    // Engine-wide utilities and constants can go here
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    [[nodiscard]] constexpr auto GetVersionString() noexcept -> const char * { return "1.0.0"; }

    class Engine {
    public:
        Engine();

        ~Engine();

        bool Init(std::uint32_t window_width, uint32_t window_height);

        void Update(float dt);

        void Shutdown();

        // Subsystems (owned instances)
        ecs::ECSWorld ecs;
        // Rendering system: Renderer instance
        rendering::Renderer *renderer{nullptr};
        // OS/Platform system
        //platform::PlatformSystem *platform;
        GLFWwindow *window{nullptr};
        // Input system: managed via input::Input namespace (no instance needed)
        // Scene system: add if you have a SceneSystem class
        // ...add other systems as needed...
    };

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
} // namespace engine
