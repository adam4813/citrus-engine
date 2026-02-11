#include "code_editor_panel.h"

#include "asset_editor_registry.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>

import engine;

namespace editor {

CodeEditorPanel::CodeEditorPanel() = default;

CodeEditorPanel::~CodeEditorPanel() = default;

std::string_view CodeEditorPanel::GetPanelName() const { return "Code Editor"; }

void CodeEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	for (const auto& ext : {".lua", ".as", ".glsl", ".vert", ".frag", ".cpp", ".h", ".hpp", ".c", ".shader"}) {
		registry.RegisterExtension(ext, [this](const std::string& path) { OpenFile(path); SetVisible(true); });
	}
}

void CodeEditorPanel::Render() {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Code Editor", &VisibleRef(), ImGuiWindowFlags_MenuBar);

	RenderMenuBar();
	RenderFileTabs();

	if (show_find_bar_) {
		RenderFindBar();
	}

	RenderEditor();

	ImGui::End();
}

void CodeEditorPanel::RenderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				NewFile();
			}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
				// For now, just show a simple input dialog
				// In a full implementation, this would use a file dialog
				ImGui::OpenPopup("OpenFileDialog");
			}
			if (ImGui::MenuItem("Save", "Ctrl+S", false, active_tab_index_ >= 0)) {
				SaveCurrentFile();
			}
			if (ImGui::MenuItem("Save As", nullptr, false, active_tab_index_ >= 0)) {
				SaveCurrentFileAs();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Close", "Ctrl+W", false, active_tab_index_ >= 0)) {
				CloseTab(active_tab_index_);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
				// ImGui::InputTextMultiline handles undo internally
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
				// ImGui::InputTextMultiline handles redo internally
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Find", "Ctrl+F", false, active_tab_index_ >= 0)) {
				ShowFindBar();
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	// Simple "Open File" dialog popup
	if (ImGui::BeginPopupModal("OpenFileDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter file path:");
		ImGui::InputText("##filepath", file_path_buffer_, sizeof(file_path_buffer_));

		if (ImGui::Button("Open")) {
			if (file_path_buffer_[0] != '\0') {
				OpenFile(file_path_buffer_);
				file_path_buffer_[0] = '\0';
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Handle keyboard shortcuts
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
		if (ImGui::IsKeyPressed(ImGuiKey_N) && ImGui::GetIO().KeyCtrl) {
			NewFile();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl) {
			SaveCurrentFile();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_W) && ImGui::GetIO().KeyCtrl) {
			if (active_tab_index_ >= 0) {
				CloseTab(active_tab_index_);
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl) {
			ShowFindBar();
		}
	}
}

void CodeEditorPanel::RenderFileTabs() {
	if (open_files_.empty()) {
		return;
	}

	if (ImGui::BeginTabBar("CodeFileTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {
		for (int i = 0; i < static_cast<int>(open_files_.size()); ++i) {
			auto& file = open_files_[i];

			// Tab label with modified indicator
			std::string label = file.display_name;
			if (file.is_modified) {
				label += "*";
			}

			bool tab_open = true;
			ImGuiTabItemFlags flags = 0;
			if (file.is_modified) {
				flags |= ImGuiTabItemFlags_UnsavedDocument;
			}

			if (ImGui::BeginTabItem(label.c_str(), &tab_open, flags)) {
				active_tab_index_ = i;
				ImGui::EndTabItem();
			}

			// Handle tab close
			if (!tab_open) {
				CloseTab(i);
				break; // Don't continue iterating after removing an item
			}
		}

		ImGui::EndTabBar();
	}
}

void CodeEditorPanel::RenderEditor() {
	if (active_tab_index_ < 0 || active_tab_index_ >= static_cast<int>(open_files_.size())) {
		ImGui::TextDisabled("No file open");
		return;
	}

	auto& file = open_files_[active_tab_index_];

	// Calculate available space
	const ImVec2 content_size = ImGui::GetContentRegionAvail();
	const float line_number_width = 50.0f;

	// Split area: line numbers on left, editor on right
	ImGui::BeginChild("LineNumbers", ImVec2(line_number_width, content_size.y), true);

	// Draw line numbers
	const int line_count = CountLines(file.content);
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use monospace font if available
	for (int i = 1; i <= line_count; ++i) {
		ImGui::TextDisabled("%4d", i);
	}
	ImGui::PopFont();

	ImGui::EndChild();

	ImGui::SameLine();

	// Text editor area
	ImGui::BeginChild("EditorContent", ImVec2(0, content_size.y), true);

	// Make text input take all available space
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

	// Resize buffer if needed (add extra space for editing)
	static std::string edit_buffer;
	edit_buffer = file.content;
	edit_buffer.resize(edit_buffer.size() + 1024 * 16); // Add 16KB buffer

	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use monospace font if available

	if (ImGui::InputTextMultiline("##editor", &edit_buffer[0], edit_buffer.capacity(),
								   ImVec2(-FLT_MIN, -FLT_MIN), flags)) {
		// Content changed
		edit_buffer.resize(std::strlen(edit_buffer.c_str())); // Trim to actual size
		if (edit_buffer != file.content) {
			file.content = edit_buffer;
			file.is_modified = true;
		}
	}

	ImGui::PopFont();

	ImGui::EndChild();
}

void CodeEditorPanel::RenderFindBar() {
	ImGui::Separator();
	ImGui::Text("Find:");
	ImGui::SameLine();

	ImGui::SetNextItemWidth(200);
	if (ImGui::InputText("##find", find_buffer_, sizeof(find_buffer_))) {
		// Search text changed, reset results
		current_find_index_ = -1;
		if (find_buffer_[0] != '\0') {
			auto* file = GetCurrentFile();
			if (file) {
				find_results_ = FindOccurrences(file->content, find_buffer_);
				if (!find_results_.empty()) {
					current_find_index_ = 0;
				}
			}
		} else {
			find_results_.clear();
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Previous") && !find_results_.empty()) {
		FindPrevious();
	}

	ImGui::SameLine();
	if (ImGui::Button("Next") && !find_results_.empty()) {
		FindNext();
	}

	ImGui::SameLine();
	if (!find_results_.empty()) {
		ImGui::Text("(%d/%d)", current_find_index_ + 1, static_cast<int>(find_results_.size()));
	}

	ImGui::SameLine();
	if (ImGui::Button("Close")) {
		show_find_bar_ = false;
		find_buffer_[0] = '\0';
		find_results_.clear();
		current_find_index_ = -1;
	}

	ImGui::Separator();
}

void CodeEditorPanel::NewFile() {
	CodeFile file;
	file.display_name = "Untitled";
	file.path = "";
	file.content = "";
	file.is_modified = false;

	// Find unique name
	int counter = 1;
	while (true) {
		bool name_exists = false;
		std::string test_name = counter == 1 ? "Untitled" : "Untitled (" + std::to_string(counter) + ")";
		for (const auto& f : open_files_) {
			if (f.display_name == test_name) {
				name_exists = true;
				break;
			}
		}
		if (!name_exists) {
			file.display_name = test_name;
			break;
		}
		counter++;
	}

	open_files_.push_back(file);
	active_tab_index_ = static_cast<int>(open_files_.size()) - 1;
}

void CodeEditorPanel::OpenFile(const std::string& path) {
	// Check if file is already open
	for (int i = 0; i < static_cast<int>(open_files_.size()); ++i) {
		if (open_files_[i].path == path) {
			active_tab_index_ = i;
			return;
		}
	}

	// Try to read file
	auto content = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
	if (!content) {
		// TODO: Show error message
		return;
	}

	// Create new tab
	CodeFile code_file;
	code_file.path = path;
	code_file.content = *content;
	code_file.display_name = std::filesystem::path(path).filename().string();
	code_file.is_modified = false;

	open_files_.push_back(code_file);
	active_tab_index_ = static_cast<int>(open_files_.size()) - 1;
}

bool CodeEditorPanel::SaveCurrentFile() {
	auto* file = GetCurrentFile();
	if (!file) {
		return false;
	}

	// If no path, prompt for Save As
	if (file->path.empty()) {
		return SaveCurrentFileAs();
	}

	// Write to file
	if (!engine::assets::AssetManager::SaveTextFile(std::filesystem::path(file->path), file->content)) {
		// TODO: Show error message
		return false;
	}

	file->is_modified = false;
	return true;
}

bool CodeEditorPanel::SaveCurrentFileAs() {
	// For now, just show a simple input dialog
	// In a full implementation, this would use a file dialog
	ImGui::OpenPopup("SaveAsDialog");
	return false;
}

void CodeEditorPanel::CloseTab(int index) {
	if (index < 0 || index >= static_cast<int>(open_files_.size())) {
		return;
	}

	// TODO: Prompt to save if modified

	open_files_.erase(open_files_.begin() + index);

	// Adjust active tab index
	if (active_tab_index_ >= static_cast<int>(open_files_.size())) {
		active_tab_index_ = static_cast<int>(open_files_.size()) - 1;
	}
}

void CodeEditorPanel::ShowFindBar() {
	show_find_bar_ = true;
	find_buffer_[0] = '\0';
	find_results_.clear();
	current_find_index_ = -1;
}

void CodeEditorPanel::FindNext() {
	if (find_results_.empty()) {
		return;
	}

	current_find_index_ = (current_find_index_ + 1) % static_cast<int>(find_results_.size());
	// TODO: Scroll to result and highlight it
}

void CodeEditorPanel::FindPrevious() {
	if (find_results_.empty()) {
		return;
	}

	current_find_index_--;
	if (current_find_index_ < 0) {
		current_find_index_ = static_cast<int>(find_results_.size()) - 1;
	}
	// TODO: Scroll to result and highlight it
}

CodeFile* CodeEditorPanel::GetCurrentFile() {
	if (active_tab_index_ < 0 || active_tab_index_ >= static_cast<int>(open_files_.size())) {
		return nullptr;
	}
	return &open_files_[active_tab_index_];
}

const CodeFile* CodeEditorPanel::GetCurrentFile() const {
	if (active_tab_index_ < 0 || active_tab_index_ >= static_cast<int>(open_files_.size())) {
		return nullptr;
	}
	return &open_files_[active_tab_index_];
}

int CodeEditorPanel::CountLines(const std::string& text) {
	if (text.empty()) {
		return 1;
	}

	int count = 1;
	for (char c : text) {
		if (c == '\n') {
			count++;
		}
	}
	return count;
}

std::vector<size_t> CodeEditorPanel::FindOccurrences(const std::string& text, const std::string& search) const {
	std::vector<size_t> results;
	if (search.empty()) {
		return results;
	}

	size_t pos = 0;
	while ((pos = text.find(search, pos)) != std::string::npos) {
		results.push_back(pos);
		pos += search.length();
	}

	return results;
}

} // namespace editor
