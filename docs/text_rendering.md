# Text Rendering System

## Overview

The citrus-engine text rendering system provides high-performance text rendering using font atlases and batch rendering. Text is rendered as textured quads using glyph data from TrueType fonts.

## Features

- **Font Atlas Generation**: Automatic generation of texture atlases from TrueType (.ttf) fonts
- **UTF-8 Support**: Full UTF-8 text encoding support
- **Text Layout**: Single-line and multi-line text with word wrapping
- **Alignment**: Horizontal (left, center, right) and vertical (top, center, bottom) alignment
- **Batch Rendering**: Efficient rendering with automatic batching
- **Texture Measuring**: Built-in text measurement for layout calculations
- **Font Caching**: Automatic font caching by path and size

## Basic Usage

### Initialization

Initialize the font manager with a default font before rendering text:

```cpp
using namespace engine::ui::batch_renderer;
using namespace engine::ui::text_renderer;

// Initialize font manager with default font
FontManager::Initialize("assets/fonts/Roboto-Regular.ttf", 16);
```

### Rendering Text

Use `BatchRenderer::SubmitText()` for simple text rendering:

```cpp
// Initialize batch renderer
BatchRenderer::Initialize();

// Each frame:
BatchRenderer::BeginFrame();

// Render simple text
Color white{1.0f, 1.0f, 1.0f, 1.0f};
BatchRenderer::SubmitText("Hello World!", 100.0f, 100.0f, 16, white);

BatchRenderer::EndFrame();
```

### Text with Clipping

Use `BatchRenderer::SubmitTextRect()` for text with automatic clipping and wrapping:

```cpp
// Define a bounding rectangle
Rectangle textBox{50.0f, 50.0f, 300.0f, 200.0f};

// Render text within bounds (with wrapping)
Color textColor{1.0f, 1.0f, 1.0f, 1.0f};
BatchRenderer::SubmitTextRect(
    textBox,
    "This is a long text that will automatically wrap within the bounding box.",
    16,
    textColor
);
```

### Measuring Text

Calculate text dimensions before rendering:

```cpp
auto* font = FontManager::GetDefaultFont();
if (font) {
    Rectangle textBounds = TextLayout::MeasureText(
        "Sample Text",
        *font,
        0.0f  // max_width: 0 = no wrapping
    );
    
    // textBounds.width and textBounds.height contain the text dimensions
    float textWidth = textBounds.width;
    float textHeight = textBounds.height;
}
```

### Advanced Layout

Use `TextLayout` directly for custom layout:

```cpp
auto* font = FontManager::GetDefaultFont();
if (font) {
    // Configure layout options
    LayoutOptions options;
    options.h_align = HorizontalAlign::Center;
    options.v_align = VerticalAlign::Top;
    options.max_width = 400.0f;  // Enable wrapping at 400 pixels
    options.line_height = 1.2f;  // 120% line height

    // Layout text
    auto glyphs = TextLayout::Layout("Multi-line\nText Example", *font, options);
    
    // Render positioned glyphs
    uint32_t textureId = font->GetTextureId();
    for (const auto& glyph : glyphs) {
        if (glyph.metrics->size.x > 0 && glyph.metrics->size.y > 0) {
            Rectangle quad{
                glyph.position.x,
                glyph.position.y,
                glyph.metrics->size.x,
                glyph.metrics->size.y
            };
            
            BatchRenderer::SubmitQuad(
                quad,
                Color{1, 1, 1, 1},
                glyph.metrics->atlas_rect,
                textureId
            );
        }
    }
}
```

### Loading Multiple Fonts

Load and cache additional fonts:

```cpp
// Load a custom font
auto* customFont = FontManager::GetFont("assets/fonts/CustomFont.ttf", 24);
if (customFont) {
    // Use the custom font...
}

// Get the default font
auto* defaultFont = FontManager::GetDefaultFont();
```

## Implementation Details

### Font Atlas

- **Atlas Size**: 512x512 pixels
- **Format**: R8 (single-channel grayscale)
- **Character Set**: ASCII + Latin-1 (codepoints 32-255)
- **Packing**: Shelf-based rectangle packing algorithm
- **Filtering**: Linear filtering for smooth text

### Text Layout

1. **UTF-8 Decoding**: Text strings are decoded into Unicode codepoints
2. **Glyph Lookup**: Each codepoint is mapped to glyph metrics
3. **Line Breaking**: Text is broken into lines based on max_width
4. **Alignment**: Glyphs are positioned according to alignment settings
5. **Rendering**: Glyphs are submitted as textured quads to BatchRenderer

