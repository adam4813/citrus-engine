#include "build/build_reporter.h"

namespace editor::build {

const char* PhaseLabel(BuildPhase phase) {
	switch (phase) {
	case BuildPhase::Idle: return "Idle";
	case BuildPhase::Staging: return "Staging files";
	case BuildPhase::Configuring: return "Configuring CMake";
	case BuildPhase::Compiling: return "Compiling";
	case BuildPhase::Installing: return "Installing";
	case BuildPhase::Succeeded: return "Succeeded";
	case BuildPhase::Failed: return "Failed";
	case BuildPhase::Cancelled: return "Cancelled";
	}
	return "?";
}

void BuildReporter::SetPhase(BuildPhase phase) {
	std::lock_guard lock(mutex_);
	phase_ = phase;
}

void BuildReporter::SetProgress(float percent) {
	std::lock_guard lock(mutex_);
	progress_ = percent;
}

void BuildReporter::AppendLine(std::string line) {
	std::lock_guard lock(mutex_);
	lines_.push_back(std::move(line));
	// Trim oldest lines past the cap, tracking how many were dropped so absolute
	// indices stay monotonic for incremental consumers.
	while (lines_.size() > MAX_LOG_LINES) {
		lines_.pop_front();
		++base_index_;
	}
}

void BuildReporter::Finish(BuildPhase terminal) {
	std::lock_guard lock(mutex_);
	phase_ = terminal;
	if (terminal == BuildPhase::Succeeded) progress_ = 100.0f;
}

BuildPhase BuildReporter::Phase() const {
	std::lock_guard lock(mutex_);
	return phase_;
}

float BuildReporter::Progress() const {
	std::lock_guard lock(mutex_);
	return progress_;
}

void BuildReporter::CopyNewLines(std::vector<std::string>& dst, size_t& cursor, bool& out_truncated) const {
	std::lock_guard lock(mutex_);
	const size_t total = base_index_ + lines_.size();
	out_truncated = false;

	// Stale cursor (e.g. a Reset() shrank the log out from under the consumer): restart
	// from the oldest retained line.
	if (cursor > total) cursor = base_index_;

	// Consumer fell behind the trim window: older lines are gone.
	if (cursor < base_index_) {
		cursor = base_index_;
		out_truncated = true;
	}

	const size_t first = cursor - base_index_;
	dst.reserve(dst.size() + (lines_.size() - first));
	for (size_t i = first; i < lines_.size(); ++i) {
		dst.push_back(lines_[i]);
	}
	cursor = total;
}

std::string BuildReporter::FullLog() const {
	std::lock_guard lock(mutex_);
	std::string out;
	for (const auto& l : lines_) {
		out += l;
		out += '\n';
	}
	return out;
}

bool BuildReporter::IsTerminal() const {
	std::lock_guard lock(mutex_);
	return phase_ == BuildPhase::Succeeded || phase_ == BuildPhase::Failed || phase_ == BuildPhase::Cancelled;
}

void BuildReporter::Reset() {
	std::lock_guard lock(mutex_);
	phase_ = BuildPhase::Idle;
	progress_ = -1.0f;
	lines_.clear();
	base_index_ = 0;
}

} // namespace editor::build
