module;

#include <memory>
#include <vector>
#include <algorithm>

export module engine.ui:ui_element;

import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui {
    using namespace batch_renderer;
    
    /**
     * @brief Base class for all UI elements
     * 
     * Implements the Composite pattern for hierarchical UI structure.
     * Provides:
     * - Parent-child relationships
     * - Coordinate space transformations (relative → absolute)
     * - Hit testing for mouse interaction
     * - Bubble-down event propagation
     * - Template method for rendering
     * 
     * Design Pattern: Composite + Template Method
     * 
     * @example
     * ```cpp
     * class Button : public UIElement {
     * public:
     *     void Render() const override {
     *         auto bounds = GetAbsoluteBounds();
     *         BatchRenderer::SubmitQuad(bounds, background_color_);
     *     }
     *     
     *     bool OnClick(const MouseEvent& event) override {
     *         if (Contains(event.x, event.y)) {
     *             if (callback_) callback_();
     *             return true;  // Event consumed
     *         }
     *         return false;
     *     }
     * };
     * ```
     */
    class UIElement : public IMouseInteractive {
    public:
        virtual ~UIElement() = default;
        
        // ========================================================================
        // Composite Pattern - Hierarchy Management
        // ========================================================================
        
        /**
         * @brief Add a child element
         * 
         * Transfers ownership of the child to this element.
         * Sets the child's parent pointer to this.
         * 
         * @param child Unique pointer to child element
         */
        void AddChild(std::unique_ptr<UIElement> child) {
            if (child) {
                child->parent_ = this;
                children_.push_back(std::move(child));
            }
        }
        
        /**
         * @brief Remove a child element by pointer
         * 
         * @param child Raw pointer to child to remove
         */
        void RemoveChild(UIElement* child) {
            children_.erase(
                std::remove_if(children_.begin(), children_.end(),
                    [child](const std::unique_ptr<UIElement>& elem) {
                        return elem.get() == child;
                    }),
                children_.end()
            );
        }
        
        /**
         * @brief Get parent element (nullptr if root)
         */
        UIElement* GetParent() const { return parent_; }
        
        /**
         * @brief Get children vector
         */
        const std::vector<std::unique_ptr<UIElement>>& GetChildren() const { 
            return children_; 
        }
        
        // ========================================================================
        // Coordinate Space Transformations
        // ========================================================================
        
        /**
         * @brief Get absolute screen-space bounds
         * 
         * Walks up the parent chain to calculate absolute position.
         * Cached during rendering for performance.
         * 
         * @return Rectangle in screen coordinates
         */
        Rectangle GetAbsoluteBounds() const {
            Rectangle bounds = GetRelativeBounds();
            
            // Walk up parent chain
            const UIElement* current = parent_;
            while (current != nullptr) {
                Rectangle parent_bounds = current->GetRelativeBounds();
                bounds.x += parent_bounds.x;
                bounds.y += parent_bounds.y;
                current = current->parent_;
            }
            
            return bounds;
        }
        
        /**
         * @brief Get relative bounds (relative to parent)
         */
        Rectangle GetRelativeBounds() const {
            return Rectangle{relative_x_, relative_y_, width_, height_};
        }
        
        /**
         * @brief Set relative position (relative to parent)
         */
        void SetRelativePosition(float x, float y) {
            relative_x_ = x;
            relative_y_ = y;
        }
        
        /**
         * @brief Set size (width, height)
         */
        void SetSize(float width, float height) {
            width_ = width;
            height_ = height;
        }
        
        /**
         * @brief Get width
         */
        float GetWidth() const { return width_; }
        
        /**
         * @brief Get height
         */
        float GetHeight() const { return height_; }
        
        // ========================================================================
        // Hit Testing
        // ========================================================================
        
        /**
         * @brief Test if a point is within this element's bounds
         * 
         * Uses absolute coordinates for hit testing.
         * 
         * @param x Screen-space X coordinate
         * @param y Screen-space Y coordinate
         * @return true if point is within bounds
         */
        bool Contains(float x, float y) const {
            Rectangle bounds = GetAbsoluteBounds();
            return x >= bounds.x && x <= bounds.x + bounds.width &&
                   y >= bounds.y && y <= bounds.y + bounds.height;
        }
        
        // ========================================================================
        // State Management
        // ========================================================================
        
        /**
         * @brief Check if element has keyboard focus
         */
        bool IsFocused() const { return is_focused_; }
        
        /**
         * @brief Set focus state
         */
        void SetFocused(bool focused) { is_focused_ = focused; }
        
        /**
         * @brief Check if mouse is hovering over element
         */
        bool IsHovered() const { return is_hovered_; }
        
        /**
         * @brief Set hover state (usually called by event system)
         */
        void SetHovered(bool hovered) { is_hovered_ = hovered; }
        
        /**
         * @brief Check if element is visible
         */
        bool IsVisible() const { return is_visible_; }
        
        /**
         * @brief Set visibility
         */
        void SetVisible(bool visible) { is_visible_ = visible; }
        
        // ========================================================================
        // Event Propagation (Bubble-Down)
        // ========================================================================
        
        /**
         * @brief Process mouse event with bubble-down propagation
         * 
         * Algorithm:
         * 1. Check if event is within bounds
         * 2. Propagate to children (reverse order, top-most first)
         * 3. If no child consumed event, call own handlers
         * 4. Return true if event was consumed
         * 
         * Pattern: Chain of Responsibility with bubble-down
         * 
         * @param event Mouse event to process
         * @return true if event was consumed (stops further propagation)
         */
        bool ProcessMouseEvent(const MouseEvent& event) {
            if (!is_visible_) {
                return false;
            }
            
            // First, give children a chance to handle (reverse order = top to bottom)
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                if ((*it)->ProcessMouseEvent(event)) {
                    return true;  // Child consumed event
                }
            }
            
            // No child consumed event, check if it's within our bounds
            if (!Contains(event.x, event.y)) {
                // Not hovering anymore
                if (is_hovered_) {
                    is_hovered_ = false;
                }
                return false;
            }
            
            // Update hover state
            is_hovered_ = true;
            
            // Try event handlers in order of specificity
            if (event.left_pressed || event.right_pressed || event.middle_pressed) {
                if (OnClick(event)) {
                    return true;
                }
            }
            
            if (event.scroll_delta != 0.0f) {
                if (OnScroll(event)) {
                    return true;
                }
            }
            
            if (event.left_down || event.right_down || event.middle_down) {
                if (OnDrag(event)) {
                    return true;
                }
            }
            
            // Always call OnHover if mouse is over element
            return OnHover(event);
        }
        
        // ========================================================================
        // Template Method - Rendering
        // ========================================================================
        
        /**
         * @brief Render this element and all children
         * 
         * Template method pattern - override in derived classes.
         * Call UIElement::Render() to render children.
         */
        virtual void Render() const {
            if (!is_visible_) {
                return;
            }
            
            // Render children
            for (const auto& child : children_) {
                child->Render();
            }
        }
        
    protected:
        /**
         * @brief Constructor for derived classes
         * 
         * @param x Relative X position
         * @param y Relative Y position
         * @param width Width
         * @param height Height
         */
        UIElement(float x = 0.0f, float y = 0.0f, float width = 0.0f, float height = 0.0f)
            : relative_x_(x), relative_y_(y), width_(width), height_(height) {}
        
        // Position and size (relative to parent)
        float relative_x_{0.0f};
        float relative_y_{0.0f};
        float width_{0.0f};
        float height_{0.0f};
        
        // State flags
        bool is_focused_{false};
        bool is_hovered_{false};
        bool is_visible_{true};
        
        // Hierarchy
        UIElement* parent_{nullptr};  // Non-owning pointer
        std::vector<std::unique_ptr<UIElement>> children_;  // Owning
    };
}
