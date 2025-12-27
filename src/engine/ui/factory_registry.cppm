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
import :factory;

export namespace engine::ui {

/**
 * @brief Registry for UI element factory functions
 *
 * Provides a registration-based approach to creating UI elements from
 * JSON. This enables:
 * - Adding new element types without modifying existing code
 * - Plugin-style extensibility
 * - Decoupled JSON-to-element mapping
 *
 * The registry delegates to UIFactory::Create() methods by default,
 * avoiding code duplication. Custom types can register their own creators.
 *
 * ## Built-in Types
 *
 * The following types are registered by default when Initialize() is called:
 * - "button" - ButtonDescriptor -> elements::Button
 * - "panel" - PanelDescriptor -> elements::Panel
 * - "label" - LabelDescriptor -> elements::Label
 * - "slider" - SliderDescriptor -> elements::Slider
 * - "checkbox" - CheckboxDescriptor -> elements::Checkbox
 * - "divider" - DividerDescriptor -> elements::Divider
 * - "progress_bar" - ProgressBarDescriptor -> elements::ProgressBar
 * - "image" - ImageDescriptor -> elements::Image
 * - "container" - ContainerDescriptor -> elements::Container
 *
 * ## Usage
 *
 * @code
 * // Initialize built-in factories (call once at startup)
 * UIFactoryRegistry::Initialize();
 *
 * // Create from JSON using the type field
 * auto element = UIFactoryRegistry::CreateFromJson(json_obj);
 *
 * // Register custom type
 * UIFactoryRegistry::RegisterJsonCreator("my_widget", [](const nlohmann::json& j) {
 *     auto desc = j.get<MyWidgetDescriptor>();
 *     return UIFactory::Create(desc);  // Assuming UIFactory::Create overload exists
 * });
 * @endcode
 *
 * ## Adding New Element Types
 *
 * 1. Create descriptor in `descriptors/your_element.cppm`
 * 2. Add `to_json`/`from_json` functions
 * 3. Add `UIFactory::Create()` overload in `factory.cppm`
 * 4. Add to `CompleteUIDescriptor` variant in `container.cppm`
 * 5. Register factory in `RegisterBuiltinFactories()` or call `RegisterJsonCreator()`
 */
class UIFactoryRegistry {
public:
	/// Factory function type that creates UIElement from JSON
	using JsonCreator = std::function<std::unique_ptr<UIElement>(const nlohmann::json&)>;

	/**
	 * @brief Initialize the registry with built-in factories
	 *
	 * Call this once at application startup to register all built-in
	 * element types. Safe to call multiple times (idempotent).
	 */
	static void Initialize() {
		static bool initialized = false;
		if (initialized) {
			return;
		}
		initialized = true;

		RegisterBuiltinFactories();
	}

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
		// Auto-initialize if not already done
		Initialize();

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

	/**
	 * @brief Register all built-in element factories
	 *
	 * Each registration delegates to UIFactory::Create() to avoid code duplication.
	 */
	static void RegisterBuiltinFactories() {
		// Button - delegates to UIFactory::Create(ButtonDescriptor)
		RegisterJsonCreator("button", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::ButtonDescriptor>());
		});

		// Panel - delegates to UIFactory::Create(PanelDescriptor)
		RegisterJsonCreator("panel", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::PanelDescriptor>());
		});

		// Label - delegates to UIFactory::Create(LabelDescriptor)
		RegisterJsonCreator("label", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::LabelDescriptor>());
		});

		// Slider - delegates to UIFactory::Create(SliderDescriptor)
		RegisterJsonCreator("slider", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::SliderDescriptor>());
		});

		// Checkbox - delegates to UIFactory::Create(CheckboxDescriptor)
		RegisterJsonCreator("checkbox", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::CheckboxDescriptor>());
		});

		// Divider - delegates to UIFactory::Create(DividerDescriptor)
		RegisterJsonCreator("divider", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::DividerDescriptor>());
		});

		// ProgressBar - delegates to UIFactory::Create(ProgressBarDescriptor)
		RegisterJsonCreator("progress_bar", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::ProgressBarDescriptor>());
		});

		// Image - delegates to UIFactory::Create(ImageDescriptor)
		RegisterJsonCreator("image", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			return UIFactory::Create(j.get<descriptor::ImageDescriptor>());
		});

		// Container - needs special handling for recursive children from JSON
		RegisterJsonCreator("container", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			// Create container from descriptor (without children, since JSON children
			// are not in descriptor format but in JSON format)
			auto desc = j.get<descriptor::ContainerDescriptor>();
			// Clear any parsed children since we'll create them from JSON directly
			desc.children.clear();
			auto container = UIFactory::Create(desc);

			// Recursively create children from JSON
			if (j.contains("children") && j["children"].is_array()) {
				for (const auto& child_json : j["children"]) {
					auto child = CreateFromJson(child_json);
					if (child) {
						container->AddChild(std::move(child));
					}
				}
			}

			return container;
		});
	}
};

} // namespace engine::ui
