module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

export module engine.ui:descriptor;

import :ui_element;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

// Forward declarations
struct ButtonDescriptor;
struct PanelDescriptor;
struct LabelDescriptor;
struct SliderDescriptor;
struct CheckboxDescriptor;
struct ContainerDescriptor;
struct DividerDescriptor;
struct ProgressBarDescriptor;
struct ImageDescriptor;

// ============================================================================
// Common Types
// ============================================================================

/**
 * @brief Position descriptor for UI elements
 *
 * Supports both absolute pixel positions and relative positions.
 * When used with layouts, position may be ignored as the layout
 * determines placement.
 */
struct Position {
	float x{0.0f};
	float y{0.0f};
};

/**
 * @brief Size descriptor for UI elements
 */
struct Size {
	float width{0.0f};
	float height{0.0f};
};

/**
 * @brief Bounds descriptor combining position and size
 */
struct Bounds {
	float x{0.0f};
	float y{0.0f};
	float width{100.0f};
	float height{100.0f};

	/**
	 * @brief Create bounds from position and size
	 */
	static Bounds From(const Position& pos, const Size& size) {
		return {pos.x, pos.y, size.width, size.height};
	}

	/**
	 * @brief Create bounds with just size (position = 0,0)
	 */
	static Bounds WithSize(const float w, const float h) { return {0.0f, 0.0f, w, h}; }
};

/**
 * @brief Border styling descriptor
 */
struct BorderStyle {
	float width{0.0f};
	batch_renderer::Color color{batch_renderer::Colors::LIGHT_GRAY};
};

/**
 * @brief Text styling descriptor
 */
struct TextStyle {
	float font_size{16.0f};
	batch_renderer::Color color{batch_renderer::Colors::WHITE};
};

/**
 * @brief UV coordinates for texture sampling
 */
struct UVCoords {
	float u0{0.0f};  // Left
	float v0{0.0f};  // Top
	float u1{1.0f};  // Right
	float v1{1.0f};  // Bottom
};

// ============================================================================
// Button Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Button elements
 *
 * Use designated initializers for clean, readable construction:
 * @code
 * auto desc = ButtonDescriptor{
 *     .bounds = {10, 10, 120, 40},
 *     .label = "Click Me",
 *     .on_click = [](const MouseEvent&) {
 *         DoSomething();
 *         return true;
 *     }
 * };
 * auto button = UIFactory::Create(desc);
 * @endcode
 */
struct ButtonDescriptor {
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

	/// Click callback (optional)
	UIElement::ClickCallback on_click;
};

// ============================================================================
// Panel Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Panel elements
 *
 * Use designated initializers for clean, readable construction:
 * @code
 * auto desc = PanelDescriptor{
 *     .bounds = {0, 0, 400, 300},
 *     .background = Colors::DARK_GRAY,
 *     .padding = 10.0f,
 *     .border = {.width = 2.0f, .color = Colors::GOLD}
 * };
 * auto panel = UIFactory::Create(desc);
 * @endcode
 */
struct PanelDescriptor {
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

// ============================================================================
// Label Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Label elements
 *
 * @code
 * auto desc = LabelDescriptor{
 *     .bounds = {10, 10, 0, 0},  // Size auto-calculated from text
 *     .text = "Hello World",
 *     .style = {.font_size = 18.0f, .color = Colors::GOLD}
 * };
 * @endcode
 */
struct LabelDescriptor {
	/// Bounds (position and size, size may be auto-calculated)
	Bounds bounds;

	/// Label text
	std::string text;

	/// Text styling
	TextStyle style;

	/// Whether label is initially visible
	bool visible{true};
};

// ============================================================================
// Slider Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Slider elements
 *
 * @code
 * auto desc = SliderDescriptor{
 *     .bounds = {10, 100, 200, 30},
 *     .min_value = 0.0f,
 *     .max_value = 100.0f,
 *     .initial_value = 50.0f,
 *     .on_value_changed = [](float value) {
 *         SetVolume(value);
 *     }
 * };
 * @endcode
 */
struct SliderDescriptor {
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

