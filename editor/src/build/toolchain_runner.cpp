#include "build/toolchain_runner.h"

#include <cstdlib>
#include <iostream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace editor::build {

namespace {

std::filesystem::path Which(const std::string& exe) {
#if defined(_WIN32)
	const char* path_env = std::getenv("PATH");
	const std::string extensions[] = {".exe", ".bat", ".cmd", ""};
	const char sep = ';';
#else
	const char* path_env = std::getenv("PATH");
	const std::string extensions[] = {""};
	const char sep = ':';
#endif
	if (!path_env) return {};
	std::string path_str = path_env;
	size_t start = 0;
	while (start < path_str.size()) {
		size_t end = path_str.find(sep, start);
		if (end == std::string::npos) end = path_str.size();
		std::filesystem::path dir = path_str.substr(start, end - start);
		for (const auto& ext : extensions) {
			std::error_code ec;
			auto candidate = dir / (exe + ext);
			if (std::filesystem::is_regular_file(candidate, ec)) return candidate;
		}
		start = end + 1;
	}
	return {};
}

#if defined(_WIN32)

std::filesystem::path FindVcvarsBat() {
	// Try vswhere.exe at well-known location first.
	const char* pf86 = std::getenv("ProgramFiles(x86)");
	std::filesystem::path vswhere;
	if (pf86) {
		vswhere = std::filesystem::path(pf86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
	}
	std::error_code ec;
	if (!vswhere.empty() && std::filesystem::is_regular_file(vswhere, ec)) {
		const std::string cmd = "\""
								+ vswhere.string()
								+ "\" -latest -prerelease -products * "
								  "-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
								  "-property installationPath";
		FILE* pipe = _popen(cmd.c_str(), "r");
		if (pipe) {
			char buf[1024]{};
			if (fgets(buf, sizeof(buf), pipe)) {
				std::string line = buf;
				while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
					line.pop_back();
				}
				_pclose(pipe);
				if (!line.empty()) {
					auto candidate = std::filesystem::path(line) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
					if (std::filesystem::is_regular_file(candidate, ec)) return candidate;
				}
			}
			else {
				_pclose(pipe);
			}
		}
	}

	// Fallback: scan default install locations.
	const char* pf_env[] = {"ProgramFiles", "ProgramFiles(x86)"};
	const char* editions[] = {"Community", "Professional", "Enterprise", "BuildTools"};
	const char* years[] = {"2022", "2019"};
	for (const char* p : pf_env) {
		const char* base = std::getenv(p);
		if (!base) continue;
		for (const char* y : years) {
			for (const char* e : editions) {
				// clang-format off
				auto candidate = std::filesystem::path(base) / "Microsoft Visual Studio" / y / e / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
				// clang-format on
				if (std::filesystem::is_regular_file(candidate, ec)) return candidate;
			}
		}
	}
	return {};
}

std::string QuoteWindowsArg(const std::string& arg) {
	if (!arg.empty() && arg.find_first_of(" \t\"") == std::string::npos) return arg;
	std::string out = "\"";
	for (size_t i = 0; i < arg.size(); ++i) {
		size_t backslashes = 0;
		while (i < arg.size() && arg[i] == '\\') {
			++backslashes;
			++i;
		}
		if (i == arg.size()) {
			out.append(backslashes * 2, '\\');
			break;
		}
		if (arg[i] == '"') {
			out.append(backslashes * 2 + 1, '\\');
			out.push_back('"');
		}
		else {
			out.append(backslashes, '\\');
			out.push_back(arg[i]);
		}
	}
	out.push_back('"');
	return out;
}

#endif

} // namespace

