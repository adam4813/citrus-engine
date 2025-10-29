module;

#include <cstdint>
#include <algorithm>
#include <string>

module engine.assets;

import glm;
import :tileset;
import engine.rendering;

namespace engine::assets {
    void Tileset::AddTile(uint32_t id, const glm::vec4 &texture_rect) {
        // Check if tile with this ID already exists
        const auto it = std::ranges::find_if(tiles_,
                                             [id](const TileDescription &tile) { return tile.id == id; });

        if (it != tiles_.end()) {
            // Update existing tile
            it->texture_rect = texture_rect;
        } else {
            // Add new tile
            tiles_.emplace_back(id, texture_rect);
        }
    }

    const TileDescription *Tileset::GetTile(uint32_t id) const {
        const auto it = std::ranges::find_if(tiles_,
                                             [id](const TileDescription &tile) { return tile.id == id; });
        return (it != tiles_.end()) ? &(*it) : nullptr;
    }

    void Tileset::SetImagePath(const std::string &image_path) {
        image_path_ = image_path;
    }
}
