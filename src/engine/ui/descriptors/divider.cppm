module;

#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.divider;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Divider elements
 */
struct DividerDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size)
	Bounds bounds;

	/// Divider color
	batch_renderer::Color color{batch_renderer::Colors::GRAY};

	/// Whether divider is horizontal (true) or vertical (false)
	bool horizontal{true};

	/// Whether divider is initially visible
	bool visible{true};
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const DividerDescriptor& d) {
	j = nlohmann::json{
		{"type", "divider"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"color", detail::color_to_json(d.color)},
		{"horizontal", d.horizontal},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, DividerDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	if (j.contains("color")) {
		d.color = detail::color_from_json(j["color"]);
	}
	d.horizontal = j.value("horizontal", true);
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