ToolchainStatus DetectToolchain(bool needs_emsdk, bool needs_msvc_env) {
	ToolchainStatus status;
	status.cmake_path = Which("cmake");
	if (status.cmake_path.empty()) {
		status.error_message = "cmake was not found on PATH. Install CMake 3.28+ and ensure it is on PATH.";
		return status;
	}

	if (const char* vr = std::getenv("VCPKG_ROOT")) {
		status.vcpkg_root = vr;
		std::error_code ec;
		if (!std::filesystem::is_directory(status.vcpkg_root, ec)) {
			status.error_message = "VCPKG_ROOT is set but does not exist: " + status.vcpkg_root.string();
			return status;
		}
	}
	else {
		status.error_message = "VCPKG_ROOT environment variable is not set. Point it at your vcpkg checkout.";
		return status;
	}

	if (needs_emsdk) {
		const char* emsdk = std::getenv("EMSDK");
		if (!emsdk) {
			status.error_message = "EMSDK environment variable is not set. Activate emsdk before building for Web.";
			return status;
		}
		status.emsdk_env = std::filesystem::path(emsdk);
	}

#if defined(_WIN32)
	if (needs_msvc_env) {
		status.vcvars_bat = FindVcvarsBat();
		if (status.vcvars_bat.empty()) {
			status.error_message = "vcvars64.bat not found. Install Visual Studio 2019/2022 with the C++ build tools "
								   "(component: Microsoft.VisualStudio.Component.VC.Tools.x86.x64).";
			return status;
		}
	}
#else
	(void) needs_msvc_env;
#endif

	status.ok = true;
	return status;
}

std::unordered_map<std::string, std::string> CaptureVcvarsEnv(const std::filesystem::path& vcvars_bat) {
	std::unordered_map<std::string, std::string> env;
#if defined(_WIN32)
	if (vcvars_bat.empty()) return env;
	std::error_code ec;
	if (!std::filesystem::is_regular_file(vcvars_bat, ec)) return env;

	// Run `cmd /c "<vcvars64.bat>" >nul && set` and parse KEY=VALUE pairs from stdout.
	const std::string cmd = "cmd /c \"\"" + vcvars_bat.string() + "\" >nul && set\"";
	FILE* pipe = _popen(cmd.c_str(), "r");
	if (!pipe) return env;
	char buf[8192];
	while (fgets(buf, sizeof(buf), pipe)) {
		std::string line = buf;
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
		const auto eq = line.find('=');
		if (eq == std::string::npos || eq == 0) continue;
		env.emplace(line.substr(0, eq), line.substr(eq + 1));
	}
	_pclose(pipe);
#else
	(void) vcvars_bat;
#endif
	return env;
}

#if defined(_WIN32)

