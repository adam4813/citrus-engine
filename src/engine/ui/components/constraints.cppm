module;

#include <algorithm>
#include <cstdint>
#include <optional>
#include <tuple>

export module engine.ui:components.constraints;

import :ui_element;
import engine.ui.batch_renderer;

export namespace engine::ui::components {

/**
 * @brief Edge flags for anchoring
 */
enum class Edge : uint8_t {
	None = 0,
	Left = 1 << 0,
	Right = 1 << 1,
	Top = 1 << 2,
	Bottom = 1 << 3,

	// Common combinations
	TopLeft = Top | Left,
	TopRight = Top | Right,
	BottomLeft = Bottom | Left,
	BottomRight = Bottom | Right,
	Horizontal = Left | Right,
	Vertical = Top | Bottom,
	All = Left | Right | Top | Bottom
};

// Enable bitwise operators for Edge
constexpr Edge operator|(Edge a, Edge b) {
	return static_cast<Edge>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr Edge operator&(Edge a, Edge b) {
	return static_cast<Edge>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

constexpr bool HasEdge(Edge edges, Edge test) {
	return (static_cast<uint8_t>(edges) & static_cast<uint8_t>(test)) != 0;
}

/**
 * @brief Anchor constraints for positioning relative to parent edges
 *
 * Defines how an element should be positioned and sized relative to its parent.
 * Supports fixed distances from edges and optional growth to opposite edges.
 *
 * @code
 * // Fixed 10px from left, 20px from top
 * Anchor anchor;
 * anchor.SetLeft(10.0f);
 * anchor.SetTop(20.0f);
 *
 * // Stretch horizontally: 10px from both left and right edges
 * anchor.SetLeft(10.0f);
 * anchor.SetRight(10.0f);
 *
 * // Apply to element
 * anchor.Apply(element, parent_bounds);
 * @endcode
 */
class Anchor {
public:
	Anchor() = default;

	/**
	 * @brief Set left edge distance (from parent left)
	 * @param distance Distance in pixels (nullopt to clear)
	 */
	void SetLeft(std::optional<float> distance) { left_ = distance; }
	std::optional<float> GetLeft() const { return left_; }

	/**
	 * @brief Set right edge distance (from parent right)
	 * @param distance Distance in pixels (nullopt to clear)
	 */
	void SetRight(std::optional<float> distance) { right_ = distance; }
	std::optional<float> GetRight() const { return right_; }

	/**
	 * @brief Set top edge distance (from parent top)
	 * @param distance Distance in pixels (nullopt to clear)
	 */
	void SetTop(std::optional<float> distance) { top_ = distance; }
	std::optional<float> GetTop() const { return top_; }

	/**
	 * @brief Set bottom edge distance (from parent bottom)
	 * @param distance Distance in pixels (nullopt to clear)
	 */
	void SetBottom(std::optional<float> distance) { bottom_ = distance; }
	std::optional<float> GetBottom() const { return bottom_; }

	/**
	 * @brief Apply anchor constraints to an element
	 *
	 * Calculates position and size based on anchor settings.
	 * If opposing edges are set (left+right or top+bottom), the element will stretch.
	 *
	 * @param element Element to apply constraints to
	 * @param parent_bounds Parent's content bounds
	 */
	void Apply(UIElement* element, const batch_renderer::Rectangle& parent_bounds) const {
		if (!element) {
			return;
		}

		float x = element->GetRelativeBounds().x;
		float y = element->GetRelativeBounds().y;
		float width = element->GetWidth();
		float height = element->GetHeight();

		// Handle horizontal positioning/sizing
		if (left_.has_value() && right_.has_value()) {
			// Stretch horizontally
			x = *left_;
			width = parent_bounds.width - *left_ - *right_;
		}
		else if (left_.has_value()) {
			// Fixed from left
			x = *left_;
		}
		else if (right_.has_value()) {
			// Fixed from right
			x = parent_bounds.width - width - *right_;
		}

		// Handle vertical positioning/sizing
		if (top_.has_value() && bottom_.has_value()) {
			// Stretch vertically
			y = *top_;
			height = parent_bounds.height - *top_ - *bottom_;
		}
		else if (top_.has_value()) {
			// Fixed from top
			y = *top_;
		}
		else if (bottom_.has_value()) {
			// Fixed from bottom
			y = parent_bounds.height - height - *bottom_;
		}

		// Ensure positive dimensions
		width = std::max(0.0f, width);
		height = std::max(0.0f, height);

		element->SetRelativePosition(x, y);
		element->SetSize(width, height);
	}

	/**
	 * @brief Check if any anchor is set
	 */
	bool HasAnchor() const {
		return left_.has_value() || right_.has_value() || top_.has_value() || bottom_.has_value();
	}

	/**
	 * @brief Clear all anchors
	 */
	void Clear() {
		left_.reset();
		right_.reset();
		top_.reset();
		bottom_.reset();
	}

	// === Convenience factory methods ===

	/**
	 * @brief Create anchor for top-left corner
	 */
	static Anchor TopLeft(float margin = 0.0f) {
		Anchor a;
		a.SetLeft(margin);
		a.SetTop(margin);
		return a;
	}

	/**
	 * @brief Create anchor for top-right corner
	 */
	static Anchor TopRight(float margin = 0.0f) {
		Anchor a;
		a.SetRight(margin);
		a.SetTop(margin);
		return a;
	}

	/**
	 * @brief Create anchor for bottom-left corner
	 */
	static Anchor BottomLeft(float margin = 0.0f) {
		Anchor a;
		a.SetLeft(margin);
		a.SetBottom(margin);
		return a;
	}

	/**
	 * @brief Create anchor for bottom-right corner
	 */
	static Anchor BottomRight(float margin = 0.0f) {
		Anchor a;
		a.SetRight(margin);
		a.SetBottom(margin);
		return a;
	}

	/**
	 * @brief Create anchor that stretches horizontally
	 */
	static Anchor StretchHorizontal(float left_margin = 0.0f, float right_margin = 0.0f) {
		Anchor a;
		a.SetLeft(left_margin);
		a.SetRight(right_margin);
		return a;
	}

	/**
	 * @brief Create anchor that stretches vertically
	 */
	static Anchor StretchVertical(float top_margin = 0.0f, float bottom_margin = 0.0f) {
		Anchor a;
		a.SetTop(top_margin);
		a.SetBottom(bottom_margin);
		return a;
	}

	/**
	 * @brief Create anchor that fills parent with margins
	 */
	static Anchor Fill(float margin = 0.0f) {
		Anchor a;
		a.SetLeft(margin);
		a.SetRight(margin);
		a.SetTop(margin);
		a.SetBottom(margin);
		return a;
	}

	/**
	 * @brief Get all anchor values for inspection/serialization
	 * @return Tuple of (left, right, top, bottom) optional values
	 */
	auto GetValues() const { return std::make_tuple(left_, right_, top_, bottom_); }

private:
	std::optional<float> left_;
	std::optional<float> right_;
	std::optional<float> top_;
	std::optional<float> bottom_;
};

/**
 * @brief Size constraint modes
 */
enum class SizeMode : uint8_t {
	Fixed, ///< Fixed pixel size
	Percentage, ///< Percentage of parent size
	MinContent, ///< Use minimum content size
	MaxContent, ///< Use maximum content size
	FitContent ///< Fit content with optional min/max bounds
};

/**
 * @brief Size constraint for a single dimension
 *
 * Controls how an element's width or height is calculated relative to parent.
 */
class SizeConstraint {
public:
	SizeConstraint() = default;

	/**
	 * @brief Set fixed pixel size
	 */
	static SizeConstraint Fixed(float pixels) {
		SizeConstraint s;
		s.mode_ = SizeMode::Fixed;
		s.value_ = pixels;
		return s;
	}

	/**
	 * @brief Set percentage of parent size
	 * @param percent Value from 0.0 to 1.0 (e.g., 0.5 = 50%)
	 */
	static SizeConstraint Percent(float percent) {
		SizeConstraint s;
		s.mode_ = SizeMode::Percentage;
		s.value_ = std::clamp(percent, 0.0f, 1.0f);
		return s;
	}

	/**
	 * @brief Set to use minimum content size with optional min/max bounds
	 */
	static SizeConstraint
	FitContent(std::optional<float> min_size = std::nullopt, std::optional<float> max_size = std::nullopt) {
		SizeConstraint s;
		s.mode_ = SizeMode::FitContent;
		s.min_size_ = min_size;
		s.max_size_ = max_size;
		return s;
	}

	/**
	 * @brief Calculate the actual size based on constraint
	 * @param parent_size Parent's dimension (width or height)
	 * @param content_size Element's current/content size
	 * @return Calculated size in pixels
	 */
	float Calculate(float parent_size, float content_size) const {
		float result = content_size;

		switch (mode_) {
		case SizeMode::Fixed: result = value_; break;
		case SizeMode::Percentage: result = parent_size * value_; break;
		case SizeMode::MinContent:
		case SizeMode::MaxContent:
		case SizeMode::FitContent: result = content_size; break;
		}

		// Apply min/max bounds
		if (min_size_.has_value()) {
			result = std::max(result, *min_size_);
		}
		if (max_size_.has_value()) {
			result = std::min(result, *max_size_);
		}

		return std::max(0.0f, result);
	}

	SizeMode GetMode() const { return mode_; }

	void SetMinSize(std::optional<float> min) { min_size_ = min; }
	void SetMaxSize(std::optional<float> max) { max_size_ = max; }

private:
	SizeMode mode_{SizeMode::FitContent}; // Default: preserve original size
	float value_{0.0f};
	std::optional<float> min_size_;
	std::optional<float> max_size_;
};

/**
 * @brief Combined size constraints for both dimensions
 */
class SizeConstraints {
public:
	SizeConstraints() = default;

	SizeConstraint width;
	SizeConstraint height;

	/**
	 * @brief Apply constraints to element
	 * @param element Element to resize
	 * @param parent_bounds Parent's content bounds
	 */
	void Apply(UIElement* element, const batch_renderer::Rectangle& parent_bounds) const {
		if (!element) {
			return;
		}

		const float new_width = width.Calculate(parent_bounds.width, element->GetWidth());
		const float new_height = height.Calculate(parent_bounds.height, element->GetHeight());
		element->SetSize(new_width, new_height);
	}

	// === Convenience factory methods ===

	/**
	 * @brief Create fixed size constraints
	 */
	static SizeConstraints Fixed(float w, float h) {
		SizeConstraints c;
		c.width = SizeConstraint::Fixed(w);
		c.height = SizeConstraint::Fixed(h);
		return c;
	}

	/**
	 * @brief Create percentage-based constraints
	 */
	static SizeConstraints Percent(float w_percent, float h_percent) {
		SizeConstraints c;
		c.width = SizeConstraint::Percent(w_percent);
		c.height = SizeConstraint::Percent(h_percent);
		return c;
	}

	/**
	 * @brief Create full-size constraints (100% of parent)
	 */
	static SizeConstraints Full() { return Percent(1.0f, 1.0f); }
};

/**
 * @brief Constraint component - positions/sizes element relative to parent
 *
 * Applies anchor and size constraints during the update phase.
 * Constraints are recalculated when marked dirty or parent bounds change.
 *
 * @code
 * element->AddComponent<ConstraintComponent>(
 *     Anchor::Fill(10.0f),
 *     SizeConstraints::Percent(0.8f, 0.5f)
 * );
 * @endcode
 */
class ConstraintComponent : public IUIComponent {
public:
	ConstraintComponent() = default;

	explicit ConstraintComponent(const Anchor& anchor) : anchor_(anchor) {}

	ConstraintComponent(const Anchor& anchor, const SizeConstraints& size) : anchor_(anchor), size_(size) {}

	/**
	 * @brief Set anchor constraints
	 */
	void SetAnchor(const Anchor& anchor) {
		anchor_ = anchor;
		dirty_ = true;
	}

	/**
	 * @brief Get current anchor
	 */
	const Anchor& GetAnchor() const { return anchor_; }

	/**
	 * @brief Set size constraints
	 */
	void SetSizeConstraints(const SizeConstraints& size) {
		size_ = size;
		dirty_ = true;
	}

	/**
	 * @brief Get current size constraints
	 */
	const SizeConstraints& GetSizeConstraints() const { return size_; }

	/**
	 * @brief Mark constraints as needing recalculation
	 */
	void Invalidate() { dirty_ = true; }

	/**
	 * @brief Called when owner element state changes
	 */
	void OnInvalidate() override { dirty_ = true; }

	/**
	 * @brief Apply constraints to owner
	 */
	void ApplyConstraints() {
		if (!owner_ || !owner_->GetParent()) {
			dirty_ = false;
			return;
		}
		const auto parent_bounds = owner_->GetParent()->GetRelativeBounds();
		size_.Apply(owner_, parent_bounds);
		anchor_.Apply(owner_, parent_bounds);
		dirty_ = false;
	}

	void OnUpdate([[maybe_unused]] float delta_time) override {
		// Always apply constraints - parent bounds may have changed
		ApplyConstraints();
	}

private:
	Anchor anchor_;
	SizeConstraints size_;
	bool dirty_{true};
};

} // namespace engine::ui::components
