# MOD_UI_TEXT_RENDERING_v1

> **Text Rendering System for Batch UI Renderer - Foundation Module**

## Executive Summary

The UI text rendering module provides high-performance text rendering capabilities for the citrus-engine batch UI renderer. This module implements a font atlas system using signed distance field (SDF) rendering for crisp, resolution-independent text across all platforms. The system integrates seamlessly with the existing vertex-based batch renderer, treating text as textured quads batched alongside other UI primitives for optimal GPU efficiency. Text rendering supports multiple fonts, sizes, colors, alignment, and clipping, enabling rich typography for menus, HUD elements, tooltips, and in-game UI.

## Scope and Objectives

### In Scope

- [ ] Font atlas generation from TrueType/OpenType fonts using SDF technique
- [ ] Glyph metrics calculation (advance, bearing, kerning)
- [ ] Text layout engine (single-line and multi-line with wrapping)
- [ ] Integration with existing BatchRenderer vertex batching system
- [ ] Text alignment (left, center, right, top, bottom)
- [ ] UTF-8 text encoding support for internationalization
- [ ] Font caching and atlas texture management
- [ ] Text measurement utilities (width, height, line count)

### Out of Scope

- [ ] Rich text markup (HTML/Markdown styling)
- [ ] Advanced typography (ligatures, complex scripts like Arabic)
- [ ] Text editing widgets (cursors, selection - handled by UI components)
- [ ] Emoji rendering (future extension)
- [ ] Runtime font glyph generation (all glyphs pre-baked into atlas)
- [ ] Variable fonts and font animation

### Primary Objectives

1. **High Performance**: Render 10,000+ characters at 60 FPS with minimal draw calls
2. **Visual Quality**: Crisp text at any scale using SDF rendering technique
3. **Minimal Memory**: Font atlases under 2MB per font, shared across all UI

### Secondary Objectives

- Sub-millisecond text layout for dynamic UI updates
- Automatic atlas regeneration when new glyphs needed
- Support 4+ fonts loaded simultaneously without performance impact

## Architecture/Design

### High-Level Overview

```
Font Loading Pipeline:
  TrueType Font (.ttf) → stb_truetype Parser → SDF Generator → Atlas Packer → GPU Texture
                                                                                    ↓
Text Rendering Pipeline:                                                    Font Atlas Texture
  Text String → Layout Engine → Glyph Lookup → Quad Generation → BatchRenderer SubmitQuad()
                     ↓              ↓               ↓                           ↓
                 Line Breaking   UV Coords      Vertex Data              GPU Draw Batches
```

### Core Components

#### Component 1: Font Atlas System

- **Purpose**: Manage font atlas textures containing all glyphs for each font
- **Responsibilities**:
  - Load TrueType fonts using stb_truetype
  - Generate signed distance field (SDF) bitmaps for each glyph
  - Pack glyph bitmaps into texture atlases using rectangle packing
  - Store glyph metrics (advance, bearing, texture coordinates)
  - Manage texture uploads to GPU
- **Key Classes/Interfaces**:
  ```cpp
  class FontAtlas {
  public:
      // Load font from file, generate atlas texture
      FontAtlas(const std::string& font_path, int font_size_px);
      
      // Get glyph metrics for character
      const GlyphMetrics* GetGlyph(uint32_t codepoint) const;
      
      // Get atlas texture ID for BatchRenderer
      uint32_t GetTextureId() const;
      
      // Get font vertical metrics
      float GetAscent() const;
      float GetDescent() const;
      float GetLineGap() const;
      
  private:
      std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
      uint32_t atlas_texture_id_;
      float ascent_, descent_, line_gap_;
  };
  
  struct GlyphMetrics {
      Rectangle atlas_rect;  // UV coordinates in atlas texture (normalized 0-1)
      Vector2 bearing;       // Offset from baseline to left/top of glyph
      float advance;         // Horizontal advance to next glyph
      Vector2 size;          // Glyph bitmap dimensions
  };
  ```
- **Data Flow**: Font file → stb_truetype → SDF bitmap → Atlas packer → GPU texture

#### Component 2: Text Layout Engine

- **Purpose**: Convert text strings into positioned glyph quads ready for rendering
- **Responsibilities**:
  - UTF-8 decoding to Unicode codepoints
  - Line breaking and text wrapping
  - Horizontal alignment (left, center, right)
  - Vertical alignment (top, center, bottom)
  - Calculate text bounding boxes
  - Handle newlines and whitespace
