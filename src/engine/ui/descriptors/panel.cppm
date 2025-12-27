module;

#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.panel;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Panel elements
 *
 * @code
 * auto desc = PanelDescriptor{
 *     .id = "settings_panel",
 *     .bounds = {0, 0, 400, 300},
 *     .background = Colors::DARK_GRAY,
 *     .padding = 10.0f,
 *     .border = {.width = 2.0f, .color = Colors::GOLD}
 * };
 * auto panel = UIFactory::Create(desc);
 * @endcode
 */
struct PanelDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size)
	Bounds bounds;

	/// Background color
	batch_renderer::Color background{batch_renderer::Colors::DARK_GRAY};

	/// Border styling
	BorderStyle border;

	/// Inner padding
	float padding{0.0f};

	/// Opacity (0.0-1.0)
	float opacity{1.0f};

	/// Whether to clip children to bounds
	bool clip_children{false};

	/// Whether panel is initially visible
	bool visible{true};
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const PanelDescriptor& d) {
	j = nlohmann::json{
		{"type", "panel"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"background", detail::color_to_json(d.background)},
		{"border", detail::border_to_json(d.border)},
		{"padding", d.padding},
		{"opacity", d.opacity},
		{"clip_children", d.clip_children},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, PanelDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	if (j.contains("background")) {
		d.background = detail::color_from_json(j["background"]);
	}
	if (j.contains("border")) {
		d.border = detail::border_from_json(j["border"]);
	}
	d.padding = j.value("padding", 0.0f);
	d.opacity = j.value("opacity", 1.0f);
	d.clip_children = j.value("clip_children", false);
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
