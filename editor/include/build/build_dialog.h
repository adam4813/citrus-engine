#pragma once

#include "build/build_job.h"

#include <memory>
#include <string>
#include <vector>

namespace editor::build {

/// ImGui modal that polls a BuildJob's reporter each frame and displays progress / log.
class BuildDialog {
public:
	BuildDialog();
	~BuildDialog();

	/// Open the dialog and bind it to the given (started) job.
	void Open(std::unique_ptr<BuildJob> job);

	/// Render the modal. Must be called every frame while the editor is up.
	void Render();

	[[nodiscard]] bool IsOpen() const { return open_; }

private:
	std::unique_ptr<BuildJob> job_;
	bool open_ = false;
	bool open_requested_ = false;

	// UI-thread-local mirror of the reporter log, pulled incrementally each frame so we
	// only copy newly-emitted lines and render visible rows via ImGuiListClipper.
	std::vector<std::string> log_cache_;
	size_t log_cursor_ = 0;
};

} // namespace editor::build
