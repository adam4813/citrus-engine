module;

#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

module engine.data;

import :data_asset;
import :data_serializer;
import :data_table;
import :data_asset_registry;
import glm;

using json = nlohmann::json;

namespace engine::data {

// Helper to convert DataValue to JSON
static void data_value_to_json(json& j, const DataValue& value) {
	std::visit(
		[&j](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, int>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, float>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, glm::vec2>) {
				j = {arg.x, arg.y};
			}
			else if constexpr (std::is_same_v<T, glm::vec3>) {
				j = {arg.x, arg.y, arg.z};
			}
			else if constexpr (std::is_same_v<T, glm::vec4>) {
				j = {arg.x, arg.y, arg.z, arg.w};
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				j = arg;
			}
		},
		value);
}

// Helper to get type name from DataValue
static std::string get_value_type_name(const DataValue& value) {
	return std::visit(
		[](auto&& arg) -> std::string {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool>)
				return "bool";
			else if constexpr (std::is_same_v<T, int>)
				return "int";
			else if constexpr (std::is_same_v<T, float>)
				return "float";
			else if constexpr (std::is_same_v<T, glm::vec2>)
				return "vec2";
			else if constexpr (std::is_same_v<T, glm::vec3>)
				return "vec3";
			else if constexpr (std::is_same_v<T, glm::vec4>)
				return "vec4";
			else if constexpr (std::is_same_v<T, std::string>)
				return "string";
			else
				return "unknown";
		},
		value);
}

// Helper to convert JSON to DataValue based on type name
static DataValue json_to_data_value(const json& j, const std::string& type_name) {
	if (type_name == "bool") {
		return j.get<bool>();
	}
	else if (type_name == "int") {
		return j.get<int>();
	}
	else if (type_name == "float") {
		return j.get<float>();
	}
	else if (type_name == "vec2") {
		if (j.is_array() && j.size() >= 2) {
			return glm::vec2(j[0].get<float>(), j[1].get<float>());
		}
		return glm::vec2(0.0f);
	}
	else if (type_name == "vec3") {
		if (j.is_array() && j.size() >= 3) {
			return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
		}
		return glm::vec3(0.0f);
	}
	else if (type_name == "vec4") {
		if (j.is_array() && j.size() >= 4) {
			return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(),
							 j[3].get<float>());
		}
		return glm::vec4(0.0f);
	}
	else if (type_name == "string") {
		return j.get<std::string>();
	}
	// Default: return float 0
	return 0.0f;
}

std::string DataSerializer::SerializeAsset(const DataAsset& asset) {
	json j;
	j["id"] = asset.id;
	j["type_name"] = asset.type_name;

	// Serialize properties with type information
	json properties = json::object();
	for (const auto& [name, value] : asset.properties) {
		json prop;
		prop["type"] = get_value_type_name(value);
		data_value_to_json(prop["value"], value);
		properties[name] = prop;
	}
	j["properties"] = properties;

	return j.dump(2);
}

DataAsset DataSerializer::DeserializeAsset(const std::string& json_str) {
	json j = json::parse(json_str);

	DataAsset asset;
	asset.id = j["id"].get<std::string>();
	asset.type_name = j["type_name"].get<std::string>();

	// Deserialize properties
	if (j.contains("properties") && j["properties"].is_object()) {
		for (auto& [name, prop] : j["properties"].items()) {
			std::string type_name = prop["type"].get<std::string>();
			DataValue value = json_to_data_value(prop["value"], type_name);
			asset.properties[name] = value;
		}
	}

	return asset;
}

std::string DataSerializer::SerializeTable(const DataTable& table) {
	json j;
	j["name"] = table.GetName();

	// Serialize columns
	json columns = json::array();
	for (const auto& col : table.GetColumns()) {
		json col_json;
		col_json["name"] = col.name;
		columns.push_back(col_json);
	}
	j["columns"] = columns;

	// Serialize rows
	json rows = json::array();
	for (const auto& row : table.GetAllRows()) {
		json row_json;
		row_json["key"] = row.key;

		json values = json::object();
		for (const auto& [col_name, value] : row.values) {
			json val_json;
			val_json["type"] = get_value_type_name(value);
			data_value_to_json(val_json["value"], value);
			values[col_name] = val_json;
		}
		row_json["values"] = values;
		rows.push_back(row_json);
	}
	j["rows"] = rows;

	return j.dump(2);
}

DataTable DataSerializer::DeserializeTable(const std::string& json_str) {
	json j = json::parse(json_str);

	DataTable table;
	if (j.contains("name")) {
		table.SetName(j["name"].get<std::string>());
	}

	// Deserialize columns
	if (j.contains("columns") && j["columns"].is_array()) {
		for (const auto& col : j["columns"]) {
			ColumnDefinition col_def;
			col_def.name = col["name"].get<std::string>();
			table.AddColumn(col_def);
		}
	}

	// Deserialize rows
	if (j.contains("rows") && j["rows"].is_array()) {
		for (const auto& row_json : j["rows"]) {
			DataRow row;
			row.key = row_json["key"].get<std::string>();

			if (row_json.contains("values") && row_json["values"].is_object()) {
				for (auto& [col_name, val_json] : row_json["values"].items()) {
					std::string type_name = val_json["type"].get<std::string>();
					DataValue value = json_to_data_value(val_json["value"], type_name);
					row.values[col_name] = value;
				}
			}

			table.AddRow(std::move(row));
		}
	}

	return table;
}

