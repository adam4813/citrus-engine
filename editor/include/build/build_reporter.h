#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace editor::build {

enum class BuildPhase {
	Idle,
	Staging,
	Configuring,
	Compiling,
	Installing,
	Succeeded,
	Failed,
	Cancelled,
};

const char* PhaseLabel(BuildPhase phase);

/// Thread-safe sink for build progress + log lines. Producer = BuildJob worker thread.
/// Consumer = BuildDialog (UI thread, polled each frame).
///
/// The log is stored as an append-only list addressed by absolute line index, so the
/// UI can pull only the lines it hasn't seen yet (see CopyNewLines). A generous safety
/// cap bounds memory for pathological builds; when the oldest lines are trimmed the next
/// CopyNewLines reports truncation so the consumer can surface a marker.
class BuildReporter {
public:
	// Upper bound on retained lines. A first-time vcpkg build can emit tens of thousands
	// of lines; keep enough history to be useful without risking runaway memory.
	static constexpr size_t MAX_LOG_LINES = 200000;

	void SetPhase(BuildPhase phase);
	void SetProgress(float percent); // 0..100, or -1 for indeterminate
	void AppendLine(std::string line);
	void Finish(BuildPhase terminal);

	[[nodiscard]] BuildPhase Phase() const;
	[[nodiscard]] float Progress() const;

	/// Append lines the consumer hasn't seen yet (those at absolute index >= cursor) to
	/// `dst`, advancing `cursor` to the new total. If older lines were trimmed since the
	/// last call (cursor points before the retained window), sets `out_truncated` true
	/// and resumes from the oldest retained line. Append-only: never clears `dst`.
	void CopyNewLines(std::vector<std::string>& dst, size_t& cursor, bool& out_truncated) const;

	[[nodiscard]] std::string FullLog() const;
	[[nodiscard]] bool IsTerminal() const;

	void Reset();

private:
	mutable std::mutex mutex_;
	BuildPhase phase_ = BuildPhase::Idle;
	float progress_ = -1.0f;
	std::deque<std::string> lines_;
	size_t base_index_ = 0; // absolute index of lines_.front()
};

} // namespace editor::build
