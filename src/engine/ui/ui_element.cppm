module;

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

export module engine.ui:ui_element;

import engine.ui.batch_renderer;
import :mouse_event;

export namespace engine::ui {
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
	batch_renderer::Rectangle GetRelativeBounds() const { return {relative_x_, relative_y_, width_, height_}; }

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

	// === Event Callbacks (Observer Pattern) ===

	/**
         * @brief Callback type for mouse click events
         *
         * Receives the mouse event that triggered the click.
         * Return true to consume the event and stop propagation.
         */
	using ClickCallback = std::function<bool(const MouseEvent&)>;

	/**
         * @brief Callback type for mouse hover events
         *
         * Receives the mouse event for hover state.
         * Return true to consume the event and stop propagation.
         */
	using HoverCallback = std::function<bool(const MouseEvent&)>;

	/**
         * @brief Callback type for mouse drag events
         *
         * Receives the mouse event while dragging.
         * Return true to consume the event and stop propagation.
         */
	using DragCallback = std::function<bool(const MouseEvent&)>;

	/**
         * @brief Callback type for mouse scroll events
         *
         * Receives the mouse event with scroll delta.
         * Return true to consume the event and stop propagation.
         */
	using ScrollCallback = std::function<bool(const MouseEvent&)>;

	/**
         * @brief Set callback for click events
         *
         * The callback is invoked before the virtual OnClick() method.
         * Allows composition-based event handling without subclassing.
         *
         * @param callback Function to call on click (can be nullptr to clear)
         *
         * @code
         * button->SetClickCallback([this](const MouseEvent& event) {
         *     if (event.left_pressed) {
         *         ExecuteAction();
         *         return true;  // Event consumed
         *     }
         *     return false;
         * });
         * @endcode
         */
	void SetClickCallback(ClickCallback callback) { click_callback_ = std::move(callback); }

	/**
         * @brief Set callback for hover events
         * @param callback Function to call on hover (can be nullptr to clear)
         */
	void SetHoverCallback(HoverCallback callback) { hover_callback_ = std::move(callback); }

	/**
         * @brief Set callback for drag events
         * @param callback Function to call on drag (can be nullptr to clear)
         */
	void SetDragCallback(DragCallback callback) { drag_callback_ = std::move(callback); }

	/**
         * @brief Set callback for scroll events
         * @param callback Function to call on scroll (can be nullptr to clear)
         */
	void SetScrollCallback(ScrollCallback callback) { scroll_callback_ = std::move(callback); }

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
	virtual bool ProcessMouseEvent(const MouseEvent& event) {
		if (!is_visible_) {
			return false;
		}

		// First, give children a chance to handle (reverse order = top to bottom)
		for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
			if ((*it)->ProcessMouseEvent(event)) {
				return true; // Child consumed event
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
		// Callbacks are checked first, then virtual methods
		if (event.left_pressed || event.right_pressed || event.middle_pressed) {
			// Try callback first
			if (click_callback_ && click_callback_(event)) {
				return true;
			}
			// Then virtual method
			if (OnClick(event)) {
				return true;
			}
		}

		if (event.scroll_delta != 0.0f) {
			// Try callback first
			if (scroll_callback_ && scroll_callback_(event)) {
				return true;
			}
			// Then virtual method
			if (OnScroll(event)) {
				return true;
			}
		}

		if (event.left_down || event.right_down || event.middle_down) {
			// Try callback first
			if (drag_callback_ && drag_callback_(event)) {
				return true;
			}
			// Then virtual method
			if (OnDrag(event)) {
				return true;
			}
		}

		// Always call hover last
		// Try callback first
		if (hover_callback_ && hover_callback_(event)) {
			return true;
		}
		// Then virtual method
		return OnHover(event);
	}

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

	/**
         * @brief Handle mouse drag (stub)
         *
         * Called when mouse is dragged with button held.
         * Currently returns false (not implemented).
         *
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
	virtual bool OnDrag(const MouseEvent& event) { return false; }

	/**
         * @brief Handle mouse scroll (stub)
         *
         * Called when scroll wheel is used within element bounds.
         * Currently returns false (not implemented).
         *
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
	virtual bool OnScroll(const MouseEvent& event) { return false; }

protected:
	// Subclasses can construct with initial bounds
	UIElement(float x, float y, float width, float height) :
			relative_x_(x), relative_y_(y), width_(width), height_(height) {}

	/**
         * @brief Get content bounds (area where children are positioned)
         *
         * By default, returns the same as GetRelativeBounds(). Container elements
         * like Panel can override this to account for padding/margins.
         *
         * This is used internally when positioning children within this element.
         *
         * @return Rectangle representing the content area in parent-relative coordinates
         */
	virtual batch_renderer::Rectangle GetContentBounds() const { return GetRelativeBounds(); }

	/**
         * @brief Get absolute content bounds (where children are positioned in screen space)
         *
         * Walks up the parent chain using GetContentBounds() to compute the absolute
         * position of this element's content area. This is where children at (0,0)
         * will actually be positioned.
         *
         * @return Rectangle in absolute screen coordinates representing content area
         */
	batch_renderer::Rectangle GetAbsoluteContentBounds() const;

	/**
	 * @brief Get absolute content bounds of the bounds of the parent element
	 *
	 * @return Rectangle in absolute screen coordinates representing content area of the parent
	 */
	batch_renderer::Rectangle GetAbsoluteParentContentBounds() const;

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

	// Event callbacks (optional, for composition-based event handling)
	ClickCallback click_callback_;
	HoverCallback hover_callback_;
	DragCallback drag_callback_;
	ScrollCallback scroll_callback_;

	// Tree structure
	UIElement* parent_ = nullptr; ///< Non-owning pointer to parent
	std::vector<std::unique_ptr<UIElement>> children_; ///< Owned children
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
			std::remove_if(
					children_.begin(),
					children_.end(),
					[child](const std::unique_ptr<UIElement>& elem) { return elem.get() == child; }),
			children_.end());
}

inline batch_renderer::Rectangle UIElement::GetAbsoluteBounds() const {
	const batch_renderer::Rectangle bounds = GetRelativeBounds();
	const batch_renderer::Rectangle parents_bounds = GetAbsoluteParentContentBounds();

	return {bounds.x + parents_bounds.x,
			bounds.y + parents_bounds.y,
			bounds.width - parents_bounds.width,
			bounds.height - parents_bounds.height};
}

inline batch_renderer::Rectangle UIElement::GetAbsoluteContentBounds() const {
	const batch_renderer::Rectangle bounds = GetContentBounds();
	const batch_renderer::Rectangle parents_bounds = GetAbsoluteParentContentBounds();

	return {bounds.x + parents_bounds.x,
			bounds.y + parents_bounds.y,
			bounds.width - parents_bounds.width,
			bounds.height - parents_bounds.height};
}

inline batch_renderer::Rectangle UIElement::GetAbsoluteParentContentBounds() const {
	batch_renderer::Rectangle bounds{0.0f, 0.0f, 0.0f, 0.0f};

	const UIElement* current_parent = parent_;
	while (current_parent != nullptr) {
		const batch_renderer::Rectangle parent_content_bounds = current_parent->GetContentBounds();
		bounds.x += parent_content_bounds.x;
		bounds.y += parent_content_bounds.y;
		current_parent = current_parent->parent_;
	}

	return bounds;
}

inline bool UIElement::Contains(float x, float y) const {
	const batch_renderer::Rectangle bounds = GetAbsoluteBounds();
	return x >= bounds.x && x < bounds.x + bounds.width && y >= bounds.y && y < bounds.y + bounds.height;
}

} // namespace engine::ui