- **Key Classes/Interfaces**:
  ```cpp
  class TextLayout {
  public:
      struct LayoutOptions {
          HorizontalAlign h_align = HorizontalAlign::Left;
          VerticalAlign v_align = VerticalAlign::Top;
          float max_width = 0.0f;  // 0 = no wrapping
          float line_height = 1.0f;  // Multiplier of font line height
      };
      
      // Layout text string, return positioned glyphs
      std::vector<PositionedGlyph> Layout(
          const std::string& text,
          const FontAtlas& font,
          const LayoutOptions& options
      );
      
      // Measure text dimensions without full layout
      Rectangle MeasureText(
          const std::string& text,
          const FontAtlas& font,
          float max_width = 0.0f
      );
  };
  
  struct PositionedGlyph {
      uint32_t codepoint;
      Vector2 position;     // Screen-space position (top-left)
      const GlyphMetrics* metrics;
  };
  ```
- **Data Flow**: Text string → UTF-8 decode → Glyph lookup → Position calculation → PositionedGlyph array

#### Component 3: BatchRenderer Text Integration

- **Purpose**: Submit text as textured quads to existing BatchRenderer
- **Responsibilities**:
  - Convert PositionedGlyph array to BatchRenderer quad submissions
  - Apply text color uniformly to all glyphs
  - Handle font atlas texture binding
  - Support scissor clipping for text in bounded rects
- **Key Classes/Interfaces**:
  ```cpp
  // Existing BatchRenderer methods to implement:
  
  static void SubmitText(
      const std::string& text,
      float x,
      float y,
      int font_size,
      const Color& color
  ) {
      // 1. Get font atlas for requested size
      // 2. Layout text at (x, y)
      // 3. For each glyph, submit quad with atlas texture UV coords
      // 4. BatchRenderer automatically batches all quads
  }
  
  static void SubmitTextRect(
      const Rectangle& rect,
      const std::string& text,
      int font_size,
      const Color& color
  ) {
      // 1. Get font atlas
      // 2. Layout text with max_width = rect.width, wrapping enabled
      // 3. Apply alignment within rect
      // 4. Push scissor rect for clipping
      // 5. Submit glyph quads
      // 6. Pop scissor rect
  }
  ```
- **Data Flow**: PositionedGlyph array → For each glyph → BatchRenderer::SubmitQuad() with atlas UV

#### Component 4: Font Manager

- **Purpose**: Centralized font loading, caching, and atlas management
- **Responsibilities**:
  - Load fonts on demand
  - Cache FontAtlas instances by (font_path, font_size)
  - Provide default fallback font
  - Manage font atlas texture lifecycle
- **Key Classes/Interfaces**:
  ```cpp
  class FontManager {
  public:
      // Initialize with default font
      void Initialize(const std::string& default_font_path);
      
      // Get or load font atlas
      FontAtlas* GetFont(const std::string& font_path, int font_size);
      
      // Get default font
      FontAtlas* GetDefaultFont(int font_size);
      
      // Cleanup all fonts
      void Shutdown();
      
  private:
      struct FontKey {
          std::string path;
          int size;
          bool operator==(const FontKey& other) const;
      };
      
      std::unordered_map<FontKey, std::unique_ptr<FontAtlas>> fonts_;
      std::string default_font_path_;
  };
  ```
- **Data Flow**: Font request → Cache lookup → (Miss: Load + atlas generation) → Return FontAtlas*

### Signed Distance Field (SDF) Rendering

#### Why SDF?

Traditional bitmap fonts become pixelated when scaled. SDF encoding stores the **distance to the nearest edge** in each pixel, allowing sharp edges at any scale with simple fragment shader logic.

#### SDF Generation

```cpp
// Pseudo-code for SDF bitmap generation
std::vector<float> GenerateSDF(const std::vector<uint8_t>& bitmap, int width, int height) {
    std::vector<float> sdf(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float min_dist = FindMinDistanceToEdge(bitmap, x, y, width, height);
            
            // Normalize distance to 0-1 range (0.5 = edge)
            bool inside = bitmap[y * width + x] > 128;
            sdf[y * width + x] = inside ? (0.5f + min_dist) : (0.5f - min_dist);
        }
    }
    
    return sdf;
}
```

#### SDF Fragment Shader

