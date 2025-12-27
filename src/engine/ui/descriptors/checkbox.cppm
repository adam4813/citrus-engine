module;

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.checkbox;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Checkbox elements
 *
 * @code
 * auto desc = CheckboxDescriptor{
 *     .id = "fullscreen_checkbox",
 *     .bounds = {10, 50, 150, 24},
 *     .label = "Enable Feature",
 *     .initial_checked = true,
 *     .on_toggled = [](bool checked) {
 *         SetFeatureEnabled(checked);
 *     }
 * };
 * @endcode
 */
struct CheckboxDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size)
	Bounds bounds;

	/// Checkbox label text
	std::string label;

	/// Initial checked state
	bool initial_checked{false};

	/// Unchecked background color
	batch_renderer::Color unchecked_color{batch_renderer::Colors::DARK_GRAY};

	/// Checked background color
	batch_renderer::Color checked_color{batch_renderer::Colors::GOLD};

	/// Text styling
	TextStyle text_style;

	/// Whether checkbox is initially enabled
	bool enabled{true};

	/// Whether checkbox is initially visible
	bool visible{true};

	/// Toggle callback (not serializable to JSON)
	std::function<void(bool)> on_toggled;
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const CheckboxDescriptor& d) {
	j = nlohmann::json{
		{"type", "checkbox"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"label", d.label},
		{"initial_checked", d.initial_checked},
		{"unchecked_color", detail::color_to_json(d.unchecked_color)},
		{"checked_color", detail::color_to_json(d.checked_color)},
		{"text_style", detail::textstyle_to_json(d.text_style)},
		{"enabled", d.enabled},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, CheckboxDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.label = j.value("label", "");
	d.initial_checked = j.value("initial_checked", false);
	if (j.contains("unchecked_color")) {
		d.unchecked_color = detail::color_from_json(j["unchecked_color"]);
	}
	if (j.contains("checked_color")) {
		d.checked_color = detail::color_from_json(j["checked_color"]);
	}
	if (j.contains("text_style")) {
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	}
	d.enabled = j.value("enabled", true);
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
