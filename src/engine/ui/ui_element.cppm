module;

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

export module engine.ui:ui_element;

import engine.ui.batch_renderer;
import :mouse_event;

export namespace engine::ui {

// Forward declaration
class UIElement;

/**
 * @brief Unique identifier for component types
 */
using ComponentTypeId = std::type_index;

/**
 * @brief Get the type ID for a component type
 */
template <typename T> ComponentTypeId GetComponentTypeId() { return std::type_index(typeid(T)); }

/**
 * @brief Base interface for all UI components (Component Pattern)
 *
 * Components add optional behaviors to UIElements. Each element can have
 * at most one component of each type. Components participate in the
 * element's lifecycle (update, render, events).
 */
class IUIComponent {
public:
	virtual ~IUIComponent() = default;

	/**
	 * @brief Called when component is attached to an element
	 * @param owner The element this component is attached to
	 */
	virtual void OnAttach(UIElement* owner) { owner_ = owner; }

	/**
	 * @brief Called when component is detached from an element
	 */
	virtual void OnDetach() { owner_ = nullptr; }

	/**
	 * @brief Called each frame to update component state
	 * @param delta_time Time since last frame in seconds
	 */
	virtual void OnUpdate([[maybe_unused]] float delta_time) {}

	/**
	 * @brief Called during rendering, after the element renders itself
	 */
	virtual void OnRender() const {}

	/**
	 * @brief Called when the component needs to invalidate its state
	 */
	virtual void OnInvalidate() {}

	/**
	 * @brief Called to handle mouse events
	 * @param event The mouse event
	 * @return true if event was consumed, false to continue propagation
	 */
	virtual bool OnMouseEvent([[maybe_unused]] const MouseEvent& event) { return false; }

	/**
	 * @brief Get the owning element
	 */
	UIElement* GetOwner() const { return owner_; }

protected:
	UIElement* owner_ = nullptr;
};

/**
 * @brief Container for managing components on a UIElement
 *
 * Provides type-safe storage and retrieval of components.
 * Each component type can only be added once per element.
 */
class ComponentContainer {
public:
	/**
	 * @brief Add a component of type T
	 * @return Pointer to the added component, or nullptr if type already exists
	 */
	template <typename T, typename... Args> T* Add(UIElement* owner, Args&&... args) {
		static_assert(std::is_base_of_v<IUIComponent, T>, "T must derive from IUIComponent");

		auto type_id = GetComponentTypeId<T>();
		if (components_.contains(type_id)) {
			return nullptr; // Already has this component type
		}

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = component.get();
		component->OnAttach(owner);
		components_[type_id] = std::move(component);
		return ptr;
	}