```glsl
// ui_text.frag - SDF text rendering fragment shader
#version 300 es
precision mediump float;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_TexIndex;

uniform sampler2D u_Textures[8];

out vec4 FragColor;

void main() {
    // Sample SDF texture
    float distance = texture(u_Textures[int(v_TexIndex)], v_TexCoord).a;
    
    // Anti-aliased edge detection
    float smoothing = 0.25 / (dFdx(v_TexCoord.x) * textureSize(u_Textures[0], 0).x);
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    
    FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
}
```

### UTF-8 Decoding

```cpp
// Decode UTF-8 string to Unicode codepoints
std::vector<uint32_t> DecodeUTF8(const std::string& utf8_string) {
    std::vector<uint32_t> codepoints;
    
    for (size_t i = 0; i < utf8_string.size();) {
        uint32_t codepoint = 0;
        uint8_t byte = utf8_string[i];
        
        if ((byte & 0x80) == 0) {
            // 1-byte character (ASCII)
            codepoint = byte;
            i += 1;
        } else if ((byte & 0xE0) == 0xC0) {
            // 2-byte character
            codepoint = ((byte & 0x1F) << 6) | (utf8_string[i+1] & 0x3F);
            i += 2;
        } else if ((byte & 0xF0) == 0xE0) {
            // 3-byte character
            codepoint = ((byte & 0x0F) << 12) | 
                       ((utf8_string[i+1] & 0x3F) << 6) | 
                       (utf8_string[i+2] & 0x3F);
            i += 3;
        } else if ((byte & 0xF8) == 0xF0) {
            // 4-byte character
            codepoint = ((byte & 0x07) << 18) | 
                       ((utf8_string[i+1] & 0x3F) << 12) | 
                       ((utf8_string[i+2] & 0x3F) << 6) | 
                       (utf8_string[i+3] & 0x3F);
            i += 4;
        } else {
            // Invalid UTF-8, skip
            i += 1;
        }
        
        codepoints.push_back(codepoint);
    }
    
    return codepoints;
}
```

### Atlas Packing Algorithm

Use a simple shelf-based rectangle packing algorithm:

```cpp
class AtlasPacker {
public:
    AtlasPacker(int width, int height) 
        : width_(width), height_(height), current_y_(0), current_x_(0), row_height_(0) {}
    
    // Attempt to pack rectangle, return position or nullopt if full
    std::optional<Rectangle> Pack(int w, int h) {
        // Check if current row has space
        if (current_x_ + w > width_) {
            // Move to next row
            current_x_ = 0;
            current_y_ += row_height_;
            row_height_ = 0;
        }
        
        // Check if atlas is full
        if (current_y_ + h > height_) {
            return std::nullopt;  // Atlas full
        }
        
        // Allocate space
        Rectangle rect(current_x_, current_y_, w, h);
        current_x_ += w;
        row_height_ = std::max(row_height_, h);
        
        return rect;
    }
    
private:
    int width_, height_;
    int current_x_, current_y_, row_height_;
};
```

### Text Layout Algorithm

```cpp
std::vector<PositionedGlyph> TextLayout::Layout(
    const std::string& text,
    const FontAtlas& font,
    const LayoutOptions& options
) {
    std::vector<PositionedGlyph> result;
    std::vector<uint32_t> codepoints = DecodeUTF8(text);
    
    float cursor_x = 0.0f;
    float cursor_y = font.GetAscent();  // Start at baseline
    
    std::vector<PositionedGlyph> current_line;
    float line_width = 0.0f;
    
    for (uint32_t cp : codepoints) {
        // Handle newline
        if (cp == '\n') {
            ApplyAlignment(current_line, line_width, options.h_align, options.max_width);
            result.insert(result.end(), current_line.begin(), current_line.end());
            current_line.clear();
            
            cursor_x = 0.0f;
            cursor_y += font.GetLineHeight() * options.line_height;
            line_width = 0.0f;
            continue;
        }
        
        const GlyphMetrics* glyph = font.GetGlyph(cp);
        if (!glyph) continue;  // Skip missing glyphs
        
        // Check if line wrapping needed
        if (options.max_width > 0.0f && cursor_x + glyph->advance > options.max_width) {
            ApplyAlignment(current_line, line_width, options.h_align, options.max_width);
            result.insert(result.end(), current_line.begin(), current_line.end());
            current_line.clear();
            
            cursor_x = 0.0f;
            cursor_y += font.GetLineHeight() * options.line_height;
            line_width = 0.0f;
        }
        
        // Position glyph
        PositionedGlyph pg;
        pg.codepoint = cp;
        pg.position = Vector2(cursor_x + glyph->bearing.x, cursor_y + glyph->bearing.y);
        pg.metrics = glyph;
        
        current_line.push_back(pg);
        cursor_x += glyph->advance;
        line_width = cursor_x;
    }
    
    // Flush last line
    if (!current_line.empty()) {
        ApplyAlignment(current_line, line_width, options.h_align, options.max_width);
        result.insert(result.end(), current_line.begin(), current_line.end());
    }
    
    // Apply vertical alignment
    if (options.v_align != VerticalAlign::Top) {
        float total_height = cursor_y + font.GetDescent();
        // ... shift all glyphs vertically ...
    }
    
    return result;
}
```

