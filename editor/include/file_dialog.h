
#pragma once

#include <filesystem>
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

#include "file_utils.h"

namespace editor {

/// Mode for the file dialog: Open picks an existing file, Save allows a new name.
enum class FileDialogMode { Open, Save };

/**
 * @brief Reusable ImGui file dialog popup.
 *
 * Shows a directory tree on the left, file list on the right, and a filename
 * input at the bottom. Call Open() to show, Render() every frame, and check
 * the result callback for the selected path.
 *
 * Usage:
 *   FileDialogPopup dialog("Open Tileset", FileDialogMode::Open, {".json"});
 *   dialog.Open();                     // trigger popup
 *   dialog.Render();                   // call every frame
 *   // callback fires with selected path when user confirms
 */
class FileDialogPopup {
public:
	using Callback = std::function<void(const std::string& path)>;

	FileDialogPopup(std::string title, const FileDialogMode mode, std::vector<std::string> extensions = {}) :
			title_(std::move(title)), mode_(mode), extensions_(std::move(extensions)) {
		file_name_buffer_[0] = '\0';
	}

	/// Set the callback fired when the user confirms a selection.
	void SetCallback(Callback cb) { callback_ = std::move(cb); }

	/// Set the root directory to browse from (default: "assets").
	void SetRoot(const std::filesystem::path& root) {
		root_ = root;
		current_dir_ = root;
		needs_refresh_ = true;
	}

	/// Open the popup (call once, then Render() each frame).
	void Open() { should_open_ = true; }

	/// Open with a suggested filename pre-filled (Save mode).
	void Open(const std::string& suggested_name) {
		std::strncpy(file_name_buffer_, suggested_name.c_str(), sizeof(file_name_buffer_) - 1);
		file_name_buffer_[sizeof(file_name_buffer_) - 1] = '\0';
		should_open_ = true;
	}

	/// Render the popup. Call every frame.
	void Render() {
		if (should_open_) {
			ImGui::OpenPopup(title_.c_str());
			should_open_ = false;
			current_dir_ = root_;
			needs_refresh_ = true;
		}

		ImGui::SetNextWindowSize(ImVec2(640, 420), ImGuiCond_Appearing);
		if (!ImGui::BeginPopupModal(title_.c_str(), nullptr, ImGuiWindowFlags_None)) {
			return;
		}

		if (needs_refresh_) {
			entries_ = ListDirectory(current_dir_, extensions_);
			needs_refresh_ = false;
		}

		// Left panel: directory tree
		ImGui::BeginChild("DirTree", ImVec2(180, -ImGui::GetFrameHeightWithSpacing() - 4), true);
		if (std::filesystem::path new_dir; RenderDirectoryTree(root_, current_dir_, new_dir, true)) {
			current_dir_ = new_dir;
			needs_refresh_ = true;
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Right panel: file list
		ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 4), true);
		RenderFileList();
		ImGui::EndChild();

		// Bottom: filename input + buttons
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 160);
		if (ImGui::InputText(
					"##filename", file_name_buffer_, sizeof(file_name_buffer_), ImGuiInputTextFlags_EnterReturnsTrue)) {
			Confirm();
		}
		ImGui::SameLine();
		if (const char* confirm_label = (mode_ == FileDialogMode::Save) ? "Save" : "Open";
			ImGui::Button(confirm_label, ImVec2(70, 0))) {
			Confirm();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(70, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	std::filesystem::path& RootDirectory() { return root_; }

private:
	void RenderFileList() {
		ImGui::TextDisabled("%s", current_dir_.string().c_str());
		ImGui::Separator();

		// Parent directory entry
		if (current_dir_ != root_ && ImGui::Selectable("[..] ..", false, ImGuiSelectableFlags_AllowDoubleClick)
			&& ImGui::IsMouseDoubleClicked(0)) {
			current_dir_ = current_dir_.parent_path();
			needs_refresh_ = true;
		}

		for (const auto& [path, name, is_directory, icon] : entries_) {
			std::string label = icon + " " + name;

			if (const bool selected = (name == std::string(file_name_buffer_));
				ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
				if (is_directory) {
					if (ImGui::IsMouseDoubleClicked(0)) {
						current_dir_ = path;
						needs_refresh_ = true;
					}
				}
				else {
					std::strncpy(file_name_buffer_, name.c_str(), sizeof(file_name_buffer_) - 1);
					file_name_buffer_[sizeof(file_name_buffer_) - 1] = '\0';
					if (ImGui::IsMouseDoubleClicked(0)) {
						Confirm();
					}
				}
			}
		}
	}

	void Confirm() const {
		if (file_name_buffer_[0] == '\0') {
			return;
		}
		const auto selected = current_dir_ / file_name_buffer_;
		if (callback_) {
			callback_(selected.string());
		}
		ImGui::CloseCurrentPopup();
	}

	std::string title_;
	FileDialogMode mode_;
	std::vector<std::string> extensions_;
	Callback callback_;

	std::filesystem::path root_{"assets"};
	std::filesystem::path current_dir_{"assets"};
	std::vector<FileEntry> entries_;
	bool should_open_ = false;
	bool needs_refresh_ = true;
	char file_name_buffer_[512] = "";
};

} // namespace editor