int RunProcess(
	const std::vector<std::string>& argv,
	const std::filesystem::path& working_dir,
	const std::unordered_map<std::string, std::string>& env_overrides,
	const std::atomic<bool>& cancel_flag,
	const std::function<void(std::string)>& on_line
) {
	if (argv.empty()) return -1;

	std::string cmdline;
	for (size_t i = 0; i < argv.size(); ++i) {
		if (i) cmdline += ' ';
		cmdline += QuoteWindowsArg(argv[i]);
	}

	// Build an environment block merging current env + overrides.
	std::string env_block;
	std::unordered_map<std::string, std::string> merged;
	if (char** e = _environ) {
		for (; *e; ++e) {
			std::string entry = *e;
			auto eq = entry.find('=');
			if (eq != std::string::npos && eq > 0) {
				merged[entry.substr(0, eq)] = entry.substr(eq + 1);
			}
		}
	}
	for (const auto& [k, v] : env_overrides) merged[k] = v;
	for (const auto& [k, v] : merged) {
		env_block += k;
		env_block += '=';
		env_block += v;
		env_block.push_back('\0');
	}
	env_block.push_back('\0');

	SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
	HANDLE read_h = nullptr, write_h = nullptr;
	if (!CreatePipe(&read_h, &write_h, &sa, 0)) return -1;
	SetHandleInformation(read_h, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = write_h;
	si.hStdError = write_h;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	PROCESS_INFORMATION pi{};

	std::string mutable_cmdline = cmdline;
	const std::string wd = working_dir.string();
	BOOL ok = CreateProcessA(
		nullptr,
		mutable_cmdline.data(),
		nullptr,
		nullptr,
		TRUE,
		CREATE_NO_WINDOW,
		env_block.data(),
		wd.empty() ? nullptr : wd.c_str(),
		&si,
		&pi
	);
	CloseHandle(write_h); // parent doesn't write
	if (!ok) {
		CloseHandle(read_h);
		return -1;
	}

	std::string buffer;
	char chunk[4096];
	DWORD bytes_read = 0;
	bool cancelled = false;

	while (true) {
		if (cancel_flag.load() && !cancelled) {
			TerminateProcess(pi.hProcess, 1);
			cancelled = true;
		}

		DWORD avail = 0;
		if (!PeekNamedPipe(read_h, nullptr, 0, nullptr, &avail, nullptr)) {
			break;
		}
		if (avail == 0) {
			// Check if the process is still running
			DWORD status = WaitForSingleObject(pi.hProcess, 50);
			if (status == WAIT_OBJECT_0) {
				// Drain remaining bytes once more
				if (PeekNamedPipe(read_h, nullptr, 0, nullptr, &avail, nullptr) && avail == 0) break;
			}
			continue;
		}
		if (!ReadFile(read_h, chunk, sizeof(chunk), &bytes_read, nullptr) || bytes_read == 0) {
			break;
		}
		buffer.append(chunk, bytes_read);
		size_t pos;
		while ((pos = buffer.find('\n')) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (on_line) on_line(std::move(line));
			buffer.erase(0, pos + 1);
		}
	}
	if (!buffer.empty()) {
		if (!buffer.empty() && buffer.back() == '\r') buffer.pop_back();
		if (on_line) on_line(std::move(buffer));
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exit_code = 0;
	GetExitCodeProcess(pi.hProcess, &exit_code);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(read_h);
	if (cancelled) return -2;
	return static_cast<int>(exit_code);
}

#else

int RunProcess(
	const std::vector<std::string>& argv,
	const std::filesystem::path& working_dir,
	const std::unordered_map<std::string, std::string>& env_overrides,
	const std::atomic<bool>& cancel_flag,
	const std::function<void(std::string)>& on_line
) {
	if (argv.empty()) return -1;

	int pipefd[2];
	if (pipe(pipefd) != 0) return -1;

	pid_t pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}
	if (pid == 0) {
		// child
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		if (!working_dir.empty()) {
			(void) chdir(working_dir.c_str());
		}
		for (const auto& [k, v] : env_overrides) {
			setenv(k.c_str(), v.c_str(), 1);
		}

		std::vector<char*> c_argv;
		c_argv.reserve(argv.size() + 1);
		for (const auto& s : argv) c_argv.push_back(const_cast<char*>(s.c_str()));
		c_argv.push_back(nullptr);
		execvp(c_argv[0], c_argv.data());
		_exit(127);
	}

	close(pipefd[1]);
	// Make read end non-blocking so we can poll the cancel flag.
	int flags = fcntl(pipefd[0], F_GETFL, 0);
	fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

	std::string buffer;
	char chunk[4096];
	bool cancelled = false;

	while (true) {
		if (cancel_flag.load() && !cancelled) {
			kill(pid, SIGTERM);
			cancelled = true;
		}

		ssize_t n = read(pipefd[0], chunk, sizeof(chunk));
		if (n > 0) {
			buffer.append(chunk, static_cast<size_t>(n));
			size_t pos;
			while ((pos = buffer.find('\n')) != std::string::npos) {
				std::string line = buffer.substr(0, pos);
				if (on_line) on_line(std::move(line));
				buffer.erase(0, pos + 1);
			}
		}
		else if (n == 0) {
			break; // EOF
		}
		else {
			int status = 0;
			pid_t r = waitpid(pid, &status, WNOHANG);
			if (r == pid) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
	if (!buffer.empty() && on_line) on_line(std::move(buffer));

	int status = 0;
	waitpid(pid, &status, 0);
	close(pipefd[0]);
	if (cancelled) return -2;
	if (WIFEXITED(status)) return WEXITSTATUS(status);
	return -1;
}

#endif

} // namespace editor::build