## Implementation Plan

### Phase 1: Foundation (Week 1)

1. **Font Atlas System**
   - Implement `FontAtlas` class with stb_truetype integration
   - Implement basic glyph rasterization (no SDF yet)
   - Implement shelf-based atlas packing algorithm
   - Upload atlas texture to GPU

2. **UTF-8 Decoding**
   - Implement `DecodeUTF8()` utility function
   - Add unit tests for UTF-8 edge cases

### Phase 2: Text Layout (Week 2)

3. **Text Layout Engine**
   - Implement `TextLayout` class with single-line layout
   - Add multi-line layout with wrapping
   - Implement alignment (horizontal and vertical)
   - Add `MeasureText()` utility

4. **BatchRenderer Integration**
   - Implement `SubmitText()` using layout engine
   - Implement `SubmitTextRect()` with scissor clipping
   - Ensure proper texture slot management

### Phase 3: SDF Enhancement (Week 3)

5. **SDF Generation**
   - Implement SDF bitmap generation algorithm
   - Add SDF fragment shader (`ui_text.frag`)
   - Test SDF rendering quality at various scales

6. **Font Manager**
   - Implement `FontManager` singleton with caching
   - Add default font loading
   - Integrate with BatchRenderer initialization

### Phase 4: Testing & Optimization (Week 4)

7. **Testing**
   - Unit tests for UTF-8 decoding
   - Unit tests for text layout (alignment, wrapping)
   - Integration tests with BatchRenderer
   - Visual tests for text rendering quality

8. **Optimization**
   - Profile text rendering performance
   - Optimize atlas packing efficiency
   - Minimize draw calls through batching
   - Memory usage profiling

## Integration with Existing Systems

### BatchRenderer Integration

Text rendering integrates with the existing `BatchRenderer` by submitting textured quads:

```cpp
void BatchRenderer::SubmitText(
    const std::string& text,
    float x,
    float y,
    int font_size,
    const Color& color
) {
    // Get font from manager
    FontAtlas* font = FontManager::GetInstance().GetDefaultFont(font_size);
    if (!font) return;
    
    // Layout text
    TextLayout::LayoutOptions options;
    auto glyphs = TextLayout::Layout(text, *font, options);
    
    // Submit each glyph as a textured quad
    uint32_t texture_id = font->GetTextureId();
    
    for (const auto& pg : glyphs) {
        Rectangle screen_rect(
            x + pg.position.x,
            y + pg.position.y,
            pg.metrics->size.x,
            pg.metrics->size.y
        );
        
        // Submit quad with glyph's UV coordinates
        SubmitQuad(screen_rect, color, pg.metrics->atlas_rect, texture_id);
    }
}
```

**Key Points:**
- Each glyph is a quad submission to `BatchRenderer::SubmitQuad()`
- All glyphs from same font share the same texture (batched automatically)
- Text color is applied via the `color` parameter
- No need to flush between glyphs - BatchRenderer handles batching

### Rendering Module Integration

The font atlas textures are managed through the existing `TextureManager`:

```cpp
// FontAtlas::FontAtlas() constructor
FontAtlas::FontAtlas(const std::string& font_path, int font_size_px) {
    // ... generate atlas bitmap ...
    
    auto& renderer = rendering::GetRenderer();
    auto& texture_mgr = renderer.GetTextureManager();
    
    rendering::TextureCreateInfo tex_info{};
    tex_info.width = atlas_width_;
    tex_info.height = atlas_height_;
    tex_info.format = rendering::TextureFormat::R8;  // Single-channel for SDF
    tex_info.data = atlas_bitmap_.data();
    tex_info.parameters = {
        .min_filter = rendering::TextureFilter::Linear,
        .mag_filter = rendering::TextureFilter::Linear,
        .wrap_s = rendering::TextureWrap::ClampToEdge,
        .wrap_t = rendering::TextureWrap::ClampToEdge,
        .generate_mipmaps = false
    };
    
    atlas_texture_id_ = texture_mgr.CreateTexture(font_path, tex_info);
}
```

