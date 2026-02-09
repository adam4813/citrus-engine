#include "debug-ui.h"
#include "editor_scene.h"

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

import engine;

// =============================================================================
// Application State
// =============================================================================

struct AppState {
	DebugUi debug_ui;
	engine::Engine engine;
	editor::EditorScene editor_scene;
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

	// Poll events first
	// TODO: Remove when we can call g_app_state->engine.Update
	engine::input::Input::PollEvents();

	// Begin rendering
	if (g_app_state->engine.renderer) {
		g_app_state->engine.renderer->BeginFrame();
	}

	// Update editor scene
	g_app_state->editor_scene.Update(g_app_state->engine, delta_time);

	// Update engine systems
	// We cannot call this in edit mode, all in-game systems are ran e.g. physics
	//g_app_state->engine.Update(delta_time);

	// Render
	if (g_app_state->engine.renderer) {
		// Render editor scene (viewport content)
		g_app_state->editor_scene.Render(g_app_state->engine);

		// Render ImGui UI
		g_app_state->debug_ui.BeginFrame();
		g_app_state->editor_scene.RenderUI(g_app_state->engine);
		g_app_state->debug_ui.EndFrame();

		g_app_state->engine.renderer->EndFrame();
	}

	// Swap buffers
	glfwSwapBuffers(g_app_state->engine.window);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main(int argc, char* argv[]) {
	std::cout << "Citrus Engine 2D Scene Editor" << std::endl;
	std::cout << "Version: " << engine::GetVersionString() << std::endl;

	// Parse command line arguments
	std::string scene_file;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--scene" && i + 1 < argc) {
			scene_file = argv[i + 1];
			++i;
		}
		else if (arg.find("--scene=") == 0) {
			scene_file = arg.substr(8);
		}
	}

	if (!scene_file.empty()) {
		std::cout << "Scene file requested: " << scene_file << std::endl;
	}

	// Create app state
	AppState app_state;
	g_app_state = &app_state;

	// Initialize engine
	const uint32_t window_width = 1600;
	const uint32_t window_height = 900;

	if (!app_state.engine.Init(window_width, window_height)) {
		std::cerr << "Failed to initialize engine" << std::endl;
		return 1;
	}

	// Set window title
	glfwSetWindowTitle(app_state.engine.window, "Citrus Scene Editor");

	// Initialize debug UI (ImGui)
	app_state.debug_ui.Init(app_state.engine.window);

	// Initialize editor scene
	app_state.editor_scene.Initialize(app_state.engine);

	// If a scene file was provided, open it
	// if (!scene_file.empty()) {
	// 	app_state.editor_scene.OpenScene(scene_file);
	// }

	// Initialize timing
	app_state.last_frame_time = static_cast<float>(glfwGetTime());

	std::cout << "Starting editor main loop..." << std::endl;

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
	std::cout << "Shutting down editor..." << std::endl;
	app_state.editor_scene.Shutdown(app_state.engine);
	app_state.debug_ui.Shutdown();
	app_state.engine.Shutdown();

	g_app_state = nullptr;
	return 0;
}
