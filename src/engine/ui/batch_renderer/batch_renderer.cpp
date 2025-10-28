module;

#include <cmath>
#include <algorithm>

module engine.ui.batch_renderer;

namespace engine::ui::batch_renderer {
    struct BatchRenderer::BatchState {
        // Current batch buffers
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<uint32_t, int> texture_slots;

        // Scissor stack
        std::vector<ScissorRect> scissor_stack;
        ScissorRect current_scissor;

        // Stats
        size_t draw_call_count = 0;
        bool initialized = false;
        bool in_frame = false;

        // White pixel texture for untextured draws
        uint32_t white_texture_id = 0;

        BatchState() {
            vertices.reserve(INITIAL_VERTEX_CAPACITY);
            indices.reserve(INITIAL_INDEX_CAPACITY);
        }
    };

    std::unique_ptr<BatchRenderer::BatchState> BatchRenderer::state_ = nullptr;

    void BatchRenderer::Initialize() {
        if (!state_) {
            state_ = std::make_unique<BatchState>();

            // Create 1x1 white texture for untextured draws
            Image white_img = GenImageColor(1, 1, WHITE);
            Texture2D white_tex = LoadTextureFromImage(white_img);
            UnloadImage(white_img);

            state_->white_texture_id = white_tex.id;
            state_->initialized = true;
        }
    }

    void BatchRenderer::Shutdown() {
        if (state_ && state_->initialized) {
            if (state_->white_texture_id != 0) {
                UnloadTexture(Texture2D{state_->white_texture_id});
            }
            state_.reset();
        }
    }

    void BatchRenderer::BeginFrame() {
        if (!state_ || !state_->initialized) {
            Initialize();
        }

        state_->vertices.clear();
        state_->indices.clear();
        state_->texture_slots.clear();
        state_->scissor_stack.clear();
        state_->draw_call_count = 0;
        state_->in_frame = true;

        // Initialize scissor to full screen
        const int screen_width = GetScreenWidth();
        const int screen_height = GetScreenHeight();
        state_->current_scissor = ScissorRect(0, 0,
                                              static_cast<float>(screen_width),
                                              static_cast<float>(screen_height));
    }

    void BatchRenderer::EndFrame() {
        if (!state_ || !state_->in_frame) return;

        // Flush any remaining batched draws
        if (!state_->vertices.empty()) {
            FlushBatch();
        }

        state_->in_frame = false;
    }

    void BatchRenderer::PushScissor(const ScissorRect &scissor) {
        if (!state_) return;

        // Intersect with current scissor
        ScissorRect new_scissor = state_->current_scissor.Intersect(scissor);

        // If scissor changed, flush current batch
        if (new_scissor != state_->current_scissor && !state_->vertices.empty()) {
            FlushBatch();
        }

        state_->scissor_stack.push_back(state_->current_scissor);
        state_->current_scissor = new_scissor;
    }

    void BatchRenderer::PopScissor() {
        if (!state_ || state_->scissor_stack.empty()) return;

        ScissorRect previous = state_->scissor_stack.back();
        state_->scissor_stack.pop_back();

        // If scissor changed, flush current batch
        if (previous != state_->current_scissor && !state_->vertices.empty()) {
            FlushBatch();
        }

        state_->current_scissor = previous;
    }

    ScissorRect BatchRenderer::GetCurrentScissor() {
        return state_ ? state_->current_scissor : ScissorRect();
    }

    void BatchRenderer::SubmitQuad(
        const Rectangle &rect,
        const Color &color,
        const std::optional<Rectangle> &uv_coords,
        uint32_t texture_id
    ) {
        if (!state_) return;

        // Use white texture if none specified
        if (texture_id == 0) {
            texture_id = state_->white_texture_id;
        }

        // Check if we need to flush
        if (ShouldFlush(texture_id)) {
            FlushBatch();
        }

        // Get texture slot
        const int tex_slot = GetOrAddTextureSlot(texture_id);
        const uint32_t packed_color = ColorToRGBA(color);

        // UV coordinates (default to full texture)
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        if (uv_coords.has_value()) {
            const Rectangle &uv = uv_coords.value();
            u0 = uv.x;
            v0 = uv.y;
            u1 = uv.x + uv.width;
            v1 = uv.y + uv.height;
        }

        // Quad vertices (top-left origin)

        const float x0 = rect.x;
        const float y0 = rect.y + rect.height; // Note: y-axis inverted
        const float x1 = rect.x + rect.width;
        const float y1 = rect.y;

        PushQuadVertices(
            x0, y0, u0, v0, // Top-left
            x1, y0, u1, v0, // Top-right
            x1, y1, u1, v1, // Bottom-right
            x0, y1, u0, v1, // Bottom-left
            packed_color,
            static_cast<float>(tex_slot)
        );

        const uint32_t base = static_cast<uint32_t>(state_->vertices.size()) - 4;
        PushQuadIndices(base);
    }

