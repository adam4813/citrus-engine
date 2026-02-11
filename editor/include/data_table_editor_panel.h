#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

#include "editor_panel.h"

import engine.data;
import glm;

namespace editor {

/**
 * @brief Data table editor panel for editing data tables in a spreadsheet-like interface
 * 
 * Provides a visual editor for creating and editing data tables with schema management,
 * inline cell editing, sorting, filtering, and CSV import/export capabilities.
 */
class DataTableEditorPanel : public EditorPanel {
public:
	DataTableEditorPanel();
	~DataTableEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;

	/**
	 * @brief Render the data table editor panel
	 */
	void Render();

	/**
	 * @brief Create a new empty data table
	 */
	void NewTable();

	/**
	 * @brief Save the current data table to a file
	 */
	bool SaveTable(const std::string& path);

	/**
	 * @brief Load a data table from a file
	 */
	bool LoadTable(const std::string& path);

	/**
	 * @brief Register asset type handlers for this panel
	 */
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

	/**
	 * @brief Open a data table file (for asset browser integration)
	 */
	void OpenTable(const std::string& path);

	/**
	 * @brief Export current table to CSV
	 */
	bool ExportToCSV(const std::string& path);

private:
	void RenderMenuBar();
	void RenderToolbar();
	void RenderSpreadsheet();
	void RenderSchemaEditor();
	
	// Cell rendering for different data types
	void RenderCell(const std::string& column_name, engine::data::DataValue& value, size_t row_index);
	bool RenderValueEditor(const std::string& label, engine::data::DataValue& value, const std::string& type_hint);
	
	// Row operations
	void AddRow();
	void DeleteRow(size_t row_index);
	void DuplicateRow(size_t row_index);
	
	// Column operations
	void AddColumn();
	void DeleteColumn(const std::string& column_name);
	void RenameColumn(const std::string& old_name, const std::string& new_name);
	
	// Sorting and filtering
	void SortByColumn(const std::string& column_name);
	void ApplyFilter();
	
	// Helper to get type name from DataValue
	static std::string GetTypeName(const engine::data::DataValue& value);
	
	// Helper to create default value for a type
	static engine::data::DataValue CreateDefaultValue(const std::string& type_name);
	
	// Helper to generate unique row key
	std::string GenerateUniqueRowKey() const;

	std::unique_ptr<engine::data::DataTable> table_;
	std::string current_file_path_;
	
	// UI state
	char table_name_buffer_[256] = "";
	char filter_text_[256] = "";
	char new_column_name_buffer_[256] = "";
	int new_column_type_index_ = 0; // Index into type dropdown
	
	// Sorting state
	std::string sort_column_;
	bool sort_ascending_ = true;
	
	// Schema editor state
	bool show_schema_editor_ = false;
	char schema_edit_column_name_[256] = "";
	int schema_edit_type_index_ = 0;
	
	// Confirmation dialogs
	bool show_delete_column_confirm_ = false;
	std::string column_to_delete_;
	
	// Available data types for dropdowns
	static constexpr const char* TYPE_NAMES[] = {"String", "Int", "Float", "Bool", "Vec2", "Vec3", "Vec4"};
	static constexpr int NUM_TYPES = 7;
};

} // namespace editor
