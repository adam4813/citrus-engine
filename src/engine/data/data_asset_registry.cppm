module;

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module engine.data:data_asset_registry;

import :data_asset;

export namespace engine::data {

/// Field definition in a schema
struct SchemaField {
	std::string name;
	std::string type_name;  // "bool", "int", "float", "vec2", "vec3", "vec4", "string"
	DataValue default_value;

	SchemaField() = default;

	SchemaField(std::string field_name, std::string field_type, DataValue field_default = 0.0f)
		: name(std::move(field_name)), type_name(std::move(field_type)),
		  default_value(std::move(field_default)) {}
};

/// Schema definition - defines structure for data assets
struct Schema {
	std::string name;
	std::vector<SchemaField> fields;
	std::string category;     // Optional: for organization
	std::string description;  // Optional: documentation

	Schema() = default;

	explicit Schema(std::string schema_name) : name(std::move(schema_name)) {}

	/// Add a field to the schema
	void AddField(SchemaField field) { fields.push_back(std::move(field)); }

	/// Get a field by name
	[[nodiscard]] std::optional<SchemaField> GetField(const std::string& field_name) const {
		for (const auto& field : fields) {
			if (field.name == field_name) {
				return field;
			}
		}
		return std::nullopt;
	}

	/// Check if schema has a field
	[[nodiscard]] bool HasField(const std::string& field_name) const {
		for (const auto& field : fields) {
			if (field.name == field_name) {
				return true;
			}
		}
		return false;
	}

	/// Remove a field from schema
	bool RemoveField(const std::string& field_name) {
		for (auto it = fields.begin(); it != fields.end(); ++it) {
			if (it->name == field_name) {
				fields.erase(it);
				return true;
			}
		}
		return false;
	}
};

/// Global registry for data asset schemas
/// Singleton pattern similar to NodeTypeRegistry
class DataAssetRegistry {
public:
	/// Get the singleton instance
	static DataAssetRegistry& Instance() {
		static DataAssetRegistry instance;
		return instance;
	}

	// Disable copy/move
	DataAssetRegistry(const DataAssetRegistry&) = delete;
	DataAssetRegistry& operator=(const DataAssetRegistry&) = delete;
	DataAssetRegistry(DataAssetRegistry&&) = delete;
	DataAssetRegistry& operator=(DataAssetRegistry&&) = delete;

	/// Register a new schema
	void RegisterSchema(Schema schema) { schemas_[schema.name] = std::move(schema); }

	/// Get a schema by name
	[[nodiscard]] std::optional<Schema> GetSchema(const std::string& name) const {
		auto it = schemas_.find(name);
		if (it != schemas_.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	/// Check if a schema exists
	[[nodiscard]] bool HasSchema(const std::string& name) const {
		return schemas_.find(name) != schemas_.end();
	}

	/// Unregister a schema
	bool UnregisterSchema(const std::string& name) {
		auto it = schemas_.find(name);
		if (it != schemas_.end()) {
			schemas_.erase(it);
			return true;
		}
		return false;
	}

	/// Get all registered schema names
	[[nodiscard]] std::vector<std::string> GetAllSchemaNames() const {
		std::vector<std::string> names;
		names.reserve(schemas_.size());
		for (const auto& [name, schema] : schemas_) {
			names.push_back(name);
		}
		return names;
	}

	/// Get all registered schemas
	[[nodiscard]] const std::map<std::string, Schema>& GetAllSchemas() const { return schemas_; }

	/// Create a data asset instance from a schema
	[[nodiscard]] std::optional<DataAsset> CreateAssetFromSchema(const std::string& schema_name,
																 const std::string& asset_id) const {
		auto schema_opt = GetSchema(schema_name);
		if (!schema_opt) {
			return std::nullopt;
		}

		const auto& schema = *schema_opt;
		DataAsset asset(asset_id, schema_name);

		// Initialize all fields with default values
		for (const auto& field : schema.fields) {
			asset.SetProperty(field.name, field.default_value);
		}

		return asset;
	}

	/// Clear all registered schemas
	void Clear() { schemas_.clear(); }

private:
	DataAssetRegistry() = default;
	~DataAssetRegistry() = default;

	std::map<std::string, Schema> schemas_;
};

} // namespace engine::data
