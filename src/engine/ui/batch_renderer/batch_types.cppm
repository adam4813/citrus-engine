module;

#include <algorithm>
#include <cstdint>
#include <optional>

export module engine.ui.batch_renderer:batch_types;

export namespace engine::ui::batch_renderer {
/**
     * @brief Color type for batch renderer (RGBA, 0.0-1.0 range)
     */
struct Color {
	float r, g, b, a;

	constexpr Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}

	constexpr Color(const float r, const float g, const float b, const float a = 1.0f) : r(r), g(g), b(b), a(a) {}

	/**
         * @brief Create a color with modified alpha channel
         * @param color Base color
         * @param alpha New alpha value (0.0-1.0)
         * @return Color with new alpha, preserving RGB
         */
	static constexpr Color Alpha(const Color& color, const float alpha) {
		return Color(color.r, color.g, color.b, std::clamp(alpha, 0.0f, 1.0f));
	}

	/**
         * @brief Create a color with adjusted brightness
         * @param color Base color
         * @param factor Brightness adjustment (-1.0 to 1.0, where 0 = no change)
         * @return Color with adjusted RGB values, clamped to [0, 1]
         */
	static constexpr Color Brightness(const Color& color, const float factor) {
		return Color(
				std::clamp(color.r + factor, 0.0f, 1.0f),
				std::clamp(color.g + factor, 0.0f, 1.0f),
				std::clamp(color.b + factor, 0.0f, 1.0f),
				color.a);
	}
};

