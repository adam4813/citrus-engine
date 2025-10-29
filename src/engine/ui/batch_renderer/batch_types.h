#pragma once

#include <cstdint>
#include <optional>
#include <algorithm>

namespace engine::ui::batch_renderer {
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

    /**
     * @brief Scissor rectangle for clipping (screen-space, top-left origin)
     * 
     * Raylib uses top-left origin coordinate system.
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
     * @brief Convert Raylib Color to packed RGBA uint32_t
     * 
     * Format: 0xRRGGBBAA (straight alpha, not premultiplied)
     * Raylib uses straight alpha blending by default.
     */
    inline uint32_t ColorToRGBA(const Color &c) {
        return (static_cast<uint32_t>(c.r) << 24) |
               (static_cast<uint32_t>(c.g) << 16) |
               (static_cast<uint32_t>(c.b) << 8) |
               static_cast<uint32_t>(c.a);
    }

    /**
     * @brief Convert packed RGBA to Raylib Color
     */
    inline Color RGBAToColor(const uint32_t rgba) {
        return Color{
            static_cast<unsigned char>((rgba >> 24) & 0xFF),
            static_cast<unsigned char>((rgba >> 16) & 0xFF),
            static_cast<unsigned char>((rgba >> 8) & 0xFF),
            static_cast<unsigned char>(rgba & 0xFF)
        };
    }
} // namespace egine::ui::batch_renderer
