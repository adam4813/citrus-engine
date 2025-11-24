module;

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

export module engine.ui:text_renderer;

import engine.ui.batch_renderer;

export namespace engine::ui::text_renderer {
/**
     * @brief Glyph metrics for a single character.
     *
     * Contains all information needed to render a single glyph from a font atlas,
     * including texture coordinates and positioning data.
     */
struct GlyphMetrics {
	batch_renderer::Rectangle atlas_rect; ///< UV coordinates in atlas texture (normalized 0-1)
	batch_renderer::Vector2 bearing; ///< Offset from baseline to left/top of glyph
	float advance; ///< Horizontal advance to next glyph position
	batch_renderer::Vector2 size; ///< Glyph bitmap dimensions in pixels
};

/**
     * @brief Font atlas containing pre-rasterized glyphs.
     *
     * A FontAtlas loads a TrueType font and rasterizes all supported glyphs
     * into a single texture atlas. Currently supports ASCII and Latin-1 character
     * sets (codepoints 32-255).
     *
     * The atlas uses a 512x512 grayscale (R8) texture with shelf-based packing.
     * Each font size requires a separate atlas (~256 KB of GPU memory).
     *
     * @note FontAtlas objects are typically managed by FontManager, not created directly.
     *
     * @code
     * // Typically accessed through FontManager
     * auto* font = FontManager::GetFont("assets/fonts/Roboto.ttf", 16);
     * if (font && font->IsValid()) {
     *     auto* glyph = font->GetGlyph('A');
     *     if (glyph) {
     *         // Use glyph metrics for rendering
     *     }
     * }
     * @endcode
     *
     * @see FontManager
     */
class FontAtlas {
public:
	/**
         * @brief Load a TrueType font and generate atlas texture.
         *
         * Loads a .ttf font file, rasterizes glyphs for ASCII + Latin-1 characters,
         * packs them into a texture atlas, and uploads to GPU.
         *
         * @param font_path Path to TrueType (.ttf) font file
         * @param font_size_px Font size in pixels (recommended: 12-48)
         *
         * @note Check IsValid() after construction to verify successful loading.
         * @note Loading is CPU-intensive; use FontManager for caching.
         *
         * @warning File must be accessible and contain valid TrueType data.
         */
	FontAtlas(const std::string& font_path, int font_size_px);

	~FontAtlas();

	// Prevent copying (contains GPU resources)
	FontAtlas(const FontAtlas&) = delete;
	FontAtlas& operator=(const FontAtlas&) = delete;

	/**
         * @brief Get glyph metrics for a Unicode codepoint.
         *
         * Retrieves pre-computed metrics for rendering a specific character.
         * Returns nullptr if the codepoint is not in the atlas (unsupported character).
         *
         * @param codepoint Unicode codepoint value (e.g., 'A' = 65, 'é' = 0xE9)
         * @return Pointer to glyph metrics, or nullptr if codepoint not found
         *
         * @note Currently supports ASCII (32-126) and Latin-1 (128-255) only.
         * @note Returned pointer is valid for the lifetime of the FontAtlas.
         *
         * @code
         * auto* glyph = font->GetGlyph('A');
         * if (glyph) {
         *     float char_width = glyph->advance;
         *     // Render using glyph->atlas_rect for UV coordinates
         * }
         * @endcode
         */
	const GlyphMetrics* GetGlyph(uint32_t codepoint) const;

	/**
         * @brief Get atlas texture ID for rendering.
         *
         * Returns the OpenGL texture ID of the font atlas texture.
         * Use this with BatchRenderer to render glyphs.
         *
         * @return OpenGL texture ID (0 if atlas failed to load)
         *
         * @code
         * uint32_t texId = font->GetTextureId();
         * BatchRenderer::SubmitQuad(rect, color, uv_coords, texId);
         * @endcode
         */
	uint32_t GetTextureId() const { return atlas_texture_id_; }

	/**
         * @brief Get font ascent (baseline to top).
         * @return Distance from baseline to top of tallest glyph (pixels)
         */
	float GetAscent() const { return ascent_; }

	/**
         * @brief Get font descent (baseline to bottom).
         * @return Distance from baseline to bottom of lowest glyph (pixels, typically negative)
         */
	float GetDescent() const { return descent_; }

