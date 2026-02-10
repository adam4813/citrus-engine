#include "data_table_editor_panel.h"

#include <algorithm>
#include <fstream>
#include <imgui.h>
#include <sstream>
#include <variant>

namespace editor {

DataTableEditorPanel::DataTableEditorPanel() : table_(std::make_unique<engine::data::DataTable>()) {
	table_->SetName("NewTable");
	std::strcpy(table_name_buffer_, "NewTable");
}

DataTableEditorPanel::~DataTableEditorPanel() = default;

void DataTableEditorPanel::Render() {
	if (!is_visible_) {
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Data Table Editor", &is_visible_, ImGuiWindowFlags_MenuBar);

	RenderMenuBar();
	RenderToolbar();

	// Split view: Spreadsheet on top, schema editor on bottom (if visible)
	if (show_schema_editor_) {
		ImGui::BeginChild("SpreadsheetArea", ImVec2(0, -200), true);
		RenderSpreadsheet();
		ImGui::EndChild();

		ImGui::BeginChild("SchemaEditorArea", ImVec2(0, 0), true);
		RenderSchemaEditor();
		ImGui::EndChild();
	}
	else {
		RenderSpreadsheet();
	}

	ImGui::End();
}

void DataTableEditorPanel::RenderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewTable();
			}
			if (ImGui::MenuItem("Open...")) {
				// Placeholder: Would use file dialog
				LoadTable("data_table.json");
			}
			if (ImGui::MenuItem("Save", nullptr, false, !current_file_path_.empty())) {
				SaveTable(current_file_path_);
			}
			if (ImGui::MenuItem("Save As...")) {
				// Placeholder: Would use file dialog
				SaveTable("data_table.json");
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export CSV...")) {
				ExportToCSV("data_table.csv");
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Add Row")) {
				AddRow();
			}
			if (ImGui::MenuItem("Add Column")) {
				show_schema_editor_ = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Delete Row", nullptr, false, false)) {
				// Requires row selection - not yet implemented
			}
			if (ImGui::MenuItem("Delete Column", nullptr, false, !table_->GetColumns().empty())) {
				// Show delete column UI
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Schema Editor", nullptr, &show_schema_editor_);
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void DataTableEditorPanel::RenderToolbar() {
	// Table name
	ImGui::Text("Table Name:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	if (ImGui::InputText("##table_name", table_name_buffer_, sizeof(table_name_buffer_))) {
		table_->SetName(table_name_buffer_);
	}

	ImGui::SameLine();
	ImGui::Separator();

	// Row count
	ImGui::SameLine();
	ImGui::Text("Rows: %zu", table_->GetRowCount());

	ImGui::SameLine();
	ImGui::Text("Columns: %zu", table_->GetColumns().size());

	// Filter
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	if (ImGui::InputTextWithHint("##filter", "Filter...", filter_text_, sizeof(filter_text_))) {
		ApplyFilter();
	}

	// Quick actions
	ImGui::SameLine();
	if (ImGui::Button("Add Row")) {
		AddRow();
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Column")) {
		show_schema_editor_ = true;
	}
}

void DataTableEditorPanel::RenderSpreadsheet() {
	const auto& columns = table_->GetColumns();
	auto& rows = table_->GetAllRowsMutable();

	if (columns.empty()) {
		ImGui::TextDisabled("No columns defined. Use 'Add Column' to get started.");
		return;
	}

	// Create table with scrolling
	ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX
							| ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable
							| ImGuiTableFlags_SizingFixedFit;

	// +1 for row number column, +1 for key column, +1 for actions column
	if (ImGui::BeginTable("DataTable", static_cast<int>(columns.size() + 3), flags)) {
		// Setup columns
		ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 40.0f);
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 100.0f);

		for (const auto& col : columns) {
			ImGui::TableSetupColumn(col.name.c_str(), ImGuiTableColumnFlags_WidthFixed, 120.0f);
		}

		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 100.0f);

		// Header row
		ImGui::TableHeadersRow();

