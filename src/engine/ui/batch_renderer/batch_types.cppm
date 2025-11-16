module;

#include <cstdint>
#include <optional>
#include <algorithm>

export module engine.ui.batch_renderer:batch_types;

export namespace engine::ui::batch_renderer {
    /**
     * @brief Color type for batch renderer (RGBA, 0.0-1.0 range)
     */
    struct Color {
        float r, g, b, a;

        constexpr Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {
        }

        constexpr Color(const float r, const float g, const float b, const float a = 1.0f) : r(r), g(g), b(b), a(a) {
        }

        /**
         * @brief Create a color with modified alpha channel
         * @param color Base color
         * @param alpha New alpha value (0.0-1.0)
         * @return Color with new alpha, preserving RGB
         */
        static Color Alpha(const Color& color, const float alpha) {
            return Color(color.r, color.g, color.b, std::clamp(alpha, 0.0f, 1.0f));
        }

        /**
         * @brief Create a color with adjusted brightness
         * @param color Base color
         * @param factor Brightness adjustment (-1.0 to 1.0, where 0 = no change)
         * @return Color with adjusted RGB values, clamped to [0, 1]
         */
        static Color Brightness(const Color& color, const float factor) {
            return Color(
                std::clamp(color.r + factor, 0.0f, 1.0f),
                std::clamp(color.g + factor, 0.0f, 1.0f),
                std::clamp(color.b + factor, 0.0f, 1.0f),
                color.a
            );
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
        constexpr Color GOLD{1.0f, 0.84f, 0.0f, 1.0f};  // Primary accent
        constexpr Color ORANGE{1.0f, 0.65f, 0.0f, 1.0f};
        constexpr Color PURPLE{0.5f, 0.0f, 0.5f, 1.0f};
    }

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

        Vertex(const float x, const float y, const float u, const float v, const Color& color,
               const float tex_index = 0.0f)
            : x(x), y(y), u(u), v(v), r(color.r), g(color.g), b(color.b), a(color.a), tex_index(tex_index) {
        }
    };

    struct Vector2 {
        float x, y;

        Vector2() : x(0), y(0) {
        }

        Vector2(const float x, const float y) : x(x), y(y) {
        }
    };

    /**
     * @brief Scissor rectangle for clipping (screen-space, top-left origin)
     */
    struct ScissorRect {
        float x, y, width, height;

        ScissorRect() : x(0), y(0), width(0), height(0) {
        }

        ScissorRect(const float x, const float y, const float w, const float h) : x(x), y(y), width(w), height(h) {
        }

        /**
         * @brief Intersect this scissor with another
         * @return Intersection rectangle; may have zero width/height if no overlap
         */
        ScissorRect Intersect(const ScissorRect &other) const {
            const float left = std::max(x, other.x);
            const float top = std::max(y, other.y);
            const float right = std::min(x + width, other.x + other.width);
            const float bottom = std::min(y + height, other.y + other.height);

            return ScissorRect(
                left,
                top,
                std::max(0.0f, right - left),
                std::max(0.0f, bottom - top)
            );
        }

        /**
         * @brief Check if scissor has non-zero area
         */
        bool IsValid() const {
            return width > 0.0f && height > 0.0f;
        }

        /**
         * @brief Equality comparison for batch key matching
         */
        bool operator==(const ScissorRect &other) const {
            return x == other.x && y == other.y &&
                   width == other.width && height == other.height;
        }

        bool operator!=(const ScissorRect &other) const {
            return !(*this == other);
        }
    };

    /**
     * @brief Rectangle type for batch renderer
     */
    struct Rectangle {
        float x, y, width, height;

        Rectangle() : x(0), y(0), width(0), height(0) {
        }

        Rectangle(const float x, const float y, const float w, const float h) : x(x), y(y), width(w), height(h) {
        }
    };

    /**
     * @brief Convert Color to packed RGBA uint32_t
     * 
     * Format: 0xRRGGBBAA (straight alpha, not premultiplied)
     */
    inline uint32_t ColorToRGBA(const Color &c) {
        return (static_cast<uint32_t>(c.r * 255.0f) << 24) |
               (static_cast<uint32_t>(c.g * 255.0f) << 16) |
               (static_cast<uint32_t>(c.b * 255.0f) << 8) |
               static_cast<uint32_t>(c.a * 255.0f);
    }

    /**
     * @brief Convert packed RGBA to Color
     */
    inline Color RGBAToColor(const uint32_t rgba) {
        return Color{
            static_cast<float>((rgba >> 24) & 0xFF) / 255.0f,
            static_cast<float>((rgba >> 16) & 0xFF) / 255.0f,
            static_cast<float>((rgba >> 8) & 0xFF) / 255.0f,
            static_cast<float>(rgba & 0xFF) / 255.0f
        };
    }
} // namespace engine::ui::batch_renderer
