module;

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// stb_rect_pack for improved packing (include before stb_truetype)
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

// stb_truetype for font loading
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

module engine.ui;

import :text_renderer;
import engine.ui.batch_renderer;
import engine.rendering;
import engine.platform;
import engine.assets;

namespace engine::ui::text_renderer {

// Helper to store stbtt_packedchar data
struct PackedCharData {
	stbtt_packedchar packed_chars[224]; // ASCII 32-255 (224 characters)
};

// FontAtlas implementation
FontAtlas::FontAtlas(const std::string& font_path, int font_size_px) : font_size_(font_size_px) {

	// Load font file through asset manager
	auto font_data_opt = assets::AssetManager::LoadBinaryFile(font_path);
	if (!font_data_opt.has_value()) {
		// Failed to load font file
		return;
	}

	const std::vector<uint8_t>& file_data = font_data_opt.value();

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

	atlas_width_ = 512;
	atlas_height_ = 512;
	std::vector<uint8_t> atlas_bitmap(atlas_width_ * atlas_height_, 0);

	PackedCharData packed_data;
	stbtt_pack_context pack_context;

	if (!stbtt_PackBegin(&pack_context, atlas_bitmap.data(), atlas_width_, atlas_height_, 0, 1, nullptr)) {
		return;
	}

	stbtt_PackSetOversampling(&pack_context, 2, 2);

	stbtt_pack_range range;
	range.font_size = static_cast<float>(font_size_px);
	range.first_unicode_codepoint_in_range = 32;
	range.array_of_unicode_codepoints = nullptr;
	range.num_chars = 224;
	range.chardata_for_range = packed_data.packed_chars;

	if (!stbtt_PackFontRanges(&pack_context, file_data.data(), 0, &range, 1)) {
		// Packing failed, but continue with what we have
	}

	stbtt_PackEnd(&pack_context);

	// stbtt_packedchar stores pixel coordinates in the atlas texture
	for (int i = 0; i < 224; ++i) {
		const uint32_t codepoint = 32 + i;
		const stbtt_packedchar& pc = packed_data.packed_chars[i];

		// Normalize pixel coordinates to [0,1] range
		float uv_x = pc.x0 / static_cast<float>(atlas_width_);
		float uv_y = pc.y0 / static_cast<float>(atlas_height_);
		float uv_w = (pc.x1 - pc.x0) / static_cast<float>(atlas_width_);
		float uv_h = (pc.y1 - pc.y0) / static_cast<float>(atlas_height_);

		GlyphMetrics metrics;
		metrics.atlas_rect = batch_renderer::Rectangle(uv_x, uv_y, uv_w, uv_h);
		metrics.bearing = batch_renderer::Vector2(pc.xoff, pc.yoff);
		metrics.advance = pc.xadvance;
		metrics.size = batch_renderer::Vector2(pc.xoff2 - pc.xoff, pc.yoff2 - pc.yoff);

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
			.generate_mipmaps = false};

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
		bool valid = false;
		const uint8_t byte = static_cast<uint8_t>(utf8_string[i]);

		if ((byte & 0x80) == 0) {
			// 1-byte character (ASCII)
			codepoint = byte;
			i += 1;
			valid = true;
		}
		else if ((byte & 0xE0) == 0xC0) {
			// 2-byte character
			if (i + 1 < utf8_string.size() && (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80) {
				codepoint = ((byte & 0x1F) << 6) | (static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F);
				i += 2;
				valid = true;
			}
			else {
				i += 1; // Invalid UTF-8, skip
			}
		}
		else if ((byte & 0xF0) == 0xE0) {
			// 3-byte character
			if (i + 2 < utf8_string.size() && (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80
				&& (static_cast<uint8_t>(utf8_string[i + 2]) & 0xC0) == 0x80) {
				codepoint = ((byte & 0x0F) << 12) | ((static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F) << 6)
							| (static_cast<uint8_t>(utf8_string[i + 2]) & 0x3F);
				i += 3;
				valid = true;
			}
			else {
				i += 1; // Invalid UTF-8, skip
			}
		}
		else if ((byte & 0xF8) == 0xF0) {
			// 4-byte character
			if (i + 3 < utf8_string.size() && (static_cast<uint8_t>(utf8_string[i + 1]) & 0xC0) == 0x80
				&& (static_cast<uint8_t>(utf8_string[i + 2]) & 0xC0) == 0x80
				&& (static_cast<uint8_t>(utf8_string[i + 3]) & 0xC0) == 0x80) {
				codepoint = ((byte & 0x07) << 18) | ((static_cast<uint8_t>(utf8_string[i + 1]) & 0x3F) << 12)
							| ((static_cast<uint8_t>(utf8_string[i + 2]) & 0x3F) << 6)
							| (static_cast<uint8_t>(utf8_string[i + 3]) & 0x3F);
				i += 4;
				valid = true;
			}
			else {
				i += 1; // Invalid UTF-8, skip
			}
		}
		else {
			// Invalid UTF-8 sequence, skip
			i += 1;
		}

		// Only push valid codepoints
		if (valid) {
			codepoints.push_back(codepoint);
		}
	}

	return codepoints;
}
} // namespace utf8

// TextLayout implementation
std::vector<PositionedGlyph>
TextLayout::Layout(const std::string& text, const FontAtlas& font, const LayoutOptions& options) {
	std::vector<PositionedGlyph> result;

	if (text.empty() || !font.IsValid()) {
		return result;
	}

	// Decode UTF-8
	auto codepoints = utf8::Decode(text);

	float cursor_x = 0.0f;
	float cursor_y = font.GetAscent(); // Start at baseline
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
				}
				else if (options.h_align == HorizontalAlign::Right) {
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
				}
				else if (options.h_align == HorizontalAlign::Right) {
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
		pg.position = batch_renderer::Vector2(cursor_x + glyph->bearing.x, cursor_y + glyph->bearing.y);
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
			}
			else if (options.h_align == HorizontalAlign::Right) {
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
		}
		else if (options.v_align == VerticalAlign::Bottom) {
			offset_y = -total_height;
		}

		for (auto& glyph : result) {
			glyph.position.y += offset_y;
		}
	}

	return result;
}

batch_renderer::Rectangle TextLayout::MeasureText(const std::string& text, const FontAtlas& font, float max_width) {
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
} // namespace engine::ui::text_renderer
