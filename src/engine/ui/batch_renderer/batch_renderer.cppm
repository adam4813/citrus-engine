module;

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

export module engine.ui.batch_renderer:batch_renderer;

import :batch_types;

export namespace engine::ui::batch_renderer {
    /**
     * @brief Core batched UI renderer
     *
     * Converts individual draw calls into batched vertex/index buffers
     * to minimize GPU submissions. Supports scissor clipping and texture
     * slot management (up to 8 texture units).
     *
     * Usage:
     *   BatchRenderer::Initialize();
     *
     *   // Each frame:
     *   BatchRenderer::BeginFrame();
     *   BatchRenderer::SubmitQuad(...);
     *   BatchRenderer::SubmitText(...);
     *   BatchRenderer::EndFrame();  // Flushes all batches
     *
     *   BatchRenderer::Shutdown();
     */
    class BatchRenderer {
    public:
        // Maximum texture slots supported (modern GL standard)
        static constexpr int MAX_TEXTURE_SLOTS = 8;

        // Reserve capacity to avoid frequent reallocations
        static constexpr size_t INITIAL_VERTEX_CAPACITY = 32768;
        static constexpr size_t INITIAL_INDEX_CAPACITY = 98304;

        /**
         * @brief Initialize the batch renderer
         *
         * Call once at application startup.
         */
        static void Initialize();

        /**
         * @brief Shutdown and cleanup resources
         *
         * Call once at application shutdown.
         */
        static void Shutdown();

        /**
         * @brief Begin a new frame
         *
         * Resets batch state and prepares for new submissions.
         * Call at the start of each frame's UI rendering.
         */
        static void BeginFrame();

        /**
         * @brief End frame and flush remaining batches
         *
         * Submits all pending draw calls to GPU.
         * Call at the end of each frame's UI rendering.
         */
        static void EndFrame();

        /**
         * @brief Push a scissor region (clipping)
         *
         * New scissor is intersected with current top of stack.
         * Changing scissor triggers batch flush if needed.
         *
         * @param scissor Screen-space scissor rectangle
         */
        static void PushScissor(const ScissorRect &scissor);

        /**
         * @brief Pop scissor region
         *
         * Restores previous scissor state.
         * Triggers batch flush if scissor changes.
         */
        static void PopScissor();

        /**
         * @brief Get current scissor region
         */
        static ScissorRect GetCurrentScissor();

        /**
         * @brief Submit a quad (filled rectangle)
         *
         * @param rect Screen-space rectangle
         * @param color Fill color
         * @param uv_coords Optional texture coordinates (default: white pixel)
         * @param texture_id Texture ID (0 = white pixel texture)
         */
        static void SubmitQuad(
            const Rectangle &rect,
            const Color &color,
            const std::optional<Rectangle> &uv_coords = std::nullopt,
            uint32_t texture_id = 0
        );

        /**
         * @brief Submit a line (tessellated as quad)
         *
         * @param x0 Start X
         * @param y0 Start Y
         * @param x1 End X
         * @param y1 End Y
         * @param thickness Line thickness in pixels
         * @param color Line color
         * @param texture_id Texture ID (0 = white pixel)
         */
        static void SubmitLine(
            float x0, float y0,
            float x1, float y1,
            float thickness,
            const Color &color,
            uint32_t texture_id = 0
        );

        /**
         * @brief Submit a circle (tessellated as triangle fan)
         *
         * @param center_x Circle center X
         * @param center_y Circle center Y
         * @param radius Circle radius
         * @param color Fill color
         * @param segments Number of segments (default: 32)
         */
        static void SubmitCircle(
            float center_x,
            float center_y,
            float radius,
            const Color &color,
            int segments = 32
        );

        /**
         * @brief Submit rounded rectangle (corners tessellated)
         *
         * @param rect Screen-space rectangle
         * @param corner_radius Radius of rounded corners
         * @param color Fill color
         * @param corner_segments Segments per corner (default: 8)
         */
        static void SubmitRoundedRect(
            const Rectangle &rect,
            float corner_radius,
            const Color &color,
            int corner_segments = 8
        );

        /**
         * @brief Submit text (flushes batch, calls native rendering)
         *
         * Text rendering uses native rendering as it's already optimized.
         * We flush current batch, draw text, then resume batching.
         *
         * @param text Text to render
         * @param x X position
         * @param y Y position
         * @param font_size Font size
         * @param color Text color
         */
        static void SubmitText(
            const std::string &text,
            float x,
            float y,
            int font_size,
            const Color &color
        );

        /**
         * @brief Submit text within a bounding rect (with clipping)
         *
         * @param rect Bounding rectangle for text
         * @param text Text to render
         * @param font_size Font size
         * @param color Text color
         */
        static void SubmitTextRect(
            const Rectangle &rect,
            const std::string &text,
            int font_size,
            const Color &color
        );

        /**
         * @brief Manually flush current batch
         *
         * Typically not needed as flush happens automatically when:
         * - Scissor changes
         * - 9th texture would be added (exceeds 8-slot limit)
         * - EndFrame() is called
         *
         * Exposed for debugging and advanced use cases.
         */
        static void Flush();

        /**
         * @brief Get pending vertex count in current batch
         */
        static size_t GetPendingVertexCount();

        /**
         * @brief Get pending index count in current batch
         */
        static size_t GetPendingIndexCount();

        /**
         * @brief Get total draw call count for current frame
         */
        static size_t GetDrawCallCount();

        /**
         * @brief Reset draw call counter (for new frame)
         */
        static void ResetDrawCallCount();

    private:
        struct BatchState;
        static std::unique_ptr<BatchState> state_;

        // Internal helpers
        static void PushQuadVertices(
            float x0, float y0, float u0, float v0,
            float x1, float y1, float u1, float v1,
            float x2, float y2, float u2, float v2,
            float x3, float y3, float u3, float v3,
            uint32_t color,
            float tex_index
        );

        static void PushQuadIndices(uint32_t base_vertex);

        static bool ShouldFlush(uint32_t texture_id);

        static int GetOrAddTextureSlot(uint32_t texture_id);

        static void FlushBatch();

        static void StartNewBatch();
    };
} // namespace engine::ui::batch_renderer