### Performance

- **Batching**: All glyphs from the same font are batched into a single draw call
- **Caching**: Fonts are cached by (path, size) to avoid redundant loading
- **Memory**: ~256 KB per font atlas (512x512 R8 texture)

## API Reference

### FontManager

```cpp
// Initialize with default font
static void Initialize(const std::string& default_font_path, int default_font_size = 16);

// Shutdown and cleanup
static void Shutdown();

// Get default font
static FontAtlas* GetDefaultFont();

// Get or load a font
static FontAtlas* GetFont(const std::string& font_path, int font_size);
```

### TextLayout

```cpp
// Layout text with options
static std::vector<PositionedGlyph> Layout(
    const std::string& text,
    const FontAtlas& font,
    const LayoutOptions& options
);

// Measure text dimensions
static Rectangle MeasureText(
    const std::string& text,
    const FontAtlas& font,
    float max_width = 0.0f
);
```

### BatchRenderer

```cpp
// Render simple text
static void SubmitText(
    const std::string& text,
    float x,
    float y,
    int font_size,
    const Color& color
);

// Render text with clipping
static void SubmitTextRect(
    const Rectangle& rect,
    const std::string& text,
    int font_size,
    const Color& color
);
```

## Current Limitations

1. **Single Font Size**: Currently optimized for a single fixed font size per font atlas
2. **No SDF**: Uses bitmap fonts (not Signed Distance Field) - limited scaling quality
3. **Limited Character Set**: ASCII + Latin-1 only (codepoints 32-255)
4. **No Rich Text**: No support for mixed fonts, colors, or styles in a single string
5. **Static Atlas**: All glyphs must be pre-generated (no dynamic glyph loading)

## Future Enhancements

See `plan/modules/MOD_UI_TEXT_RENDERING_v1.md` for planned features:
- Signed Distance Field (SDF) rendering for resolution-independent text
- Variable font sizes (trivial with SDF)
- Extended Unicode support
- Rich text markup (bold, italic, colors)
- Kerning and advanced typography
- Dynamic glyph loading
- Multi-atlas support for large character sets

## Examples

### Debug Overlay

```cpp
// Simple FPS counter
void RenderDebugOverlay() {
    BatchRenderer::BeginFrame();
    
    Color debugColor{0.0f, 1.0f, 0.0f, 1.0f};  // Green
    BatchRenderer::SubmitText(
        "FPS: 60",
        10.0f,
        10.0f,
        14,
        debugColor
    );
    
    BatchRenderer::EndFrame();
}
```

### Menu System

```cpp
void RenderMenu() {
    BatchRenderer::BeginFrame();
    
    // Title
    Color titleColor{1.0f, 0.84f, 0.0f, 1.0f};  // Gold
    BatchRenderer::SubmitText("Main Menu", 300.0f, 50.0f, 32, titleColor);
    
    // Menu items
    Color normalColor{1.0f, 1.0f, 1.0f, 1.0f};
    Color highlightColor{1.0f, 1.0f, 0.0f, 1.0f};
    
    BatchRenderer::SubmitText("New Game", 300.0f, 150.0f, 24, 
        selectedItem == 0 ? highlightColor : normalColor);
    BatchRenderer::SubmitText("Load Game", 300.0f, 200.0f, 24,
        selectedItem == 1 ? highlightColor : normalColor);
    BatchRenderer::SubmitText("Settings", 300.0f, 250.0f, 24,
        selectedItem == 2 ? highlightColor : normalColor);
    
    BatchRenderer::EndFrame();
}
```

### Tooltip with Word Wrapping

```cpp
void RenderTooltip(const std::string& tooltipText, float x, float y) {
    BatchRenderer::BeginFrame();
    
    // Background rectangle
    Rectangle bgRect{x, y, 200.0f, 100.0f};
    Color bgColor{0.1f, 0.1f, 0.1f, 0.9f};
    BatchRenderer::SubmitQuad(bgRect, bgColor);
    
    // Text with padding
    Rectangle textRect{x + 5, y + 5, 190.0f, 90.0f};
    Color textColor{1.0f, 1.0f, 1.0f, 1.0f};
    BatchRenderer::SubmitTextRect(textRect, tooltipText, 12, textColor);
    
    BatchRenderer::EndFrame();
}
```