	/**
         * @brief Get vertical gap between lines.
         * @return Recommended spacing between consecutive lines (pixels)
         */
	float GetLineGap() const { return line_gap_; }

	/**
         * @brief Get total line height.
         * @return Total height for a line of text (ascent + descent + line gap)
         */
	float GetLineHeight() const { return ascent_ - descent_ + line_gap_; }

	/**
         * @brief Get the font size this atlas was generated for.
         * @return Font size in pixels
         */
	int GetFontSize() const { return font_size_; }

	/**
         * @brief Check if the atlas was successfully loaded.
         * @return true if atlas loaded successfully, false otherwise
         *
         * @note Always check this after constructing or accessing a font.
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
     * @brief Horizontal text alignment.
     */
enum class HorizontalAlign {
	Left, ///< Align text to the left edge
	Center, ///< Center text horizontally
	Right ///< Align text to the right edge
};

/**
     * @brief Vertical text alignment.
     */
enum class VerticalAlign {
	Top, ///< Align text to the top edge
	Center, ///< Center text vertically
	Bottom ///< Align text to the bottom edge
};

/**
     * @brief Options for text layout and formatting.
     *
     * Controls how text is positioned, wrapped, and spaced during layout.
     */
struct LayoutOptions {
	HorizontalAlign h_align = HorizontalAlign::Left; ///< Horizontal alignment
	VerticalAlign v_align = VerticalAlign::Top; ///< Vertical alignment
	float max_width = 0.0f; ///< Maximum width before wrapping (0 = no wrapping)
	float line_height = 1.0f; ///< Line height multiplier (1.0 = default spacing)
};

/**
     * @brief A glyph positioned in screen space, ready for rendering.
     *
     * Contains the character, its screen position, and a pointer to its
     * metrics in the font atlas.
     */
struct PositionedGlyph {
	uint32_t codepoint; ///< Unicode codepoint of this glyph
	batch_renderer::Vector2 position; ///< Screen-space position (top-left)
	const GlyphMetrics* metrics; ///< Pointer to glyph metrics (non-owning)
};

/**
     * @brief Text layout engine for positioning glyphs.
     *
     * TextLayout converts text strings into positioned glyphs, handling word wrapping,
     * line breaking, and alignment. It supports UTF-8 text with multi-line layouts.
     *
     * The layout engine performs:
     * - UTF-8 decoding to Unicode codepoints
     * - Word wrapping at max_width boundaries
     * - Horizontal and vertical alignment
     * - Line height adjustment
     * - Text measurement without rendering
     *
     * @code
     * // Simple single-line layout
     * LayoutOptions opts;
     * auto glyphs = TextLayout::Layout("Hello!", *font, opts);
     *
     * // Multi-line with wrapping and centering
     * LayoutOptions opts;
     * opts.max_width = 300.0f;
     * opts.h_align = HorizontalAlign::Center;
     * opts.line_height = 1.2f;
     * auto glyphs = TextLayout::Layout("Long text that wraps", *font, opts);
     * @endcode
     *
     * @see LayoutOptions
     * @see PositionedGlyph
     */
class TextLayout {
public:
	/**
         * @brief Layout text string into positioned glyphs.
         *
         * Converts a UTF-8 text string into an array of positioned glyphs ready
         * for rendering. Handles newlines, word wrapping, and alignment according
         * to the provided options.
         *
         * @param text UTF-8 encoded text string (supports newlines: \n)
         * @param font Font atlas containing the glyphs
         * @param options Layout configuration (alignment, wrapping, line height)
         * @return Vector of positioned glyphs in screen space
         *
         * @note Positions are relative to (0, 0). Offset by desired screen position when rendering.
         * @note Empty string returns empty vector.
         * @note Missing glyphs are skipped (no rendering).
         *
         * @code
         * auto* font = FontManager::GetDefaultFont();
         * LayoutOptions opts;
         * opts.max_width = 400.0f;  // Wrap at 400 pixels
         * opts.h_align = HorizontalAlign::Center;
         *
         * auto glyphs = TextLayout::Layout("Wrapped centered text", *font, opts);
         *
         * // Render positioned glyphs
         * float offsetX = 100.0f, offsetY = 50.0f;
         * for (const auto& g : glyphs) {
         *     BatchRenderer::SubmitQuad(
         *         {offsetX + g.position.x, offsetY + g.position.y,
         *          g.metrics->size.x, g.metrics->size.y},
         *         color, g.metrics->atlas_rect, font->GetTextureId()
         *     );
         * }
         * @endcode
         */
	static std::vector<PositionedGlyph>
	Layout(const std::string& text, const FontAtlas& font, const LayoutOptions& options);