	/// Value changed callback
	std::function<void(float)> on_value_changed;
};

// ============================================================================
// Checkbox Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Checkbox elements
 *
 * @code
 * auto desc = CheckboxDescriptor{
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

	/// Toggle callback
	std::function<void(bool)> on_toggled;
};

// ============================================================================
// Divider Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Divider elements
 */
struct DividerDescriptor {
	/// Bounds (position and size)
	Bounds bounds;

	/// Divider color
	batch_renderer::Color color{batch_renderer::Colors::GRAY};

	/// Whether divider is horizontal (true) or vertical (false)
	bool horizontal{true};

	/// Whether divider is initially visible
	bool visible{true};
};

// ============================================================================
// Progress Bar Descriptor
// ============================================================================

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

// ============================================================================
// Image Descriptor
// ============================================================================

/**
 * @brief Declarative descriptor for Image elements
 *
 * @code
 * auto desc = ImageDescriptor{
 *     .bounds = {10, 10, 64, 64},
 *     .texture_id = myTexture.GetId(),
 *     .tint = Colors::WHITE
 * };
 * @endcode
 */
struct ImageDescriptor {
	/// Bounds (position and size)
	Bounds bounds;

	/// Texture ID to display (0 = no texture)
	uint32_t texture_id{0};

	/// Tint color
	batch_renderer::Color tint{batch_renderer::Colors::WHITE};

	/// UV coordinates (optional, for atlas textures)
	std::optional<UVCoords> uv_coords;

	/// Whether image is initially visible
	bool visible{true};
};

// ============================================================================
// Container Descriptor (with children)
// ============================================================================

/**
 * @brief Variant type for all UI element descriptors
 *
 * Used in ContainerDescriptor to describe nested children.
 */
using UIDescriptorVariant = std::variant<ButtonDescriptor,
										 PanelDescriptor,
										 LabelDescriptor,
										 SliderDescriptor,
										 CheckboxDescriptor,
										 DividerDescriptor,
										 ProgressBarDescriptor,
										 ImageDescriptor>;

/**
 * @brief Declarative descriptor for Container elements with children
 *
 * Supports nested child descriptors for building complex UI hierarchies:
 * @code
 * auto desc = ContainerDescriptor{
 *     .bounds = {100, 100, 300, 400},
 *     .background = Colors::DARK_GRAY,
 *     .padding = 10.0f,
 *     .children = {
 *         LabelDescriptor{
 *             .bounds = {0, 0, 200, 24},
 *             .text = "Settings",
 *             .style = {.font_size = 20.0f, .color = Colors::GOLD}
 *         },
 *         SliderDescriptor{
 *             .bounds = {0, 40, 200, 30},
 *             .label = "Volume",
 *             .min_value = 0.0f,
 *             .max_value = 100.0f
 *         },
 *         ButtonDescriptor{
 *             .bounds = {0, 100, 100, 40},
 *             .label = "Apply"
 *         }
 *     }
 * };
 * auto container = UIFactory::Create(desc);
 * @endcode
 */
struct ContainerDescriptor {
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

	/// Whether container is initially visible
	bool visible{true};

