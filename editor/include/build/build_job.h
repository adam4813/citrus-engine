#pragma once

#include "build/build_reporter.h"
#include "build/project_model.h"

#include <atomic>
#include <memory>
#include <thread>

namespace editor::build {

class IAssetPackager;

/// Asynchronous build of a single project+target combination.
///
/// One BuildJob = one worker thread. The owning UI polls the embedded BuildReporter
/// each frame for progress/log. Construct with `Start(...)`, observe via `reporter()`,
/// terminate early via `Cancel()`.
class BuildJob {
public:
	BuildJob();
	~BuildJob();

	BuildJob(const BuildJob&) = delete;
	BuildJob& operator=(const BuildJob&) = delete;

	/// Kick off the build on a new worker thread.
	/// @return false if a job is already running.
	bool Start(ProjectModel project, BuildTarget target, std::unique_ptr<IAssetPackager> packager);

	/// Signal cancellation. Returns immediately; the worker thread will terminate the
	/// child process at the next opportunity and unwind cleanly.
	void Cancel();

	/// Block until the worker thread has exited.
	void Join();

	[[nodiscard]] bool Running() const { return running_.load(); }
	[[nodiscard]] BuildReporter& Reporter() { return reporter_; }
	[[nodiscard]] const BuildReporter& Reporter() const { return reporter_; }

	/// Absolute path to the install/dist directory once a job has completed. Empty until then.
	[[nodiscard]] const std::filesystem::path& InstallDir() const { return install_dir_; }

private:
	void Run(ProjectModel project, BuildTarget target, std::unique_ptr<IAssetPackager> packager);

	BuildReporter reporter_;
	std::thread worker_;
	std::atomic<bool> running_{false};
	std::atomic<bool> cancel_{false};
	std::filesystem::path install_dir_;
};

} // namespace editor::build