	/**
         * @brief Measure text dimensions without performing full layout.
         *
         * Calculates the bounding box of text without generating positioned glyphs.
         * Useful for pre-calculating UI element sizes or centering text.
         *
         * @param text UTF-8 encoded text string
         * @param font Font atlas to use for measurements
         * @param max_width Maximum width before wrapping (0 = single line, no limit)
         * @return Bounding rectangle with width and height (x=0, y=0)
         *
         * @note Faster than Layout() for dimension queries only.
         * @note Respects word wrapping if max_width > 0.
         * @note Handles newline characters (\n).
         *
         * @code
         * auto* font = FontManager::GetDefaultFont();
         *
         * // Measure single-line text
         * auto bounds = TextLayout::MeasureText("Hello World", *font);
         * float textWidth = bounds.width;
         * float textHeight = bounds.height;
         *
         * // Measure with wrapping
         * auto wrappedBounds = TextLayout::MeasureText("Long text", *font, 300.0f);
         * // wrappedBounds.height will be taller if text wrapped
         * @endcode
         */
	static batch_renderer::Rectangle
	MeasureText(const std::string& text, const FontAtlas& font, float max_width = 0.0f);
};

/**
     * @brief UTF-8 text encoding utilities.
     *
     * Provides utilities for working with UTF-8 encoded text, including
     * decoding to Unicode codepoints for glyph rendering.
     */
namespace utf8 {
/**
         * @brief Decode UTF-8 string into Unicode codepoints.
         *
         * Converts a UTF-8 encoded string into a vector of 32-bit Unicode
         * codepoint values. Handles 1-4 byte UTF-8 sequences correctly.
         *
         * @param utf8_string UTF-8 encoded input string
         * @return Vector of Unicode codepoint values
         *
         * @note Invalid UTF-8 sequences are skipped (not included in output).
         * @note ASCII characters (0-127) are decoded as-is.
         * @note Supports full Unicode range (U+0000 to U+10FFFF).
         *
         * @code
         * std::string text = "Café";  // Contains UTF-8 character é (U+00E9)
         * auto codepoints = utf8::Decode(text);
         * // codepoints = {67, 97, 102, 233}  ('C', 'a', 'f', 'é')
         *
         * for (uint32_t cp : codepoints) {
         *     auto* glyph = font->GetGlyph(cp);
         *     // Render glyph...
         * }
         * @endcode
         */
std::vector<uint32_t> Decode(const std::string& utf8_string);
} // namespace utf8

/**
     * @brief Font manager for centralized font loading and caching.
     *
     * FontManager provides a singleton interface for loading and caching fonts.
     * Fonts are cached by (path, size) to avoid redundant loading and atlas generation.
     *
     * The manager maintains a default font for convenient text rendering without
     * explicit font management. Each font size requires a separate atlas, so multiple
     * sizes of the same font create independent FontAtlas instances.
     *
     * **Lifecycle:**
     * 1. Initialize() - Set default font (call once at startup)
     * 2. GetFont() / GetDefaultFont() - Access fonts as needed
     * 3. Shutdown() - Clean up all fonts (call once at shutdown)
     *
     * @note FontManager must be initialized before BatchRenderer::SubmitText() can be used.
     * @note All returned FontAtlas pointers are managed by FontManager (do not delete).
     *
     * @code
     * // Initialize at startup
     * FontManager::Initialize("assets/fonts/Roboto-Regular.ttf", 16);
     *
     * // Use default font
     * auto* font = FontManager::GetDefaultFont();
     *
     * // Load additional fonts
     * auto* titleFont = FontManager::GetFont("assets/fonts/Roboto-Bold.ttf", 32);
     * auto* smallFont = FontManager::GetFont("assets/fonts/Roboto-Regular.ttf", 12);
     *
     * // Cleanup at shutdown
     * FontManager::Shutdown();
     * @endcode
     *
     * @see FontAtlas
     */
class FontManager {
public:
	/**
         * @brief Initialize the font manager with a default font.
         *
         * Sets up the font manager and loads the default font for text rendering.
         * This must be called before any text rendering operations.
         *
         * @param default_font_path Path to default TrueType (.ttf) font file
         * @param default_font_size Default font size in pixels (default: 16)
         *
         * @note Call only once at application startup.
         * @note If font loading fails, GetDefaultFont() will return nullptr.
         * @note Subsequent calls are ignored (does not re-initialize).
         *
         * @code
         * // Initialize with 16px Roboto font
         * FontManager::Initialize("assets/fonts/Roboto-Regular.ttf");
         *
         * // Initialize with custom size
         * FontManager::Initialize("assets/fonts/Arial.ttf", 24);
         * @endcode
         *
         * @see GetDefaultFont
         */
	static void Initialize(const std::string& default_font_path, int default_font_size = 16);

