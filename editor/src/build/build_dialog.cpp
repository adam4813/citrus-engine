#include "build/build_dialog.h"

#include <imgui.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <windows.h>
#endif

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace editor::build {

namespace {

constexpr const char* kTruncatedMarker = "[... earlier output truncated ...]";

void OpenInFileManager(const std::filesystem::path& path) {
#if defined(_WIN32)
	// Use ShellExecuteA: std::system("explorer ...") is unreliable for paths with spaces
	// or special characters and can crash on certain shell environments.
	const std::string p = path.string();
	HINSTANCE rc = ShellExecuteA(nullptr, "open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	(void) rc;
#elif defined(__APPLE__)
	std::string cmd = "open \"" + path.string() + "\"";
	std::system(cmd.c_str());
#else
	std::string cmd = "xdg-open \"" + path.string() + "\" &";
	std::system(cmd.c_str());
#endif
}

} // namespace

BuildDialog::BuildDialog() = default;
BuildDialog::~BuildDialog() = default;

void BuildDialog::Open(std::unique_ptr<BuildJob> job) {
	job_ = std::move(job);
	open_ = true;
	open_requested_ = true;
	// Fresh job => fresh log mirror.
	log_cache_.clear();
	log_cursor_ = 0;
}

void BuildDialog::Render() {
	if (!open_) return;

	const char* kPopupId = "Build##build_dialog";
	if (open_requested_) {
		ImGui::OpenPopup(kPopupId);
		open_requested_ = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(720, 480), ImGuiCond_Appearing);

	if (ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
		if (!job_) {
			ImGui::Text("No build job bound.");
			if (ImGui::Button("Close")) {
				open_ = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
			return;
		}

		auto& reporter = job_->Reporter();
		const auto phase = reporter.Phase();
		const float progress = reporter.Progress();
		const bool terminal = reporter.IsTerminal();

		// Header: phase + progress
		ImGui::Text("Phase: %s", PhaseLabel(phase));
		if (progress < 0.0f) {
			// Indeterminate spinner-ish bar
			const float t = static_cast<float>(ImGui::GetTime());
			const float frac = 0.5f + 0.5f * sinf(t * 2.0f);
			ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), "working...");
		}
		else {
			char buf[32];
			snprintf(buf, sizeof(buf), "%.0f%%", progress);
			ImGui::ProgressBar(progress / 100.0f, ImVec2(-FLT_MIN, 0), buf);
		}

		if (phase == BuildPhase::Configuring) {
			ImGui::TextColored(
				ImVec4(0.85f, 0.75f, 0.2f, 1.0f),
				"First build may take several minutes (vcpkg installs engine + dependencies)."
			);
		}

		ImGui::Separator();

		// Log tail. Pull only newly-emitted lines into our local mirror, then render the
		// visible window with a clipper so huge logs stay cheap to draw.
		bool truncated = false;
		reporter.CopyNewLines(log_cache_, log_cursor_, truncated);
		if (truncated && (log_cache_.empty() || log_cache_.front() != kTruncatedMarker)) {
			log_cache_.insert(log_cache_.begin(), kTruncatedMarker);
		}

		if (ImGui::BeginChild(
				"log",
				ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 8),
				true,
				ImGuiWindowFlags_HorizontalScrollbar
			)) {
			// Decide whether to stick to the bottom BEFORE the new content shifts the
			// scroll range (otherwise fresh output makes us look "scrolled up").
			const bool stick_to_bottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f;

			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(log_cache_.size()));
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
					ImGui::TextUnformatted(log_cache_[static_cast<size_t>(i)].c_str());
				}
			}

			// Auto-scroll while running. SetScrollHereY is unreliable with a clipper
			// (the last row may not be submitted), so drive scroll position directly.
			if (!terminal && stick_to_bottom) {
				ImGui::SetScrollY(ImGui::GetScrollMaxY());
			}
		}
		ImGui::EndChild();

		// Buttons
		if (!terminal) {
			if (ImGui::Button("Cancel")) {
				job_->Cancel();
			}
		}
		else {
			// Snapshot install dir + log BEFORE any close/reset so we can't dereference
			// a moved-from job_ in the same frame after the Close button is pressed.
			const auto install_dir = job_->InstallDir();
			const bool can_open_dir = (phase == BuildPhase::Succeeded) && !install_dir.empty();

			bool close_clicked = false;
			if (ImGui::Button("Close")) {
				close_clicked = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Copy Log")) {
				ImGui::SetClipboardText(reporter.FullLog().c_str());
			}
			if (can_open_dir) {
				ImGui::SameLine();
				if (ImGui::Button("Open Output Folder")) {
					OpenInFileManager(install_dir);
				}
			}

			if (close_clicked) {
				job_->Join();
				job_.reset();
				open_ = false;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
}

} // namespace editor::build
