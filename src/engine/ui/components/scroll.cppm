module;

#include <algorithm>
#include <cmath>
#include <cstdint>

export module engine.ui:components.scroll;

import :ui_element;
import :elements.panel;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::components {

/**
 * @brief Scroll direction options
 */
enum class ScrollDirection : uint8_t {
	Vertical, ///< Scroll up/down only
	Horizontal, ///< Scroll left/right only
	Both ///< Scroll in both directions
};

/**
 * @brief Scroll state and behavior for UI containers
 *
 * Provides scrolling functionality that can be applied to any container.
 * Manages scroll offset, bounds checking, and scroll indicators.
 *
 * @code
 * ScrollState scroll;
 * scroll.SetContentSize(1000.0f, 2000.0f);  // Content larger than viewport
 * scroll.SetViewportSize(400.0f, 300.0f);   // Visible area
 *
 * // In mouse event handler:
 * if (event.scroll_delta != 0.0f) {
 *     scroll.ScrollBy(0, -event.scroll_delta * 30.0f);
 * }
 *
 * // When rendering children, apply scroll offset:
 * float offset_x = scroll.GetScrollX();
 * float offset_y = scroll.GetScrollY();
 * @endcode
 */
class ScrollState {
public:
	ScrollState() = default;

	// === Content and Viewport Size ===

	/**
	 * @brief Set the total content size (what can be scrolled)
	 */
	void SetContentSize(float width, float height) {
		content_width_ = std::max(0.0f, width);
		content_height_ = std::max(0.0f, height);
		ClampScroll();
	}

	/**
	 * @brief Set the viewport size (what is visible)
	 */
	void SetViewportSize(float width, float height) {
		viewport_width_ = std::max(0.0f, width);
		viewport_height_ = std::max(0.0f, height);
		ClampScroll();
	}

	float GetContentWidth() const { return content_width_; }
	float GetContentHeight() const { return content_height_; }
	float GetViewportWidth() const { return viewport_width_; }
	float GetViewportHeight() const { return viewport_height_; }

	// === Scroll Position ===

	/**
	 * @brief Get current horizontal scroll offset
	 */
	float GetScrollX() const { return scroll_x_; }

	/**
	 * @brief Get current vertical scroll offset
	 */
	float GetScrollY() const { return scroll_y_; }

	/**
	 * @brief Set scroll position directly
	 */
	void SetScroll(float x, float y) {
		scroll_x_ = x;
		scroll_y_ = y;
		ClampScroll();
	}

	/**
	 * @brief Scroll by a delta amount
	 */
	void ScrollBy(float dx, float dy) {
		scroll_x_ += dx;
		scroll_y_ += dy;
		ClampScroll();
	}

	/**
	 * @brief Scroll to show a specific position
	 */
	void ScrollTo(float x, float y) {
		scroll_x_ = x;
		scroll_y_ = y;
		ClampScroll();
	}

	/**
	 * @brief Scroll to top-left
	 */
	void ScrollToStart() {
		scroll_x_ = 0.0f;
		scroll_y_ = 0.0f;
	}

	/**
	 * @brief Scroll to bottom-right
	 */
	void ScrollToEnd() {
		scroll_x_ = GetMaxScrollX();
		scroll_y_ = GetMaxScrollY();
	}

	// === Scroll Limits ===

	/**
	 * @brief Set minimum scroll position (for centered/offset content)
	 *
	 * When content doesn't start at (0,0), this allows scrolling to
	 * reveal content that starts before the viewport origin.
	 * Values are clamped to be <= 0 (negative scroll min means content starts before origin).
	 */
	void SetScrollMin(float min_x, float min_y) {
		scroll_min_x_ = std::min(0.0f, min_x);
		scroll_min_y_ = std::min(0.0f, min_y);
		ClampScroll();
	}

	/**
	 * @brief Get minimum horizontal scroll
	 */
	float GetMinScrollX() const { return scroll_min_x_; }

	/**
	 * @brief Get minimum vertical scroll
	 */
	float GetMinScrollY() const { return scroll_min_y_; }

	/**
	 * @brief Get maximum horizontal scroll
	 */
	float GetMaxScrollX() const { return std::max(0.0f, content_width_ - viewport_width_); }

	/**
	 * @brief Get maximum vertical scroll
	 */
	float GetMaxScrollY() const { return std::max(0.0f, content_height_ - viewport_height_); }

	/**
	 * @brief Check if horizontal scrolling is possible
	 */
	bool CanScrollX() const { return GetMinScrollX() < GetMaxScrollX(); }

	/**
	 * @brief Check if vertical scrolling is possible
	 */
	bool CanScrollY() const { return GetMinScrollY() < GetMaxScrollY(); }

	// === Scroll Direction ===

	void SetDirection(ScrollDirection dir) { direction_ = dir; }
	ScrollDirection GetDirection() const { return direction_; }

	// === Scroll Speed ===

	void SetScrollSpeed(float speed) { scroll_speed_ = speed > 0.0f ? speed : 1.0f; }
	float GetScrollSpeed() const { return scroll_speed_; }

	// === Scroll Indicators ===

	/**
	 * @brief Get normalized scroll position (0.0-1.0) for vertical scrollbar
	 */
	float GetScrollYNormalized() const {
		const float max = GetMaxScrollY();
		return max > 0.0f ? scroll_y_ / max : 0.0f;
	}

	/**
	 * @brief Get normalized scroll position (0.0-1.0) for horizontal scrollbar
	 */
	float GetScrollXNormalized() const {
		const float max = GetMaxScrollX();
		return max > 0.0f ? scroll_x_ / max : 0.0f;
	}

	/**
	 * @brief Get the thumb size ratio for vertical scrollbar
	 */
	float GetScrollYThumbRatio() const {
		return content_height_ > 0.0f ? std::min(1.0f, viewport_height_ / content_height_) : 1.0f;
	}

	/**
	 * @brief Get the thumb size ratio for horizontal scrollbar
	 */
	float GetScrollXThumbRatio() const {
		return content_width_ > 0.0f ? std::min(1.0f, viewport_width_ / content_width_) : 1.0f;
	}

	/**
	 * @brief Handle scroll event
	 * @return true if scroll was handled (content can scroll in that direction)
	 */
	bool HandleScroll(const MouseEvent& event) {
		if (event.scroll_delta_x == 0.0f && event.scroll_delta_y == 0.0f) {
			return false;
		}

		const float delta_x = -event.scroll_delta_x * scroll_speed_;
		const float delta_y = -event.scroll_delta_y * scroll_speed_;
		bool handled = false;

		switch (direction_) {
		case ScrollDirection::Vertical:
			if (CanScrollY() && event.scroll_delta_y != 0.0f) {
				ScrollBy(0.0f, delta_y);
				handled = true;
			}
			break;

		case ScrollDirection::Horizontal:
			if (CanScrollX() && event.scroll_delta_x != 0.0f) {
				ScrollBy(delta_x, 0.0f);
				handled = true;
			}
			break;

		case ScrollDirection::Both:
			if (CanScrollX() && event.scroll_delta_x != 0.0f) {
				ScrollBy(delta_x, 0.0f);
				handled = true;
			}
			if (CanScrollY() && event.scroll_delta_y != 0.0f) {
				ScrollBy(0.0f, delta_y);
				handled = true;
			}
			break;
		}

		return handled;
	}

private:
	void ClampScroll() {
		scroll_x_ = std::clamp(scroll_x_, GetMinScrollX(), GetMaxScrollX());
		scroll_y_ = std::clamp(scroll_y_, GetMinScrollY(), GetMaxScrollY());
	}

	float content_width_{0.0f};
	float content_height_{0.0f};
	float viewport_width_{0.0f};
	float viewport_height_{0.0f};
	float scroll_x_{0.0f};
	float scroll_y_{0.0f};
	float scroll_min_x_{0.0f};
	float scroll_min_y_{0.0f};
	float scroll_speed_{30.0f}; // Pixels per scroll tick
	ScrollDirection direction_{ScrollDirection::Vertical};
};

/**
 * @brief Scrollbar visual style configuration
 */
struct ScrollbarStyle {
	batch_renderer::Color track_color{0.2f, 0.2f, 0.2f, 0.5f};
	batch_renderer::Color thumb_color{0.5f, 0.5f, 0.5f, 0.8f};
	batch_renderer::Color thumb_hover_color{0.6f, 0.6f, 0.6f, 1.0f};
	float width{8.0f};
	float min_thumb_length{20.0f};
	float corner_radius{4.0f};
	bool show_track{true};
};

/**
 * @brief Calculate scrollbar geometry for rendering
 *
 * Helper class to calculate scrollbar positions and sizes.
 * Used by containers to render scroll indicators.
 */
class ScrollbarGeometry {
public:
	/**
	 * @brief Calculate vertical scrollbar geometry
	 * @param scroll Scroll state
	 * @param viewport_bounds Visible area bounds
	 * @param style Scrollbar style settings
	 * @return Rectangle for the scrollbar thumb
	 */
	static batch_renderer::Rectangle CalculateVerticalThumb(
			const ScrollState& scroll, const batch_renderer::Rectangle& viewport_bounds, const ScrollbarStyle& style) {
		if (!scroll.CanScrollY()) {
			return {};
		}

		const float track_height = viewport_bounds.height;
		const float thumb_ratio = scroll.GetScrollYThumbRatio();
		const float thumb_height = std::max(style.min_thumb_length, track_height * thumb_ratio);
		const float available_height = track_height - thumb_height;
		const float thumb_y = scroll.GetScrollYNormalized() * available_height;

		return batch_renderer::Rectangle{
				viewport_bounds.x + viewport_bounds.width - style.width,
				viewport_bounds.y + thumb_y,
				style.width,
				thumb_height};
	}

	/**
	 * @brief Calculate horizontal scrollbar geometry
	 */
	static batch_renderer::Rectangle CalculateHorizontalThumb(
			const ScrollState& scroll, const batch_renderer::Rectangle& viewport_bounds, const ScrollbarStyle& style) {
		if (!scroll.CanScrollX()) {
			return {};
		}

		const float track_width = viewport_bounds.width;
		const float thumb_ratio = scroll.GetScrollXThumbRatio();
		const float thumb_width = std::max(style.min_thumb_length, track_width * thumb_ratio);
		const float available_width = track_width - thumb_width;
		const float thumb_x = scroll.GetScrollXNormalized() * available_width;

		return batch_renderer::Rectangle{
				viewport_bounds.x + thumb_x,
				viewport_bounds.y + viewport_bounds.height - style.width,
				thumb_width,
				style.width};
	}

	/**
	 * @brief Calculate vertical track rectangle
	 */
	static batch_renderer::Rectangle
	CalculateVerticalTrack(const batch_renderer::Rectangle& viewport_bounds, const ScrollbarStyle& style) {
		return batch_renderer::Rectangle{
				viewport_bounds.x + viewport_bounds.width - style.width,
				viewport_bounds.y,
				style.width,
				viewport_bounds.height};
	}

	/**
	 * @brief Calculate horizontal track rectangle
	 */
	static batch_renderer::Rectangle
	CalculateHorizontalTrack(const batch_renderer::Rectangle& viewport_bounds, const ScrollbarStyle& style) {
		return batch_renderer::Rectangle{
				viewport_bounds.x,
				viewport_bounds.y + viewport_bounds.height - style.width,
				viewport_bounds.width,
				style.width};
	}
};

/**
 * @brief Scroll component - adds scrolling behavior to a container
 *
 * Manages scroll state, handles scroll events, and renders scrollbars.
 * Content size should be set to the total size of scrollable content.
 *
 * @code
 * auto scroll = element->AddComponent<ScrollComponent>(ScrollDirection::Vertical);
 * scroll->SetContentSize(300.0f, 1000.0f);
 * @endcode
 */
class ScrollComponent : public IUIComponent {
public:
	explicit ScrollComponent(ScrollDirection direction = ScrollDirection::Vertical) { state_.SetDirection(direction); }

	/**
	 * @brief Get the scroll state for reading/modifying
	 */
	ScrollState& GetState() { return state_; }
	const ScrollState& GetState() const { return state_; }

	/**
	 * @brief Set content size (total scrollable area)
	 */
	void SetContentSize(float width, float height) { state_.SetContentSize(width, height); }

	/**
	 * @brief Set scrollbar visual style
	 */
	void SetStyle(const ScrollbarStyle& style) { style_ = style; }
	const ScrollbarStyle& GetStyle() const { return style_; }

	/**
	 * @brief Update viewport size from owner bounds
	 */
	void UpdateViewportFromOwner() {
		if (owner_) {
			state_.SetViewportSize(owner_->GetWidth(), owner_->GetHeight());
		}
	}

	/**
	 * @brief Calculate total content size from owner's children
	 *
	 * Calculates the bounding box of all visible children relative to the
	 * content area. The content area origin (from GetContentArea) defines
	 * where (0,0) content starts.
	 */
	void CalculateContentSizeFromChildren() {
		if (!owner_) {
			return;
		}

		const auto content_area = owner_->GetContentArea();

		float min_x = 0.0f;
		float min_y = 0.0f;
		float max_x = 0.0f;
		float max_y = 0.0f;
		bool has_children = false;

		for (const auto& child : owner_->GetChildren()) {
			if (!child || !child->IsVisible()) {
				continue;
			}
			const auto bounds = child->GetRelativeBounds();
			if (!has_children) {
				min_x = bounds.x;
				min_y = bounds.y;
				max_x = bounds.x + bounds.width;
				max_y = bounds.y + bounds.height;
				has_children = true;
			}
			else {
				min_x = std::min(min_x, bounds.x);
				min_y = std::min(min_y, bounds.y);
				max_x = std::max(max_x, bounds.x + bounds.width);
				max_y = std::max(max_y, bounds.y + bounds.height);
			}
		}

		// No visible children - reset to no scrollable content
		if (!has_children) {
			state_.SetContentSize(0.0f, 0.0f);
			state_.SetScrollMin(0.0f, 0.0f);
			return;
		}

		// Content size is from content area origin to max child bounds
		// plus the content area offset (e.g., padding)
		const float content_width = max_x + content_area.x;
		const float content_height = max_y + content_area.y;

		// Min scroll allows scrolling to reveal content that starts before
		// the content area origin (e.g., centered content)
		const float scroll_min_x = min_x - content_area.x;
		const float scroll_min_y = min_y - content_area.y;

		state_.SetContentSize(content_width, content_height);
		state_.SetScrollMin(scroll_min_x, scroll_min_y);
	}

	void OnAttach(UIElement* owner) override {
		IUIComponent::OnAttach(owner);
		UpdateViewportFromOwner();
	}

	void OnInvalidate() override {
		UpdateViewportFromOwner();
		CalculateContentSizeFromChildren();
	}

	void OnUpdate([[maybe_unused]] float delta_time) override { UpdateViewportFromOwner(); }

	void OnRender() const override {
		if (!owner_) {
			return;
		}
		using namespace batch_renderer;
		const auto viewport = owner_->GetAbsoluteBounds();

		if (state_.CanScrollY()) {
			if (style_.show_track) {
				BatchRenderer::SubmitQuad(
						ScrollbarGeometry::CalculateVerticalTrack(viewport, style_), style_.track_color);
			}
			BatchRenderer::SubmitQuad(
					ScrollbarGeometry::CalculateVerticalThumb(state_, viewport, style_), style_.thumb_color);
		}
		if (state_.CanScrollX()) {
			if (style_.show_track) {
				BatchRenderer::SubmitQuad(
						ScrollbarGeometry::CalculateHorizontalTrack(viewport, style_), style_.track_color);
			}
			BatchRenderer::SubmitQuad(
					ScrollbarGeometry::CalculateHorizontalThumb(state_, viewport, style_), style_.thumb_color);
		}
	}

	bool OnMouseEvent(const MouseEvent& event) override { return state_.HandleScroll(event); }

private:
	ScrollState state_;
	ScrollbarStyle style_;
};

} // namespace engine::ui::components
