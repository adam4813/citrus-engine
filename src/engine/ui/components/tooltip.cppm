module;

#include <algorithm>
#include <cstdint>
#include <memory>

export module engine.ui:components.tooltip;

import :ui_element;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::components {

/**
 * @brief Tooltip component - adds hover tooltip behavior to any element
 *
 * Displays a provided panel at the mouse position when the owner element
 * is hovered. Features automatic repositioning to stay within window bounds
 * and configurable offset.
 *
 * **Usage Pattern:**
 * @code
 * // Create a button with tooltip
 * auto button = std::make_unique<Button>(10, 10, 100, 30, "Hover me");
 *
 * // Create tooltip content
 * auto tip_panel = std::make_unique<Panel>(0, 0, 150, 40);
 * tip_panel->SetBackgroundColor(UITheme::Background::PANEL_DARK);
 * auto tip_label = std::make_unique<Label>(5, 5, "This is a tooltip!", 12);
 * tip_panel->AddChild(std::move(tip_label));
 *
 * // Attach tooltip component
 * auto* tooltip = button->AddComponent<TooltipComponent>(std::move(tip_panel));
 * tooltip->SetOffset(10.0f, 15.0f);
 *
 * // Tooltip shows automatically on hover
 * @endcode
 *
 * **Features:**
 * - Shows content panel on hover
 * - Follows mouse position with configurable offset
 * - Automatic repositioning to stay in window bounds
 * - Content rendered at tooltip layer (above other UI)
 */
class TooltipComponent : public IUIComponent {
public:
	/**
	 * @brief Construct tooltip with content panel
	 *
	 * @param content Panel to display as tooltip (takes ownership)
	 */
	explicit TooltipComponent(std::unique_ptr<UIElement> content) : content_(std::move(content)) {
		if (content_) {
			content_->SetVisible(false);
		}
	}

	/**
	 * @brief Default constructor (content can be set later)
	 */
	TooltipComponent() = default;

	// === Content Configuration ===

	/**
	 * @brief Set the tooltip content panel
	 *
	 * @param content Panel to display as tooltip (takes ownership)
	 */
	void SetContent(std::unique_ptr<UIElement> content) {
		content_ = std::move(content);
		if (content_) {
			content_->SetVisible(is_showing_);
		}
	}

	/**
	 * @brief Get the tooltip content panel
	 * @return Raw pointer to content panel
	 */
	UIElement* GetContent() const { return content_.get(); }

	// === Positioning Configuration ===

	/**
	 * @brief Set tooltip offset from mouse position
	 *
	 * @param x Horizontal offset in pixels (positive = right of cursor)
	 * @param y Vertical offset in pixels (positive = below cursor)
	 */
	void SetOffset(const float x, const float y) {
		offset_x_ = x;
		offset_y_ = y;
	}

	/**
	 * @brief Get horizontal offset
	 * @return X offset in pixels
	 */
	float GetOffsetX() const { return offset_x_; }

	/**
	 * @brief Get vertical offset
	 * @return Y offset in pixels
	 */
	float GetOffsetY() const { return offset_y_; }

	/**
	 * @brief Set window bounds for repositioning
	 *
	 * Tooltip will stay within these bounds. If not set, uses
	 * BatchRenderer viewport size.
	 *
	 * @param width Window width
	 * @param height Window height
	 */
	void SetWindowBounds(const float width, const float height) {
		window_width_ = width;
		window_height_ = height;
		use_custom_bounds_ = true;
	}

	/**
	 * @brief Clear custom window bounds (use viewport size)
	 */
	void ClearWindowBounds() { use_custom_bounds_ = false; }

	// === State ===

	/**
	 * @brief Check if tooltip is currently showing
	 * @return True if tooltip content is visible
	 */
	bool IsShowing() const { return is_showing_; }

	/**
	 * @brief Manually show the tooltip at a position
	 *
	 * @param mouse_x Mouse X position
	 * @param mouse_y Mouse Y position
	 */
	void Show(const float mouse_x, const float mouse_y) {
		if (!content_) {
			return;
		}

		is_showing_ = true;
		content_->SetVisible(true);
		UpdateContentPosition(mouse_x, mouse_y);
	}

	/**
	 * @brief Manually hide the tooltip
	 */
	void Hide() {
		is_showing_ = false;
		if (content_) {
			content_->SetVisible(false);
		}
	}

	// === Component Lifecycle ===

	void OnAttach(UIElement* owner) override {
		IUIComponent::OnAttach(owner);
		// Content is positioned independently, not parented to owner
	}

	void OnDetach() override {
		Hide();
		IUIComponent::OnDetach();
	}

	bool OnMouseEvent(const MouseEvent& event) override {
		if (!owner_ || !content_) {
			return false;
		}

		// Check if owner is being hovered
		const bool is_hovering = owner_->Contains(event.x, event.y);

		if (is_hovering && !was_hovering_) {
			// Mouse entered owner - show tooltip
			Show(event.x, event.y);
		}
		else if (!is_hovering && was_hovering_) {
			// Mouse left owner - hide tooltip
			Hide();
		}
		else if (is_hovering && is_showing_) {
			// Update position while hovering
			UpdateContentPosition(event.x, event.y);
		}

		was_hovering_ = is_hovering;

		// Don't consume the event - let owner handle it too
		return false;
	}

	void OnRender() const override {
		// Render tooltip content if showing
		// Note: Rendered after owner, so appears on top
		// For true z-ordering, a layer system would be needed
		if (is_showing_ && content_ && content_->IsVisible()) {
			content_->Render();
		}
	}

private:
	/**
	 * @brief Update content position based on mouse position
	 *
	 * Positions the tooltip at the mouse position plus offset,
	 * then adjusts to stay within window bounds.
	 */
	void UpdateContentPosition(const float mouse_x, const float mouse_y) {
		if (!content_) {
			return;
		}

		// Get window bounds
		float win_width = window_width_;
		float win_height = window_height_;

		if (!use_custom_bounds_) {
			uint32_t vp_width = 0;
			uint32_t vp_height = 0;
			batch_renderer::BatchRenderer::GetViewportSize(vp_width, vp_height);
			win_width = static_cast<float>(vp_width);
			win_height = static_cast<float>(vp_height);
		}

		// Calculate initial position (below and right of cursor)
		float content_x = mouse_x + offset_x_;
		float content_y = mouse_y + offset_y_;

		const float content_width = content_->GetWidth();
		const float content_height = content_->GetHeight();

		// Adjust to stay within window bounds

		// Right edge overflow - flip to left of cursor
		if (content_x + content_width > win_width) {
			content_x = mouse_x - offset_x_ - content_width;
		}

		// Bottom edge overflow - flip to above cursor
		if (content_y + content_height > win_height) {
			content_y = mouse_y - offset_y_ - content_height;
		}

		// Left edge clamp
		if (content_x < 0) {
			content_x = 0;
		}

		// Top edge clamp
		if (content_y < 0) {
			content_y = 0;
		}

		// Set position (content uses absolute positioning since it's not parented)
		content_->SetRelativePosition(content_x, content_y);
	}

	// Content panel (owned by component, not parented to owner)
	std::unique_ptr<UIElement> content_;

	// Positioning
	float offset_x_{10.0f};
	float offset_y_{10.0f};

	// Window bounds for repositioning
	float window_width_{0.0f};
	float window_height_{0.0f};
	bool use_custom_bounds_{false};

	// State
	bool was_hovering_{false};
	bool is_showing_{false};
};

} // namespace engine::ui::components
