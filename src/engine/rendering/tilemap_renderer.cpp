module;

#include <algorithm>
#include <string>
#include <memory>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import glm;
import engine;

namespace engine::rendering {
    TilemapRenderer::TilemapRenderer() {
        current_batch_.Reserve(max_batch_size_);
    }

    bool TilemapRenderer::Initialize(const ShaderManager &shader_manager) {
        if (initialized_) return true;

        // Create OpenGL objects for batch rendering
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);

        // Setup vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, max_batch_size_ * 4 * sizeof(TileVertex), nullptr, GL_DYNAMIC_DRAW);

        // Setup element buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_batch_size_ * 6 * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

        // Setup vertex attributes
        // Position (vec3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void *) offsetof(TileVertex, position));
        glEnableVertexAttribArray(0);

        // Texture coordinates (vec2)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void *) offsetof(TileVertex, tex_coords));
        glEnableVertexAttribArray(1);

        // Opacity (float)
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void *) offsetof(TileVertex, opacity));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        shader_id_ = CreateDefaultShader(shader_manager);
        initialized_ = true;
        return shader_manager.IsValid(shader_id_);
    }

    void TilemapRenderer::Cleanup() {
        if (!initialized_) return;

        if (vao_ != 0) {
            glDeleteVertexArrays(1, &vao_);
            vao_ = 0;
        }
        if (vbo_ != 0) {
            glDeleteBuffers(1, &vbo_);
            vbo_ = 0;
        }
        if (ebo_ != 0) {
            glDeleteBuffers(1, &ebo_);
            ebo_ = 0;
        }

        initialized_ = false;
    }

    void TilemapRenderer::Render(const components::Tilemap &tilemap,
                                 const glm::mat4 &view_matrix,
                                 const glm::mat4 &projection_matrix,
                                 ShaderManager &shader_manager,
                                 const TextureManager &texture_manager) {
        if (!initialized_ || !shader_manager.IsValid(shader_id_)) return;

        // Reset stats
        stats_ = {};

        const glm::mat4 mvp_matrix = projection_matrix * view_matrix;

        // Enable alpha blending for tile transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Render each layer from bottom to top
        constexpr float z_step = 0.01f; // Small step to avoid z-fighting
        int layer_index = 0;
        for (const auto &layer_ptr: tilemap.layers) {
            if (!layer_ptr || !layer_ptr->visible || !layer_ptr->tileset) continue;

            current_batch_.Clear();

            BuildLayerBatch(layer_ptr, tilemap.tile_size, tilemap.grid_offset, current_batch_, texture_manager,
                            layer_index, z_step);

            if (current_batch_.tile_count > 0) {
                RenderBatch(current_batch_, mvp_matrix, shader_manager);
            }
            layer_index++;
        }

        glDisable(GL_BLEND);
    }

    void TilemapRenderer::BuildLayerBatch(const std::shared_ptr<components::TilemapLayer> &layer,
                                          const glm::ivec2 &tile_size,
                                          const glm::vec2 &grid_offset,
                                          TileBatch &batch,
                                          const TextureManager &texture_manager,
                                          const int layer_index,
                                          const float z_step) {
        if (!layer->tileset) return;

        const std::string &image_path = layer->tileset->GetImagePath();
        if (image_path.empty()) return;

        batch.texture = texture_manager.LoadTexture(image_path);

        for (const auto &[packed_coords, cell]: layer->cells) {
            if (!cell.HasTiles()) continue;

            auto [grid_x, grid_y] = components::TilemapLayer::UnpackCoords(packed_coords);

            // Calculate world position for this cell
            glm::vec2 world_pos = grid_offset + glm::vec2(
                                      grid_x * tile_size.x,
                                      grid_y * tile_size.y
                                  );
            float z = layer_index * z_step;

            // Render all tiles in this cell (for layering within the cell)
            for (const uint32_t tile_id: cell.tile_ids) {
                const auto *tile_desc = layer->tileset->GetTile(tile_id);
                if (!tile_desc) continue;

                AddTileToBatch(world_pos, tile_size, tile_desc->texture_rect, layer->opacity, batch, z);

                // Check if we need to flush the batch
                if (batch.tile_count >= max_batch_size_) {
                    // Would need to render current batch and start a new one
                    // For now, just stop adding tiles
                    return;
                }
            }
        }
    }

    void TilemapRenderer::AddTileToBatch(const glm::vec2 &world_pos,
                                         const glm::ivec2 &tile_size,
                                         const glm::vec4 &tex_coords,
                                         const float opacity,
                                         TileBatch &batch,
                                         const float z) {
        const uint32_t vertex_offset = static_cast<uint32_t>(batch.vertices.size());

        // Create quad vertices
        const float half_width = tile_size.x * 0.5f;
        const float half_height = tile_size.y * 0.5f;

        // Bottom-left
        batch.vertices.push_back({
            {world_pos.x - half_width, world_pos.y - half_height, z},
            {tex_coords.x, tex_coords.y + tex_coords.w},
            opacity
        });

        // Bottom-right
        batch.vertices.push_back({
            {world_pos.x + half_width, world_pos.y - half_height, z},
            {tex_coords.x + tex_coords.z, tex_coords.y + tex_coords.w},
            opacity
        });

        // Top-right
        batch.vertices.push_back({
            {world_pos.x + half_width, world_pos.y + half_height, z},
            {tex_coords.x + tex_coords.z, tex_coords.y},
            opacity
        });

        // Top-left
        batch.vertices.push_back({
            {world_pos.x - half_width, world_pos.y + half_height, z},
            {tex_coords.x, tex_coords.y},
            opacity
        });

        // Create quad indices (two triangles)
        batch.indices.insert(batch.indices.end(), {
                                 vertex_offset + 0, vertex_offset + 1, vertex_offset + 2, // First triangle
                                 vertex_offset + 2, vertex_offset + 3, vertex_offset + 0 // Second triangle
                             });

        batch.tile_count++;
    }

    void TilemapRenderer::RenderBatch(const TileBatch &batch,
                                      const glm::mat4 &mvp_matrix,
                                      ShaderManager &shader_manager) {
        if (batch.vertices.empty()) return;

        // Get and use shader
        const auto &shader = shader_manager.GetShader(shader_id_);
        shader.Use();
        shader.SetUniform("u_mvp", mvp_matrix);
        shader.SetTexture("u_texture", batch.texture, 0);

        // Update vertex buffer
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        batch.vertices.size() * sizeof(TileVertex),
                        batch.vertices.data());

        // Update index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                        batch.indices.size() * sizeof(uint32_t),
                        batch.indices.data());

        // Draw
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indices.size()), GL_UNSIGNED_INT, 0);

        // Update stats
        stats_.draw_calls++;
        stats_.triangles += batch.indices.size() / 3;
        stats_.vertices += batch.vertices.size();

        glBindVertexArray(0);
    }

    ShaderId TilemapRenderer::CreateDefaultShader(const ShaderManager &shader_manager) {
        // Vertex shader source
        const std::string vertex_source = R"(
            #version 300 es
            layout (location = 0) in vec3 a_position;
            layout (location = 1) in vec2 a_tex_coords;
            layout (location = 2) in float a_opacity;

            uniform mat4 u_mvp;

            out vec2 v_tex_coords;
            out float v_opacity;

            void main() {
                gl_Position = u_mvp * vec4(a_position, 1.0);
                v_tex_coords = a_tex_coords;
                v_opacity = a_opacity;
            }
        )";

        // Fragment shader source
        const std::string fragment_source = R"(
            #version 300 es
            precision mediump float;

            in vec2 v_tex_coords;
            in float v_opacity;

            uniform sampler2D u_texture;

            out vec4 FragColor;

            void main() {
                vec4 tex_color = texture(u_texture, v_tex_coords);
                FragColor = vec4(tex_color.rgb, tex_color.a * v_opacity);
            }
        )";

        // Create shader using the existing shader manager
        return shader_manager.LoadShaderFromString("tilemap_default", vertex_source, fragment_source);
    }
}