### Asset Management

Fonts are loaded as engine assets:

```
assets/
└── fonts/
    ├── Roboto-Regular.ttf     (Default UI font)
    ├── Roboto-Bold.ttf        (Headings)
    └── RobotoMono-Regular.ttf (Monospace for debug)
```

Font loading via `FontManager`:
```cpp
FontManager::Initialize("assets/fonts/Roboto-Regular.ttf");
```

## Performance Considerations

### Memory Budget

- **Font Atlas Texture**: 512x512 R8 texture = 256 KB per font size
- **Target**: 4 font sizes × 2 fonts = 2 MB total atlas memory
- **Glyph Metrics**: ~500 glyphs × 32 bytes = 16 KB per font (negligible)

### Rendering Performance

- **Target**: 10,000 characters at 60 FPS = 166,000 chars/sec
- **Batching Efficiency**: All text from same font shares one texture → single draw call per font
- **Layout Caching**: Cache laid-out text for static UI elements (future optimization)

### CPU Performance

- **Text Layout**: O(n) where n = character count
- **UTF-8 Decode**: O(n) linear scan
- **Atlas Lookup**: O(1) hash table lookup per glyph
- **Target**: <1ms layout time for 1000 characters

### Draw Call Reduction

Example UI frame:
- Menu buttons: 200 chars (Font: Roboto-Regular 24px)
- Tooltip: 50 chars (Font: Roboto-Regular 16px)
- Debug overlay: 100 chars (Font: RobotoMono-Regular 14px)

**Without batching**: 350 draw calls (one per character)
**With batching**: 3 draw calls (one per font/size combination)

**Batching wins**: 117x fewer draw calls

## Error Handling

### Missing Glyphs

If a codepoint is not in the font atlas:
1. Log warning: `"Glyph not found: U+{codepoint:04X}"`
2. Substitute with '□' (replacement character U+FFFD)
3. Continue rendering remaining text

### Font Loading Failures

If font file fails to load:
1. Log error: `"Failed to load font: {font_path}"`
2. Fall back to built-in default font (embedded in executable)
3. Continue with degraded visuals

### Atlas Overflow

If atlas texture is full:
1. Log warning: `"Font atlas full for {font_path}, some glyphs unavailable"`
2. Skip remaining glyphs
3. Future: Implement multi-texture atlas support

## Testing Strategy

### Unit Tests

```cpp
TEST(TextLayoutTest, SingleLineNoWrapping) {
    FontAtlas font("test_font.ttf", 16);
    TextLayout::LayoutOptions options;
    options.max_width = 0.0f;  // No wrapping
    
    auto glyphs = TextLayout::Layout("Hello World", font, options);
    
    EXPECT_EQ(glyphs.size(), 11);  // 11 characters (including space)
    EXPECT_GT(glyphs[0].position.x, 0.0f);
    EXPECT_GT(glyphs[10].position.x, glyphs[9].position.x);  // Last char right of previous
}

TEST(TextLayoutTest, MultiLineWrapping) {
    FontAtlas font("test_font.ttf", 16);
    TextLayout::LayoutOptions options;
    options.max_width = 100.0f;  // Force wrapping
    
    auto glyphs = TextLayout::Layout("This is a very long line that should wrap", font, options);
    
    // Check that glyphs are on multiple lines
    float first_y = glyphs[0].position.y;
    bool found_different_y = false;
    for (const auto& g : glyphs) {
        if (g.position.y != first_y) {
            found_different_y = true;
            break;
        }
    }
    EXPECT_TRUE(found_different_y);
}

TEST(UTF8DecodeTest, ASCIICharacters) {
    auto codepoints = DecodeUTF8("Hello");
    EXPECT_EQ(codepoints.size(), 5);
    EXPECT_EQ(codepoints[0], 'H');
    EXPECT_EQ(codepoints[4], 'o');
}

TEST(UTF8DecodeTest, MultiByteCharacters) {
    auto codepoints = DecodeUTF8("Café");  // é is U+00E9 (2 bytes)
    EXPECT_EQ(codepoints.size(), 4);
    EXPECT_EQ(codepoints[3], 0x00E9);
}

TEST(UTF8DecodeTest, EmojisAndSymbols) {
    auto codepoints = DecodeUTF8("👍");  // U+1F44D (4 bytes)
    EXPECT_EQ(codepoints.size(), 1);
    EXPECT_EQ(codepoints[0], 0x1F44D);
}
```