// Common color constants
namespace Colors {
constexpr Color WHITE{1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color BLACK{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color RED{1.0f, 0.0f, 0.0f, 1.0f};
constexpr Color GREEN{0.0f, 1.0f, 0.0f, 1.0f};
constexpr Color BLUE{0.0f, 0.0f, 1.0f, 1.0f};
constexpr Color YELLOW{1.0f, 1.0f, 0.0f, 1.0f};
constexpr Color CYAN{0.0f, 1.0f, 1.0f, 1.0f};
constexpr Color MAGENTA{1.0f, 0.0f, 1.0f, 1.0f};
constexpr Color GRAY{0.5f, 0.5f, 0.5f, 1.0f};
constexpr Color LIGHT_GRAY{0.7f, 0.7f, 0.7f, 1.0f};
constexpr Color DARK_GRAY{0.3f, 0.3f, 0.3f, 1.0f};
constexpr Color TRANSPARENT{0.0f, 0.0f, 0.0f, 0.0f};

// UI theme colors
constexpr Color GOLD{1.0f, 0.84f, 0.0f, 1.0f}; // Primary accent
constexpr Color ORANGE{1.0f, 0.65f, 0.0f, 1.0f};
constexpr Color PURPLE{0.5f, 0.0f, 0.5f, 1.0f};
} // namespace Colors

/**
     * @brief UI Theme constants for consistent styling across components
     *
     * Provides centralized theme values for colors, spacing, fonts, and borders.
     * Components should use these constants to maintain visual consistency.
     */
namespace UITheme {
// ============================================================================
// Color Palette
// ============================================================================

// Primary colors
namespace Primary {
constexpr Color NORMAL = Colors::GOLD;
constexpr Color HOVER = Color::Brightness(Colors::GOLD, 0.15f);
constexpr Color ACTIVE = Color::Brightness(Colors::GOLD, -0.15f);
constexpr Color DISABLED = Color::Alpha(Colors::GOLD, 0.5f);
} // namespace Primary

// Background colors
namespace Background {
constexpr Color PANEL = Colors::DARK_GRAY;
constexpr Color PANEL_DARK = Color::Brightness(Colors::DARK_GRAY, -0.1f);
constexpr Color BUTTON = Colors::GRAY;
constexpr Color BUTTON_HOVER = Color::Brightness(Colors::GRAY, 0.1f);
constexpr Color BUTTON_ACTIVE = Color::Brightness(Colors::GRAY, -0.1f);
constexpr Color INPUT = Color::Brightness(Colors::DARK_GRAY, -0.05f);
constexpr Color DISABLED = Color::Brightness(Colors::DARK_GRAY, -0.15f);
} // namespace Background

// Text colors
namespace Text {
constexpr Color PRIMARY = Colors::WHITE;
constexpr Color SECONDARY = Colors::LIGHT_GRAY;
constexpr Color DISABLED = Color::Alpha(Colors::WHITE, 0.5f);
constexpr Color ACCENT = Colors::GOLD;
constexpr Color ERROR = Colors::RED;
constexpr Color SUCCESS = Colors::GREEN;
constexpr Color WARNING = Colors::ORANGE;
} // namespace Text

// Border colors
namespace Border {
constexpr Color DEFAULT = Colors::GRAY;
constexpr Color HOVER = Colors::LIGHT_GRAY;
constexpr Color FOCUS = Colors::GOLD;
constexpr Color DISABLED = Color::Alpha(Colors::GRAY, 0.5f);
constexpr Color ERROR = Colors::RED;
} // namespace Border

// State colors
namespace State {
constexpr Color SELECTED = Colors::GOLD;
constexpr Color CHECKED = Colors::GREEN;
constexpr Color UNCHECKED = Colors::GRAY;
constexpr Color HOVER_OVERLAY = Color::Alpha(Colors::WHITE, 0.1f);
constexpr Color PRESS_OVERLAY = Color::Alpha(Colors::BLACK, 0.1f);
} // namespace State

// ============================================================================
// Spacing Constants
// ============================================================================

namespace Spacing {
constexpr float NONE = 0.0f;
constexpr float TINY = 2.0f;
constexpr float SMALL = 4.0f;
constexpr float MEDIUM = 8.0f;
constexpr float LARGE = 12.0f;
constexpr float XL = 16.0f;
constexpr float XXL = 24.0f;
} // namespace Spacing

// ============================================================================
// Padding Constants
// ============================================================================

namespace Padding {
// Button padding
constexpr float BUTTON_HORIZONTAL = 16.0f;
constexpr float BUTTON_VERTICAL = 8.0f;

// Panel padding
constexpr float PANEL_HORIZONTAL = 12.0f;
constexpr float PANEL_VERTICAL = 12.0f;

// Input padding
constexpr float INPUT_HORIZONTAL = 8.0f;
constexpr float INPUT_VERTICAL = 6.0f;

// Label padding
constexpr float LABEL_HORIZONTAL = 4.0f;
constexpr float LABEL_VERTICAL = 4.0f;

// Generic padding levels
constexpr float SMALL = 4.0f;
constexpr float MEDIUM = 8.0f;
constexpr float LARGE = 12.0f;
} // namespace Padding

// ============================================================================
// Font Size Constants
// ============================================================================

namespace FontSize {
constexpr float TINY = 10.0f;
constexpr float SMALL = 12.0f;
constexpr float NORMAL = 14.0f;
constexpr float MEDIUM = 16.0f;
constexpr float LARGE = 18.0f;
constexpr float XL = 20.0f;
constexpr float XXL = 24.0f;
constexpr float HEADING_1 = 32.0f;
constexpr float HEADING_2 = 28.0f;
constexpr float HEADING_3 = 24.0f;
} // namespace FontSize

// ============================================================================
// Border Constants
// ============================================================================

namespace BorderSize {
constexpr float NONE = 0.0f;
constexpr float THIN = 1.0f;
constexpr float MEDIUM = 2.0f;
constexpr float THICK = 3.0f;
} // namespace BorderSize

namespace BorderRadius {
constexpr float NONE = 0.0f;
constexpr float SMALL = 2.0f;
constexpr float MEDIUM = 4.0f;
constexpr float LARGE = 8.0f;
constexpr float ROUND = 999.0f; // Fully rounded (for pills/circles)
} // namespace BorderRadius

// ============================================================================
// Component-Specific Constants
// ============================================================================

namespace Button {
constexpr float MIN_WIDTH = 80.0f;
constexpr float MIN_HEIGHT = 32.0f;
constexpr float DEFAULT_HEIGHT = 36.0f;
} // namespace Button

namespace Panel {
constexpr float MIN_WIDTH = 100.0f;
constexpr float MIN_HEIGHT = 100.0f;
} // namespace Panel

namespace Slider {
constexpr float TRACK_HEIGHT = 4.0f;
constexpr float THUMB_SIZE = 16.0f;
constexpr float MIN_WIDTH = 100.0f;
} // namespace Slider

namespace Checkbox {
constexpr float SIZE = 20.0f;
constexpr float CHECK_MARK_THICKNESS = 2.0f;
} // namespace Checkbox

namespace Input {
constexpr float MIN_WIDTH = 120.0f;
constexpr float DEFAULT_HEIGHT = 32.0f;
} // namespace Input

// ============================================================================
// Animation Constants
// ============================================================================

namespace Animation {
constexpr float FAST = 0.1f; // 100ms
constexpr float NORMAL = 0.2f; // 200ms
constexpr float SLOW = 0.3f; // 300ms
} // namespace Animation

// ============================================================================
// Z-Index / Layer Constants
// ============================================================================

namespace Layer {
constexpr int BACKGROUND = 0;
constexpr int CONTENT = 10;
constexpr int OVERLAY = 20;
constexpr int MODAL = 30;
constexpr int TOOLTIP = 40;
constexpr int NOTIFICATION = 50;
} // namespace Layer
} // namespace UITheme

/**
     * @brief Vertex layout for batched rendering
     *
     * Packed vertex format for efficient GPU upload.
     * Compatible with standard 2D shaders.
     */
struct Vertex {
	float x, y; // Position (screen-space)
	float u, v; // Texture coordinates (0.0-1.0)
	float r, g, b, a; // Color components (0.0-1.0)
	float tex_index; // Texture slot index (0-7)

	Vertex() = default;

	Vertex(const float x,
		   const float y,
		   const float u,
		   const float v,
		   const Color& color,
		   const float tex_index = 0.0f) :
			x(x), y(y), u(u), v(v), r(color.r), g(color.g), b(color.b), a(color.a), tex_index(tex_index) {}
};

struct Vector2 {
	float x, y;

	Vector2() : x(0), y(0) {}

	Vector2(const float x, const float y) : x(x), y(y) {}
};

/**
     * @brief Scissor rectangle for clipping (screen-space, top-left origin)
     */
struct ScissorRect {
	float x, y, width, height;

	ScissorRect() : x(0), y(0), width(0), height(0) {}

	ScissorRect(const float x, const float y, const float w, const float h) : x(x), y(y), width(w), height(h) {}

	/**
         * @brief Intersect this scissor with another
         * @return Intersection rectangle; may have zero width/height if no overlap
         */
	ScissorRect Intersect(const ScissorRect& other) const {
		const float left = std::max(x, other.x);
		const float top = std::max(y, other.y);
		const float right = std::min(x + width, other.x + other.width);
		const float bottom = std::min(y + height, other.y + other.height);

		return ScissorRect(left, top, std::max(0.0f, right - left), std::max(0.0f, bottom - top));
	}

	/**
         * @brief Check if scissor has non-zero area
         */
	bool IsValid() const { return width > 0.0f && height > 0.0f; }

	/**
         * @brief Equality comparison for batch key matching
         */
	bool operator==(const ScissorRect& other) const {
		return x == other.x && y == other.y && width == other.width && height == other.height;
	}

	bool operator!=(const ScissorRect& other) const { return !(*this == other); }
};

/**
     * @brief Rectangle type for batch renderer
     */
struct Rectangle {
	float x, y, width, height;

	Rectangle() : x(0), y(0), width(0), height(0) {}

	Rectangle(const float x, const float y, const float w, const float h) : x(x), y(y), width(w), height(h) {}
};

/**
     * @brief Convert Color to packed RGBA uint32_t
     *
     * Format: 0xRRGGBBAA (straight alpha, not premultiplied)
     */
inline uint32_t ColorToRGBA(const Color& c) {
	return (static_cast<uint32_t>(c.r * 255.0f) << 24) | (static_cast<uint32_t>(c.g * 255.0f) << 16)
		   | (static_cast<uint32_t>(c.b * 255.0f) << 8) | static_cast<uint32_t>(c.a * 255.0f);
}

/**
     * @brief Convert packed RGBA to Color
     */
inline Color RGBAToColor(const uint32_t rgba) {
	return Color{
			static_cast<float>((rgba >> 24) & 0xFF) / 255.0f,
			static_cast<float>((rgba >> 16) & 0xFF) / 255.0f,
			static_cast<float>((rgba >> 8) & 0xFF) / 255.0f,
			static_cast<float>(rgba & 0xFF) / 255.0f};
}
} // namespace engine::ui::batch_renderer
