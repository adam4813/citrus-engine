# Tilemap System Documentation

The tilemap system provides efficient 2D tile-based rendering capabilities for your game engine. It consists of three
main components: Tileset assets, Tilemap components, and the TilemapRenderer.

## Table of Contents

1. [Overview](#overview)
2. [Integration Status](#integration-status)
3. [Core Components](#core-components)
4. [Basic Usage](#basic-usage)
5. [Advanced Usage](#advanced-usage)
6. [Performance Considerations](#performance-considerations)
7. [Examples](#examples)
8. [API Reference](#api-reference)

## Overview

The tilemap system is designed with the following principles:

- **Decoupled Architecture**: The renderer is separate from the tilemap data
- **Multi-layer Support**: Each tilemap can have multiple layers with different tilesets
- **Sparse Storage**: Only stores tiles that exist, saving memory
- **Batch Rendering**: Efficient GPU usage through batched draw calls
- **Flexible Layering**: Multiple tiles can exist in the same grid cell

### Architecture

```
Tilemap Component (Data)
├── Multiple Layers
│   ├── Tileset Reference
│   ├── Grid Cells (sparse)
│   └── Layer Properties (visibility, opacity)
└── Grid Configuration

TilemapRenderer (Rendering)
├── Batch Processing
├── Shader Management
└── OpenGL State Management
```

## Integration Status

✅ **FULLY INTEGRATED** - The tilemap system has been fully integrated into your engine:

- **Build System**: Added to CMakeLists.txt with proper module dependencies
- **Shader System**: Integrated with your existing ShaderManager
- **Texture System**: Uses your existing TextureId system
- **Asset System**: Tileset integrated into your assets module
- **Component System**: Tilemap components added to your components module
- **Module System**: All modules properly exported and imported

### Files Added/Modified:

**New Files:**

- `src/engine/assets/tileset.cppm` - Tileset asset interface
- `src/engine/assets/tileset.cpp` - Tileset implementation
- `src/engine/rendering/tilemap_renderer.cppm` - Renderer interface
- `src/engine/rendering/tilemap_renderer.cpp` - Renderer implementation

**Modified Files:**

- `src/engine/CMakeLists.txt` - Added tilemap files to build
- `src/engine/assets/assets.cppm` - Export tileset module
- `src/engine/rendering/rendering.cppm` - Export tilemap renderer
- `src/engine/components/components.cppm` - Added tilemap components

## Core Components

### Tileset (`engine::assets::Tileset`)

A tileset contains:

- A texture atlas (using your TextureId system)
- Tile descriptions with IDs and texture coordinates
- Tile size information

### Tilemap (`engine::components::Tilemap`)

The main component containing:

- Multiple layers
- Tile size (uniform for all layers)
- Grid offset for positioning

### TilemapLayer (`engine::components::TilemapLayer`)

Individual layers containing:

- Sparse grid of cells
- Tileset reference
- Visibility and opacity settings

### TilemapCell (`engine::components::TilemapCell`)

Individual grid cells that can contain multiple tiles for layering effects.

### TilemapRenderer (`engine::rendering::TilemapRenderer`)

Handles the actual rendering with batch optimization.

## Basic Usage

### 1. Create a Tileset

```cpp
import engine.assets;
import engine.rendering;

// Create tileset
auto tileset = std::make_shared<engine::assets::Tileset>();

// Set image path (TextureManager will handle loading and caching)
tileset->SetImagePath("assets/textures/tiles.png");
tileset->SetTileSize({32, 32}); // 32x32 pixel tiles

// Add tile definitions (normalized texture coordinates 0-1)
tileset->AddTile(1, {0.0f, 0.0f, 0.25f, 0.25f}); // Tile 1: top-left quarter
tileset->AddTile(2, {0.25f, 0.0f, 0.25f, 0.25f}); // Tile 2: next quarter
tileset->AddTile(3, {0.0f, 0.25f, 0.25f, 0.25f}); // Tile 3: second row
```

### 2. Create a Tilemap Component

```cpp
import engine.components;

// Create tilemap component on an entity
auto& tilemap = entity.AddComponent<engine::components::Tilemap>();

// Configure tilemap
tilemap.tile_size = {32, 32}; // Match your tileset
tilemap.grid_offset = {0.0f, 0.0f}; // World position offset

// Add a layer
size_t layer_index = tilemap.AddLayer();
auto* layer = tilemap.GetLayer(layer_index);
layer->SetTileset(tileset);
```

### 3. Place Tiles

```cpp
// Set individual tiles
layer->SetTile(0, 0, 1); // Place tile ID 1 at grid position (0,0)
layer->SetTile(1, 0, 2); // Place tile ID 2 at grid position (1,0)
layer->SetTile(0, 1, 3); // Place tile ID 3 at grid position (0,1)

// You can also add multiple tiles to the same cell
auto& cell = layer->GetCell(2, 2);
cell.AddTile(1); // Background tile
cell.AddTile(4); // Foreground decoration
```

### 4. Render the Tilemap

```cpp
import engine.rendering;

// Create and initialize renderer (typically done once at startup)
engine::rendering::TilemapRenderer tilemap_renderer;
tilemap_renderer.Initialize(shader_manager);

// In your render loop
void RenderScene() {
    // Get view and projection matrices from your camera
    glm::mat4 view_matrix = camera.GetViewMatrix();
    glm::mat4 projection_matrix = camera.GetProjectionMatrix();
    
    // Render all tilemaps in your scene
    for (auto entity : entities_with_tilemaps) {
        auto& tilemap = entity.GetComponent<engine::components::Tilemap>();
        tilemap_renderer.Render(tilemap, view_matrix, projection_matrix, shader_manager, texture_manager);
    }
}
```

### 5. Complete Working Example

```cpp
// Complete minimal example for your engine
#include <memory>
import engine.components;
import engine.assets;
import engine.rendering;

void SetupTilemap(EntityManager& entity_manager, 
                  TextureManager& texture_manager,
                  ShaderManager& shader_manager) {
    
    // 1. Create and setup tileset
    auto tileset = std::make_shared<engine::assets::Tileset>();
    tileset->SetImagePath("assets/textures/tileset.png");
    tileset->SetTileSize({32, 32});
    
    // Define some basic tiles (adjust coordinates for your tileset)
    tileset->AddTile(1, {0.0f, 0.0f, 0.125f, 0.125f});    // Grass
    tileset->AddTile(2, {0.125f, 0.0f, 0.125f, 0.125f});  // Stone
    tileset->AddTile(3, {0.25f, 0.0f, 0.125f, 0.125f});   // Water
    
    // 2. Create entity with tilemap
    auto entity = entity_manager.CreateEntity();
    auto& tilemap = entity.AddComponent<engine::components::Tilemap>();
    tilemap.tile_size = {32, 32};
    tilemap.grid_offset = {0.0f, 0.0f};
    
    // 3. Add layer and set tileset
    size_t layer_idx = tilemap.AddLayer();
    auto* layer = tilemap.GetLayer(layer_idx);
    layer->SetTileset(tileset);
    
    // 4. Create a simple 10x10 level
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            uint32_t tile_id = 1; // Default to grass
            
            // Add some variation
            if (x == 0 || x == 9 || y == 0 || y == 9) {
                tile_id = 2; // Stone border
            } else if (x == 5 && y == 5) {
                tile_id = 3; // Water in center
            }
            
            layer->SetTile(x, y, tile_id);
        }
    }
    
    // 5. Initialize renderer (do this once at startup)
    static engine::rendering::TilemapRenderer renderer;
    static bool initialized = false;
    if (!initialized) {
        renderer.Initialize(shader_manager);
        initialized = true;
    }
}
```

## Advanced Usage

### Custom Shaders

You can use custom shaders by modifying the `CreateDefaultShader` method or by creating your own shader and setting it
in the renderer:

```cpp
// Load custom shader
auto custom_shader_id = shader_manager.LoadShader(
    "custom_tilemap", 
    "assets/shaders/custom_tilemap.vert",
    "assets/shaders/custom_tilemap.frag"
);

tilemap_renderer.SetShader(custom_shader_id);

// The tilemap renderer will use the shader returned by CreateDefaultShader,
// but you can modify that method to return your custom shader ID
```

### Multiple Layers with Different Properties

```cpp
// Create multiple layers with different tilesets and properties
auto background_tileset = std::make_shared<engine::assets::Tileset>();
auto foreground_tileset = std::make_shared<engine::assets::Tileset>();

// Setup tilesets with different textures...

// Add layers
size_t bg_layer = tilemap.AddLayer();
size_t fg_layer = tilemap.AddLayer();

// Configure background layer
auto* bg = tilemap.GetLayer(bg_layer);
bg->SetTileset(background_tileset);
bg->opacity = 1.0f;
bg->visible = true;

// Configure foreground layer
auto* fg = tilemap.GetLayer(fg_layer);
fg->SetTileset(foreground_tileset);
fg->opacity = 0.8f; // Semi-transparent
fg->visible = true;

// Background tiles (rendered first)
bg->SetTile(5, 5, 1);

// Foreground tiles (rendered on top)
fg->SetTile(5, 5, 10); // Same position, different layer
```

### Dynamic Tile Management

```cpp
// Clear tiles
layer->ClearTile(x, y);

// Check if cell has tiles
if (auto* cell = layer->GetCell(x, y)) {
    if (cell->HasTiles()) {
        // Process existing tiles
        for (uint32_t tile_id : cell->tile_ids) {
            // Handle each tile
        }
    }
}

// Toggle layer visibility
layer->visible = !layer->visible;

// Adjust layer opacity
layer->opacity = 0.5f;
```

### Coordinate Conversion

```cpp
// Convert mouse/world position to grid coordinates
glm::vec2 world_pos = GetMouseWorldPosition();
glm::ivec2 grid_pos = tilemap.WorldToGrid(world_pos);

// Convert grid coordinates back to world position
glm::vec2 tile_center = tilemap.GridToWorld(grid_pos);

// Get tile under mouse
auto* layer = tilemap.GetLayer(0);
auto* cell = layer->GetCell(grid_pos.x, grid_pos.y);
if (cell && cell->HasTiles()) {
    uint32_t first_tile = cell->tile_ids[0];
    // Handle tile interaction
}
```

### Performance Optimization

```cpp
// Adjust batch size for performance (larger = fewer draw calls, more memory)
tilemap_renderer.SetMaxBatchSize(2000);

// Check rendering statistics
auto stats = tilemap_renderer.GetStats();
std::cout << "Draw calls: " << stats.draw_calls << std::endl;
std::cout << "Triangles: " << stats.triangles << std::endl;
std::cout << "Vertices: " << stats.vertices << std::endl;
```

## Performance Considerations

### Memory Usage

- **Sparse Storage**: Only occupied cells are stored in memory
- **Shared Tilesets**: Multiple layers can share the same tileset
- **Batch Size**: Larger batches use more memory but reduce draw calls

### Rendering Performance

- **Layer Count**: Each visible layer requires at least one draw call
- **Texture Switches**: Using different tilesets requires texture binding
- **Tile Density**: More tiles per screen = more vertices to process

### Best Practices

1. **Group Similar Tiles**: Use the same tileset for tiles that should be rendered together
2. **Limit Active Layers**: Hide layers that aren't needed
3. **Optimize Tile Placement**: Remove tiles outside the visible area
4. **Use Appropriate Batch Sizes**: Balance memory usage vs. draw call reduction

## API Reference

### Tileset Class

```cpp
class Tileset {
public:
    void AddTile(uint32_t id, const glm::vec4& texture_rect);
    const TileDescription* GetTile(uint32_t id) const;
    void SetImagePath(std:string image_path);
    std:string GetImagePath() const;
    glm::ivec2 GetTileSize() const;
    void SetTileSize(const glm::ivec2& size);
};
```

### TilemapLayer Struct

```cpp
struct TilemapLayer {
    std::unordered_map<uint64_t, TilemapCell> cells;
    std::shared_ptr<engine::assets::Tileset> tileset;
    bool visible = true;
    float opacity = 1.0f;
    
    TilemapCell& GetCell(int32_t x, int32_t y);
    const TilemapCell* GetCell(int32_t x, int32_t y) const;
    void SetTileset(std::shared_ptr<engine::assets::Tileset> tileset_ptr);
    void SetTile(int32_t x, int32_t y, uint32_t tile_id);
    void ClearTile(int32_t x, int32_t y);
};
```

### Tilemap Component

```cpp
struct Tilemap {
    std::vector<TilemapLayer> layers;
    glm::ivec2 tile_size{32, 32};
    glm::vec2 grid_offset{0.0f, 0.0f};
    
    size_t AddLayer();
    TilemapLayer* GetLayer(size_t index);
    const TilemapLayer* GetLayer(size_t index) const;
    size_t GetLayerCount() const;
    glm::ivec2 WorldToGrid(const glm::vec2& world_pos) const;
    glm::vec2 GridToWorld(const glm::ivec2& grid_pos) const;
};
```

### TilemapRenderer Class

```cpp
class TilemapRenderer {
public:
    bool Initialize(ShaderManager& shader_manager);
    void Cleanup();
    void Render(const Tilemap& tilemap, 
               const glm::mat4& view_matrix, 
               const glm::mat4& projection_matrix,
               ShaderManager& shader_manager,
               TextureManager& texture_manager);
    void SetMaxBatchSize(size_t max_tiles);
    const RenderStats& GetStats() const;
};
```

---

**Status**: ✅ **READY TO USE** - The tilemap system is fully integrated into your engine and ready for immediate use.
All components work with your existing systems (ShaderManager, TextureManager, ECS, etc.).