### Integration Tests

```cpp
TEST(BatchRendererTextTest, RenderSimpleText) {
    BatchRenderer::Initialize();
    FontManager::Initialize("test_font.ttf");
    
    BatchRenderer::BeginFrame();
    BatchRenderer::SubmitText("Hello", 10.0f, 10.0f, 16, Color{1,1,1,1});
    BatchRenderer::EndFrame();
    
    // Verify quads were submitted
    EXPECT_GT(BatchRenderer::GetDrawCallCount(), 0);
}

TEST(BatchRendererTextTest, BatchingMultipleTextCalls) {
    BatchRenderer::Initialize();
    FontManager::Initialize("test_font.ttf");
    
    BatchRenderer::BeginFrame();
    
    // Submit multiple text calls with same font
    BatchRenderer::SubmitText("Line 1", 10.0f, 10.0f, 16, Color{1,1,1,1});
    BatchRenderer::SubmitText("Line 2", 10.0f, 30.0f, 16, Color{1,1,1,1});
    BatchRenderer::SubmitText("Line 3", 10.0f, 50.0f, 16, Color{1,1,1,1});
    
    BatchRenderer::EndFrame();
    
    // All three should batch into single draw call
    EXPECT_EQ(BatchRenderer::GetDrawCallCount(), 1);
}
```

### Visual Tests

Manual visual verification checklist:
- [ ] Text renders at correct position and size
- [ ] Text scales crisply without pixelation (SDF quality)
- [ ] Multi-line text wraps correctly
- [ ] Text aligns properly (left, center, right)
- [ ] Text color is accurate
- [ ] Scissor clipping works for `SubmitTextRect()`
- [ ] UTF-8 characters render correctly (accents, emoji)

## Dependencies

### External Libraries

- **stb_truetype** (already in stb dependency): TrueType font loading
- **stb_image_write** (already in stb dependency): For debugging atlas textures

### Internal Dependencies

- `engine.rendering` - Texture and shader management
- `engine.platform` - File I/O for loading fonts
- `engine.ui.batch_renderer` - Core rendering infrastructure

### Font Assets Required

Embed or ship with engine:
- **Roboto-Regular.ttf** (Apache License 2.0) - Default UI font
- **License files** - Ensure proper attribution

## Future Enhancements

### Phase 5+ (Post-MVP)

1. **Rich Text Markup**
   - Support for `<b>`, `<i>`, `<color=#RRGGBB>` tags
   - Mixed font sizes and styles in single string

2. **Advanced Typography**
   - Kerning pair adjustments
   - Ligatures (fi, fl, etc.)
   - Complex script support (Arabic, Thai)

3. **Performance Optimizations**
   - Layout caching for static text
   - Multi-texture atlas support (16+ fonts)
   - Dynamic glyph loading (add glyphs at runtime)

4. **Accessibility**
   - Outline/shadow rendering for readability
   - Adjustable font scale multiplier (user preference)
   - High-contrast text modes

5. **Emoji Support**
   - Color emoji rendering (CBDT/COLR tables)
   - Emoji substitution for `:smile:` syntax

## References

- **stb_truetype**: https://github.com/nothings/stb/blob/master/stb_truetype.h
- **SDF Text Rendering**: "Improved Alpha-Tested Magnification for Vector Textures and Special Effects" by Chris Green (Valve)
- **UTF-8 Specification**: RFC 3629
- **TrueType Font Specification**: https://developer.apple.com/fonts/TrueType-Reference-Manual/

## Acceptance Criteria

Text rendering system is complete when:

- [ ] `BatchRenderer::SubmitText()` renders single-line text at specified position
- [ ] `BatchRenderer::SubmitTextRect()` renders multi-line text with wrapping and alignment
- [ ] Text batches efficiently with other UI primitives (≤3 draw calls for mixed UI)
- [ ] SDF rendering provides crisp text at 0.5x - 4x scale range
- [ ] UTF-8 text renders correctly (tested with Latin, accents, symbols)
- [ ] Font manager caches fonts and provides fast lookup
- [ ] Unit tests cover layout logic, UTF-8 decoding, and edge cases
- [ ] Integration tests verify BatchRenderer batching behavior
- [ ] Visual quality matches or exceeds ImGui debug text (for testing reference)
- [ ] Documentation explains how to load fonts and render text in UI components

---

**Status**: Planning Complete - Ready for Implementation
**Version**: 1.0
**Last Updated**: 2025-10-30
