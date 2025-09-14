module;

#include <vector>
#include <memory>

export module engine.rendering:tilemap_renderer;

import glm;
import engine.components;
import engine.assets;
import :types;
import :texture;
import :renderer;
import :shader;
import :mesh;

export namespace engine::rendering {
    // Vertex data for a single tile quad
    struct TileVertex {
        glm::vec3 position;
        glm::vec2 tex_coords;
        float opacity;
    };

    // Batch data for efficient rendering
    struct TileBatch {
        std::vector<TileVertex> vertices;
        std::vector<uint32_t> indices;
        TextureId texture;
        size_t tile_count = 0;

        void Clear() {
            vertices.clear();
            indices.clear();
            tile_count = 0;
        }

        void Reserve(const size_t max_tiles) {
            vertices.reserve(max_tiles * 4); // 4 vertices per tile
            indices.reserve(max_tiles * 6); // 6 indices per tile (2 triangles)
        }
    };

    // Tilemap renderer class - decoupled from tilemap state
    class TilemapRenderer {
    public:
        TilemapRenderer();

        ~TilemapRenderer() = default;

        // Initialize the renderer (create shaders, buffers, etc.)
        bool Initialize(const ShaderManager &shader_manager);

        // Cleanup resources
        void Cleanup();

        // Render a tilemap with the given view and projection matrices
        // Now requires texture manager to load textures from image paths
        void Render(const components::Tilemap &tilemap,
                    const glm::mat4 &view_matrix,
                    const glm::mat4 &projection_matrix,
                    ShaderManager &shader_manager,
                    const TextureManager &texture_manager);

        // Set maximum number of tiles that can be batched together
        void SetMaxBatchSize(const size_t max_tiles) { max_batch_size_ = max_tiles; }

        // Get rendering statistics
        const RenderStats &GetStats() const { return stats_; }

        void SetShader(const ShaderId shader_id) { shader_id_ = shader_id; }
        ShaderId GetShader() const { return shader_id_; }

    private:
        // Build vertex data for a single layer
        void BuildLayerBatch(const std::shared_ptr<components::TilemapLayer> &layer,
                             const glm::ivec2 &tile_size,
                             const glm::vec2 &grid_offset,
                             TileBatch &batch,
                             const TextureManager &texture_manager,
                             const int layer_index,
                             const float z_step);

        void AddTileToBatch(const glm::vec2 &world_pos,
                            const glm::ivec2 &tile_size,
                            const glm::vec4 &tex_coords,
                            float opacity,
                            TileBatch &batch,
                            const float z);

        // Render a single batch
        void RenderBatch(const TileBatch &batch,
                         const glm::mat4 &mvp_matrix,
                         ShaderManager &shader_manager);

        // Create the default tilemap shader
        static ShaderId CreateDefaultShader(const ShaderManager &shader_manager);

        ShaderId shader_id_ = 0;

        // Batch rendering
        TileBatch current_batch_;
        size_t max_batch_size_ = 1000; // Maximum tiles per batch

        // Render statistics
        RenderStats stats_;

        // OpenGL objects (will be created during initialization)
        uint32_t vao_ = 0;
        uint32_t vbo_ = 0;
        uint32_t ebo_ = 0;

        bool initialized_ = false;
    };
}