	/**
	 * @brief Get a component of type T
	 * @return Pointer to component, or nullptr if not found
	 */
	template <typename T> T* Get() const {
		static_assert(std::is_base_of_v<IUIComponent, T>, "T must derive from IUIComponent");

		auto type_id = GetComponentTypeId<T>();
		auto it = components_.find(type_id);
		if (it != components_.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	/**
	 * @brief Check if element has a component of type T
	 */
	template <typename T> bool Has() const { return Get<T>() != nullptr; }

	/**
	 * @brief Remove a component of type T
	 * @return true if component was removed
	 */
	template <typename T> bool Remove() {
		static_assert(std::is_base_of_v<IUIComponent, T>, "T must derive from IUIComponent");

		auto type_id = GetComponentTypeId<T>();
		auto it = components_.find(type_id);
		if (it != components_.end()) {
			it->second->OnDetach();
			components_.erase(it);
			return true;
		}
		return false;
	}

	/**
	 * @brief Remove all components
	 */
	void Clear() {
		for (const auto& component : components_ | std::views::values) {
			component->OnDetach();
		}
		components_.clear();
	}

	/**
	 * @brief Update all components
	 */
	void Update(const float delta_time) {
		for (const auto& component : components_ | std::views::values) {
			component->OnUpdate(delta_time);
		}
	}

	/**
	 * @brief Marks all components as invalid
	 */
	void Invalidate() {
		for (const auto& component : components_ | std::views::values) {
			component->OnInvalidate();
		}
	}

	/**
	 * @brief Render all components
	 */
	void Render() const {
		for (const auto& component : components_ | std::views::values) {
			component->OnRender();
		}
	}

	/**
	 * @brief Process mouse event through all components
	 * @return true if any component consumed the event
	 */
	bool ProcessMouseEvent(const MouseEvent& event) {
		for (const auto& component : components_ | std::views::values) {
			if (component->OnMouseEvent(event)) {
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief Check if container is empty
	 */
	bool Empty() const { return components_.empty(); }

	/**
	 * @brief Get number of components
	 */
	size_t Size() const { return components_.size(); }

private:
	std::unordered_map<ComponentTypeId, std::unique_ptr<IUIComponent>> components_;
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
	virtual ~UIElement() { components_.Clear(); }

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
	 * @brief Get the content area where children are placed
	 *
	 * Override in elements that have padding or insets. Children are
	 * positioned relative to this area. Default returns full element bounds.
	 *
	 * @return Rectangle where x/y indicate offset from element origin to content start,
	 *         and width/height indicate the content area dimensions
	 */
	virtual batch_renderer::Rectangle GetContentArea() const { return {0.0f, 0.0f, width_, height_}; }

	/**
         * @brief Set relative position (within parent)
         * @param x X coordinate relative to parent
         * @param y Y coordinate relative to parent
         */
	void SetRelativePosition(const float x, const float y) {
		relative_x_ = x;
		relative_y_ = y;
	}

	/**
         * @brief Set element size
         * @param width Element width in pixels
         * @param height Element height in pixels
         */
	void SetSize(const float width, const float height) {
		width_ = width;
		height_ = height;
	}

	/**
         * @brief Set element width
         * @param width Element width in pixels
         */
	void SetWidth(const float width) { width_ = width; }

	/**
         * @brief Set element height
         * @param height Element height in pixels
         */
	void SetHeight(const float height) { height_ = height; }

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

	/**
         * @brief Get relative X position
         * @return X coordinate relative to parent
         */
	float GetRelativeX() const { return relative_x_; }

	/**
         * @brief Get relative Y position
         * @return Y coordinate relative to parent
         */
	float GetRelativeY() const { return relative_y_; }

	/**
         * @brief Set relative X position
         * @param x X coordinate relative to parent
         */
	void SetRelativeX(const float x) { relative_x_ = x; }

	/**
         * @brief Set relative Y position
         * @param y Y coordinate relative to parent
         */
	void SetRelativeY(const float y) { relative_y_ = y; }

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
	void SetFocused(const bool focused) { is_focused_ = focused; }

	/**
         * @brief Check if element is focused
         * @return true if focused, false otherwise
         */
	bool IsFocused() const { return is_focused_; }

	/**
         * @brief Set hovered state
         * @param hovered True if mouse is over element
         */
	void SetHovered(const bool hovered) { is_hovered_ = hovered; }

	/**
         * @brief Check if element is hovered
         * @return true if hovered, false otherwise
         */
	bool IsHovered() const { return is_hovered_; }

	/**
         * @brief Set visibility
         * @param visible True to show, false to hide
         */
	void SetVisible(const bool visible) { is_visible_ = visible; }

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

		// Let components handle first
		if (components_.ProcessMouseEvent(event)) {
			return true;
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

		if (event.scroll_delta_x != 0.0f || event.scroll_delta_y != 0.0f) {
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
	virtual bool OnHover([[maybe_unused]] const MouseEvent& event) { return false; }

	/**
         * @brief Handle mouse click (stub)
         *
         * Called when mouse button is clicked within element bounds.
         * Currently returns false (not implemented).
         *
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
	virtual bool OnClick([[maybe_unused]] const MouseEvent& event) { return false; }

	/**
         * @brief Handle mouse drag (stub)
         *
         * Called when mouse is dragged with button held.
         * Currently returns false (not implemented).
         *
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
	virtual bool OnDrag([[maybe_unused]] const MouseEvent& event) { return false; }

	/**
         * @brief Handle mouse scroll (stub)
         *
         * Called when scroll wheel is used within element bounds.
         * Currently returns false (not implemented).
         *
         * @param event Mouse event data
         * @return true if event was consumed, false otherwise
         */
	virtual bool OnScroll([[maybe_unused]] const MouseEvent& event) { return false; }

	// === Content Offset (for scrollable containers) ===

	/**
	 * @brief Get content offset applied to children (e.g., scroll offset)
	 *
	 * Override in scrollable containers to offset children's positions.
	 * Called by GetAbsoluteParentBounds() when walking the parent chain.
	 *
	 * @return Offset (x, y) to subtract from children's positions
	 */
	virtual std::pair<float, float> GetContentOffset() const { return {0.0f, 0.0f}; }

	// === Component Management ===

	/**
	 * @brief Add a component of type T
	 *
	 * Components add optional behaviors to elements like constraints,
	 * animations, tooltips, or drag handling.
	 *
	 * @return Pointer to the added component, or nullptr if type already exists
	 *
	 * @code
	 * button->AddComponent<ConstraintComponent>(Anchor::TopRight(10.0f));
	 * @endcode
	 */
	template <typename T, typename... Args> T* AddComponent(Args&&... args) {
		return components_.Add<T>(this, std::forward<Args>(args)...);
	}

	/**
	 * @brief Get a component of type T
	 * @return Pointer to component, or nullptr if not found
	 */
	template <typename T> T* GetComponent() const { return components_.Get<T>(); }

	/**
	 * @brief Check if element has a component of type T
	 */
	template <typename T> bool HasComponent() const { return components_.Has<T>(); }

	/**
	 * @brief Remove a component of type T
	 * @return true if component was removed
	 */
	template <typename T> bool RemoveComponent() { return components_.Remove<T>(); }

	/**
	 * @brief Invalidate all components
	 *
	 * Call this when the element's state changes and components
	 * need to refresh their internal state.
	 */
	void InvalidateComponents() { components_.Invalidate(); }

	/**
	 * @brief Update all components
	 *
	 * Call this each frame before rendering if components need updating.
	 * Note: Layout and constraint components auto-update when dirty.
	 */
	void UpdateComponents(const float delta_time = 0.0f) { components_.Update(delta_time); }

	/**
	 * @brief Recursively update components on this element and all children
	 *
	 * Call this on the root element to update the entire UI tree.
	 */
	void UpdateComponentsRecursive(const float delta_time = 0.0f) {
		components_.Update(delta_time);
		for (auto& child : children_) {
			if (child) {
				child->UpdateComponentsRecursive(delta_time);
			}
		}
	}

	/**
	 * @brief Render all component visuals (e.g., scrollbars)
	 */
	void RenderComponents() const { components_.Render(); }

	/**
	 * @brief Recursively render components on this element and all children
	 *
	 * Call this on the root element to update the entire UI tree.
	 */
	void RenderComponentsRecursive() const {
		components_.Render();
		for (const auto& child : children_) {
			if (child) {
				child->RenderComponentsRecursive();
			}
		}
	}

protected:
	// Subclasses can construct with initial bounds
	UIElement(const float x, const float y, const float width, const float height) :
			relative_x_(x), relative_y_(y), width_(width), height_(height) {}

	// Default constructor for layout-managed elements (position/size set by layout)
	UIElement() = default;

	/**
	 * @brief Get absolute bounds of parent's content area
	 *
	 * Walks up the parent chain accumulating relative positions to compute
	 * where this element's (0,0) position maps to in screen coordinates.
	 *
	 * @return Rectangle in absolute screen coordinates representing parent content area
	 */
	batch_renderer::Rectangle GetAbsoluteParentBounds() const;

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

	// Component system
	ComponentContainer components_;
};

// === Implementation ===

inline void UIElement::AddChild(std::unique_ptr<UIElement> child) {
	if (child) {
		child->SetParent(this);
		children_.push_back(std::move(child));
	}
}

inline void UIElement::RemoveChild(UIElement* child) {
	std::erase_if(children_, [child](const std::unique_ptr<UIElement>& elem) { return elem.get() == child; });
}

inline batch_renderer::Rectangle UIElement::GetAbsoluteBounds() const {
	const batch_renderer::Rectangle bounds = GetRelativeBounds();
	const batch_renderer::Rectangle parent_bounds = GetAbsoluteParentBounds();

	return {bounds.x + parent_bounds.x, bounds.y + parent_bounds.y, bounds.width, bounds.height};
}

inline batch_renderer::Rectangle UIElement::GetAbsoluteParentBounds() const {
	batch_renderer::Rectangle bounds{0.0f, 0.0f, 0.0f, 0.0f};

	const UIElement* current_parent = parent_;
	while (current_parent != nullptr) {
		const batch_renderer::Rectangle parent_bounds = current_parent->GetRelativeBounds();
		bounds.x += parent_bounds.x;
		bounds.y += parent_bounds.y;

		// Apply content offset (e.g., scroll offset from scrollable containers)
		const auto [offset_x, offset_y] = current_parent->GetContentOffset();
		bounds.x -= offset_x;
		bounds.y -= offset_y;

		current_parent = current_parent->parent_;
	}

	return bounds;
}

inline bool UIElement::Contains(const float x, const float y) const {
	const batch_renderer::Rectangle bounds = GetAbsoluteBounds();
	return x >= bounds.x && x < bounds.x + bounds.width && y >= bounds.y && y < bounds.y + bounds.height;
}

} // namespace engine::ui
