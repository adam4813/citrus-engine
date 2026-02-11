#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "editor_panel.h"

namespace editor {

/**
 * @brief A single open file tab in the code editor
 */
struct CodeFile {
	std::string path;         // Full file path
	std::string content;      // File content
	std::string display_name; // Filename for tab display
	bool is_modified = false; // Dirty flag
	int cursor_position = 0;  // Cursor position in the text
};

/**
 * @brief Lightweight in-editor code editor panel
 *
 * Features:
 * - Multiple file tabs
 * - Basic text editing with ImGui::InputTextMultiline
 * - Line numbers
 * - File operations (New, Open, Save, Save As)
 * - Find functionality (Ctrl+F)
 * - Modified indicator on tabs
 */
class CodeEditorPanel : public EditorPanel {
public:
	CodeEditorPanel();
	~CodeEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;

	/**
	 * @brief Render the code editor panel
	 */
	void Render();

	/**
	 * @brief Open a file from disk
	 * @param path File path to open
	 */
	void OpenFile(const std::string& path);

	/**
	 * @brief Register asset type handlers for this panel
	 */
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

private:
	void RenderMenuBar();
	void RenderFileTabs();
	void RenderEditor();
	void RenderFindBar();

	// File operations
	void NewFile();
	bool SaveCurrentFile();
	bool SaveCurrentFileAs();
	void CloseTab(int index);

	// Find functionality
	void ShowFindBar();
	void FindNext();
	void FindPrevious();

	// Get current active file (nullptr if none)
	CodeFile* GetCurrentFile();
	const CodeFile* GetCurrentFile() const;

	// Count lines in a string
	static int CountLines(const std::string& text);

	// Find occurrences of search term
	std::vector<size_t> FindOccurrences(const std::string& text, const std::string& search) const;

	std::vector<CodeFile> open_files_;
	int active_tab_index_ = -1;

	// Find state
	bool show_find_bar_ = false;
	char find_buffer_[256] = "";
	int current_find_index_ = -1;
	std::vector<size_t> find_results_;

	// File dialog state
	char file_path_buffer_[512] = "";
};

} // namespace editor
