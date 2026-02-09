module;

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

export module engine.components;

import glm;
import engine.assets;

export namespace engine::components {
// === CORE COMPONENTS ===

// Transform component for position, rotation, scale
struct Transform {
	glm::vec3 position{0.0F};
	glm::vec3 rotation{0.0F}; // Euler angles in radians
	glm::vec3 scale{1.0F};

	void Translate(const glm::vec3& offset) { position += offset; }

	void Rotate(const glm::vec3& euler_angles) { rotation += euler_angles; }

	void SetScale(const glm::vec3& new_scale) { scale = new_scale; }

	void SetScale(const float uniform_scale) { scale = glm::vec3(uniform_scale); }
};

struct WorldTransform {
	glm::mat4 matrix{1.0F};
};

// Velocity component for movement
struct Velocity {
	glm::vec3 linear{0.0F};
	glm::vec3 angular{0.0F}; // Angular velocity in radians/sec
};

// Simple tag component for testing
struct Rotating {};

// Camera component - shared between ECS and rendering
struct Camera {
	glm::vec3 target{0.0F, 0.0F, 0.0F};
	glm::vec3 up{0.0F, 1.0F, 0.0F};
	float fov{60.0F};
	float aspect_ratio{16.0F / 9.0F};
	float near_plane{0.1F};
	float far_plane{100.0F};
	glm::mat4 view_matrix{1.0F};
	glm::mat4 projection_matrix{1.0F};
};

struct ActiveCamera {}; // Tag for the currently active camera

// === SCENE ORGANIZATION COMPONENTS ===

// Tag component for organizational group entities (folders in hierarchy)
struct Group {};

// Tags for categorizing and filtering entities
struct Tags {
	std::vector<std::string> tags;

	void AddTag(const std::string& tag) {
		if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
			tags.push_back(tag);
		}
	}

	void RemoveTag(const std::string& tag) {
		std::erase(tags, tag);
	}

	bool HasTag(const std::string& tag) const {
		return std::find(tags.begin(), tags.end(), tag) != tags.end();
	}

	void ClearTags() {
		tags.clear();
	}
};

// === TILEMAP COMPONENTS ===

// A single cell in the tilemap that can contain multiple tiles
struct TilemapCell {
	std::vector<uint32_t> tile_ids; // List of tile IDs in this cell (for layering)

	void AddTile(const uint32_t tile_id) { tile_ids.push_back(tile_id); }

	void RemoveTile(const uint32_t tile_id) { std::erase(tile_ids, tile_id); }

	void ClearTiles() { tile_ids.clear(); }

	bool HasTiles() const { return !tile_ids.empty(); }
};

// A single layer of the tilemap
struct TilemapLayer {
	std::unordered_map<uint64_t, TilemapCell> cells; // Key: packed (x,y) coordinates
	std::shared_ptr<assets::Tileset> tileset;
	bool visible = true;
	float opacity = 1.0f;

	// Pack x,y coordinates into a single key for the hash map
	static uint64_t PackCoords(const int32_t x, const int32_t y) {
		return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32)
			   | static_cast<uint64_t>(static_cast<uint32_t>(y));
	}

	// Unpack coordinates from the key
	static std::pair<int32_t, int32_t> UnpackCoords(const uint64_t key) {
		int32_t x = static_cast<int32_t>(key >> 32);
		int32_t y = static_cast<int32_t>(key & 0xFFFFFFFF);
		return {x, y};
	}

	// Get or create a cell at the given grid position
	TilemapCell& GetCell(const int32_t x, const int32_t y) { return cells[PackCoords(x, y)]; }

	// Get a cell at the given grid position (const version)
	const TilemapCell* GetCell(const int32_t x, const int32_t y) const {
		const auto it = cells.find(PackCoords(x, y));
		return it != cells.end() ? &it->second : nullptr;
	}

	// Set the tileset for this layer
	void SetTileset(const std::shared_ptr<assets::Tileset>& tileset_ptr) { tileset = tileset_ptr; }

	// Add a tile to a specific grid position
	void SetTile(const int32_t x, const int32_t y, const uint32_t tile_id) { GetCell(x, y).AddTile(tile_id); }

	// Remove all tiles from a specific grid position
	void ClearTile(const int32_t x, const int32_t y) {
		const auto it = cells.find(PackCoords(x, y));
		if (it != cells.end()) {
			it->second.ClearTiles();
			if (!it->second.HasTiles()) {
				cells.erase(it);
			}
		}
	}
};

// Main tilemap component containing multiple layers
struct Tilemap {
	std::vector<std::shared_ptr<TilemapLayer>> layers;
	glm::ivec2 tile_size{32, 32}; // Size of each tile in pixels
	glm::vec2 grid_offset{0.0f, 0.0f}; // Offset for the entire tilemap

	// Add a new layer and return its index
	size_t AddLayer() {
		layers.emplace_back(std::make_shared<TilemapLayer>());
		return layers.size() - 1;
	}

	std::shared_ptr<TilemapLayer> GetLayer(const size_t index) {
		return (index < layers.size()) ? layers[index] : nullptr;
	}

	std::shared_ptr<const TilemapLayer> GetLayer(const size_t index) const {
		return (index < layers.size()) ? layers[index] : nullptr;
	}

	template <typename Func> bool WithLayer(const size_t index, Func&& func) {
		if (index < layers.size() && layers[index]) {
			func(*layers[index]);
			return true;
		}
		return false;
	}

	template <typename Func> bool WithLayer(const size_t index, Func&& func) const {
		if (index < layers.size() && layers[index]) {
			func(*layers[index]);
			return true;
		}
		return false;
	}

	// Reserve capacity to prevent reallocations
	void ReserveLayers(const size_t capacity) { layers.reserve(capacity); }

	// Get the number of layers
	size_t GetLayerCount() const { return layers.size(); }

	// Convert world position to grid coordinates
	glm::ivec2 WorldToGrid(const glm::vec2& world_pos) const {
		const glm::vec2 adjusted_pos = world_pos - grid_offset;
		return glm::ivec2(
				static_cast<int32_t>(std::floor(adjusted_pos.x / tile_size.x)),
				static_cast<int32_t>(std::floor(adjusted_pos.y / tile_size.y)));
	}

	// Convert grid coordinates to world position (center of tile)
	glm::vec2 GridToWorld(const glm::ivec2& grid_pos) const {
		return grid_offset + glm::vec2((grid_pos.x + 0.5f) * tile_size.x, (grid_pos.y + 0.5f) * tile_size.y);
	}
};
} // namespace engine::components
