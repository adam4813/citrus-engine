module;

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

export module engine.data:data_table;

import :data_asset;

export namespace engine::data {

/// Data table row - represents one row in a data table
struct DataRow {
	std::string key; // Unique row identifier
	std::map<std::string, DataValue> values; // Column values

	DataRow() = default;

	explicit DataRow(std::string row_key) : key(std::move(row_key)) {}

	/// Get a column value by name
	[[nodiscard]] DataValue GetValue(const std::string& column_name) const {
		auto it = values.find(column_name);
		if (it != values.end()) {
			return it->second;
		}
		return 0.0f; // Default value
	}

	/// Set a column value
	void SetValue(const std::string& column_name, DataValue value) { values[column_name] = std::move(value); }

	/// Check if a column exists
	[[nodiscard]] bool HasValue(const std::string& column_name) const {
		return values.find(column_name) != values.end();
	}
};

/// Column definition for a data table
struct ColumnDefinition {
	std::string name;
	// Note: Type information can be stored as int/string for now
	// In the future, this could be extended with a ColumnType enum
};

/// Data table - tabular data structure with typed columns
class DataTable {
public:
	DataTable() = default;

	explicit DataTable(std::string table_name) : name_(std::move(table_name)) {}

	/// Get table name
	[[nodiscard]] const std::string& GetName() const { return name_; }

	/// Set table name
	void SetName(std::string table_name) { name_ = std::move(table_name); }

	/// Add a column definition
	void AddColumn(ColumnDefinition column) { columns_.push_back(std::move(column)); }

	/// Get all column definitions
	[[nodiscard]] const std::vector<ColumnDefinition>& GetColumns() const { return columns_; }

	/// Add a row to the table
	void AddRow(DataRow row) { rows_.push_back(std::move(row)); }

	/// Get a row by key
	[[nodiscard]] std::optional<DataRow> GetRow(const std::string& key) const {
		auto it = std::find_if(rows_.begin(), rows_.end(), [&key](const DataRow& row) { return row.key == key; });
		if (it != rows_.end()) {
			return *it;
		}
		return std::nullopt;
	}

	/// Get a row by index
	[[nodiscard]] std::optional<DataRow> GetRowByIndex(size_t index) const {
		if (index < rows_.size()) {
			return rows_[index];
		}
		return std::nullopt;
	}

	/// Get all rows
	[[nodiscard]] const std::vector<DataRow>& GetAllRows() const { return rows_; }

	/// Get mutable reference to all rows
	[[nodiscard]] std::vector<DataRow>& GetAllRowsMutable() { return rows_; }

	/// Find rows by column value
	[[nodiscard]] std::vector<DataRow> FindByColumn(const std::string& column_name, const DataValue& value) const {
		std::vector<DataRow> results;
		for (const auto& row : rows_) {
			if (row.HasValue(column_name)) {
				auto row_value = row.GetValue(column_name);
				// Simple equality check - compares variant values
				if (row_value.index() == value.index()) {
					// Both variants hold the same type, compare values
					bool is_equal = std::visit(
							[&value](auto&& arg) {
								using T = std::decay_t<decltype(arg)>;
								if (const auto* val_ptr = std::get_if<T>(&value)) {
									return arg == *val_ptr;
								}
								return false;
							},
							row_value);
					if (is_equal) {
						results.push_back(row);
					}
				}
			}
		}
		return results;
	}

	/// Remove a row by key
	bool RemoveRow(const std::string& key) {
		auto it = std::find_if(rows_.begin(), rows_.end(), [&key](const DataRow& row) { return row.key == key; });
		if (it != rows_.end()) {
			rows_.erase(it);
			return true;
		}
		return false;
	}

	/// Remove a row by index
	bool RemoveRowByIndex(size_t index) {
		if (index < rows_.size()) {
			rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(index));
			return true;
		}
		return false;
	}

	/// Clear all rows
	void ClearRows() { rows_.clear(); }

	/// Get row count
	[[nodiscard]] size_t GetRowCount() const { return rows_.size(); }

	/// Check if table is empty
	[[nodiscard]] bool IsEmpty() const { return rows_.empty(); }

private:
	std::string name_;
	std::vector<ColumnDefinition> columns_;
	std::vector<DataRow> rows_;
};

} // namespace engine::data