		// Handle sorting
		if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
			if (sorts_specs->SpecsDirty) {
				if (sorts_specs->SpecsCount > 0) {
					const ImGuiTableColumnSortSpecs& spec = sorts_specs->Specs[0];
					// Column 0 is row number, column 1 is key, columns 2+ are data columns
					if (spec.ColumnIndex == 1) {
						// Sort by key
						std::sort(rows.begin(), rows.end(), [&spec](const auto& a, const auto& b) {
							return spec.SortDirection == ImGuiSortDirection_Ascending ? a.key < b.key : a.key > b.key;
						});
					}
					else if (spec.ColumnIndex >= 2 && spec.ColumnIndex < static_cast<int>(columns.size() + 2)) {
						// Sort by data column
						const std::string& col_name = columns[spec.ColumnIndex - 2].name;
						SortByColumn(col_name);
						sort_ascending_ = (spec.SortDirection == ImGuiSortDirection_Ascending);
					}
				}
				sorts_specs->SpecsDirty = false;
			}
		}

		// Data rows
		for (size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
			auto& row = rows[row_idx];

			ImGui::TableNextRow();

			// Row number
			ImGui::TableNextColumn();
			ImGui::Text("%zu", row_idx + 1);

			// Row key (editable)
			ImGui::TableNextColumn();
			ImGui::PushID(static_cast<int>(row_idx));
			char key_buffer[256];
			std::strncpy(key_buffer, row.key.c_str(), sizeof(key_buffer) - 1);
			key_buffer[sizeof(key_buffer) - 1] = '\0';
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputText("##key", key_buffer, sizeof(key_buffer))) {
				row.key = key_buffer;
			}

			// Data columns
			for (const auto& col : columns) {
				ImGui::TableNextColumn();
				RenderCell(col.name, row.values[col.name], row_idx);
			}

			// Actions column
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("Delete")) {
				DeleteRow(row_idx);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Duplicate")) {
				DuplicateRow(row_idx);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}

