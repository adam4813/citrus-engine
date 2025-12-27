module;

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.slider;

import :descriptors.common;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Declarative descriptor for Slider elements
 *
 * @code
 * auto desc = SliderDescriptor{
 *     .id = "volume_slider",
 *     .bounds = {10, 100, 200, 30},
 *     .min_value = 0.0f,
 *     .max_value = 100.0f,
 *     .initial_value = 50.0f,
 *     .on_value_changed = [](float value) {
 *         SetVolume(value);
 *     }
 * };
 * @endcode
 *
 * For JSON-based event binding, use `on_change_action` with ActionRegistry:
 * @code
 * // In JSON: {"type": "slider", "on_change_action": "set_volume"}
 * // In code: ActionRegistry::RegisterFloatAction("set_volume", handler);
 * @endcode
 */
struct SliderDescriptor {
	/// Element ID for event binding (optional)
	std::string id;

	/// Bounds (position and size)
	Bounds bounds;

	/// Minimum value
	float min_value{0.0f};

	/// Maximum value
	float max_value{1.0f};

	/// Initial value
	float initial_value{0.0f};

	/// Optional label
	std::string label;

	/// Track color
	batch_renderer::Color track_color{batch_renderer::Colors::DARK_GRAY};

	/// Fill color (progress portion)
	batch_renderer::Color fill_color{batch_renderer::Colors::GOLD};

	/// Thumb color
	batch_renderer::Color thumb_color{batch_renderer::Colors::WHITE};

	/// Whether slider is initially visible
	bool visible{true};

	/// Value changed callback (not serializable to JSON)
	std::function<void(float)> on_value_changed;

	/// Named action for value change (serializable to JSON, resolved via ActionRegistry)
	std::string on_change_action;
};

// --- JSON Serialization ---

inline void to_json(nlohmann::json& j, const SliderDescriptor& d) {
	j = nlohmann::json{
		{"type", "slider"},
		{"id", d.id},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"min_value", d.min_value},
		{"max_value", d.max_value},
		{"initial_value", d.initial_value},
		{"label", d.label},
		{"track_color", detail::color_to_json(d.track_color)},
		{"fill_color", detail::color_to_json(d.fill_color)},
		{"thumb_color", detail::color_to_json(d.thumb_color)},
		{"visible", d.visible}
	};
	if (!d.on_change_action.empty()) {
		j["on_change_action"] = d.on_change_action;
	}
}

inline void from_json(const nlohmann::json& j, SliderDescriptor& d) {
	d.id = j.value("id", "");
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	d.min_value = j.value("min_value", 0.0f);
	d.max_value = j.value("max_value", 1.0f);
	d.initial_value = j.value("initial_value", 0.0f);
	d.label = j.value("label", "");
	if (j.contains("track_color")) {
		d.track_color = detail::color_from_json(j["track_color"]);
	}
	if (j.contains("fill_color")) {
		d.fill_color = detail::color_from_json(j["fill_color"]);
	}
	if (j.contains("thumb_color")) {
		d.thumb_color = detail::color_from_json(j["thumb_color"]);
	}
	d.visible = j.value("visible", true);
	d.on_change_action = j.value("on_change_action", "");
}

} // namespace engine::ui::descriptor
