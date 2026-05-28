#include "build/project_model.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

namespace editor::build {

namespace {

TargetPlatform ParsePlatform(const std::string& s) {
	if (s == "wasm" || s == "emscripten" || s == "web") return TargetPlatform::Wasm;
	return TargetPlatform::Native;
}

const char* PlatformString(TargetPlatform p) {
	switch (p) {
		case TargetPlatform::Wasm: return "wasm";
		case TargetPlatform::Native:
		default: return "native";
	}
}

} // namespace

std::string BuildTarget::DisplayName() const {
	std::string label;
	switch (platform) {
		case TargetPlatform::Native:
#if defined(_WIN32)
			label = "Windows";
#elif defined(__APPLE__)
			label = "macOS";
#else
			label = "Linux";
#endif
			break;
		case TargetPlatform::Wasm:
			label = "Web (WASM)";
			break;
	}
	label += " (";
	label += configuration;
	label += ")";
	return label;
}

std::string BuildTarget::VcpkgTriplet() const {
	if (platform == TargetPlatform::Wasm) return "wasm32-emscripten";
#if defined(_WIN32)
	// Static triplet so the produced game executable is portable (no DLL deps).
	return "x64-windows-static";
#elif defined(__APPLE__)
	return "x64-osx";
#else
	return "x64-linux";
#endif
}

std::string BuildTarget::StagingSubdir() const {
	std::string s = PlatformString(platform);
	s += "-";
	s += configuration;
	return s;
}

std::optional<std::filesystem::path> FindProjectFile(const std::filesystem::path& start) {
	std::error_code ec;
	std::filesystem::path dir = std::filesystem::absolute(start, ec);
	if (ec) return std::nullopt;
	if (std::filesystem::is_regular_file(dir, ec)) {
		dir = dir.parent_path();
	}
	while (!dir.empty()) {
		auto candidate = dir / "project.json";
		if (std::filesystem::is_regular_file(candidate, ec)) {
			return candidate;
		}
		const auto parent = dir.parent_path();
		if (parent == dir) break;
		dir = parent;
	}
	return std::nullopt;
}

std::optional<ProjectModel> LoadProject(const std::filesystem::path& project_file, std::string& out_error) {
	std::ifstream in(project_file);
	if (!in.is_open()) {
		out_error = "Could not open " + project_file.string();
		return std::nullopt;
	}

	nlohmann::json j;
	try {
		in >> j;
	}
	catch (const std::exception& e) {
		out_error = std::string("JSON parse error: ") + e.what();
		return std::nullopt;
	}

	ProjectModel m;
	m.project_file = project_file;
	m.project_root = project_file.parent_path();

	m.name = j.value("name", m.name);
	m.version = j.value("version", m.version);
	m.author = j.value("author", "");
	m.description = j.value("description", "");

	if (j.contains("window") && j["window"].is_object()) {
		const auto& w = j["window"];
		m.window.title = w.value("title", m.window.title);
		m.window.width = w.value("width", m.window.width);
		m.window.height = w.value("height", m.window.height);
		m.window.fullscreen = w.value("fullscreen", m.window.fullscreen);
		m.window.vsync = w.value("vsync", m.window.vsync);
	}

	m.startup_scene = j.value("startup_scene", std::string{});

	if (j.contains("assets") && j["assets"].is_object()) {
		const auto& a = j["assets"];
		m.assets_base = a.value("base_path", std::string{"assets"});
		if (a.contains("directories") && a["directories"].is_array()) {
			for (const auto& d : a["directories"]) {
				if (d.is_string()) m.asset_directories.push_back(d.get<std::string>());
			}
		}
	}
	else {
		m.assets_base = "assets";
	}

	if (j.contains("build") && j["build"].is_object() && j["build"].contains("targets")) {
		for (const auto& t : j["build"]["targets"]) {
			BuildTarget bt;
			bt.platform = ParsePlatform(t.value("platform", std::string{"native"}));
			bt.configuration = t.value("configuration", bt.configuration);
			if (t.contains("emscripten") && t["emscripten"].is_object()) {
				bt.initial_memory_mb = t["emscripten"].value("initial_memory_mb", bt.initial_memory_mb);
				bt.max_memory_mb = t["emscripten"].value("max_memory_mb", bt.max_memory_mb);
			}
			m.targets.push_back(bt);
		}
	}
	if (m.targets.empty()) {
		m.targets.push_back(BuildTarget{});
	}

	return m;
}

std::optional<ProjectModel> TryLoadProjectForScene(const std::filesystem::path& scene_path) {
	auto file = FindProjectFile(scene_path);
	if (!file) return std::nullopt;
	std::string err;
	auto model = LoadProject(*file, err);
	if (!model) {
		std::cerr << "[build] Failed to load project: " << err << std::endl;
		return std::nullopt;
	}
	return model;
}

} // namespace editor::build