    void BatchRenderer::SubmitLine(
        const float x0, const float y0,
        const float x1, const float y1,
        const float thickness,
        const Color &color,
        uint32_t texture_id
    ) {
        if (!state_) return;

        // Tessellate line as quad (2 triangles)
        const float dx = x1 - x0;
        const float dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);

        if (len < 0.001f) return; // Degenerate line

        // Perpendicular vector (normalized, scaled by half thickness)
        const float nx = -dy / len * (thickness * 0.5f);
        const float ny = dx / len * (thickness * 0.5f);

        // Quad corners
        const float xa = x0 + nx;
        const float ya = y0 + ny;
        const float xb = x0 - nx;
        const float yb = y0 - ny;
        const float xc = x1 - nx;
        const float yc = y1 - ny;
        const float xd = x1 + nx;
        const float yd = y1 + ny;

        // Use white texture
        if (texture_id == 0) {
            texture_id = state_->white_texture_id;
        }

        if (ShouldFlush(texture_id)) {
            FlushBatch();
        }

        const int tex_slot = GetOrAddTextureSlot(texture_id);
        const uint32_t packed_color = ColorToRGBA(color);

        PushQuadVertices(
            xa, ya, 0.0f, 0.0f,
            xd, yd, 1.0f, 0.0f,
            xc, yc, 1.0f, 1.0f,
            xb, yb, 0.0f, 1.0f,
            packed_color,
            static_cast<float>(tex_slot)
        );

