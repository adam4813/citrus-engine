module;

#include <cstdint>
#include <map>
#include <string>
#include <variant>

export module engine.data:data_asset;

import glm;

export namespace engine::data {

/// Data value type - supports common data types for assets
using DataValue = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, std::string>;

/// Base data asset structure
/// Represents a single instance of data with type-tagged properties
struct DataAsset {
	std::string id;                                // Unique identifier
	std::string type_name;                         // Schema type name
	std::map<std::string, DataValue> properties;   // Property values

	DataAsset() = default;

	DataAsset(std::string asset_id, std::string asset_type)
		: id(std::move(asset_id)), type_name(std::move(asset_type)) {}

	/// Get a property value by name
	/// Returns a default-constructed DataValue if not found
	[[nodiscard]] DataValue GetProperty(const std::string& name) const {
		auto it = properties.find(name);
		if (it != properties.end()) {
			return it->second;
		}
		return 0.0f; // Default value
	}

	/// Set a property value
	void SetProperty(const std::string& name, DataValue value) { properties[name] = std::move(value); }

	/// Check if a property exists
	[[nodiscard]] bool HasProperty(const std::string& name) const {
		return properties.find(name) != properties.end();
	}

	/// Remove a property
	void RemoveProperty(const std::string& name) { properties.erase(name); }

	/// Clear all properties
	void ClearProperties() { properties.clear(); }
};

} // namespace engine::data