void DataTableEditorPanel::RenderSchemaEditor() {
	ImGui::Text("Schema Editor");
	ImGui::Separator();

	// Add new column
	ImGui::Text("Add Column:");
	ImGui::SetNextItemWidth(200);
	ImGui::InputTextWithHint("##new_col_name", "Column Name", new_column_name_buffer_, sizeof(new_column_name_buffer_));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::Combo("##new_col_type", &new_column_type_index_, TYPE_NAMES, NUM_TYPES);
	ImGui::SameLine();
	if (ImGui::Button("Add##add_column")) {
		if (std::strlen(new_column_name_buffer_) > 0) {
			AddColumn();
		}
	}

	ImGui::Separator();

	// List existing columns
	ImGui::Text("Existing Columns:");

	const auto& columns = table_->GetColumns();
	for (size_t i = 0; i < columns.size(); ++i) {
		const auto& col = columns[i];
		ImGui::PushID(static_cast<int>(i));

		ImGui::Text("%s", col.name.c_str());
		ImGui::SameLine(200);
		if (ImGui::SmallButton("Delete")) {
			column_to_delete_ = col.name;
			show_delete_column_confirm_ = true;
		}

		ImGui::PopID();
	}

	// Confirmation dialog for column deletion
	if (show_delete_column_confirm_) {
		ImGui::OpenPopup("Delete Column?");
		show_delete_column_confirm_ = false;
	}

	if (ImGui::BeginPopupModal("Delete Column?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Delete column '%s'?", column_to_delete_.c_str());
		ImGui::Text("This will remove the column and all its data.");

		if (ImGui::Button("Delete", ImVec2(120, 0))) {
			DeleteColumn(column_to_delete_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void DataTableEditorPanel::RenderCell(
		const std::string& column_name, engine::data::DataValue& value, size_t row_index) {
	ImGui::PushID(static_cast<int>(row_index * 1000 + std::hash<std::string>{}(column_name)));
	ImGui::SetNextItemWidth(-FLT_MIN);
	RenderValueEditor("##cell", value, "");
	ImGui::PopID();
}

bool DataTableEditorPanel::RenderValueEditor(
		const std::string& label, engine::data::DataValue& value, const std::string& type_hint) {
	bool changed = false;

	// Use visitor to handle different types
	std::visit(
			[&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, bool>) {
					changed = ImGui::Checkbox(label.c_str(), &arg);
				}
				else if constexpr (std::is_same_v<T, int>) {
					changed = ImGui::InputInt(label.c_str(), &arg);
				}
				else if constexpr (std::is_same_v<T, float>) {
					changed = ImGui::InputFloat(label.c_str(), &arg);
				}
				else if constexpr (std::is_same_v<T, glm::vec2>) {
					changed = ImGui::InputFloat2(label.c_str(), &arg.x);
				}
				else if constexpr (std::is_same_v<T, glm::vec3>) {
					changed = ImGui::InputFloat3(label.c_str(), &arg.x);
				}
				else if constexpr (std::is_same_v<T, glm::vec4>) {
					changed = ImGui::InputFloat4(label.c_str(), &arg.x);
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					char buffer[256];
					std::strncpy(buffer, arg.c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer))) {
						arg = buffer;
						changed = true;
					}
				}
			},
			value);

	return changed;
}

void DataTableEditorPanel::AddRow() {
	engine::data::DataRow row;
	row.key = GenerateUniqueRowKey();

	// Initialize all columns with default values
	const auto& columns = table_->GetColumns();
	for (const auto& col : columns) {
		// Default to string for now
		row.values[col.name] = std::string("");
	}

	table_->AddRow(std::move(row));
}

void DataTableEditorPanel::DeleteRow(size_t row_index) { table_->RemoveRowByIndex(row_index); }

void DataTableEditorPanel::DuplicateRow(size_t row_index) {
	if (auto opt_row = table_->GetRowByIndex(row_index)) {
		engine::data::DataRow new_row = *opt_row;
		new_row.key = GenerateUniqueRowKey();
		table_->AddRow(std::move(new_row));
	}
}

void DataTableEditorPanel::AddColumn() {
	if (std::strlen(new_column_name_buffer_) == 0) {
		return;
	}

	// Add column to schema
	engine::data::ColumnDefinition col_def;
	col_def.name = new_column_name_buffer_;
	table_->AddColumn(col_def);

	// Add default value to all existing rows
	auto& rows = table_->GetAllRowsMutable();
	const std::string type_name = TYPE_NAMES[new_column_type_index_];
	engine::data::DataValue default_value = CreateDefaultValue(type_name);

	for (auto& row : rows) {
		row.values[col_def.name] = default_value;
	}

	// Clear input
	new_column_name_buffer_[0] = '\0';
	new_column_type_index_ = 0;
}

void DataTableEditorPanel::DeleteColumn(const std::string& column_name) {
	// Remove column from all rows
	auto& rows = table_->GetAllRowsMutable();
	for (auto& row : rows) {
		row.values.erase(column_name);
	}

	// Remove column from schema (we need to manually do this since DataTable doesn't have RemoveColumn)
	// For now, we'll recreate the columns list
	const auto& old_columns = table_->GetColumns();
	std::vector<engine::data::ColumnDefinition> new_columns;
	for (const auto& col : old_columns) {
		if (col.name != column_name) {
			new_columns.push_back(col);
		}
	}

	// Recreate table with new columns
	auto new_table = std::make_unique<engine::data::DataTable>();
	new_table->SetName(table_->GetName());
	for (const auto& col : new_columns) {
		new_table->AddColumn(col);
	}
	for (auto& row : rows) {
		new_table->AddRow(std::move(row));
	}

	table_ = std::move(new_table);
}

void DataTableEditorPanel::RenameColumn(const std::string& old_name, const std::string& new_name) {
	// Rename in all rows
	auto& rows = table_->GetAllRowsMutable();
	for (auto& row : rows) {
		if (auto it = row.values.find(old_name); it != row.values.end()) {
			auto value = std::move(it->second);
			row.values.erase(it);
			row.values[new_name] = std::move(value);
		}
	}

	// Update column definition (similar to DeleteColumn, we recreate)
	const auto& old_columns = table_->GetColumns();
	std::vector<engine::data::ColumnDefinition> new_columns;
	for (const auto& col : old_columns) {
		if (col.name == old_name) {
			engine::data::ColumnDefinition new_col;
			new_col.name = new_name;
			new_columns.push_back(new_col);
		}
		else {
			new_columns.push_back(col);
		}
	}

	auto new_table = std::make_unique<engine::data::DataTable>();
	new_table->SetName(table_->GetName());
	for (const auto& col : new_columns) {
		new_table->AddColumn(col);
	}
	for (auto& row : rows) {
		new_table->AddRow(std::move(row));
	}

	table_ = std::move(new_table);
}

void DataTableEditorPanel::SortByColumn(const std::string& column_name) {
	auto& rows = table_->GetAllRowsMutable();

	std::sort(rows.begin(), rows.end(), [&](const auto& a, const auto& b) {
		const auto& val_a = a.GetValue(column_name);
		const auto& val_b = b.GetValue(column_name);

		// Compare based on the type (only for types that support operator<)
		bool result = false;
		std::visit(
				[&](auto&& arg_a) {
					using T = std::decay_t<decltype(arg_a)>;
					if constexpr (requires(T x, T y) { x < y; }) {
						if (const auto* arg_b = std::get_if<T>(&val_b)) {
							result = arg_a < *arg_b;
						}
					}
				},
				val_a);

		return sort_ascending_ ? result : !result;
	});
}

void DataTableEditorPanel::ApplyFilter() {
	// For now, filtering is not fully implemented
	// Would need to maintain a separate filtered view of rows
}

std::string DataTableEditorPanel::GetTypeName(const engine::data::DataValue& value) {
	return std::visit(
			[](auto&& arg) -> std::string {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, bool>)
					return "Bool";
				else if constexpr (std::is_same_v<T, int>)
					return "Int";
				else if constexpr (std::is_same_v<T, float>)
					return "Float";
				else if constexpr (std::is_same_v<T, glm::vec2>)
					return "Vec2";
				else if constexpr (std::is_same_v<T, glm::vec3>)
					return "Vec3";
				else if constexpr (std::is_same_v<T, glm::vec4>)
					return "Vec4";
				else if constexpr (std::is_same_v<T, std::string>)
					return "String";
				else
					return "Unknown";
			},
			value);
}

