module;

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>

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
         * @brief Submit text for rendering using the font atlas system.
         *
         * Renders text at the specified position using the default font from FontManager.
         * Text is rendered as textured quads from the font atlas, automatically batched
         * with other UI elements for efficient GPU submission.
         *
         * @param text UTF-8 encoded text string to render (supports newlines)
         * @param x X position in screen space (top-left of text)
         * @param y Y position in screen space (top-left of text)
         * @param font_size Font size in pixels (currently uses default font size; for variable sizes, pre-load fonts with FontManager::GetFont)
         * @param color Text color (RGBA, 0.0-1.0 range)
         * 
         * @note Requires FontManager to be initialized with a default font.
         * @note Text is laid out left-aligned with no word wrapping.
         * @note All glyphs from the same font are batched into a single draw call.
         * 
         * @code
         * FontManager::Initialize("assets/fonts/Roboto.ttf", 16);
         * BatchRenderer::BeginFrame();
         * 
         * Color white{1.0f, 1.0f, 1.0f, 1.0f};
         * BatchRenderer::SubmitText("Score: 100", 10.0f, 10.0f, 16, white);
         * 
         * BatchRenderer::EndFrame();
         * @endcode
         * 
         * @see text_renderer::FontManager
         * @see SubmitTextRect
         */
        static void SubmitText(
            const std::string &text,
            float x,
            float y,
            int font_size,
            const Color &color
        );

        /**
         * @brief Submit text within a bounding rectangle with clipping and wrapping.
         *
         * Renders text within the specified rectangle, with automatic word wrapping
         * and scissor-based clipping. Text that exceeds the rectangle bounds is clipped.
         * 
         * @param rect Bounding rectangle for text (x, y, width, height)
         * @param text UTF-8 encoded text string to render
         * @param font_size Font size in pixels (currently uses default font size; for variable sizes, pre-load fonts with FontManager::GetFont)
         * @param color Text color (RGBA, 0.0-1.0 range)
         * 
         * @note Requires FontManager to be initialized with a default font.
         * @note Text automatically wraps at rectangle width boundaries.
         * @note Text is left-aligned and top-aligned within the rectangle.
         * @note Scissor test ensures text doesn't render outside the rectangle.
         * 
         * @code
         * FontManager::Initialize("assets/fonts/Roboto.ttf", 16);
         * BatchRenderer::BeginFrame();
         * 
         * Rectangle textBox{50.0f, 50.0f, 300.0f, 150.0f};
         * Color white{1.0f, 1.0f, 1.0f, 1.0f};
         * BatchRenderer::SubmitTextRect(
         *     textBox,
         *     "This is a long text that will wrap within the box",
         *     16, white
         * );
         * 
         * BatchRenderer::EndFrame();
         * @endcode
         * 
         * @see text_renderer::FontManager
         * @see SubmitText
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
            const Color& color,
            float tex_index
        );

        static void PushQuadIndices(uint32_t base_vertex);

        static bool ShouldFlush(uint32_t texture_id);

        static int GetOrAddTextureSlot(uint32_t texture_id);

        static void FlushBatch();

        static void StartNewBatch();
    };
} // namespace engine::ui::batch_renderer
