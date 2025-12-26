module;

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:json_serializer;

import :descriptor;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief JSON serialization utilities for UI descriptors
 *
 * This module provides convenience functions for serializing UI descriptors
 * to and from JSON. It leverages nlohmann's ADL (Argument Dependent Lookup)
 * pattern - the actual to_json/from_json functions are defined in the
 * descriptor module alongside each descriptor type.
 *
 * **Usage with nlohmann's ADL pattern (preferred):**
 * @code
 * using namespace engine::ui::descriptor;
 *
 * // Serialize using nlohmann directly
 * ButtonDescriptor button{.label = "OK"};
 * nlohmann::json j = button;  // Calls to_json automatically
 *
 * // Deserialize using nlohmann directly
 * auto restored = j.get<ButtonDescriptor>();  // Calls from_json
 * @endcode
 *
 * **Using the convenience class:**
 * @code
 * std::string json_str = UIJsonSerializer::ToJsonString(button);
 * auto desc = UIJsonSerializer::FromJsonString<ButtonDescriptor>(json_str);
 * @endcode
 *
 * **Auto-detection from "type" field:**
 * @code
 * std::string json = R"({"type": "button", "label": "OK"})";
 * auto variant = UIJsonSerializer::FromJsonAuto(json);
 * // variant is CompleteUIDescriptor containing ButtonDescriptor
 * @endcode
 */
class UIJsonSerializer {
public:
	/**
	 * @brief Serialize any descriptor to JSON string
	 *
	 * Uses nlohmann's ADL to_json automatically.
	 *
	 * @tparam T Descriptor type
	 * @param desc Descriptor to serialize
	 * @param indent Indentation for pretty printing (-1 for compact)
	 * @return JSON string
	 */
	template <typename T>
	static std::string ToJsonString(const T& desc, const int indent = 2) {
		nlohmann::json j = desc;  // ADL to_json called here
		return j.dump(indent);
	}

	/**
	 * @brief Deserialize a descriptor from JSON string
	 *
	 * @tparam T Descriptor type to deserialize to
	 * @param json_str JSON string
	 * @return Deserialized descriptor
	 */
	template <typename T>
	static T FromJsonString(const std::string& json_str) {
		return nlohmann::json::parse(json_str).get<T>();  // ADL from_json called
	}

	/**
	 * @brief Deserialize any descriptor type from JSON string (uses "type" field)
	 *
	 * Automatically detects the descriptor type from the "type" field.
	 *
	 * @param json_str JSON string with "type" field
	 * @return Variant containing the appropriate descriptor
	 */
	static CompleteUIDescriptor FromJsonAuto(const std::string& json_str) {
		return nlohmann::json::parse(json_str).get<CompleteUIDescriptor>();
	}

	/**
	 * @brief Deserialize any descriptor type from JSON object (uses "type" field)
	 *
	 * @param j JSON object with "type" field
	 * @return Variant containing the appropriate descriptor
	 */
	static CompleteUIDescriptor FromJsonAuto(const nlohmann::json& j) {
		return j.get<CompleteUIDescriptor>();
	}
};

} // namespace engine::ui::descriptor
