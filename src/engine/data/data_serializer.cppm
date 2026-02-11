module;

#include <cstdint>
#include <string>

export module engine.data:data_serializer;

import :data_asset;
import :data_table;
import :data_asset_registry;

export namespace engine::data {

/// Serializer for data assets and data tables
/// Handles conversion to/from JSON format
class DataSerializer {
public:
	/// Serialize a DataAsset to JSON string
	static std::string SerializeAsset(const DataAsset& asset);

	/// Deserialize a DataAsset from JSON string
	static DataAsset DeserializeAsset(const std::string& json_str);

	/// Serialize a DataTable to JSON string
	static std::string SerializeTable(const DataTable& table);

	/// Deserialize a DataTable from JSON string
	static DataTable DeserializeTable(const std::string& json_str);

	/// Serialize a Schema to JSON string
	static std::string SerializeSchema(const Schema& schema);

	/// Deserialize a Schema from JSON string
	static Schema DeserializeSchema(const std::string& json_str);

	/// Export DataTable to CSV format
	static std::string ExportTableToCSV(const DataTable& table);

	/// Import DataTable from CSV format
	/// Returns a DataTable with column names from first row
	static DataTable ImportTableFromCSV(const std::string& csv_str, const std::string& table_name);
};

} // namespace engine::data
