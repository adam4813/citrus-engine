module;

#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

export module engine.ui:ui_element;

import engine.ui.batch_renderer;

export namespace engine::ui {
    /**
     * @brief Mouse event data for UI interaction
     * 
     * Contains mouse position and button states for event handling.
     * Button states distinguish between "down" (currently pressed) and
     * "clicked" (just pressed this frame).
     */
    struct MouseEvent {
        float x, y;                      ///< Screen coordinates
        bool left_button_down;           ///< Left button currently held
        bool right_button_down;          ///< Right button currently held
        bool left_button_clicked;        ///< Left button just pressed this frame
        bool right_button_clicked;       ///< Right button just pressed this frame
        float scroll_delta;              ///< Scroll wheel delta
    };

    /**
     * @brief Base class for all UI elements
     * 
     * UIElement implements the Composite pattern (UI_DEVELOPMENT_BIBLE.md §2.1),
     * allowing elements to form tree structures. It provides:
     * 
     * - **Tree Structure**: Parent-child hierarchy management
     * - **Bounds Calculation**: Relative and absolute positioning
     * - **Hit Testing**: Mouse intersection detection
     * - **State Management**: Focus, hover, visibility flags
     * - **Event Handling**: Stub handlers for mouse events (to be implemented later)
     * 
     * **Usage Pattern (Declarative + Reactive):**
     * @code
     * // Build once (declarative)
     * auto panel = std::make_unique<Panel>(0, 0, 200, 100);
     * auto button = std::make_unique<Button>(10, 10, 80, 30, "Click");
     * button->SetClickCallback([](){ // react to event
     * });
     * panel->AddChild(std::move(button));
     * 
     * // Render many times (no per-frame rebuilding)
     * panel->Render();
     * @endcode
     * 
     * **Coordinate Systems:**
     * - **Relative**: Position relative to parent (stored in relative_x_, relative_y_)
     * - **Absolute**: Screen-space position (computed via GetAbsoluteBounds())
     * 
     * **Ownership:**
     * - Parent OWNS children via std::unique_ptr
     * - Children store non-owning raw pointer to parent
     * - Safe because children never outlive parent
     * 
     * @see UI_DEVELOPMENT_BIBLE.md for complete patterns and best practices
     */
    class UIElement {
    public:
        virtual ~UIElement() = default;

        // === Tree Structure (Composite Pattern) ===

        /**
         * @brief Add a child element
         * 
         * Transfers ownership of child to this element. Sets this as the child's parent.
         * 
         * @param child Child element to add (ownership transferred)
         * 
         * @code
         * auto panel = std::make_unique<Panel>(0, 0, 200, 100);
         * auto button = std::make_unique<Button>(10, 10, 80, 30, "Click");
         * panel->AddChild(std::move(button));
         * @endcode
         */
        void AddChild(std::unique_ptr<UIElement> child);

        /**
         * @brief Remove a child element
         * 
         * Searches for child by pointer and removes it, destroying the child.
         * 
         * @param child Pointer to child to remove (must be a direct child)
         * 
         * @code
         * Button* btn_ptr = button.get();
         * panel->AddChild(std::move(button));
         * // Later...
         * panel->RemoveChild(btn_ptr);  // Destroys the button
         * @endcode
         */
        void RemoveChild(UIElement* child);

        /**
         * @brief Set parent element
         * 
         * Sets the parent pointer. Typically called automatically by AddChild().
         * 
         * @param parent Parent element (non-owning pointer)
         */
        void SetParent(UIElement* parent) { parent_ = parent; }

        /**
         * @brief Get parent element
         * @return Pointer to parent, or nullptr if root element
         */
        UIElement* GetParent() const { return parent_; }

        /**
         * @brief Get all children
         * @return Reference to children vector (read-only)
         */
        const std::vector<std::unique_ptr<UIElement>>& GetChildren() const { return children_; }

        // === Bounds Calculation ===

        /**
         * @brief Get absolute bounds in screen space
         * 
         * Walks up the parent chain to compute screen-space position.
         * This is the position where the element is actually rendered.
         * 
         * @return Rectangle in absolute screen coordinates
         * 
         * @code
         * // Panel at (100, 50), button at relative (10, 10)
         * auto abs_bounds = button->GetAbsoluteBounds();
         * // abs_bounds.x = 110, abs_bounds.y = 60
         * @endcode
         * 
         * @see UI_DEVELOPMENT_BIBLE.md §9 for coordinate system details
         */
        batch_renderer::Rectangle GetAbsoluteBounds() const;

        /**
         * @brief Get relative bounds (position relative to parent)
         * @return Rectangle with relative position and size
         */
        batch_renderer::Rectangle GetRelativeBounds() const {
            return {relative_x_, relative_y_, width_, height_};
        }

        /**
         * @brief Set relative position (within parent)
         * @param x X coordinate relative to parent
         * @param y Y coordinate relative to parent
         */
        void SetRelativePosition(float x, float y) {
            relative_x_ = x;
            relative_y_ = y;
        }

        /**
         * @brief Set element size
         * @param width Element width in pixels
         * @param height Element height in pixels
         */
        void SetSize(float width, float height) {
            width_ = width;
            height_ = height;
        }