	/// Child element descriptors
	std::vector<UIDescriptorVariant> children;
};

// ============================================================================
// Complete UI Descriptor (for top-level variant including Container)
// ============================================================================

/**
 * @brief Complete variant type including ContainerDescriptor
 *
 * Use this for JSON serialization and factory creation of any UI element.
 */
using CompleteUIDescriptor = std::variant<ButtonDescriptor,
										  PanelDescriptor,
										  LabelDescriptor,
										  SliderDescriptor,
										  CheckboxDescriptor,
										  DividerDescriptor,
										  ProgressBarDescriptor,
										  ImageDescriptor,
										  ContainerDescriptor>;

// ============================================================================
// JSON Serialization (nlohmann ADL pattern)
// ============================================================================
// These to_json/from_json functions use nlohmann's ADL (Argument Dependent Lookup)
// pattern for automatic serialization. This allows:
// - nlohmann::json j = descriptor;  (calls to_json)
// - descriptor = j.get<DescriptorType>();  (calls from_json)
//
// Note: Callbacks (on_click, on_value_changed, etc.) cannot be serialized.
// Wire them after loading from JSON.
// ============================================================================

// --- Private helper functions for types in other namespaces ---

namespace detail {

inline nlohmann::json color_to_json(const batch_renderer::Color& c) {
	return nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
}

inline batch_renderer::Color color_from_json(const nlohmann::json& j) {
	return batch_renderer::Color{
		j.value("r", 1.0f),
		j.value("g", 1.0f),
		j.value("b", 1.0f),
		j.value("a", 1.0f)
	};
}

inline nlohmann::json bounds_to_json(const Bounds& b) {
	return nlohmann::json{{"x", b.x}, {"y", b.y}, {"width", b.width}, {"height", b.height}};
}

inline Bounds bounds_from_json(const nlohmann::json& j) {
	return Bounds{
		j.value("x", 0.0f),
		j.value("y", 0.0f),
		j.value("width", 100.0f),
		j.value("height", 100.0f)
	};
}

inline nlohmann::json border_to_json(const BorderStyle& b) {
	return nlohmann::json{{"width", b.width}, {"color", color_to_json(b.color)}};
}

inline BorderStyle border_from_json(const nlohmann::json& j) {
	BorderStyle b;
	b.width = j.value("width", 0.0f);
	if (j.contains("color")) {
		b.color = color_from_json(j["color"]);
	}
	return b;
}

inline nlohmann::json textstyle_to_json(const TextStyle& t) {
	return nlohmann::json{{"font_size", t.font_size}, {"color", color_to_json(t.color)}};
}

inline TextStyle textstyle_from_json(const nlohmann::json& j) {
	TextStyle t;
	t.font_size = j.value("font_size", 16.0f);
	if (j.contains("color")) {
		t.color = color_from_json(j["color"]);
	}
	return t;
}

inline nlohmann::json uv_to_json(const UVCoords& uv) {
	return nlohmann::json{{"u0", uv.u0}, {"v0", uv.v0}, {"u1", uv.u1}, {"v1", uv.v1}};
}

inline UVCoords uv_from_json(const nlohmann::json& j) {
	return UVCoords{
		j.value("u0", 0.0f),
		j.value("v0", 0.0f),
		j.value("u1", 1.0f),
		j.value("v1", 1.0f)
	};
}

} // namespace detail

// --- ADL functions for Bounds (in this namespace) ---

inline void to_json(nlohmann::json& j, const Bounds& b) {
	j = detail::bounds_to_json(b);
}

inline void from_json(const nlohmann::json& j, Bounds& b) {
	b = detail::bounds_from_json(j);
}

inline void to_json(nlohmann::json& j, const BorderStyle& b) {
	j = detail::border_to_json(b);
}

inline void from_json(const nlohmann::json& j, BorderStyle& b) {
	b = detail::border_from_json(j);
}

inline void to_json(nlohmann::json& j, const TextStyle& t) {
	j = detail::textstyle_to_json(t);
}

inline void from_json(const nlohmann::json& j, TextStyle& t) {
	t = detail::textstyle_from_json(j);
}

inline void to_json(nlohmann::json& j, const UVCoords& uv) {
	j = detail::uv_to_json(uv);
}

inline void from_json(const nlohmann::json& j, UVCoords& uv) {
	uv = detail::uv_from_json(j);
}

// --- Descriptor serialization ---

inline void to_json(nlohmann::json& j, const ButtonDescriptor& d) {
	j = nlohmann::json{
		{"type", "button"},
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
}

inline void from_json(const nlohmann::json& j, ButtonDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.label = j.value("label", "");
	if (j.contains("text_style"))
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	if (j.contains("normal_color"))
		d.normal_color = detail::color_from_json(j["normal_color"]);
	if (j.contains("hover_color"))
		d.hover_color = detail::color_from_json(j["hover_color"]);
	if (j.contains("pressed_color"))
		d.pressed_color = detail::color_from_json(j["pressed_color"]);
	if (j.contains("disabled_color"))
		d.disabled_color = detail::color_from_json(j["disabled_color"]);
	if (j.contains("border"))
		d.border = detail::border_from_json(j["border"]);
	d.enabled = j.value("enabled", true);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const PanelDescriptor& d) {
	j = nlohmann::json{
		{"type", "panel"},
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
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	if (j.contains("background"))
		d.background = detail::color_from_json(j["background"]);
	if (j.contains("border"))
		d.border = detail::border_from_json(j["border"]);
	d.padding = j.value("padding", 0.0f);
	d.opacity = j.value("opacity", 1.0f);
	d.clip_children = j.value("clip_children", false);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const LabelDescriptor& d) {
	j = nlohmann::json{
		{"type", "label"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"text", d.text},
		{"style", detail::textstyle_to_json(d.style)},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, LabelDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.text = j.value("text", "");
	if (j.contains("style"))
		d.style = detail::textstyle_from_json(j["style"]);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const SliderDescriptor& d) {
	j = nlohmann::json{
		{"type", "slider"},
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
}

inline void from_json(const nlohmann::json& j, SliderDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.min_value = j.value("min_value", 0.0f);
	d.max_value = j.value("max_value", 1.0f);
	d.initial_value = j.value("initial_value", 0.0f);
	d.label = j.value("label", "");
	if (j.contains("track_color"))
		d.track_color = detail::color_from_json(j["track_color"]);
	if (j.contains("fill_color"))
		d.fill_color = detail::color_from_json(j["fill_color"]);
	if (j.contains("thumb_color"))
		d.thumb_color = detail::color_from_json(j["thumb_color"]);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const CheckboxDescriptor& d) {
	j = nlohmann::json{
		{"type", "checkbox"},
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
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.label = j.value("label", "");
	d.initial_checked = j.value("initial_checked", false);
	if (j.contains("unchecked_color"))
		d.unchecked_color = detail::color_from_json(j["unchecked_color"]);
	if (j.contains("checked_color"))
		d.checked_color = detail::color_from_json(j["checked_color"]);
	if (j.contains("text_style"))
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	d.enabled = j.value("enabled", true);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const DividerDescriptor& d) {
	j = nlohmann::json{
		{"type", "divider"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"color", detail::color_to_json(d.color)},
		{"horizontal", d.horizontal},
		{"visible", d.visible}
	};
}

inline void from_json(const nlohmann::json& j, DividerDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	if (j.contains("color"))
		d.color = detail::color_from_json(j["color"]);
	d.horizontal = j.value("horizontal", true);
	d.visible = j.value("visible", true);
}

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
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.initial_progress = j.value("initial_progress", 0.0f);
	d.label = j.value("label", "");
	d.show_percentage = j.value("show_percentage", false);
	if (j.contains("track_color"))
		d.track_color = detail::color_from_json(j["track_color"]);
	if (j.contains("fill_color"))
		d.fill_color = detail::color_from_json(j["fill_color"]);
	if (j.contains("text_style"))
		d.text_style = detail::textstyle_from_json(j["text_style"]);
	if (j.contains("border"))
		d.border = detail::border_from_json(j["border"]);
	d.visible = j.value("visible", true);
}

inline void to_json(nlohmann::json& j, const ImageDescriptor& d) {
	j = nlohmann::json{
		{"type", "image"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"texture_id", d.texture_id},
		{"tint", detail::color_to_json(d.tint)},
		{"visible", d.visible}
	};
	if (d.uv_coords) {
		j["uv_coords"] = detail::uv_to_json(*d.uv_coords);
	}
}

inline void from_json(const nlohmann::json& j, ImageDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	d.texture_id = j.value("texture_id", 0u);
	if (j.contains("tint"))
		d.tint = detail::color_from_json(j["tint"]);
	if (j.contains("uv_coords"))
		d.uv_coords = detail::uv_from_json(j["uv_coords"]);
	d.visible = j.value("visible", true);
}

// Forward declaration for recursive container serialization
inline void to_json(nlohmann::json& j, const UIDescriptorVariant& v);
inline void from_json(const nlohmann::json& j, UIDescriptorVariant& v);

inline void to_json(nlohmann::json& j, const ContainerDescriptor& d) {
	j = nlohmann::json{
		{"type", "container"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"background", detail::color_to_json(d.background)},
		{"border", detail::border_to_json(d.border)},
		{"padding", d.padding},
		{"opacity", d.opacity},
		{"clip_children", d.clip_children},
		{"visible", d.visible}
	};
	if (!d.children.empty()) {
		j["children"] = nlohmann::json::array();
		for (const auto& child : d.children) {
			nlohmann::json child_json;
			to_json(child_json, child);
			j["children"].push_back(child_json);
		}
	}
}

inline void from_json(const nlohmann::json& j, ContainerDescriptor& d) {
	if (j.contains("bounds"))
		d.bounds = detail::bounds_from_json(j["bounds"]);
	if (j.contains("background"))
		d.background = detail::color_from_json(j["background"]);
	if (j.contains("border"))
		d.border = detail::border_from_json(j["border"]);
	d.padding = j.value("padding", 0.0f);
	d.opacity = j.value("opacity", 1.0f);
	d.clip_children = j.value("clip_children", false);
	d.visible = j.value("visible", true);
	if (j.contains("children") && j["children"].is_array()) {
		for (const auto& child_json : j["children"]) {
			UIDescriptorVariant child;
			from_json(child_json, child);
			d.children.push_back(std::move(child));
		}
	}
}

// --- Variant serialization (for children) ---

inline void to_json(nlohmann::json& j, const UIDescriptorVariant& v) {
	std::visit([&j](const auto& d) { to_json(j, d); }, v);
}

inline void from_json(const nlohmann::json& j, UIDescriptorVariant& v) {
	const std::string type = j.value("type", "panel");
	if (type == "button") {
		v = j.get<ButtonDescriptor>();
	}
	else if (type == "label") {
		v = j.get<LabelDescriptor>();
	}
	else if (type == "slider") {
		v = j.get<SliderDescriptor>();
	}
	else if (type == "checkbox") {
		v = j.get<CheckboxDescriptor>();
	}
	else if (type == "divider") {
		v = j.get<DividerDescriptor>();
	}
	else if (type == "progress_bar") {
		v = j.get<ProgressBarDescriptor>();
	}
	else if (type == "image") {
		v = j.get<ImageDescriptor>();
	}
	else {
		v = j.get<PanelDescriptor>();
	}
}

inline void to_json(nlohmann::json& j, const CompleteUIDescriptor& v) {
	std::visit([&j](const auto& d) { to_json(j, d); }, v);
}

inline void from_json(const nlohmann::json& j, CompleteUIDescriptor& v) {
	const std::string type = j.value("type", "panel");
	if (type == "button") {
		v = j.get<ButtonDescriptor>();
	}
	else if (type == "panel") {
		v = j.get<PanelDescriptor>();
	}
	else if (type == "label") {
		v = j.get<LabelDescriptor>();
	}
	else if (type == "slider") {
		v = j.get<SliderDescriptor>();
	}
	else if (type == "checkbox") {
		v = j.get<CheckboxDescriptor>();
	}
	else if (type == "divider") {
		v = j.get<DividerDescriptor>();
	}
	else if (type == "progress_bar") {
		v = j.get<ProgressBarDescriptor>();
	}
	else if (type == "image") {
		v = j.get<ImageDescriptor>();
	}
	else if (type == "container") {
		v = j.get<ContainerDescriptor>();
	}
	else {
		v = j.get<PanelDescriptor>();
	}
}

} // namespace engine::ui::descriptor
