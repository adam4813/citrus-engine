module;

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

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

} // namespace engine::ui::descriptor
