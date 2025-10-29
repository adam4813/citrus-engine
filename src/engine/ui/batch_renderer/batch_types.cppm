module;

#include <cstdint>
#include <optional>
#include <algorithm>

export module engine.ui.batch_renderer:batch_types;

export namespace engine::ui::batch_renderer {
    /**
     * @brief Vertex layout for batched rendering
     * 
     * Packed vertex format for efficient GPU upload.
     * Compatible with standard 2D shaders.
     */
    struct Vertex {
        float x, y; // Position (screen-space)
        float u, v; // Texture coordinates (0.0-1.0)
        uint32_t color; // RGBA packed (0xRRGGBBAA)
        float tex_index; // Texture slot index (0-7)

        Vertex() = default;

        Vertex(const float x, const float y, const float u, const float v, const uint32_t color,
               const float tex_index = 0.0f)
            : x(x), y(y), u(u), v(v), color(color), tex_index(tex_index) {
        }
    };

    struct Vector2 {
        float x, y;

        Vector2() : x(0), y(0) {
        }

        Vector2(float x, float y) : x(x), y(y) {
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

        Rectangle(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {
        }
    };

    /**
     * @brief Color type for batch renderer (RGBA, 0.0-1.0 range)
     */
    struct Color {
        float r, g, b, a;

        Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {
        }

        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {
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
