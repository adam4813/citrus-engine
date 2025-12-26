module;

#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.progress_bar;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for ProgressBar elements
 *
 * @code
 * auto desc = ProgressBarDescriptor{
 *     .bounds = {10, 200, 200, 20},
 *     .initial_progress = 0.0f,
 *     .label = "Loading",
 *     .show_percentage = true
 * };
 * @endcode
 */
struct ProgressBarDescriptor {
	/// Bounds (position and size)
	Bounds bounds;

	/// Initial progress (0.0-1.0)
	float initial_progress{0.0f};

	/// Optional label
	std::string label;

	/// Whether to show percentage text
	bool show_percentage{false};

	/// Track color
	batch_renderer::Color track_color{batch_renderer::Colors::DARK_GRAY};

	/// Fill color
	batch_renderer::Color fill_color{batch_renderer::Colors::GOLD};

	/// Text styling
	TextStyle text_style;

	/// Border styling
	BorderStyle border;

	/// Whether progress bar is initially visible
	bool visible{true};
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const ProgressBarDescriptor& d) {
	j = nlohmann::json{
		{"type", "progress_bar"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"initial_progress", d.initial_progress},
		{"label", d.label},
		{"show_percentage", d.show_percentage},
		{"track_color", detail::color_to_json(d.track_color)},
		{"fill_color", detail::color_to_json(d.fill_color)},
		{"text_style", detail::textstyle_to_json(d.text_style)},
		{"border", detail::border_to_json(d.border)},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, ProgressBarDescriptor& d) {
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.initial_progress = j.value("initial_progress", 0.0f);
	d.label = j.value("label", "");
	d.show_percentage = j.value("show_percentage", false);
	if (j.contains("track_color")) {
		d.track_color = detail::color_from_json(j["track_color"]);
	}
	if (j.contains("fill_color")) {
		d.fill_color = detail::color_from_json(j["fill_color"]);
	}
	if (j.contains("text_style")) {
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	}
	if (j.contains("border")) {
		d.border = detail::border_from_json(j["border"]);
	}
	d.visible = j.value("visible", true);
}

} // namespace engine::ui::descriptor
