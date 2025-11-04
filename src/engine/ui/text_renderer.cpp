module;

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <optional>
#include <fstream>

// stb_truetype for font loading
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

module engine.ui;

import :text_renderer;
import engine.ui.batch_renderer;
import engine.rendering;
import engine.platform;

namespace engine::ui::text_renderer {
    
    // Atlas packing helper
    class AtlasPacker {
    public:
        AtlasPacker(int width, int height)
            : width_(width), height_(height), current_y_(0), current_x_(0), row_height_(0) {}

        std::optional<batch_renderer::Rectangle> Pack(int w, int h) {
            // Check if current row has space
            if (current_x_ + w > width_) {
                // Move to next row
                current_x_ = 0;
                current_y_ += row_height_;
                row_height_ = 0;
            }

            // Check if atlas is full
            if (current_y_ + h > height_) {
                return std::nullopt;
            }

            // Allocate space
            batch_renderer::Rectangle rect(
                static_cast<float>(current_x_),
                static_cast<float>(current_y_),
                static_cast<float>(w),
                static_cast<float>(h)
            );
            current_x_ += w;
            row_height_ = std::max(row_height_, h);

            return rect;
        }

    private:
        int width_, height_;
        int current_x_, current_y_, row_height_;
    };

    // FontAtlas implementation
    FontAtlas::FontAtlas(const std::string& font_path, int font_size_px)
        : font_size_(font_size_px) {
        
        // Load font file
        platform::fs::File file;
        if (!file.Open(font_path, platform::fs::FileMode::Read)) {
            // Failed to open font file
            return;
        }
        
        const std::vector<uint8_t> file_data = file.ReadAll();
        if (file_data.empty()) {
            // Failed to load font
            return;
        }

        // Initialize stb_truetype
        stbtt_fontinfo font_info;
        if (!stbtt_InitFont(&font_info, file_data.data(), 0)) {
            // Failed to parse font
            return;
        }

        // Calculate scale for desired pixel size
        const float scale = stbtt_ScaleForPixelHeight(&font_info, static_cast<float>(font_size_px));

        // Get font vertical metrics
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
        ascent_ = static_cast<float>(ascent) * scale;
        descent_ = static_cast<float>(descent) * scale;
        line_gap_ = static_cast<float>(line_gap) * scale;

        // Create atlas texture (512x512 should be enough for basic ASCII + some extended chars)
        atlas_width_ = 512;
        atlas_height_ = 512;
        std::vector<uint8_t> atlas_bitmap(atlas_width_ * atlas_height_, 0);

        AtlasPacker packer(atlas_width_, atlas_height_);

        // Rasterize common ASCII and Latin-1 characters (32-255)
        for (uint32_t codepoint = 32; codepoint < 256; ++codepoint) {
            const int glyph_index = stbtt_FindGlyphIndex(&font_info, static_cast<int>(codepoint));
            if (glyph_index == 0 && codepoint != 0) {
                continue; // Glyph not found in font
            }

            // Get glyph bounding box
            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&font_info, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

            const int glyph_width = x1 - x0;
            const int glyph_height = y1 - y0;

            // Skip empty glyphs (like space)
            if (glyph_width == 0 || glyph_height == 0) {
                // Still store metrics for spacing
                int advance, left_bearing;
                stbtt_GetGlyphHMetrics(&font_info, glyph_index, &advance, &left_bearing);

                GlyphMetrics metrics;
                metrics.atlas_rect = batch_renderer::Rectangle(0, 0, 0, 0);
                metrics.bearing = batch_renderer::Vector2(
                    static_cast<float>(left_bearing) * scale,
                    0.0f
                );
                metrics.advance = static_cast<float>(advance) * scale;
                metrics.size = batch_renderer::Vector2(0, 0);

                glyphs_[codepoint] = metrics;
                continue;
            }

            // Try to pack glyph into atlas
            auto pack_result = packer.Pack(glyph_width + 2, glyph_height + 2); // +2 for padding
            if (!pack_result.has_value()) {
                // Atlas full, skip remaining glyphs
                break;
            }

            const auto& rect = pack_result.value();

            // Rasterize glyph into atlas
            const int pixel_x = static_cast<int>(rect.x) + 1; // +1 for padding
            const int pixel_y = static_cast<int>(rect.y) + 1;

            stbtt_MakeGlyphBitmap(
                &font_info,
                atlas_bitmap.data() + pixel_y * atlas_width_ + pixel_x,
                glyph_width,
                glyph_height,
                atlas_width_,
                scale,
                scale,
                glyph_index
            );

            // Get glyph metrics
            int advance, left_bearing;
            stbtt_GetGlyphHMetrics(&font_info, glyph_index, &advance, &left_bearing);

            // Store glyph metrics
            GlyphMetrics metrics;
            metrics.atlas_rect = batch_renderer::Rectangle(
                static_cast<float>(pixel_x) / static_cast<float>(atlas_width_),
                static_cast<float>(pixel_y) / static_cast<float>(atlas_height_),
                static_cast<float>(glyph_width) / static_cast<float>(atlas_width_),
                static_cast<float>(glyph_height) / static_cast<float>(atlas_height_)
            );
            metrics.bearing = batch_renderer::Vector2(
                static_cast<float>(x0),
                static_cast<float>(-y0) // Flip Y for screen coordinates
            );
            metrics.advance = static_cast<float>(advance) * scale;
            metrics.size = batch_renderer::Vector2(
                static_cast<float>(glyph_width),
                static_cast<float>(glyph_height)
            );

            glyphs_[codepoint] = metrics;
        }

        // Upload atlas texture to GPU
        auto& renderer = rendering::GetRenderer();
        auto& texture_mgr = renderer.GetTextureManager();

        // Convert grayscale to RGBA (white RGB, grayscale in alpha channel)
        std::vector<uint8_t> rgba_bitmap;
        rgba_bitmap.reserve(atlas_bitmap.size() * 4);
        for (uint8_t gray : atlas_bitmap) {
            rgba_bitmap.push_back(255); // R
            rgba_bitmap.push_back(255); // G
            rgba_bitmap.push_back(255); // B
            rgba_bitmap.push_back(gray); // A (coverage)
        }

        rendering::TextureCreateInfo tex_info{};
        tex_info.width = static_cast<uint32_t>(atlas_width_);
        tex_info.height = static_cast<uint32_t>(atlas_height_);
        tex_info.format = rendering::TextureFormat::RGBA8;
        tex_info.data = rgba_bitmap.data();
        tex_info.parameters = {
            .min_filter = rendering::TextureFilter::Linear,
            .mag_filter = rendering::TextureFilter::Linear,
            .wrap_s = rendering::TextureWrap::ClampToEdge,
            .wrap_t = rendering::TextureWrap::ClampToEdge,
            .generate_mipmaps = false
        };

        atlas_texture_id_ = texture_mgr.CreateTexture(font_path + "_atlas_" + std::to_string(font_size_px), tex_info);
    }

