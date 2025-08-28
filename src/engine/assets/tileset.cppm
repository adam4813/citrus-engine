module;

#include <vector>
#include <string>

export module engine.assets:tileset;

import glm;

export namespace engine::assets {
    // Tile description containing ID and texture coordinates
    struct TileDescription {
        uint32_t id;
        glm::vec4 texture_rect; // x, y, width, height in normalized coordinates (0-1)

        TileDescription() = default;

        TileDescription(const uint32_t tile_id, const glm::vec4 &rect)
            : id(tile_id), texture_rect(rect) {
        }
    };

    // Tileset asset containing image reference and tile descriptions
    class Tileset {
    public:
        Tileset() = default;

        ~Tileset() = default;

        // Add a tile description to the tileset
        void AddTile(uint32_t id, const glm::vec4 &texture_rect);

        // Get tile description by ID
        const TileDescription *GetTile(uint32_t id) const;

        // Set the image path for this tileset (instead of TextureId)
        void SetImagePath(const std::string &image_path);

        // Get the image path
        const std::string &GetImagePath() const { return image_path_; }

        // Get all tile descriptions
        const std::vector<TileDescription> &GetTiles() const { return tiles_; }

        // Get tile size in pixels (assuming uniform tile size)
        glm::ivec2 GetTileSize() const { return tile_size_; }
        void SetTileSize(const glm::ivec2 &size) { tile_size_ = size; }

    private:
        std::vector<TileDescription> tiles_;
        std::string image_path_; // Store image path instead of TextureId
        glm::ivec2 tile_size_{32, 32}; // Default 32x32 pixels
    };
}