std::string DataSerializer::SerializeSchema(const Schema& schema) {
	json j;
	j["name"] = schema.name;
	j["category"] = schema.category;
	j["description"] = schema.description;

	json fields = json::array();
	for (const auto& field : schema.fields) {
		json field_json;
		field_json["name"] = field.name;
		field_json["type_name"] = field.type_name;
		data_value_to_json(field_json["default_value"], field.default_value);
		fields.push_back(field_json);
	}
	j["fields"] = fields;

	return j.dump(2);
}

Schema DataSerializer::DeserializeSchema(const std::string& json_str) {
	json j = json::parse(json_str);

	Schema schema;
	schema.name = j["name"].get<std::string>();
	if (j.contains("category")) {
		schema.category = j["category"].get<std::string>();
	}
	if (j.contains("description")) {
		schema.description = j["description"].get<std::string>();
	}

	if (j.contains("fields") && j["fields"].is_array()) {
		for (const auto& field_json : j["fields"]) {
			SchemaField field;
			field.name = field_json["name"].get<std::string>();
			field.type_name = field_json["type_name"].get<std::string>();
			field.default_value =
				json_to_data_value(field_json["default_value"], field.type_name);
			schema.fields.push_back(field);
		}
	}

	return schema;
}

std::string DataSerializer::ExportTableToCSV(const DataTable& table) {
	std::ostringstream oss;

	// Header row - column names
	const auto& columns = table.GetColumns();
	oss << "key";
	for (const auto& col : columns) {
		oss << "," << col.name;
	}
	oss << "\n";

	// Data rows
	for (const auto& row : table.GetAllRows()) {
		oss << row.key;
		for (const auto& col : columns) {
			oss << ",";
			if (row.HasValue(col.name)) {
				DataValue value = row.GetValue(col.name);
				// Convert value to CSV string
				std::visit(
					[&oss](auto&& arg) {
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, bool>) {
							oss << (arg ? "true" : "false");
						}
						else if constexpr (std::is_same_v<T, int>) {
							oss << arg;
						}
						else if constexpr (std::is_same_v<T, float>) {
							oss << arg;
						}
						else if constexpr (std::is_same_v<T, glm::vec2>) {
							oss << arg.x << ";" << arg.y;
						}
						else if constexpr (std::is_same_v<T, glm::vec3>) {
							oss << arg.x << ";" << arg.y << ";" << arg.z;
						}
						else if constexpr (std::is_same_v<T, glm::vec4>) {
							oss << arg.x << ";" << arg.y << ";" << arg.z << ";" << arg.w;
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							// Escape quotes in CSV
							std::string escaped = arg;
							size_t pos = 0;
							while ((pos = escaped.find('"', pos)) != std::string::npos) {
								escaped.replace(pos, 1, "\"\"");
								pos += 2;
							}
							oss << "\"" << escaped << "\"";
						}
					},
					value);
			}
		}
		oss << "\n";
	}

	return oss.str();
}

DataTable DataSerializer::ImportTableFromCSV(const std::string& csv_str,
											  const std::string& table_name) {
	DataTable table(table_name);

	std::istringstream iss(csv_str);
	std::string line;

	// Read header row
	if (!std::getline(iss, line)) {
		return table; // Empty CSV
	}

	// Parse column names from header
	std::vector<std::string> column_names;
	std::istringstream header_stream(line);
	std::string col_name;

	// Skip "key" column
	std::getline(header_stream, col_name, ',');

	while (std::getline(header_stream, col_name, ',')) {
		// Trim whitespace
		col_name.erase(0, col_name.find_first_not_of(" \t\r\n"));
		col_name.erase(col_name.find_last_not_of(" \t\r\n") + 1);

		column_names.push_back(col_name);
		ColumnDefinition col_def;
		col_def.name = col_name;
		table.AddColumn(col_def);
	}

	// Read data rows
	while (std::getline(iss, line)) {
		if (line.empty())
			continue;

		std::istringstream row_stream(line);
		std::string cell;

		// Read key
		if (!std::getline(row_stream, cell, ','))
			continue;

		DataRow row;
		row.key = cell;

		// Read values for each column
		size_t col_index = 0;
		while (std::getline(row_stream, cell, ',') && col_index < column_names.size()) {
			// Trim whitespace
			cell.erase(0, cell.find_first_not_of(" \t\r\n"));
			cell.erase(cell.find_last_not_of(" \t\r\n") + 1);

			// Try to parse as different types
			// For simplicity, store as string by default
			// In a real implementation, you'd need type information
			if (!cell.empty()) {
				// Remove quotes if present
				if (cell.front() == '"' && cell.back() == '"' && cell.size() >= 2) {
					cell = cell.substr(1, cell.size() - 2);
					// Unescape double quotes
					size_t pos = 0;
					while ((pos = cell.find("\"\"", pos)) != std::string::npos) {
						cell.replace(pos, 2, "\"");
						pos += 1;
					}
				}
				row.SetValue(column_names[col_index], cell);
			}
			col_index++;
		}

		table.AddRow(std::move(row));
	}

	return table;
}

} // namespace engine::data
