#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

import engine;
import glm;

namespace editor {

/**
 * @brief Per-tile metadata and properties
 */
struct TileDefinition {
	uint32_t id = 0;
	std::string name;
	bool collision = false;
	std::vector<std::string> tags;
	std::unordered_map<std::string, std::string> custom_properties;

	TileDefinition() = default;
	explicit TileDefinition(uint32_t tile_id) : id(tile_id) {}
};

/**
 * @brief Tileset definition containing source image and tile metadata
 */
struct TilesetDefinition {
	std::string source_image_path;
	int tile_width = 32;
	int tile_height = 32;
	std::vector<TileDefinition> tiles;

	// Calculate grid dimensions from image dimensions
	int GetGridColumns(int image_width) const { return (tile_width > 0) ? (image_width / tile_width) : 0; }

	int GetGridRows(int image_height) const { return (tile_height > 0) ? (image_height / tile_height) : 0; }

	// Get tile definition by ID, returns nullptr if not found
	TileDefinition* GetTile(uint32_t id) {
		for (auto& tile : tiles) {
			if (tile.id == id) {
				return &tile;
			}
		}
		return nullptr;
	}

	const TileDefinition* GetTile(uint32_t id) const {
		for (const auto& tile : tiles) {
			if (tile.id == id) {
				return &tile;
			}
		}
		return nullptr;
	}
};

/**
 * @brief Brush tool modes for tileset painting
 */
enum class BrushMode { SingleTile, RectangleFill, Eraser };

/**
 * @brief Tileset editor panel for editing tileset definitions
 * 
 * Provides a visual editor for creating and editing tilesets with per-tile
 * properties, collision flags, tags, and custom metadata. Uses placeholder
 * colored rectangles instead of actual texture loading.
 */
class TilesetEditorPanel {
public:
	TilesetEditorPanel();
	~TilesetEditorPanel();

	/**
	 * @brief Render the tileset editor panel
	 * @param engine Reference to the engine instance
	 */
	void Render(engine::Engine& engine);

	/**
	 * @brief Check if panel is visible
	 */
	[[nodiscard]] bool IsVisible() const { return is_visible_; }

	/**
	 * @brief Set panel visibility
	 */
	void SetVisible(bool visible) { is_visible_ = visible; }

	/**
	 * @brief Get mutable reference to visibility (for ImGui::MenuItem binding)
	 */
	bool& VisibleRef() { return is_visible_; }

	/**
	 * @brief Create a new empty tileset
	 */
	void NewTileset();

	/**
	 * @brief Save the current tileset to a file
	 */
	bool SaveTileset(const std::string& path);

	/**
	 * @brief Load a tileset from a file
	 */
	bool LoadTileset(const std::string& path);

	/**
	 * @brief Open a tileset file (for asset browser integration)
	 */
	void OpenTileset(const std::string& path);

private:
	void RenderToolbar(engine::Engine& engine);
	void RenderTilesetGrid();
	void RenderTileProperties();
	void RenderTilePalette();
	void RenderTilePreview();

	// Load the source image and create a GPU texture
	void LoadSourceImage(engine::Engine& engine, const std::string& path);

	// Get tile ID from grid coordinates
	uint32_t GetTileIdFromCoords(int x, int y) const;

	// Get grid coordinates from tile ID
	void GetCoordsFromTileId(uint32_t id, int& out_x, int& out_y) const;

	// Handle tile selection in the grid
	void HandleTileSelection(const ImVec2& mouse_pos, const ImVec2& grid_origin);

	// Ensure tile definition exists for the given ID
	TileDefinition* EnsureTileDefinition(uint32_t id);

	// Get actual image dimensions (loaded or placeholder)
	int GetImageWidth() const;
	int GetImageHeight() const;

	bool is_visible_ = false;
	std::unique_ptr<TilesetDefinition> tileset_;
	std::string current_file_path_;

	// Source image state
	std::shared_ptr<engine::assets::Image> loaded_image_;
	engine::rendering::TextureId gpu_texture_id_ = engine::rendering::INVALID_TEXTURE;
	char image_path_buffer_[512] = "";
	std::string load_error_message_;

	// Grid rendering state
	static constexpr int PLACEHOLDER_IMAGE_WIDTH = 512;
	static constexpr int PLACEHOLDER_IMAGE_HEIGHT = 512;
	float tile_display_scale_ = 2.0f;

	// Selection state
	std::vector<uint32_t> selected_tiles_; // Multi-selection support
	bool is_selecting_ = false;
	uint32_t selection_start_id_ = 0;

	// Brush mode
	BrushMode brush_mode_ = BrushMode::SingleTile;

	// UI state for adding new tags/properties
	char new_tag_buffer_[256] = "";
	char new_property_key_buffer_[256] = "";
	char new_property_value_buffer_[256] = "";
};

} // namespace editor
