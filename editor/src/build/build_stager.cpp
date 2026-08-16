#include "build/build_stager.h"

#include "build/asset_packager.h"
#include "build/build_reporter.h"
#include "build/project_model.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace editor::build {

namespace {

std::filesystem::path ExecutablePath() {
#if defined(_WIN32)
	char buf[MAX_PATH];
	DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
	if (n == 0) return {};
	return std::filesystem::path(std::string(buf, n));
#elif defined(__APPLE__)
	char buf[4096];
	uint32_t size = sizeof(buf);
	if (_NSGetExecutablePath(buf, &size) != 0) return {};
	return std::filesystem::path(buf);
#else
	char buf[4096];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n <= 0) return {};
	buf[n] = '\0';
	return std::filesystem::path(buf);
#endif
}

bool WriteFile(const std::filesystem::path& path, const std::string& contents) {
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	std::ofstream out(path, std::ios::binary);
	if (!out.is_open()) return false;
	out << contents;
	return out.good();
}

std::string ReplaceAll(std::string s, const std::string& from, const std::string& to) {
	if (from.empty()) return s;
	size_t pos = 0;
	while ((pos = s.find(from, pos)) != std::string::npos) {
		s.replace(pos, from.size(), to);
		pos += to.size();
	}
	return s;
}

} // namespace

std::string BuildStager::SubstituteTokens(
	const std::string& source,
	const ProjectModel& project,
	const BuildTarget& target,
	const std::filesystem::path& template_dir
) {
	std::string s = source;
	s = ReplaceAll(s, "@PROJECT_NAME@", project.name);
	s = ReplaceAll(s, "@PROJECT_VERSION@", project.version);
	s = ReplaceAll(s, "@PROJECT_DESCRIPTION@", project.description);
	s = ReplaceAll(s, "@WINDOW_TITLE@", project.window.title);
	s = ReplaceAll(
		s,
		"@INITIAL_MEMORY_BYTES@",
		std::to_string(static_cast<uint64_t>(target.initial_memory_mb) * 1024 * 1024)
	);
	s = ReplaceAll(
		s,
		"@MAXIMUM_MEMORY_BYTES@",
		std::to_string(static_cast<uint64_t>(target.max_memory_mb) * 1024 * 1024)
	);

	// Absolute path to the engine's overlay-ports directory, resolved from the template
	// location. Use forward slashes so the substituted value is safe inside JSON.
	std::error_code ec;
	std::filesystem::path engine_root = std::filesystem::weakly_canonical(template_dir / ".." / "..", ec);
	if (ec || engine_root.empty()) engine_root = template_dir.parent_path().parent_path();
	const std::filesystem::path overlay_ports = engine_root / "ports";
	const std::filesystem::path overlay_triplets = engine_root / "triplets";
	s = ReplaceAll(s, "@OVERLAY_PORTS_PATH@", overlay_ports.generic_string());
	s = ReplaceAll(s, "@OVERLAY_TRIPLETS_PATH@", overlay_triplets.generic_string());
	return s;
}

std::filesystem::path BuildStager::LocateTemplateDir() {
	namespace fs = std::filesystem;
	std::error_code ec;
	if (const char* env = std::getenv("CITRUS_TEMPLATE_DIR")) {
		fs::path p = env;
		if (fs::is_directory(p, ec)) return p;
	}
#ifdef CITRUS_EDITOR_TEMPLATE_DIR
	{
		fs::path p = CITRUS_EDITOR_TEMPLATE_DIR;
		if (fs::is_directory(p, ec)) return p;
	}
#endif
	// Walk up from executable looking for templates/game-project.
	auto exe = ExecutablePath();
	if (!exe.empty()) {
		fs::path dir = exe.parent_path();
		for (int i = 0; i < 6 && !dir.empty(); ++i) {
			fs::path candidate = dir / "templates" / "game-project";
			if (fs::is_directory(candidate, ec)) return candidate;
			auto parent = dir.parent_path();
			if (parent == dir) break;
			dir = parent;
		}
	}
	return {};
}

BuildStager::StagingPaths BuildStager::ComputePaths(const ProjectModel& project, const BuildTarget& target) {
	StagingPaths p;
	const auto target_root = project.BuildRoot() / target.StagingSubdir();
	p.staging_dir = target_root / "staging";
	p.staging_assets = p.staging_dir / "assets";
	p.staging_project_json = p.staging_dir / "project.json";
	// CMake source dir is the project root itself; the binaryDir lives at
	// `${sourceDir}/build/<preset>` per CMakePresets.json. We don't manage a
	// duplicate copy of the CMake tree anymore.
	const char* preset = (target.platform == TargetPlatform::Wasm) ? "wasm" : "cli-native";
	p.cmake_build_dir = project.project_root / "build" / preset;
	p.install_dir = target_root / "dist";
	return p;
}

bool BuildStager::Stage(
	const ProjectModel& project,
	const BuildTarget& target,
	IAssetPackager& packager,
	const StagingPaths& paths,
	BuildReporter& reporter
) {
	namespace fs = std::filesystem;
	reporter.SetPhase(BuildPhase::Staging);

	std::error_code ec;
	fs::create_directories(paths.staging_dir, ec);
	if (ec) {
		reporter.AppendLine("[stage] Could not create staging dir: " + ec.message());
		return false;
	}

	// Refresh project.json in the staging directory. We re-substitute tokens so
	// any project-level field updates land before the runtime reads it. The on-disk
	// project.json in project_root is the source of truth; the staged copy is what
	// gets installed alongside the executable.
	const auto template_dir = LocateTemplateDir();
	{
		const auto src = project.project_file;
		std::ifstream in(src, std::ios::binary);
		std::ostringstream ss;
		ss << in.rdbuf();
		auto contents = ss.str();
		if (!template_dir.empty()) {
			contents = SubstituteTokens(contents, project, target, template_dir);
		}
		if (!WriteFile(paths.staging_project_json, contents)) {
			reporter.AppendLine("[stage] Failed to write project.json: " + paths.staging_project_json.string());
			return false;
		}
	}

	// Stage assets via the configured packager.
	if (!packager.Package(project, project.AssetsDir(), paths.staging_assets, reporter)) {
		return false;
	}

	// Verify startup_scene exists in staged assets (warning only). The path stored in
	// project.json is relative to the install root (project.json sits next to it), so
	// strip a leading "assets/" segment when checking the staged tree.
	if (!project.startup_scene.empty()) {
		fs::path scene_rel = project.startup_scene;
		if (!scene_rel.empty() && scene_rel.begin()->string() == "assets") {
			fs::path stripped;
			bool first = true;
			for (const auto& seg : scene_rel) {
				if (first) {
					first = false;
					continue;
				}
				stripped /= seg;
			}
			scene_rel = stripped;
		}
		const auto staged_scene = paths.staging_assets / scene_rel;
		if (!fs::is_regular_file(staged_scene, ec)) {
			reporter.AppendLine("[stage] WARNING: startup_scene not found after staging: " + staged_scene.string());
		}
	}

	reporter.AppendLine("[stage] Staging complete: " + paths.staging_dir.string());
	return true;
}

} // namespace editor::build
