module;

#include <cstdint>

export module engine.ui:elements.divider;

import :ui_element;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {

/**
 * @brief Orientation for divider lines
 */
enum class Orientation : uint8_t {
	Horizontal, ///< Horizontal line (default)
	Vertical    ///< Vertical line
};

/**
 * @brief Visual divider/separator line for layouts
 *
 * Divider draws a simple line to separate content in layouts.
 * Horizontal dividers stretch to container width, vertical dividers
 * stretch to container height (when used with Stretch alignment).
 *
 * **Usage in layout:**
 * @code
 * auto container = std::make_unique<Container>();
 * container->AddChild(std::make_unique<Button>(100, 30, "Above"));
 * container->AddChild(std::make_unique<Divider>());  // Horizontal, 2px
 * container->AddChild(std::make_unique<Button>(100, 30, "Below"));
 * @endcode
 *
 * **Vertical divider:**
 * @code
 * auto divider = std::make_unique<Divider>(Orientation::Vertical, 2.0f);
 * @endcode
 */
class Divider : public UIElement {
public:
	/**
	 * @brief Construct a horizontal divider with default thickness
	 */
	Divider() : Divider(Orientation::Horizontal, 2.0f) {}

	/**
	 * @brief Construct a divider with orientation and thickness
	 * @param orientation Horizontal or Vertical
	 * @param thickness Line thickness in pixels (default: 2)
	 */
	explicit Divider(const Orientation orientation, const float thickness = 2.0f) :
			UIElement(0, 0, orientation == Orientation::Horizontal ? 0.0f : thickness,
					  orientation == Orientation::Horizontal ? thickness : 0.0f),
			orientation_(orientation), thickness_(thickness) {}

	/**
	 * @brief Construct a divider with specific thickness (horizontal)
	 * @param thickness Line thickness in pixels
	 */
	explicit Divider(const float thickness) : Divider(Orientation::Horizontal, thickness) {}

	~Divider() override = default;

	// === Configuration ===

	/**
	 * @brief Set line color
	 * @param color Divider line color
	 */
	void SetColor(const batch_renderer::Color& color) { color_ = color; }
	batch_renderer::Color GetColor() const { return color_; }

	/**
	 * @brief Set line thickness
	 * @param thickness Thickness in pixels
	 */
	void SetThickness(const float thickness) {
		thickness_ = thickness > 0.0f ? thickness : 1.0f;
		// Update size based on orientation
		if (orientation_ == Orientation::Horizontal) {
			height_ = thickness_;
		} else {
			width_ = thickness_;
		}
	}
	float GetThickness() const { return thickness_; }

	/**
	 * @brief Set orientation
	 * @param orientation Horizontal or Vertical
	 */
	void SetOrientation(const Orientation orientation) {
		orientation_ = orientation;
		// Swap width/height based on orientation
		if (orientation == Orientation::Horizontal) {
			height_ = thickness_;
			width_ = 0.0f; // Will be stretched by layout
		} else {
			width_ = thickness_;
			height_ = 0.0f; // Will be stretched by layout
		}
	}
	Orientation GetOrientation() const { return orientation_; }

	// === Rendering ===

	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		if (orientation_ == Orientation::Horizontal) {
			// Draw horizontal line centered in the element's height
			const float y = bounds.y + bounds.height / 2.0f;
			BatchRenderer::SubmitLine(bounds.x, y, bounds.x + bounds.width, y, thickness_, color_);
		} else {
			// Draw vertical line centered in the element's width
			const float x = bounds.x + bounds.width / 2.0f;
			BatchRenderer::SubmitLine(x, bounds.y, x, bounds.y + bounds.height, thickness_, color_);
		}
	}

private:
	Orientation orientation_{Orientation::Horizontal};
	float thickness_{2.0f};
	batch_renderer::Color color_{batch_renderer::Colors::GRAY};
};

} // namespace engine::ui::elements