engine::data::DataValue DataTableEditorPanel::CreateDefaultValue(const std::string& type_name) {
	if (type_name == "Bool")
		return false;
	if (type_name == "Int")
		return 0;
	if (type_name == "Float")
		return 0.0f;
	if (type_name == "Vec2")
		return glm::vec2(0.0f);
	if (type_name == "Vec3")
		return glm::vec3(0.0f);
	if (type_name == "Vec4")
		return glm::vec4(0.0f);
	return std::string(""); // Default to string
}

std::string DataTableEditorPanel::GenerateUniqueRowKey() const {
	// Simple unique key generation
	size_t row_count = table_->GetRowCount();
	std::string base_key = "row_" + std::to_string(row_count + 1);

	// Ensure uniqueness
	std::string key = base_key;
	int suffix = 1;
	while (table_->GetRow(key).has_value()) {
		key = base_key + "_" + std::to_string(suffix);
		++suffix;
	}

	return key;
}

void DataTableEditorPanel::NewTable() {
	table_ = std::make_unique<engine::data::DataTable>();
	table_->SetName("NewTable");
	std::strcpy(table_name_buffer_, "NewTable");
	current_file_path_.clear();
}

bool DataTableEditorPanel::SaveTable(const std::string& path) {
	try {
		std::string json_str = engine::data::DataSerializer::SerializeTable(*table_);

		std::ofstream file(path);
		if (!file.is_open()) {
			return false;
		}

		file << json_str;
		file.close();

		current_file_path_ = path;
		return true;
	}
	catch (...) {
		return false;
	}
}

bool DataTableEditorPanel::LoadTable(const std::string& path) {
	try {
		std::ifstream file(path);
		if (!file.is_open()) {
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string json_str = buffer.str();
		file.close();

		*table_ = engine::data::DataSerializer::DeserializeTable(json_str);

		// Update UI buffers
		std::strncpy(table_name_buffer_, table_->GetName().c_str(), sizeof(table_name_buffer_) - 1);
		table_name_buffer_[sizeof(table_name_buffer_) - 1] = '\0';

		current_file_path_ = path;
		return true;
	}
	catch (...) {
		return false;
	}
}

void DataTableEditorPanel::OpenTable(const std::string& path) {
	LoadTable(path);
	SetVisible(true);
}

bool DataTableEditorPanel::ExportToCSV(const std::string& path) {
	try {
		std::string csv_str = engine::data::DataSerializer::ExportTableToCSV(*table_);

		std::ofstream file(path);
		if (!file.is_open()) {
			return false;
		}

		file << csv_str;
		file.close();

		return true;
	}
	catch (...) {
		return false;
	}
}

} // namespace editor
