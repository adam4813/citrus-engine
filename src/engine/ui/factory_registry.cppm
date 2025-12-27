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
import :elements.button;
import :elements.panel;
import :elements.container;
import :elements.label;
import :elements.slider;
import :elements.checkbox;
import :elements.divider;
import :elements.progress_bar;
import :elements.image;
import engine.ui.batch_renderer;
import engine.rendering;

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
 *     return CreateMyWidget(j);
 * });
 * @endcode
 *
 * ## Adding New Element Types
 *
 * 1. Create descriptor in `descriptors/your_element.cppm`
 * 2. Add `to_json`/`from_json` functions
 * 3. Add to `CompleteUIDescriptor` variant in `container.cppm`
 * 4. Register factory in `RegisterBuiltinFactories()` or call `RegisterJsonCreator()`
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
	 */
	static void RegisterBuiltinFactories() {
		// Button
		RegisterJsonCreator("button", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::ButtonDescriptor>();
			auto button = std::make_unique<elements::Button>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height,
				desc.label, desc.text_style.font_size);
			button->SetNormalColor(desc.normal_color);
			button->SetHoverColor(desc.hover_color);
			button->SetPressedColor(desc.pressed_color);
			button->SetDisabledColor(desc.disabled_color);
			button->SetTextColor(desc.text_style.color);
			button->SetBorderWidth(desc.border.width);
			button->SetBorderColor(desc.border.color);
			button->SetEnabled(desc.enabled);
			button->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				button->SetId(desc.id);
			}
			return button;
		});

		// Panel
		RegisterJsonCreator("panel", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::PanelDescriptor>();
			auto panel = std::make_unique<elements::Panel>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height);
			panel->SetBackgroundColor(desc.background);
			panel->SetBorderWidth(desc.border.width);
			panel->SetBorderColor(desc.border.color);
			panel->SetPadding(desc.padding);
			panel->SetOpacity(desc.opacity);
			panel->SetClipChildren(desc.clip_children);
			panel->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				panel->SetId(desc.id);
			}
			return panel;
		});

		// Label
		RegisterJsonCreator("label", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::LabelDescriptor>();
			auto label = std::make_unique<elements::Label>(
				desc.bounds.x, desc.bounds.y, desc.text, desc.style.font_size);
			label->SetColor(desc.style.color);
			label->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				label->SetId(desc.id);
			}
			return label;
		});

		// Slider
		RegisterJsonCreator("slider", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::SliderDescriptor>();
			auto slider = std::make_unique<elements::Slider>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height,
				desc.min_value, desc.max_value);
			slider->SetValue(desc.initial_value);
			slider->SetTrackColor(desc.track_color);
			slider->SetFillColor(desc.fill_color);
			slider->SetThumbColor(desc.thumb_color);
			if (!desc.label.empty()) {
				slider->SetLabel(desc.label);
			}
			slider->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				slider->SetId(desc.id);
			}
			return slider;
		});

		// Checkbox
		RegisterJsonCreator("checkbox", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::CheckboxDescriptor>();
			auto checkbox = std::make_unique<elements::Checkbox>(
				desc.bounds.x, desc.bounds.y, desc.label,
				desc.text_style.font_size, desc.initial_checked);
			checkbox->SetBoxColor(desc.unchecked_color);
			checkbox->SetCheckmarkColor(desc.checked_color);
			checkbox->SetLabelColor(desc.text_style.color);
			checkbox->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				checkbox->SetId(desc.id);
			}
			return checkbox;
		});

		// Divider
		RegisterJsonCreator("divider", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::DividerDescriptor>();
			const float thickness = desc.horizontal ? desc.bounds.height : desc.bounds.width;
			const elements::Orientation orientation =
				desc.horizontal ? elements::Orientation::Horizontal : elements::Orientation::Vertical;
			auto divider = std::make_unique<elements::Divider>(
				orientation, thickness > 0.0f ? thickness : 2.0f);
			divider->SetRelativePosition(desc.bounds.x, desc.bounds.y);
			if (desc.horizontal) {
				divider->SetWidth(desc.bounds.width);
			} else {
				divider->SetHeight(desc.bounds.height);
			}
			divider->SetColor(desc.color);
			divider->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				divider->SetId(desc.id);
			}
			return divider;
		});

		// ProgressBar
		RegisterJsonCreator("progress_bar", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::ProgressBarDescriptor>();
			auto progress_bar = std::make_unique<elements::ProgressBar>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height,
				desc.initial_progress);
			progress_bar->SetTrackColor(desc.track_color);
			progress_bar->SetFillColor(desc.fill_color);
			progress_bar->SetBorderWidth(desc.border.width);
			if (!desc.label.empty()) {
				progress_bar->SetLabel(desc.label);
			}
			progress_bar->SetShowPercentage(desc.show_percentage);
			progress_bar->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				progress_bar->SetId(desc.id);
			}
			return progress_bar;
		});

		// Image
		RegisterJsonCreator("image", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::ImageDescriptor>();
			auto image = std::make_unique<elements::Image>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height);
			if (desc.texture_id != 0) {
				auto sprite = std::make_shared<rendering::Sprite>();
				sprite->texture = desc.texture_id;
				sprite->color = {desc.tint.r, desc.tint.g, desc.tint.b, desc.tint.a};
				if (desc.uv_coords) {
					sprite->texture_offset = {desc.uv_coords->u0, desc.uv_coords->v0};
					sprite->texture_scale = {
						desc.uv_coords->u1 - desc.uv_coords->u0,
						desc.uv_coords->v1 - desc.uv_coords->v0};
				}
				image->SetSprite(sprite);
			}
			image->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				image->SetId(desc.id);
			}
			return image;
		});

		// Container (with recursive children)
		RegisterJsonCreator("container", [](const nlohmann::json& j) -> std::unique_ptr<UIElement> {
			auto desc = j.get<descriptor::ContainerDescriptor>();
			auto container = std::make_unique<elements::Container>(
				desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height);
			container->SetBackgroundColor(desc.background);
			container->SetBorderWidth(desc.border.width);
			container->SetBorderColor(desc.border.color);
			container->SetPadding(desc.padding);
			container->SetOpacity(desc.opacity);
			container->SetClipChildren(desc.clip_children);
			container->SetVisible(desc.visible);
			if (!desc.id.empty()) {
				container->SetId(desc.id);
			}

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
