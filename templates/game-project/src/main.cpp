#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#endif
#include <GLFW/glfw3.h>
#include <flecs.h>
#include <nlohmann/json.hpp>

import glm;
import engine;

// =============================================================================
// Project configuration loaded from project.json
// =============================================================================

struct ProjectConfig {
	std::string window_title = "My Game";
	uint32_t window_width = 1280;
	uint32_t window_height = 720;
	std::string startup_scene;
	std::string assets_base = "assets";
	std::vector<std::string> asset_directories;
	std::filesystem::path base_dir;
};

namespace {

std::filesystem::path ExecutableDirectory(const char* argv0) {
#if defined(_WIN32)
	char buf[1024];
	const DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
	if (n > 0 && n < sizeof(buf)) {
		return std::filesystem::path(std::string(buf, n)).parent_path();
	}
#endif
	std::error_code ec;
	auto p = std::filesystem::weakly_canonical(std::filesystem::path(argv0 ? argv0 : "."), ec);
	if (!ec && !p.empty()) return p.parent_path();
	return std::filesystem::current_path();
}

bool LoadProjectConfig(const std::filesystem::path& exe_dir, ProjectConfig& out) {
	// Search exe dir, then ./assets/, then parent (dev/install layouts).
	const std::array<std::filesystem::path, 3> candidates = {
		exe_dir / "project.json",
		exe_dir / "assets" / "project.json",
		exe_dir.parent_path() / "project.json",
	};
	std::filesystem::path found;
	for (const auto& c : candidates) {
		std::error_code ec;
		if (std::filesystem::is_regular_file(c, ec)) {
			found = c;
			break;
		}
	}
	if (found.empty()) {
		std::cerr << "Warning: project.json not found, using defaults." << std::endl;
		out.base_dir = exe_dir;
		return false;
	}

	try {
		std::ifstream in(found);
		nlohmann::json j;
		in >> j;
		if (j.contains("window")) {
			const auto& w = j["window"];
			out.window_title = w.value("title", out.window_title);
			out.window_width = w.value("width", out.window_width);
			out.window_height = w.value("height", out.window_height);
		}
		out.startup_scene = j.value("startup_scene", std::string{});
		if (j.contains("assets") && j["assets"].is_object()) {
			const auto& a = j["assets"];
			out.assets_base = a.value("base_path", out.assets_base);
			if (a.contains("directories") && a["directories"].is_array()) {
				for (const auto& d : a["directories"]) {
					if (d.is_string()) out.asset_directories.push_back(d.get<std::string>());
				}
			}
		}
		out.base_dir = found.parent_path();
		std::cout << "Loaded project: " << found.string() << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to parse project.json: " << e.what() << std::endl;
		out.base_dir = exe_dir;
		return false;
	}
}

/// Recursively scan a directory and register all asset files with the engine's AssetCache.
/// Mirrors the editor's editor::ScanAssetsDirectory: file importers handle raw assets
/// (.wav, .png, etc.) and JSON files with a "type" field are registered via the type
/// registry. Returns the number of newly-registered assets.
size_t ScanAssetsDirectory(const std::filesystem::path& directory) {
	size_t registered = 0;
	std::error_code ec;
	if (!std::filesystem::is_directory(directory, ec)) {
		return 0;
	}

	auto& cache = engine::assets::AssetCache::Instance();
	auto& registry = engine::assets::AssetTypeRegistry::Instance();

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, ec)) {
		if (!entry.is_regular_file()) continue;

		const auto filename = entry.path().filename().string();
		const auto path_str = entry.path().string();

		// Raw file importers (.wav, .png, etc.) win first.
		if (auto asset = cache.TryFileImport(filename, path_str)) {
			cache.Add(asset);
			++registered;
			continue;
		}

		// JSON-defined assets must carry a "type" field that maps to a registered type.
		if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json") continue;

		std::ifstream file(entry.path());
		if (!file.is_open()) continue;
		std::ostringstream ss;
		ss << file.rdbuf();

		const auto j = nlohmann::json::parse(ss.str(), nullptr, false);
		if (j.is_discarded()) continue;

		// Identity (including type) lives under "_metadata"; fall back to legacy top-level "type".
		std::string type_str;
		if (const auto meta = j.find("_metadata"); meta != j.end() && meta->is_object()) {
			type_str = meta->value("type", std::string{});
		}
		else {
			type_str = j.value("type", std::string{});
		}
		if (type_str.empty()) continue;

		const auto* type_info = registry.GetTypeInfo(type_str);
		if (!type_info || !type_info->create_default_factory) continue;

		auto asset = type_info->create_default_factory();
		if (!asset) continue;

		asset->FromJson(j);
		cache.Add(asset);
		++registered;
	}
	return registered;
}

} // namespace

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

	const auto exe_dir = ExecutableDirectory(argc > 0 ? argv[0] : nullptr);

	ProjectConfig project;
	LoadProjectConfig(exe_dir, project);

	AppState app_state;
	g_app_state = &app_state;

	if (!app_state.engine.Init(project.window_width, project.window_height)) {
		std::cerr << "Failed to initialize engine" << std::endl;
		return 1;
	}

	glfwSetWindowTitle(app_state.engine.window, project.window_title.c_str());

	// Scan the project's declared asset directories. Asset types must be registered
	// with the engine's AssetTypeRegistry before this runs (engine::Engine::Init handles
	// that via AssetTypeRegistry::Initialize). Assets are registered but not loaded -
	// individual loaders pull resources on demand.
	{
		const auto assets_root = project.base_dir / project.assets_base;
		size_t total = 0;
		total += ScanAssetsDirectory(assets_root);
		if (!project.asset_directories.empty()) {
			for (const auto& sub : project.asset_directories) {
				total += ScanAssetsDirectory(assets_root / sub);
			}
		}
		std::cout << "Registered " << total << " asset(s) from " << assets_root.string() << std::endl;
	}

	// Attempt to load the startup scene from disk. Fall back to a default camera if none.
	auto& scene_manager = engine::scene::GetSceneManager();
	engine::scene::SceneId startup_scene_id = engine::scene::INVALID_SCENE;
	if (!project.startup_scene.empty()) {
		const auto scene_path = project.base_dir / project.startup_scene;
		std::error_code ec;
		if (std::filesystem::is_regular_file(scene_path, ec)) {
			startup_scene_id = scene_manager.LoadSceneFromFile(engine::platform::fs::Path{scene_path});
			if (startup_scene_id != engine::scene::INVALID_SCENE) {
				scene_manager.SetActiveScene(startup_scene_id);
				std::cout << "Loaded startup scene: " << scene_path.string() << std::endl;
			}
			else {
				std::cerr << "Failed to load startup scene: " << scene_path.string() << std::endl;
			}
		}
		else {
			std::cerr << "Startup scene not found: " << scene_path.string() << std::endl;
		}
	}

	// If the loaded scene didn't define an active camera, create a default fallback.
	if (!app_state.engine.ecs.GetActiveCamera().is_valid()) {
		app_state.camera_entity = app_state.engine.ecs.CreateEntity("MainCamera");

		uint32_t fb_width = project.window_width;
		uint32_t fb_height = project.window_height;
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
	}

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
	if (app_state.camera_entity.is_valid()) {
		app_state.camera_entity.destruct();
	}
	app_state.engine.Shutdown();

	g_app_state = nullptr;
	return 0;
}
