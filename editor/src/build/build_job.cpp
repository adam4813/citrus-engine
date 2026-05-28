#include "build/build_job.h"

#include "build/asset_packager.h"
#include "build/build_stager.h"
#include "build/toolchain_runner.h"

#include <cctype>
#include <regex>
#include <string>

namespace editor::build {

BuildJob::BuildJob() = default;

BuildJob::~BuildJob() {
	Cancel();
	Join();
}

bool BuildJob::Start(ProjectModel project, BuildTarget target, std::unique_ptr<IAssetPackager> packager) {
	if (running_.exchange(true)) return false;
	cancel_.store(false);
	reporter_.Reset();
	install_dir_.clear();
	worker_ = std::thread(&BuildJob::Run, this, std::move(project), std::move(target), std::move(packager));
	return true;
}

void BuildJob::Cancel() {
	cancel_.store(true);
}

void BuildJob::Join() {
	if (worker_.joinable()) worker_.join();
}

namespace {

/// Parse a line like "[ 42%]" emitted by Ninja/Make and return percent, else -1.
float ExtractProgressPercent(const std::string& line) {
	static const std::regex re("\\[\\s*(\\d{1,3})%\\]");
	std::smatch m;
	if (std::regex_search(line, m, re)) {
		try {
			int v = std::stoi(m[1].str());
			if (v >= 0 && v <= 100) return static_cast<float>(v);
		}
		catch (...) {}
	}
	return -1.0f;
}

} // namespace

void BuildJob::Run(ProjectModel project, BuildTarget target, std::unique_ptr<IAssetPackager> packager) {
	struct Guard {
		std::atomic<bool>& flag;
		~Guard() { flag.store(false); }
	} guard{running_};

	reporter_.AppendLine("[build] Project: " + project.name + " (" + project.version + ")");
	reporter_.AppendLine("[build] Target:  " + target.DisplayName() + " [" + target.VcpkgTriplet() + "]");

	// 1. Detect toolchain.
	const bool needs_emsdk = target.platform == TargetPlatform::Wasm;
#if defined(_WIN32)
	const bool needs_msvc = target.platform == TargetPlatform::Native;
#else
	const bool needs_msvc = false;
#endif
	auto tc = DetectToolchain(needs_emsdk, needs_msvc);
	if (!tc.ok) {
		reporter_.AppendLine("[build] Toolchain error: " + tc.error_message);
		reporter_.Finish(BuildPhase::Failed);
		return;
	}
	reporter_.AppendLine("[build] cmake:  " + tc.cmake_path.string());
	reporter_.AppendLine("[build] vcpkg:  " + tc.vcpkg_root.string());
	if (!tc.vcvars_bat.empty()) reporter_.AppendLine("[build] vcvars: " + tc.vcvars_bat.string());
	if (!tc.emsdk_env.empty()) reporter_.AppendLine("[build] emsdk:  " + tc.emsdk_env.string());

	// 2. Stage files.
	const auto paths = BuildStager::ComputePaths(project, target);
	install_dir_ = paths.install_dir;

	BuildStager stager;
	if (!stager.Stage(project, target, *packager, paths, reporter_)) {
		reporter_.Finish(BuildPhase::Failed);
		return;
	}
	if (cancel_.load()) {
		reporter_.Finish(BuildPhase::Cancelled);
		return;
	}

	std::error_code ec;
	std::filesystem::create_directories(paths.cmake_build_dir, ec);
	std::filesystem::create_directories(paths.install_dir, ec);

	std::unordered_map<std::string, std::string> env;
	env["VCPKG_ROOT"] = tc.vcpkg_root.string();
	if (!tc.emsdk_env.empty()) env["EMSDK"] = tc.emsdk_env.string();

	// For wasm builds, force every emcc invocation (including those spawned by vcpkg
	// when building dependencies like flecs) to add -pthread. emcc reads EMCC_CFLAGS
	// directly and appends to all compile/link commands, bypassing any CMake-level
	// flag overrides a dependency's portfile might do. The custom triplet declares
	// EMCC_CFLAGS as a TRACKED passthrough so changing it invalidates the binary
	// cache and forces rebuilds.
	if (target.platform == TargetPlatform::Wasm) {
		env["EMCC_CFLAGS"] = "-pthread";
	}

	// On Windows native builds we use Ninja Multi-Config (matching the template's
	// CMakePresets), so we need the MSVC toolchain in our environment. Capture the
	// variables vcvars64.bat sets and merge them into the child env.
#if defined(_WIN32)
	if (!tc.vcvars_bat.empty()) {
		reporter_.AppendLine("[build] Capturing MSVC environment from " + tc.vcvars_bat.string());
		auto vc_env = CaptureVcvarsEnv(tc.vcvars_bat);
		if (vc_env.empty()) {
			reporter_.AppendLine("[build] WARNING: failed to capture vcvars environment; build may fail.");
		}
		for (auto& [k, v] : vc_env) env[k] = std::move(v);
		// Preserve VCPKG_ROOT/EMSDK/EMCC_CFLAGS we set above (vcvars may not define them).
		env["VCPKG_ROOT"] = tc.vcpkg_root.string();
		if (!tc.emsdk_env.empty()) env["EMSDK"] = tc.emsdk_env.string();
		if (target.platform == TargetPlatform::Wasm) env["EMCC_CFLAGS"] = "-pthread";
	}
#endif

	auto on_line = [this](std::string line) {
		float pct = ExtractProgressPercent(line);
		if (pct >= 0) reporter_.SetProgress(pct);
		reporter_.AppendLine(std::move(line));
	};

	// 3. Configure. Use the CMakePresets shipped in the staged template (`cli-native` for
	// native, `wasm` for emscripten). We still pass triplet + install prefix on the
	// command line so they can be overridden per build.
	reporter_.SetPhase(BuildPhase::Configuring);
	reporter_.SetProgress(-1.0f);

	const std::string preset_name = (target.platform == TargetPlatform::Wasm) ? "wasm" : "cli-native";
	const std::string build_preset = preset_name + "-" + [&] {
		// CMakePresets uses lowercase debug/release labels.
		std::string s = target.configuration;
		for (auto& c : s) c = static_cast<char>(std::tolower(c));
		return s;
	}();

	const std::string triplet_arg = "-DVCPKG_TARGET_TRIPLET=" + target.VcpkgTriplet();
	const std::string install_arg = "-DCMAKE_INSTALL_PREFIX=" + paths.install_dir.string();
	// Point CMake at the staged (packaged) assets + project.json so the install rules
	// pull from there instead of the live project tree.
	const std::string assets_arg = "-DCITRUS_PACKAGED_ASSETS_DIR=" + paths.staging_assets.generic_string();
	const std::string proj_arg = "-DCITRUS_PACKAGED_PROJECT_JSON=" + paths.staging_project_json.generic_string();

	std::vector<std::string> configure_argv;
	configure_argv.push_back(tc.cmake_path.string());
	configure_argv.push_back("--preset");
	configure_argv.push_back(preset_name);
	configure_argv.push_back(triplet_arg);
	configure_argv.push_back(install_arg);
	configure_argv.push_back(assets_arg);
	configure_argv.push_back(proj_arg);

	int rc = RunProcess(configure_argv, project.project_root, env, cancel_, on_line);
	if (rc == -2) { reporter_.Finish(BuildPhase::Cancelled); return; }
	if (rc != 0) {
		reporter_.AppendLine("[build] CMake configure failed (exit " + std::to_string(rc) + ")");
		reporter_.Finish(BuildPhase::Failed);
		return;
	}

	// 4. Build via the matching build preset.
	reporter_.SetPhase(BuildPhase::Compiling);
	reporter_.SetProgress(0.0f);

	std::vector<std::string> build_argv;
	build_argv.push_back(tc.cmake_path.string());
	build_argv.push_back("--build");
	build_argv.push_back("--preset");
	build_argv.push_back(build_preset);
	build_argv.push_back("--parallel");

	rc = RunProcess(build_argv, project.project_root, env, cancel_, on_line);
	if (rc == -2) { reporter_.Finish(BuildPhase::Cancelled); return; }
	if (rc != 0) {
		reporter_.AppendLine("[build] CMake build failed (exit " + std::to_string(rc) + ")");
		reporter_.Finish(BuildPhase::Failed);
		return;
	}

	// 5. Install (build preset doesn't expose install; invoke explicitly).
	reporter_.SetPhase(BuildPhase::Installing);
	reporter_.SetProgress(-1.0f);

	std::vector<std::string> install_argv;
	install_argv.push_back(tc.cmake_path.string());
	install_argv.push_back("--install");
	install_argv.push_back(paths.cmake_build_dir.string());
	install_argv.push_back("--config");
	install_argv.push_back(target.configuration);

	rc = RunProcess(install_argv, project.project_root, env, cancel_, on_line);
	if (rc == -2) { reporter_.Finish(BuildPhase::Cancelled); return; }
	if (rc != 0) {
		reporter_.AppendLine("[build] Install failed (exit " + std::to_string(rc) + ")");
		reporter_.Finish(BuildPhase::Failed);
		return;
	}

	reporter_.AppendLine("[build] Build succeeded. Output: " + paths.install_dir.string());
	reporter_.Finish(BuildPhase::Succeeded);
}

} // namespace editor::build
