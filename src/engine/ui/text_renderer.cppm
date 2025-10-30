module;

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>
#include <memory>

export module engine.ui:text_renderer;

import engine.ui.batch_renderer;

export namespace engine::ui::text_renderer {
    /**
     * @brief Glyph metrics for a single character
     */
    struct GlyphMetrics {
        batch_renderer::Rectangle atlas_rect;  // UV coordinates in atlas (normalized 0-1)
        batch_renderer::Vector2 bearing;       // Offset from baseline to left/top
        float advance;                         // Horizontal advance to next glyph
        batch_renderer::Vector2 size;          // Glyph bitmap dimensions (pixels)
    };

    /**
     * @brief Font atlas containing pre-rasterized glyphs
     */
    class FontAtlas {
    public:
        /**
         * @brief Load font from file and generate atlas
         * @param font_path Path to TrueType font file
         * @param font_size_px Font size in pixels
         */
        FontAtlas(const std::string& font_path, int font_size_px);

        ~FontAtlas();

        // Prevent copying
        FontAtlas(const FontAtlas&) = delete;
        FontAtlas& operator=(const FontAtlas&) = delete;

        /**
         * @brief Get glyph metrics for a Unicode codepoint
         * @param codepoint Unicode codepoint (e.g., 'A' = 65)
         * @return Pointer to glyph metrics, or nullptr if not found
         */
        const GlyphMetrics* GetGlyph(uint32_t codepoint) const;

        /**
         * @brief Get atlas texture ID for rendering
         */
        uint32_t GetTextureId() const { return atlas_texture_id_; }

        /**
         * @brief Get font vertical metrics
         */
        float GetAscent() const { return ascent_; }
        float GetDescent() const { return descent_; }
        float GetLineGap() const { return line_gap_; }
        float GetLineHeight() const { return ascent_ - descent_ + line_gap_; }

        /**
         * @brief Get font size in pixels
         */
        int GetFontSize() const { return font_size_; }

        /**
         * @brief Check if atlas was successfully loaded
         */
        bool IsValid() const { return atlas_texture_id_ != 0; }

    private:
        std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
        uint32_t atlas_texture_id_ = 0;
        float ascent_ = 0.0f;
        float descent_ = 0.0f;
        float line_gap_ = 0.0f;
        int font_size_ = 0;
        int atlas_width_ = 0;
        int atlas_height_ = 0;
    };

    /**
     * @brief Text alignment options
     */
    enum class HorizontalAlign {
        Left,
        Center,
        Right
    };

    enum class VerticalAlign {
        Top,
        Center,
        Bottom
    };

    /**
     * @brief Text layout options
     */
    struct LayoutOptions {
        HorizontalAlign h_align = HorizontalAlign::Left;
        VerticalAlign v_align = VerticalAlign::Top;
        float max_width = 0.0f;  // 0 = no wrapping
        float line_height = 1.0f;  // Line height multiplier
    };

    /**
     * @brief Positioned glyph ready for rendering
     */
    struct PositionedGlyph {
        uint32_t codepoint;
        batch_renderer::Vector2 position;  // Screen position
        const GlyphMetrics* metrics;
    };

    /**
     * @brief Text layout engine
     */
    class TextLayout {
    public:
        /**
         * @brief Layout text string into positioned glyphs
         * @param text UTF-8 encoded text string
         * @param font Font atlas to use
         * @param options Layout options (alignment, wrapping, etc.)
         * @return Array of positioned glyphs ready for rendering
         */
        static std::vector<PositionedGlyph> Layout(
            const std::string& text,
            const FontAtlas& font,
            const LayoutOptions& options
        );

        /**
         * @brief Measure text dimensions without full layout
         * @param text UTF-8 encoded text string
         * @param font Font atlas to use
         * @param max_width Maximum width for wrapping (0 = no limit)
         * @return Bounding rectangle (x=0, y=0, width, height)
         */
        static batch_renderer::Rectangle MeasureText(
            const std::string& text,
            const FontAtlas& font,
            float max_width = 0.0f
        );
    };

    /**
     * @brief UTF-8 utility functions
     */
    namespace utf8 {
        /**
         * @brief Decode UTF-8 string into Unicode codepoints
         * @param utf8_string UTF-8 encoded string
         * @return Vector of Unicode codepoints
         */
        std::vector<uint32_t> Decode(const std::string& utf8_string);
    }

    /**
     * @brief Font manager - centralized font loading and caching
     */
    class FontManager {
    public:
        /**
         * @brief Initialize font manager with default font
         * @param default_font_path Path to default font
         * @param default_font_size Default font size
         */
        static void Initialize(const std::string& default_font_path, int default_font_size = 16);

        /**
         * @brief Shutdown and cleanup all fonts
         */
        static void Shutdown();

        /**
         * @brief Get default font
         * @return Pointer to default font, or nullptr if not initialized
         */
        static FontAtlas* GetDefaultFont();

        /**
         * @brief Get or load font
         * @param font_path Path to font file
         * @param font_size Font size in pixels
         * @return Pointer to font, or nullptr if loading failed
         */
        static FontAtlas* GetFont(const std::string& font_path, int font_size);

    private:
        struct FontKey {
            std::string path;
            int size;

            bool operator==(const FontKey& other) const {
                return path == other.path && size == other.size;
            }
        };

        struct FontKeyHash {
            size_t operator()(const FontKey& key) const {
                return std::hash<std::string>()(key.path) ^ std::hash<int>()(key.size);
            }
        };

        static std::unordered_map<FontKey, std::unique_ptr<FontAtlas>, FontKeyHash> fonts_;
        static std::string default_font_path_;
        static int default_font_size_;
        static bool initialized_;
    };
}
