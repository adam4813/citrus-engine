#pragma once

#include <filesystem>
#include <string>

namespace editor::build {

struct ProjectModel;
struct BuildTarget;
class BuildReporter;
class IAssetPackager;

/// Stages the build template + project assets/scenes into a per-target staging directory.
class BuildStager {
public:
	struct StagingPaths {
		std::filesystem::path staging_dir;     // <project>/build/<target>/staging
		std::filesystem::path cmake_build_dir; // <project>/build/<preset> (CMake binary dir)
		std::filesystem::path install_dir;     // <project>/build/<target>/dist
		std::filesystem::path staging_assets;  // staging/assets
		std::filesystem::path staging_project_json; // staging/project.json
	};

	/// Resolve where the runtime template lives. Search order:
	///  1. CITRUS_TEMPLATE_DIR environment variable
	///  2. CITRUS_EDITOR_TEMPLATE_DIR compile-time define (set by editor/CMakeLists.txt)
	///  3. Walk up from the editor executable looking for templates/game-project/
	static std::filesystem::path LocateTemplateDir();

	/// Compute the staging paths for a given project + target without creating anything.
	static StagingPaths ComputePaths(const ProjectModel& project, const BuildTarget& target);

	/// Apply template token substitutions (@PROJECT_NAME@ etc.) to a file's contents.
	/// Used both at project creation time and to keep project.json in sync at build time.
	static std::string SubstituteTokens(
			const std::string& source,
			const ProjectModel& project,
			const BuildTarget& target,
			const std::filesystem::path& template_dir);

	/// Stage only the runtime assets and a refreshed project.json. The project's CMake
	/// tree (CMakeLists.txt, vcpkg.json, presets, src/main.cpp) stays in the project
	/// root - we build directly from there.
	/// @return true on success.
	bool Stage(
			const ProjectModel& project,
			const BuildTarget& target,
			IAssetPackager& packager,
			const StagingPaths& paths,
			BuildReporter& reporter);
};

} // namespace editor::build
