module;

#include <functional>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.button;

import :ui_element;
import :mouse_event;
import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Button elements
 *
 * Use designated initializers for clean, readable construction:
 * @code
 * auto desc = ButtonDescriptor{
 *     .id = "save_button",
 *     .bounds = {10, 10, 120, 40},
 *     .label = "Click Me",
 *     .on_click = [](const MouseEvent&) {
 *         DoSomething();
 *         return true;
 *     }
 * };
 * auto button = UIFactory::Create(desc);
 * @endcode
 *
 * For JSON-based event binding, use `on_click_action` with ActionRegistry:
 * @code
 * // In JSON: {"type": "button", "label": "Save", "on_click_action": "save_game"}
 * // In code: ActionRegistry::RegisterClickAction("save_game", handler);
 * @endcode
 */
struct ButtonDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size)
	Bounds bounds;

	/// Button label text
	std::string label;

	/// Text styling
	TextStyle text_style;

	/// Normal state background color
	batch_renderer::Color normal_color{batch_renderer::Colors::DARK_GRAY};

	/// Hover state background color
	batch_renderer::Color hover_color{batch_renderer::Color::Brightness(batch_renderer::Colors::DARK_GRAY, 0.2f)};

	/// Pressed state background color
	batch_renderer::Color pressed_color{batch_renderer::Color::Brightness(batch_renderer::Colors::DARK_GRAY, -0.2f)};

	/// Disabled state background color
	batch_renderer::Color disabled_color{batch_renderer::Color::Alpha(batch_renderer::Colors::DARK_GRAY, 0.5f)};

	/// Border styling
	BorderStyle border;

	/// Whether button is initially enabled
	bool enabled{true};

	/// Whether button is initially visible
	bool visible{true};

	/// Click callback (optional, not serializable to JSON)
	UIElement::ClickCallback on_click;

	/// Named action for click (serializable to JSON, resolved via ActionRegistry)
	std::string on_click_action;
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const ButtonDescriptor& d) {
	j = nlohmann::json{
		{"type", "button"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"label", d.label},
		{"text_style", detail::textstyle_to_json(d.text_style)},
		{"normal_color", detail::color_to_json(d.normal_color)},
		{"hover_color", detail::color_to_json(d.hover_color)},
		{"pressed_color", detail::color_to_json(d.pressed_color)},
		{"disabled_color", detail::color_to_json(d.disabled_color)},
		{"border", detail::border_to_json(d.border)},
		{"enabled", d.enabled},
		{"visible", d.visible}
	};
	// Only include on_click_action if set
	if (!d.on_click_action.empty()) {
		j["on_click_action"] = d.on_click_action;
	}
}

inline void from_json(const nlohmann::json& j, ButtonDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.label = j.value("label", "");
	if (j.contains("text_style")) {
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	}
	if (j.contains("normal_color")) {
		d.normal_color = detail::color_from_json(j["normal_color"]);
	}
	if (j.contains("hover_color")) {
		d.hover_color = detail::color_from_json(j["hover_color"]);
	}
	if (j.contains("pressed_color")) {
		d.pressed_color = detail::color_from_json(j["pressed_color"]);
	}
	if (j.contains("disabled_color")) {
		d.disabled_color = detail::color_from_json(j["disabled_color"]);
	}
	if (j.contains("border")) {
		d.border = detail::border_from_json(j["border"]);
	}
	d.enabled = j.value("enabled", true);
	d.visible = j.value("visible", true);
	d.on_click_action = j.value("on_click_action", "");
}

} // namespace engine::ui::descriptor
