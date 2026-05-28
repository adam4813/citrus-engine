#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace editor::build {

/// Identifies a build target platform as named in project.json `build.targets[].platform`.
enum class TargetPlatform {
	Native, // x64-windows / x64-linux / x64-osx depending on host
	Wasm,
};

/// Per-target build settings parsed from project.json.
struct BuildTarget {
	TargetPlatform platform = TargetPlatform::Native;
	std::string configuration = "Release"; // "Debug" | "Release"

	// Wasm-only knobs
	uint32_t initial_memory_mb = 64;
	uint32_t max_memory_mb = 128;

	/// Human-readable label for the File > Build submenu.
	[[nodiscard]] std::string DisplayName() const;

	/// vcpkg triplet selected for this target on the current host.
	[[nodiscard]] std::string VcpkgTriplet() const;

	/// Subdirectory name under <project>/build/ for this target.
	[[nodiscard]] std::string StagingSubdir() const;
};

/// Window settings copied verbatim from project.json -> "window".
struct WindowSettings {
	std::string title = "My Game";
	uint32_t width = 1280;
	uint32_t height = 720;
	bool fullscreen = false;
	bool vsync = true;
};

/// In-memory model of a .citrus-project (project.json) file.
struct ProjectModel {
	std::filesystem::path project_root;
	std::filesystem::path project_file;

	std::string name = "my-game";
	std::string version = "0.1.0";
	std::string author;
	std::string description;

	WindowSettings window;

	std::string startup_scene;
	std::string assets_base;
	std::vector<std::string> asset_directories;

	std::vector<BuildTarget> targets;

	[[nodiscard]] std::filesystem::path AssetsDir() const { return project_root / assets_base; }
	[[nodiscard]] std::filesystem::path StartupSceneAbs() const { return project_root / startup_scene; }
	[[nodiscard]] std::filesystem::path BuildRoot() const { return project_root / "build"; }
};

std::optional<ProjectModel> LoadProject(const std::filesystem::path& project_file, std::string& out_error);
std::optional<std::filesystem::path> FindProjectFile(const std::filesystem::path& start);
std::optional<ProjectModel> TryLoadProjectForScene(const std::filesystem::path& scene_path);

} // namespace editor::build