    FontAtlas::~FontAtlas() {
        if (atlas_texture_id_ != 0) {
            auto& renderer = rendering::GetRenderer();
            auto& texture_mgr = renderer.GetTextureManager();
            texture_mgr.DestroyTexture(atlas_texture_id_);
        }
    }

    const GlyphMetrics* FontAtlas::GetGlyph(uint32_t codepoint) const {
        auto it = glyphs_.find(codepoint);
        if (it != glyphs_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // UTF-8 decoding implementation
    namespace utf8 {
        std::vector<uint32_t> Decode(const std::string& utf8_string) {
            std::vector<uint32_t> codepoints;
            codepoints.reserve(utf8_string.size()); // Conservative estimate

            for (size_t i = 0; i < utf8_string.size();) {
                uint32_t codepoint = 0;
                const uint8_t byte = static_cast<uint8_t>(utf8_string[i]);

                if ((byte & 0x80) == 0) {
                    // 1-byte character (ASCII)
                    codepoint = byte;
                    i += 1;
                } else if ((byte & 0xE0) == 0xC0) {
                    // 2-byte character
                    if (i + 1 < utf8_string.size() && 
                        (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80) {
                        codepoint = ((byte & 0x1F) << 6) |
                                    (static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F);
                        i += 2;
                    } else {
                        i += 1; // Invalid UTF-8, skip
                    }
                } else if ((byte & 0xF0) == 0xE0) {
                    // 3-byte character
                    if (i + 2 < utf8_string.size() &&
                        (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80 &&
                        (static_cast<uint8_t>(utf8_string[i + 2]) & 0xC0) == 0x80) {
                        codepoint = ((byte & 0x0F) << 12) |
                                    ((static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F) << 6) |
                                    (static_cast<uint8_t>(utf8_string[i + 2]) & 0x3F);
                        i += 3;
                    } else {
                        i += 1; // Invalid UTF-8, skip
                    }
                } else if ((byte & 0xF8) == 0xF0) {
                    // 4-byte character
                    if (i + 3 < utf8_string.size() &&
                        (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80 &&
                        (static_cast<uint8_t>(utf8_string[i + 2]) & 0xC0) == 0x80 &&
                        (static_cast<uint8_t>(utf8_string[i + 3]) & 0xC0) == 0x80) {
                        codepoint = ((byte & 0x07) << 18) |
                                    ((static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F) << 12) |
                                    ((static_cast<uint8_t>(utf8_string[i + 2]) & 0x3F) << 6) |
                                    (static_cast<uint8_t>(utf8_string[i + 3]) & 0x3F);
                        i += 4;
                    } else {
                        i += 1; // Invalid UTF-8, skip
                    }
                } else {
                    // Invalid UTF-8 sequence, skip
                    i += 1;
                }

                codepoints.push_back(codepoint);
            }

            return codepoints;
        }
    }

    // TextLayout implementation
    std::vector<PositionedGlyph> TextLayout::Layout(
        const std::string& text,
        const FontAtlas& font,
        const LayoutOptions& options
    ) {
        std::vector<PositionedGlyph> result;
        
        if (text.empty() || !font.IsValid()) {
            return result;
        }

        // Decode UTF-8
        auto codepoints = utf8::Decode(text);

        float cursor_x = 0.0f;
        float cursor_y = font.GetAscent();  // Start at baseline
        result.reserve(codepoints.size());

        std::vector<PositionedGlyph> current_line;
        float line_width = 0.0f;

        for (uint32_t cp : codepoints) {
            // Handle newline
            if (cp == '\n') {
                // Apply alignment and add to result
                if (options.h_align != HorizontalAlign::Left && options.max_width > 0.0f) {
                    float offset = 0.0f;
                    if (options.h_align == HorizontalAlign::Center) {
                        offset = (options.max_width - line_width) * 0.5f;
                    } else if (options.h_align == HorizontalAlign::Right) {
                        offset = options.max_width - line_width;
                    }
                    for (auto& glyph : current_line) {
                        glyph.position.x += offset;
                    }
                }
                result.insert(result.end(), current_line.begin(), current_line.end());
                current_line.clear();

                cursor_x = 0.0f;
                cursor_y += font.GetLineHeight() * options.line_height;
                line_width = 0.0f;
                continue;
            }

            const GlyphMetrics* glyph = font.GetGlyph(cp);
            if (!glyph) {
                continue; // Skip missing glyphs (replacement char U+FFFD not in atlas)
            }

            // Check if line wrapping needed
            if (options.max_width > 0.0f && cursor_x + glyph->advance > options.max_width && !current_line.empty()) {
                // Apply alignment and flush current line
                if (options.h_align != HorizontalAlign::Left) {
                    float offset = 0.0f;
                    if (options.h_align == HorizontalAlign::Center) {
                        offset = (options.max_width - line_width) * 0.5f;
                    } else if (options.h_align == HorizontalAlign::Right) {
                        offset = options.max_width - line_width;
                    }
                    for (auto& g : current_line) {
                        g.position.x += offset;
                    }
                }
                result.insert(result.end(), current_line.begin(), current_line.end());
                current_line.clear();

                cursor_x = 0.0f;
                cursor_y += font.GetLineHeight() * options.line_height;
                line_width = 0.0f;
            }

            // Position glyph
            PositionedGlyph pg;
            pg.codepoint = cp;
            pg.position = batch_renderer::Vector2(
                cursor_x + glyph->bearing.x,
                cursor_y + glyph->bearing.y
            );
            pg.metrics = glyph;

            current_line.push_back(pg);
            cursor_x += glyph->advance;
            line_width = cursor_x;
        }

        // Flush last line
        if (!current_line.empty()) {
            if (options.h_align != HorizontalAlign::Left && options.max_width > 0.0f) {
                float offset = 0.0f;
                if (options.h_align == HorizontalAlign::Center) {
                    offset = (options.max_width - line_width) * 0.5f;
                } else if (options.h_align == HorizontalAlign::Right) {
                    offset = options.max_width - line_width;
                }
                for (auto& glyph : current_line) {
                    glyph.position.x += offset;
                }
            }
            result.insert(result.end(), current_line.begin(), current_line.end());
        }

        // Apply vertical alignment (if needed)
        if (options.v_align != VerticalAlign::Top) {
            float total_height = cursor_y + font.GetDescent();
            float offset_y = 0.0f;
            
            // Note: This only makes sense when rendering into a bounded rect
            // For now, just implement the logic
            if (options.v_align == VerticalAlign::Center) {
                offset_y = -total_height * 0.5f;
            } else if (options.v_align == VerticalAlign::Bottom) {
                offset_y = -total_height;
            }

            for (auto& glyph : result) {
                glyph.position.y += offset_y;
            }
        }

        return result;
    }

    batch_renderer::Rectangle TextLayout::MeasureText(
        const std::string& text,
        const FontAtlas& font,
        float max_width
    ) {
        if (text.empty() || !font.IsValid()) {
            return batch_renderer::Rectangle(0, 0, 0, 0);
        }

        // Decode UTF-8
        auto codepoints = utf8::Decode(text);

        float cursor_x = 0.0f;
        float max_line_width = 0.0f;
        int line_count = 1;
        bool has_content = false;

        for (uint32_t cp : codepoints) {
            // Handle newline
            if (cp == '\n') {
                max_line_width = std::max(max_line_width, cursor_x);
                cursor_x = 0.0f;
                line_count++;
                has_content = false;
                continue;
            }

            const GlyphMetrics* glyph = font.GetGlyph(cp);
            if (!glyph) {
                continue;
            }

            // Check wrapping (use same condition as Layout for consistency)
            if (max_width > 0.0f && cursor_x + glyph->advance > max_width && has_content) {
                max_line_width = std::max(max_line_width, cursor_x);
                cursor_x = 0.0f;
                line_count++;
                has_content = false;
            }

            cursor_x += glyph->advance;
            has_content = true;
        }

        max_line_width = std::max(max_line_width, cursor_x);

        const float total_height = static_cast<float>(line_count) * font.GetLineHeight();

        return batch_renderer::Rectangle(0, 0, max_line_width, total_height);
    }

    // FontManager static members
    std::unordered_map<FontManager::FontKey, std::unique_ptr<FontAtlas>, FontManager::FontKeyHash> FontManager::fonts_;
    std::string FontManager::default_font_path_;
    int FontManager::default_font_size_ = 16;
    bool FontManager::initialized_ = false;

    void FontManager::Initialize(const std::string& default_font_path, int default_font_size) {
        if (initialized_) {
            return;
        }

        default_font_path_ = default_font_path;
        default_font_size_ = default_font_size;

        // Pre-load default font
        GetFont(default_font_path, default_font_size);

        initialized_ = true;
    }

    void FontManager::Shutdown() {
        fonts_.clear();
        initialized_ = false;
    }

    FontAtlas* FontManager::GetDefaultFont() {
        if (!initialized_) {
            return nullptr;
        }
        return GetFont(default_font_path_, default_font_size_);
    }

    FontAtlas* FontManager::GetFont(const std::string& font_path, int font_size) {
        FontKey key{font_path, font_size};

        auto it = fonts_.find(key);
        if (it != fonts_.end()) {
            return it->second.get();
        }

        // Load font
        auto font = std::make_unique<FontAtlas>(font_path, font_size);
        if (!font->IsValid()) {
            return nullptr;
        }

        FontAtlas* font_ptr = font.get();
        fonts_[key] = std::move(font);

        return font_ptr;
    }
}
