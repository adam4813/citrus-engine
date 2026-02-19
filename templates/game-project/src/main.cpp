#include <iostream>
#include <string>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#endif
#include <GLFW/glfw3.h>
#include <flecs.h>

import glm;
import engine;

// =============================================================================
// Application State
// =============================================================================

struct AppState {
	engine::Engine engine;
	bool running = true;
	float last_frame_time = 0.0f;
	flecs::entity camera_entity;
};

static AppState* g_app_state = nullptr;

// =============================================================================
// Main Loop
// =============================================================================

void main_loop() {
	if (!g_app_state || !g_app_state->running) {
		return;
	}

	float current_time = static_cast<float>(glfwGetTime());
	float delta_time = current_time - g_app_state->last_frame_time;
	g_app_state->last_frame_time = current_time;

	if (glfwWindowShouldClose(g_app_state->engine.window)) {
		g_app_state->running = false;
#ifdef __EMSCRIPTEN__
		emscripten_cancel_main_loop();
#endif
		return;
	}

	// Begin frame
	if (g_app_state->engine.renderer) {
		g_app_state->engine.renderer->BeginFrame();
	}

	// Update engine systems
	g_app_state->engine.Update(delta_time);

	// TODO: Add your game update logic here

	// End frame
	if (g_app_state->engine.renderer) {
		g_app_state->engine.renderer->EndFrame();
	}

	glfwSwapBuffers(g_app_state->engine.window);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main(int argc, char* argv[]) {
	std::cout << "Starting game..." << std::endl;
	std::cout << "Citrus Engine " << engine::GetVersionString() << std::endl;

	AppState app_state;
	g_app_state = &app_state;

	// Initialize engine
	// TODO: Read window settings from project.json
	constexpr uint32_t window_width = 1280;
	constexpr uint32_t window_height = 720;

	if (!app_state.engine.Init(window_width, window_height)) {
		std::cerr << "Failed to initialize engine" << std::endl;
		return 1;
	}

	glfwSetWindowTitle(app_state.engine.window, "My Game");

	// Create camera
	app_state.camera_entity = app_state.engine.ecs.CreateEntity("MainCamera");

	uint32_t fb_width = window_width;
	uint32_t fb_height = window_height;
	if (app_state.engine.renderer) {
		app_state.engine.renderer->GetFramebufferSize(fb_width, fb_height);
	}

	app_state.camera_entity.set<engine::components::Transform>({{0.0f, 0.0f, -1.0f}});
	app_state.camera_entity.set<engine::components::Camera>({
			.target = {0.0f, 0.0f, 0.0f},
			.up = {0.0f, 1.0f, 0.0f},
			.fov = 60.0f,
			.aspect_ratio = static_cast<float>(fb_width) / static_cast<float>(fb_height),
			.near_plane = 0.1f,
			.far_plane = 100.0f,
	});
	app_state.engine.ecs.SetActiveCamera(app_state.camera_entity);

	// TODO: Load startup scene from project.json

	app_state.last_frame_time = static_cast<float>(glfwGetTime());

	std::cout << "Starting main loop..." << std::endl;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, 0, 1);
#else
	while (app_state.running) {
		main_loop();
	}
#endif

	// Cleanup
	std::cout << "Shutting down..." << std::endl;
	app_state.camera_entity.destruct();
	app_state.engine.Shutdown();

	g_app_state = nullptr;
	return 0;
}
