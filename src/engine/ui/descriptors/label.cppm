module;

#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.label;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Label elements
 *
 * @code
 * auto desc = LabelDescriptor{
 *     .id = "title_label",
 *     .bounds = {10, 10, 0, 0},  // Size auto-calculated from text
 *     .text = "Hello World",
 *     .style = {.font_size = 18.0f, .color = Colors::GOLD}
 * };
 * @endcode
 */
struct LabelDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size, size may be auto-calculated)
	Bounds bounds;

	/// Label text
	std::string text;

	/// Text styling
	TextStyle style;

	/// Whether label is initially visible
	bool visible{true};
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const LabelDescriptor& d) {
	j = nlohmann::json{
		{"type", "label"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"text", d.text},
		{"style", detail::textstyle_to_json(d.style)},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, LabelDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.text = j.value("text", "");
	if (j.contains("style")) {
		d.style = detail::textstyle_from_json(j["style"]);
	}
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
