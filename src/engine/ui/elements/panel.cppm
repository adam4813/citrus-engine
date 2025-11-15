module;

#include <memory>
#include <string>
#include <algorithm>
#include <vector>
#include <utility>

export module engine.ui:elements.panel;

import :ui_element;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
    /**
     * @brief Container UI element with background, border, and child layout
     * 
     * Panel implements a rectangular container following the Composite pattern.
     * It provides:
     * - Background rendering with customizable color
     * - Optional border with configurable width and color
     * - Scissor clipping to constrain child rendering
     * - Padding for child layout
     * - Animation support via opacity changes
     * 
     * **Usage Pattern (Declarative + Reactive):**
     * @code
     * // Create panel once (declarative)
     * auto panel = std::make_unique<Panel>(100, 100, 300, 200);
     * panel->SetBackgroundColor(Colors::DARK_GRAY);
     * panel->SetBorderColor(Colors::GOLD);
     * panel->SetBorderWidth(2.0f);
     * panel->SetPadding(10.0f);
     * 
     * // Add children
     * auto button = std::make_unique<Button>(0, 0, 100, 30, "Click");
     * panel->AddChild(std::move(button));
     * 
     * // Render many times (no rebuilding)
     * panel->Render();
     * 
     * // React to events
     * panel->SetBackgroundColor(Colors::GRAY);  // Updates color
     * @endcode
     * 
     * **Features:**
     * - **Scissor Clipping**: Children are clipped to panel bounds
     * - **Padding**: Insets children from panel edges
     * - **Opacity**: Fade in/out animations via SetOpacity()
     * - **Borders**: Optional border rendering
     * 
     * @see UI_DEVELOPMENT_BIBLE.md §2.1 for Composite pattern details
     * @see UI_DEVELOPMENT_BIBLE.md §3 for common UI patterns (Card, Modal)
     */
    class Panel : public UIElement {
    public:
        /**
         * @brief Construct a panel with position and size
         * 
         * @param x X position relative to parent
         * @param y Y position relative to parent
         * @param width Panel width in pixels
         * @param height Panel height in pixels
         * 
         * @code
         * auto panel = std::make_unique<Panel>(100, 50, 400, 300);
         * @endcode
         */
        Panel(float x, float y, float width, float height)
            : UIElement(x, y, width, height)
            , background_color_(batch_renderer::Colors::DARK_GRAY)
            , border_color_(batch_renderer::Colors::LIGHT_GRAY)
            , border_width_(0.0f)
            , padding_(0.0f)
            , opacity_(1.0f)
            , clip_children_(true) {}

        virtual ~Panel() = default;

        // === Appearance Configuration ===

        /**
         * @brief Set background fill color
         * 
         * Color is applied with current opacity. Reactive update - no rebuild needed.
         * 
         * @param color New background color
         * 
         * @code
         * panel->SetBackgroundColor(Colors::DARK_GRAY);
         * panel->SetBackgroundColor(Color{0.2f, 0.2f, 0.3f, 1.0f});
         * @endcode
         */
        void SetBackgroundColor(const batch_renderer::Color& color) {
            background_color_ = color;
        }

        /**
         * @brief Get current background color
         * @return Background color (without opacity applied)
         */
        batch_renderer::Color GetBackgroundColor() const {
            return background_color_;
        }

        /**
         * @brief Set border color
         * 
         * Only visible if border_width > 0.
         * 
         * @param color New border color
         * 
         * @code
         * panel->SetBorderColor(Colors::GOLD);
         * panel->SetBorderWidth(2.0f);  // Make border visible
         * @endcode
         */
        void SetBorderColor(const batch_renderer::Color& color) {
            border_color_ = color;
        }

        /**
         * @brief Get current border color
         * @return Border color
         */
        batch_renderer::Color GetBorderColor() const {
            return border_color_;
        }

        /**
         * @brief Set border width in pixels
         * 
         * Set to 0 to disable border rendering.
         * 
         * @param width Border width in pixels (0 = no border)
         * 
         * @code
         * panel->SetBorderWidth(2.0f);  // 2px border
         * panel->SetBorderWidth(0.0f);  // No border
         * @endcode
         */
        void SetBorderWidth(float width) {
            border_width_ = width >= 0.0f ? width : 0.0f;
        }

        /**
         * @brief Get current border width
         * @return Border width in pixels
         */
        float GetBorderWidth() const {
            return border_width_;
        }

        /**
         * @brief Set opacity for fade animations
         * 
         * Affects background and children. Useful for show/hide animations.
         * 
         * @param opacity Opacity value (0.0 = transparent, 1.0 = opaque)
         * 
         * @code
         * // Fade in animation
         * panel->SetOpacity(0.0f);
         * panel->Show();
         * for (float t = 0; t <= 1.0f; t += 0.016f) {
         *     panel->SetOpacity(t);
         *     panel->Render();
         * }
         * @endcode
         */
        void SetOpacity(float opacity) {
            opacity_ = std::clamp(opacity, 0.0f, 1.0f);
        }

        /**
         * @brief Get current opacity
         * @return Opacity value (0.0-1.0)
         */
        float GetOpacity() const {
            return opacity_;
        }

        // === Layout Configuration ===

        /**
         * @brief Set padding (insets children from edges)
         * 
         * Padding is applied to all sides equally and automatically offsets children's
         * positions during rendering. Children positioned at (0, 0) will render at
         * (padding, padding) relative to the panel's top-left corner.
         * 
         * The padding affects:
         * - The scissor clipping rectangle (content outside padding is clipped)
         * - Children's rendered positions (automatically offset by padding amount)
         * 
         * This provides intuitive layout behavior similar to CSS padding, where children
         * are automatically inset from the panel edges.
         * 
         * @param padding Padding in pixels
         * 
         * @code
         * panel->SetPadding(10.0f);  // 10px padding on all sides
         * 
         * // Child positioned at (0, 0) will render at (10, 10):
         * auto child = std::make_unique<Button>(0, 0, 100, 30, "OK");
         * panel->AddChild(std::move(child));
         * @endcode
         */
        void SetPadding(float padding) {
            padding_ = padding >= 0.0f ? padding : 0.0f;
        }

        /**
         * @brief Get current padding
         * @return Padding in pixels
         */
        float GetPadding() const {
            return padding_;
        }

        /**
         * @brief Enable/disable scissor clipping for children
         * 
         * When enabled, children are clipped to panel bounds.
         * Disable for panels where children should overflow (e.g., tooltips).
         * 
         * @param clip True to clip children, false to allow overflow
         * 
         * @code
         * panel->SetClipChildren(true);   // Clip (default)
         * panel->SetClipChildren(false);  // Allow overflow
         * @endcode
         */
        void SetClipChildren(bool clip) {
            clip_children_ = clip;
        }

        /**
         * @brief Check if scissor clipping is enabled
         * @return True if children are clipped
         */
        bool GetClipChildren() const {
            return clip_children_;
        }

        // === Rendering ===

        /**
         * @brief Render panel background, border, and children
         * 
         * Rendering order:
         * 1. Background quad (with opacity)
         * 2. Border lines (if border_width > 0)
         * 3. Push scissor (if clip_children_ is true)
         * 4. Apply padding offset to children positions
         * 5. Render children (recursively)
         * 6. Restore children positions
         * 7. Pop scissor
         * 
         * Children are clipped to panel bounds via scissor test.
         * Padding automatically offsets children from panel edges.
         * 
         * @code
         * panel->Render();  // Renders panel and all children
         * @endcode
         */
        void Render() const override {
            using namespace batch_renderer;

            // Skip if not visible
            if (!IsVisible()) {
                return;
            }

            const Rectangle bounds = GetAbsoluteBounds();

            // Render background
            Color bg_color = Color::Alpha(background_color_, opacity_);
            BatchRenderer::SubmitQuad(bounds, bg_color);

            // Render border if width > 0
            if (border_width_ > 0.0f) {
                const Color border_color = Color::Alpha(border_color_, opacity_);
                const float x = bounds.x;
                const float y = bounds.y;
                const float w = bounds.width;
                const float h = bounds.height;

                // Top edge
                BatchRenderer::SubmitLine(x, y, x + w, y, border_width_, border_color);
                // Right edge
                BatchRenderer::SubmitLine(x + w, y, x + w, y + h, border_width_, border_color);
                // Bottom edge
                BatchRenderer::SubmitLine(x + w, y + h, x, y + h, border_width_, border_color);
                // Left edge
                BatchRenderer::SubmitLine(x, y + h, x, y, border_width_, border_color);
            }

            // Push scissor for children (if clipping enabled)
            if (clip_children_) {
                const ScissorRect scissor{
                    bounds.x + padding_,
                    bounds.y + padding_,
                    bounds.width - padding_ * 2.0f,
                    bounds.height - padding_ * 2.0f
                };
                BatchRenderer::PushScissor(scissor);
            }

            // Apply padding offset to children for rendering
            // Store original positions to restore after rendering
            std::vector<std::pair<float, float>> original_positions;
            if (padding_ > 0.0f) {
                original_positions.reserve(GetChildren().size());
                for (const auto& child : GetChildren()) {
                    const auto child_bounds = child->GetRelativeBounds();
                    original_positions.emplace_back(child_bounds.x, child_bounds.y);
                    // Offset child position by padding
                    child->SetRelativePosition(child_bounds.x + padding_, child_bounds.y + padding_);
                }
            }

            // Render children
            for (const auto& child : GetChildren()) {
                child->Render();
            }

            // Restore original positions
            if (padding_ > 0.0f) {
                size_t i = 0;
                for (const auto& child : GetChildren()) {
                    child->SetRelativePosition(original_positions[i].first, original_positions[i].second);
                    ++i;
                }
            }

            // Pop scissor
            if (clip_children_) {
                BatchRenderer::PopScissor();
            }
        }

    private:
        batch_renderer::Color background_color_;
        batch_renderer::Color border_color_;
        float border_width_;
        float padding_;
        float opacity_;
        bool clip_children_;
    };

} // namespace engine::ui::elements
