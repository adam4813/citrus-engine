#include "example_scene.h"
#include "scene_registry.h"
#include "scene_switcher.h"

#include <iostream>
#include <string>

#include <imgui.h>
#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#endif
#include <GLFW/glfw3.h>
#include <flecs.h>

#include "debug-ui.h"

import glm;
import engine;

// =============================================================================
// Simple Example Scene - Placeholder for testing
// =============================================================================

class HelloScene : public examples::ExampleScene {
private:
	uint32_t font_texture_id = 0;
	flecs::entity camera_entity;

	void CreateMainCamera(engine::ecs::ECSWorld& ecs) {
		camera_entity = ecs.CreateEntity("MainCamera");

		// Position camera at (0, 0, -1) looking towards the origin for the example scenes
		camera_entity.set<engine::components::Transform>({{0.0f, 0.0f, -1.0f}});
		camera_entity.set<engine::components::Camera>({
				.target = {0.0f, 0.0f, 0.0f}, // Look at the origin
				.up = {0.0f, 1.0f, 0.0f},
				.fov = 60.0f,
				.aspect_ratio = static_cast<float>(800) / static_cast<float>(600),
				.near_plane = 0.1f,
				.far_plane = 100.0f,
		});
		ecs.SetActiveCamera(camera_entity);
	}

public:
	const char* GetName() const override { return "Hello World"; }

	const char* GetDescription() const override { return "A simple hello world example scene"; }

	void Initialize(engine::Engine& engine) override {
		std::cout << "HelloScene initialized" << std::endl;
		engine::ui::text_renderer::FontManager::Initialize("fonts/Kenney Future.ttf", 16);
		engine::ui::batch_renderer::BatchRenderer::Initialize();
		font_texture_id = engine::ui::text_renderer::FontManager::GetDefaultFont()->GetTextureId();
		CreateMainCamera(engine.ecs);
	}

	void Shutdown(engine::Engine& engine) override {
		camera_entity.destruct();
		engine::ui::batch_renderer::BatchRenderer::Shutdown();
		engine::ui::text_renderer::FontManager::Shutdown();
		std::cout << "HelloScene shutdown" << std::endl;
	}

	void Update(engine::Engine& engine, float delta_time) override {
		// Simple update logic
	}

	void Render(engine::Engine& engine) override { // Initialize
		engine::ui::batch_renderer::BatchRenderer::BeginFrame();

		// Test: Draw a red rectangle to verify rendering works
		engine::ui::batch_renderer::Color red{1, 0, 0, 1};
		engine::ui::batch_renderer::Rectangle test_rect{200, 200, 200, 200};
		engine::ui::batch_renderer::Rectangle uv_rect{0, 0, 1, 1};
		engine::ui::batch_renderer::BatchRenderer::SubmitQuad(test_rect, red, uv_rect, font_texture_id);

		// Draw text
		engine::ui::batch_renderer::Color white{1, 1, 1, 1};
		engine::ui::batch_renderer::BatchRenderer::SubmitText("Hello World!", 100, 100, 16, white);

		engine::ui::batch_renderer::BatchRenderer::EndFrame();
	}

	void RenderUI(engine::Engine& engine) override {
		ImGui::Begin("Hello Scene");
		ImGui::Text("Welcome to Citrus Engine Examples!");
		ImGui::Text("This is a placeholder scene.");
		ImGui::Separator();
		ImGui::Text("Use the 'Scenes' menu above to switch between examples.");
		ImGui::End();
	}
};

// Register the hello scene
REGISTER_EXAMPLE_SCENE(HelloScene, "Hello World", "A simple hello world example");

// =============================================================================
// Application State
// =============================================================================

struct AppState {
	DebugUi debug_ui;
	engine::Engine engine;
	examples::SceneSwitcher scene_switcher;
	bool running = true;
	float last_frame_time = 0.0f;
};

// Global app state for main loop
static AppState* g_app_state = nullptr;

// =============================================================================
// Main Loop
// =============================================================================

void main_loop() {
	if (!g_app_state || !g_app_state->running) {
		return;
	}

	// Calculate delta time
	float current_time = static_cast<float>(glfwGetTime());
	float delta_time = current_time - g_app_state->last_frame_time;
	g_app_state->last_frame_time = current_time;

	// Check if window should close
	if (glfwWindowShouldClose(g_app_state->engine.window)) {
		std::cout << "Window close requested, exiting main loop." << std::endl;
		g_app_state->running = false;
#ifdef __EMSCRIPTEN__
		emscripten_cancel_main_loop();
#endif
		return;
	}

	if (g_app_state->engine.renderer) {
		g_app_state->engine.renderer->BeginFrame();
	}

	// Update active scene
	g_app_state->scene_switcher.Update(g_app_state->engine, delta_time);

	// Update engine
	g_app_state->engine.Update(delta_time);

	// Begin rendering
	if (g_app_state->engine.renderer) {
		// Render active scene
		g_app_state->scene_switcher.Render(g_app_state->engine);

		// Render ImGui UI
		g_app_state->debug_ui.BeginFrame();
		g_app_state->scene_switcher.RenderUI(g_app_state->engine);
		g_app_state->debug_ui.EndFrame();

		g_app_state->engine.renderer->EndFrame();
	}

	// Swap buffers and poll events
	glfwSwapBuffers(g_app_state->engine.window);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main(int argc, char* argv[]) {
	std::cout << "Citrus Engine Examples" << std::endl;
	std::cout << "Version: " << engine::GetVersionString() << std::endl;

	// Parse command line arguments for default scene
	std::string default_scene;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--scene" && i + 1 < argc) {
			default_scene = argv[i + 1];
			++i;
		}
		else if (arg.find("--scene=") == 0) {
			default_scene = arg.substr(8);
		}
	}

	if (!default_scene.empty()) {
		std::cout << "Default scene requested: " << default_scene << std::endl;
	}

	// Create app state
	AppState app_state;
	g_app_state = &app_state;

	// Initialize engine
	const uint32_t window_width = 1280;
	const uint32_t window_height = 720;

	if (!app_state.engine.Init(window_width, window_height)) {
		std::cerr << "Failed to initialize engine" << std::endl;
		return 1;
	}

	// Set window title
	glfwSetWindowTitle(app_state.engine.window, "Citrus Engine Examples");

	// Initialize scene switcher
	app_state.scene_switcher.Initialize(app_state.engine, default_scene);
	app_state.debug_ui.Init(app_state.engine.window);

	// Initialize timing
	app_state.last_frame_time = static_cast<float>(glfwGetTime());

	std::cout << "Starting main loop..." << std::endl;

	// Main loop
#ifdef __EMSCRIPTEN__
	// Emscripten uses its own main loop
	emscripten_set_main_loop(main_loop, 0, 1);
#else
	// Native main loop
	while (app_state.running) {
		main_loop();
	}
#endif

	// Cleanup
	std::cout << "Shutting down..." << std::endl;
	app_state.debug_ui.Shutdown();
	app_state.scene_switcher.Shutdown(app_state.engine);
	app_state.engine.Shutdown();

	g_app_state = nullptr;
	return 0;
}