        /**
         * @brief Get element width
         * @return Width in pixels
         */
        float GetWidth() const { return width_; }

        /**
         * @brief Get element height
         * @return Height in pixels
         */
        float GetHeight() const { return height_; }

        // === Hit Testing ===

        /**
         * @brief Check if a point is within this element's bounds
         * 
         * Uses absolute bounds for hit testing.
         * 
         * @param x Screen X coordinate
         * @param y Screen Y coordinate
         * @return true if point is inside bounds, false otherwise
         * 
         * @code
         * if (button->Contains(mouseX, mouseY)) {
         *     // Mouse is over button
         * }
         * @endcode
         */
        bool Contains(float x, float y) const;

        // === State Management ===

        /**
         * @brief Set focused state
         * @param focused True to focus, false to unfocus
         */
        void SetFocused(bool focused) { is_focused_ = focused; }

        /**
         * @brief Check if element is focused
         * @return true if focused, false otherwise
         */
        bool IsFocused() const { return is_focused_; }

        /**
         * @brief Set hovered state
         * @param hovered True if mouse is over element
         */
        void SetHovered(bool hovered) { is_hovered_ = hovered; }

        /**
         * @brief Check if element is hovered
         * @return true if hovered, false otherwise
         */
        bool IsHovered() const { return is_hovered_; }

        /**
         * @brief Set visibility
         * @param visible True to show, false to hide
         */
        void SetVisible(bool visible) { is_visible_ = visible; }

        /**
         * @brief Check if element is visible
         * @return true if visible, false otherwise
         */
        bool IsVisible() const { return is_visible_; }

        // === Rendering (Template Method Pattern) ===

        /**
         * @brief Render this element and its children
         * 
         * Pure virtual function that subclasses must implement.
         * Should submit vertices via BatchRenderer and recursively
         * render children.
         * 
         * @code
         * void MyElement::Render() const override {
         *     using namespace engine::ui::batch_renderer;
         *     
         *     // Render self
         *     BatchRenderer::SubmitQuad(GetAbsoluteBounds(), color_);
         *     
         *     // Render children
         *     for (const auto& child : GetChildren()) {
         *         child->Render();
         *     }
         * }
         * @endcode
         * 
         * @see UI_DEVELOPMENT_BIBLE.md §2.3 for Template Method pattern
         */
        virtual void Render() const = 0;

        // === Event Handlers (Stub - No Implementation Yet) ===
        // These will be implemented in future phases when mouse event
        // system is integrated. For now, they return false (event not handled).

        /**
         * @brief Process mouse event (stub)
         * 
         * Called to handle mouse input. Currently returns false (not implemented).
         * Will be implemented in future phases.
         * 
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
        virtual bool ProcessMouseEvent(const MouseEvent& event) { return false; }

        /**
         * @brief Handle mouse hover (stub)
         * 
         * Called when mouse enters/exits element bounds.
         * Currently returns false (not implemented).
         * 
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
        virtual bool OnHover(const MouseEvent& event) { return false; }

        /**
         * @brief Handle mouse click (stub)
         * 
         * Called when mouse button is clicked within element bounds.
         * Currently returns false (not implemented).
         * 
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
        virtual bool OnClick(const MouseEvent& event) { return false; }

    protected:
        // Subclasses can construct with initial bounds
        UIElement(float x, float y, float width, float height)
            : relative_x_(x), relative_y_(y), width_(width), height_(height) {}

        // Position relative to parent
        float relative_x_ = 0.0f;
        float relative_y_ = 0.0f;
        
        // Element size
        float width_ = 0.0f;
        float height_ = 0.0f;

        // State flags
        bool is_focused_ = false;
        bool is_hovered_ = false;
        bool is_visible_ = true;

        // Tree structure
        UIElement* parent_ = nullptr;  ///< Non-owning pointer to parent
        std::vector<std::unique_ptr<UIElement>> children_;  ///< Owned children
    };

    // === Implementation ===

    inline void UIElement::AddChild(std::unique_ptr<UIElement> child) {
        if (child) {
            child->SetParent(this);
            children_.push_back(std::move(child));
        }
    }

    inline void UIElement::RemoveChild(UIElement* child) {
        children_.erase(
            std::remove_if(children_.begin(), children_.end(),
                [child](const std::unique_ptr<UIElement>& elem) {
                    return elem.get() == child;
                }),
            children_.end()
        );
    }

    inline batch_renderer::Rectangle UIElement::GetAbsoluteBounds() const {
        batch_renderer::Rectangle bounds = GetRelativeBounds();
        
        // Walk up parent chain to accumulate absolute position
        const UIElement* current_parent = parent_;
        while (current_parent != nullptr) {
            const batch_renderer::Rectangle parent_bounds = current_parent->GetRelativeBounds();
            bounds.x += parent_bounds.x;
            bounds.y += parent_bounds.y;
            current_parent = current_parent->parent_;
        }
        
        return bounds;
    }

    inline bool UIElement::Contains(float x, float y) const {
        const batch_renderer::Rectangle bounds = GetAbsoluteBounds();
        return x >= bounds.x && x < bounds.x + bounds.width &&
               y >= bounds.y && y < bounds.y + bounds.height;
    }

} // namespace engine::ui