        const uint32_t base = static_cast<uint32_t>(state_->vertices.size()) - 4;
        PushQuadIndices(base);
    }

    void BatchRenderer::SubmitCircle(
        float center_x,
        float center_y,
        const float radius,
        const Color &color,
        const int segments
    ) {
        if (!state_ || segments < 3) return;

        const uint32_t texture_id = state_->white_texture_id;

        if (ShouldFlush(texture_id)) {
            FlushBatch();
        }

        const int tex_slot = GetOrAddTextureSlot(texture_id);
        const uint32_t packed_color = ColorToRGBA(color);
        const auto tex_index = static_cast<float>(tex_slot);

        // Center vertex
        const auto center_idx = static_cast<uint32_t>(state_->vertices.size());
        state_->vertices.emplace_back(center_x, center_y, 0.5f, 0.5f, packed_color, tex_index);

        // Perimeter vertices (triangle fan)
        const float angle_step = 2.0f * PI / static_cast<float>(segments);
        for (int i = 0; i <= segments; ++i) {
            const float angle = static_cast<float>(i) * angle_step;
            const float x = center_x + std::cos(angle) * radius;
            const float y = center_y + std::sin(angle) * radius;
            state_->vertices.emplace_back(x, y, 0.5f, 0.5f, packed_color, tex_index);
        }

        // Indices (triangle fan)
        for (int i = 0; i < segments; ++i) {
            state_->indices.push_back(center_idx);
            state_->indices.push_back(center_idx + 1 + i);
            state_->indices.push_back(center_idx + 1 + i + 1);
        }
    }

    void BatchRenderer::SubmitRoundedRect(
        const Rectangle &rect,
        float corner_radius,
        const Color &color,
        const int corner_segments
    ) {
        if (!state_ || corner_segments < 1) return;

        // Clamp corner radius
        const float max_radius = std::min(rect.width, rect.height) * 0.5f;
        corner_radius = std::min(corner_radius, max_radius);

        if (corner_radius < 0.1f) {
            // Degenerate to normal quad
            SubmitQuad(rect, color);
            return;
        }

        const uint32_t texture_id = state_->white_texture_id;

        if (ShouldFlush(texture_id)) {
            FlushBatch();
        }

        const int tex_slot = GetOrAddTextureSlot(texture_id);
        const uint32_t packed_color = ColorToRGBA(color);
        const auto tex_index = static_cast<float>(tex_slot);

        // Center rectangle (non-rounded part)
        const float inner_x = rect.x + corner_radius;
        const float inner_y = rect.y + corner_radius;
        const float inner_w = rect.width - 2 * corner_radius;
        const float inner_h = rect.height - 2 * corner_radius;

        // Center quad
        if (inner_w > 0 && inner_h > 0) {
            SubmitQuad(Rectangle{inner_x, inner_y, inner_w, inner_h}, color);
        }

        // Four edge rectangles
        SubmitQuad(Rectangle{inner_x, rect.y, inner_w, corner_radius}, color); // Top
        SubmitQuad(Rectangle{inner_x, rect.y + rect.height - corner_radius, inner_w, corner_radius}, color); // Bottom
        SubmitQuad(Rectangle{rect.x, inner_y, corner_radius, inner_h}, color); // Left
        SubmitQuad(Rectangle{rect.x + rect.width - corner_radius, inner_y, corner_radius, inner_h}, color); // Right

        // Four rounded corners (quarter circles)
        const Vector2 corners[4] = {
            {inner_x, inner_y}, // Top-left
            {inner_x + inner_w, inner_y}, // Top-right
            {inner_x + inner_w, inner_y + inner_h}, // Bottom-right
            {inner_x, inner_y + inner_h} // Bottom-left
        };

        constexpr float angle_offsets[4] = {PI, PI * 0.5f, 0.0f, PI * 1.5f};

        for (int c = 0; c < 4; ++c) {
            const auto center_idx = static_cast<uint32_t>(state_->vertices.size());
            state_->vertices.emplace_back(corners[c].x, corners[c].y, 0.5f, 0.5f, packed_color, tex_index);

            const float angle_start = angle_offsets[c];
            const float angle_step = (PI * 0.5f) / static_cast<float>(corner_segments);

            for (int i = 0; i <= corner_segments; ++i) {
                const float angle = angle_start + static_cast<float>(i) * angle_step;
                const float x = corners[c].x + std::cos(angle) * corner_radius;
                const float y = corners[c].y + std::sin(angle) * corner_radius;
                state_->vertices.emplace_back(x, y, 0.5f, 0.5f, packed_color, tex_index);
            }

            for (int i = 0; i < corner_segments; ++i) {
                state_->indices.push_back(center_idx);
                state_->indices.push_back(center_idx + 1 + i);
                state_->indices.push_back(center_idx + 1 + i + 1);
            }
        }
    }

    void BatchRenderer::SubmitText(
        const std::string &text,
        const float x,
        const float y,
        const int font_size,
        const Color &color
    ) {
        if (!state_ || text.empty()) return;

        // Flush current batch before native text draw
        if (!state_->vertices.empty()) {
            FlushBatch();
        }

        // Use native Raylib text rendering (already optimized)
        DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), font_size, color);

        // Count as 1 draw call
        state_->draw_call_count++;
    }

    void BatchRenderer::SubmitTextRect(
        const Rectangle &rect,
        const std::string &text,
        const int font_size,
        const Color &color
    ) {
        if (!state_ || text.empty()) return;

        // Flush and set scissor for text clipping
        if (!state_->vertices.empty()) {
            FlushBatch();
        }

        // Apply scissor
        BeginScissorMode(
            static_cast<int>(rect.x),
            static_cast<int>(rect.y),
            static_cast<int>(rect.width),
            static_cast<int>(rect.height)
        );

        DrawText(text.c_str(), static_cast<int>(rect.x), static_cast<int>(rect.y), font_size, color);

        EndScissorMode();

        state_->draw_call_count++;
    }

    void BatchRenderer::Flush() {
        if (!state_ || state_->vertices.empty()) return;
        FlushBatch();
    }

    size_t BatchRenderer::GetPendingVertexCount() {
        return state_ ? state_->vertices.size() : 0;
    }

    size_t BatchRenderer::GetPendingIndexCount() {
        return state_ ? state_->indices.size() : 0;
    }

    size_t BatchRenderer::GetDrawCallCount() {
        return state_ ? state_->draw_call_count : 0;
    }

    void BatchRenderer::ResetDrawCallCount() {
        if (state_) {
            state_->draw_call_count = 0;
        }
    }

    // Private helper implementations

    void BatchRenderer::PushQuadVertices(
        float x0, float y0, float u0, float v0,
        float x1, float y1, float u1, float v1,
        float x2, float y2, float u2, float v2,
        float x3, float y3, float u3, float v3,
        uint32_t color,
        float tex_index
    ) {
        state_->vertices.emplace_back(x0, y0, u0, v0, color, tex_index);
        state_->vertices.emplace_back(x1, y1, u1, v1, color, tex_index);
        state_->vertices.emplace_back(x2, y2, u2, v2, color, tex_index);
        state_->vertices.emplace_back(x3, y3, u3, v3, color, tex_index);
    }

    void BatchRenderer::PushQuadIndices(const uint32_t base_vertex) {
        // Two triangles: 0-1-2, 2-3-0
        state_->indices.push_back(base_vertex + 0);
        state_->indices.push_back(base_vertex + 1);
        state_->indices.push_back(base_vertex + 2);

        state_->indices.push_back(base_vertex + 2);
        state_->indices.push_back(base_vertex + 3);
        state_->indices.push_back(base_vertex + 0);
    }

    bool BatchRenderer::ShouldFlush(const uint32_t texture_id) {
        if (!state_) return false;

        // Check if adding this texture would exceed limit
        if (state_->texture_slots.find(texture_id) == state_->texture_slots.end()) {
            if (state_->texture_slots.size() >= MAX_TEXTURE_SLOTS) {
                return true;
            }
        }

        return false;
    }

    int BatchRenderer::GetOrAddTextureSlot(const uint32_t texture_id) {
        auto it = state_->texture_slots.find(texture_id);
        if (it != state_->texture_slots.end()) {
            return it->second;
        }

        const int slot = static_cast<int>(state_->texture_slots.size());
        state_->texture_slots[texture_id] = slot;
        return slot;
    }

    void BatchRenderer::FlushBatch() {
        if (!state_ || state_->vertices.empty()) return;

        // Apply scissor if active
        const bool use_scissor = state_->current_scissor.IsValid();
        if (use_scissor) {
            BeginScissorMode(
                static_cast<int>(state_->current_scissor.x),
                static_cast<int>(state_->current_scissor.y),
                static_cast<int>(state_->current_scissor.width),
                static_cast<int>(state_->current_scissor.height)
            );
        }

        // Use Raylib's internal mesh rendering
        // For simplicity, we'll use DrawTriangleStrip or manual vertex submission
        // In a real implementation, you'd upload to VBO/IBO and use glDrawElements

        // Draw triangles using indices
        for (size_t i = 0; i < state_->indices.size(); i += 3) {
            if (i + 2 >= state_->indices.size()) break;

            const Vertex &v0 = state_->vertices[state_->indices[i + 0]];
            const Vertex &v1 = state_->vertices[state_->indices[i + 1]];
            const Vertex &v2 = state_->vertices[state_->indices[i + 2]];

            const Color c0 = RGBAToColor(v0.color);
            const Color c1 = RGBAToColor(v1.color);
            const Color c2 = RGBAToColor(v2.color);

            // Use Raylib's immediate mode (simplified)
            DrawTriangle(
                Vector2{v0.x, v0.y},
                Vector2{v1.x, v1.y},
                Vector2{v2.x, v2.y},
                c0 // Simplified: use first vertex color
            );
        }

        if (use_scissor) {
            EndScissorMode();
        }

        state_->draw_call_count++;

        // Clear batch for next submission
        StartNewBatch();
    }

    void BatchRenderer::StartNewBatch() {
        if (!state_) return;

        state_->vertices.clear();
        state_->indices.clear();
        state_->texture_slots.clear();
    }
} // namespace engine::ui::batch_renderer
