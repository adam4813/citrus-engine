#pragma once

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace editor::build {

struct ToolchainStatus {
	bool ok = false;
	std::string error_message;
	std::filesystem::path cmake_path;
	std::filesystem::path vcvars_bat;   // Windows MSVC; empty if not needed/found
	std::filesystem::path vcpkg_root;
	std::filesystem::path emsdk_env;    // Web target; empty if not needed
};

/// Inspect the environment for required tools.
/// @param needs_emsdk true when the build target requires Emscripten.
/// @param needs_msvc_env true on Windows native builds when MSVC env may need bootstrapping.
ToolchainStatus DetectToolchain(bool needs_emsdk, bool needs_msvc_env);

/// On Windows, invoke `vcvars64.bat` in a subshell and capture the resulting environment
/// variables. Returns an empty map on non-Windows or if the script fails to execute.
/// The returned map can be merged into RunProcess `env_overrides` so child cmake/ninja
/// invocations see the MSVC toolchain.
std::unordered_map<std::string, std::string> CaptureVcvarsEnv(const std::filesystem::path& vcvars_bat);

/// Run a child process, streaming each stdout/stderr line via `on_line`.
/// @return process exit code (0 == success); -1 on spawn failure, -2 on cancellation.
int RunProcess(
		const std::vector<std::string>& argv,
		const std::filesystem::path& working_dir,
		const std::unordered_map<std::string, std::string>& env_overrides,
		const std::atomic<bool>& cancel_flag,
		const std::function<void(std::string)>& on_line);

} // namespace editor::build
