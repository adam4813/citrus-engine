module;

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

export module engine.ui:factory_registry;

import :ui_element;
import :descriptor;

export namespace engine::ui {

/**
 * @brief Registry for UI element factory functions
 *
 * Provides a registration-based approach to creating UI elements from
 * descriptors and JSON. This enables:
 * - Adding new element types without modifying factory code
 * - Plugin-style extensibility
 * - Decoupled descriptor-to-element mapping
 *
 * **Usage:**
 * @code
 * // Register a factory for a descriptor type
 * UIFactoryRegistry::Register<ButtonDescriptor>(
 *     "button",
 *     [](const nlohmann::json& j) { return CreateButtonFromJson(j); }
 * );
 *
 * // Create from JSON using the type field
 * auto element = UIFactoryRegistry::CreateFromJson(json_obj);
 * @endcode
 *
 * **Auto-registration pattern:**
 * Define factory registration alongside descriptor to auto-register:
 * @code
 * // In button_descriptor.cppm or similar
 * namespace {
 *     const bool registered = UIFactoryRegistry::Register<ButtonDescriptor>(
 *         "button", CreateButton);
 * }
 * @endcode
 */
class UIFactoryRegistry {
public:
	/// Factory function type that creates UIElement from JSON
	using JsonCreator = std::function<std::unique_ptr<UIElement>(const nlohmann::json&)>;

	/// Factory function type that creates UIElement from descriptor variant
	using DescriptorCreator = std::function<std::unique_ptr<UIElement>(const descriptor::CompleteUIDescriptor&)>;

	/**
	 * @brief Register a factory for a type name
	 *
	 * @param type_name The "type" field value in JSON (e.g., "button")
	 * @param creator Function to create element from JSON
	 * @return true (for static initialization)
	 */
	static bool RegisterJsonCreator(const std::string& type_name, JsonCreator creator) {
		GetJsonRegistry()[type_name] = std::move(creator);
		return true;
	}

	/**
	 * @brief Check if a type is registered
	 *
	 * @param type_name The type name to check
	 * @return true if registered
	 */
	static bool IsRegistered(const std::string& type_name) {
		const auto& registry = GetJsonRegistry();
		return registry.find(type_name) != registry.end();
	}

	/**
	 * @brief Create a UIElement from JSON using the "type" field
	 *
	 * @param j JSON object with "type" field
	 * @return Created element, or nullptr if type not registered
	 */
	static std::unique_ptr<UIElement> CreateFromJson(const nlohmann::json& j) {
		const std::string type = j.value("type", "");
		if (type.empty()) {
			return nullptr;
		}

		const auto& registry = GetJsonRegistry();
		const auto it = registry.find(type);
		if (it == registry.end()) {
			return nullptr;
		}

		return it->second(j);
	}

	/**
	 * @brief Get list of registered type names
	 *
	 * @return Vector of registered type names
	 */
	static std::vector<std::string> GetRegisteredTypes() {
		std::vector<std::string> types;
		for (const auto& [name, _] : GetJsonRegistry()) {
			types.push_back(name);
		}
		return types;
	}

private:
	static std::unordered_map<std::string, JsonCreator>& GetJsonRegistry() {
		static std::unordered_map<std::string, JsonCreator> registry;
		return registry;
	}
};

} // namespace engine::ui
