module;

#include <cstdint>
#include <flecs.h>
#include <memory>
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
export import engine.ecs;
export import engine.scene;
export import engine.components;
export import engine.rendering;
export import engine.os;
export import engine.input;
export import engine.scripting;
export import engine.ui;
export import engine.capture;

// Main engine namespace
export namespace engine {
// Engine-wide utilities and constants can go here
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
[[nodiscard]] constexpr auto GetVersionString() noexcept -> const char* { return "1.0.0"; }

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
	rendering::Renderer* renderer{nullptr};
	// OS/Platform system
	//platform::PlatformSystem *platform;
	GLFWwindow* window{nullptr};
	// Input system: managed via input::Input namespace (no instance needed)
	// Scene system: add if you have a SceneSystem class
	std::unique_ptr<scripting::ScriptingSystem> scripting_system;
};
} // namespace engine