	/**
         * @brief Shutdown and cleanup all loaded fonts.
         *
         * Releases all cached FontAtlas instances and their GPU resources.
         * Call this once at application shutdown.
         *
         * @warning All FontAtlas pointers become invalid after shutdown.
         * @note Safe to call even if not initialized.
         *
         * @code
         * // At application shutdown
         * FontManager::Shutdown();
         * @endcode
         */
	static void Shutdown();

	/**
         * @brief Get the default font.
         *
         * Returns the font configured during Initialize(). This is the font
         * used by BatchRenderer::SubmitText() for convenience.
         *
         * @return Pointer to default font, or nullptr if not initialized or load failed
         *
         * @note Pointer is valid until Shutdown() is called.
         * @note Always check for nullptr before use.
         *
         * @code
         * auto* font = FontManager::GetDefaultFont();
         * if (font && font->IsValid()) {
         *     auto bounds = TextLayout::MeasureText("Hello", *font);
         * }
         * @endcode
         *
         * @see Initialize
         */
	static FontAtlas* GetDefaultFont();

	/**
         * @brief Get or load a font with specific size.
         *
         * Returns a cached font if already loaded, otherwise loads the font
         * and caches it. Fonts are uniquely identified by (path, size) pairs,
         * so the same font file at different sizes creates separate atlases.
         *
         * @param font_path Path to TrueType (.ttf) font file
         * @param font_size Font size in pixels
         * @return Pointer to font atlas, or nullptr if loading failed
         *
         * @note First call for a (path, size) loads the font; subsequent calls return cached atlas.
         * @note Pointer is valid until Shutdown() is called.
         * @note Each font size requires ~256 KB GPU memory.
         *
         * @code
         * // Load or get cached 24px title font
         * auto* titleFont = FontManager::GetFont("assets/fonts/Roboto-Bold.ttf", 24);
         *
         * // Same font, different size = separate atlas
         * auto* smallTitle = FontManager::GetFont("assets/fonts/Roboto-Bold.ttf", 16);
         *
         * // Use the font
         * if (titleFont && titleFont->IsValid()) {
         *     BatchRenderer::SubmitText("Title", 100, 50, 24, color);
         * }
         * @endcode
         *
         * @see FontAtlas
         */
	static FontAtlas* GetFont(const std::string& font_path, int font_size);

	static std::string GetDefaultFontPath() { return default_font_path_; }

private:
	struct FontKey {
		std::string path;
		int size;

		bool operator==(const FontKey& other) const { return path == other.path && size == other.size; }
	};

	struct FontKeyHash {
		size_t operator()(const FontKey& key) const {
			return std::hash<std::string>()(key.path) ^ (std::hash<int>()(key.size) << 1);
		}
	};

	static std::unordered_map<FontKey, std::unique_ptr<FontAtlas>, FontKeyHash> fonts_;
	static std::string default_font_path_;
	static int default_font_size_;
	static bool initialized_;
};
} // namespace engine::ui::text_renderer
