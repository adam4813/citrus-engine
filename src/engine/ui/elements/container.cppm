module;

#include <algorithm>
#include <memory>
#include <vector>

export module engine.ui:elements.container;

import :ui_element;
import :elements.panel;
import :components.layout;
import :components.constraints;
import :components.scroll;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {

/**
 * @brief Panel with component-based extensibility
 *
 * Container extends Panel with convenience methods for common components:
 * - LayoutComponent: Automatic child positioning (vertical, horizontal, grid, etc.)
 * - ConstraintComponent: Position/size relative to parent
 * - ScrollComponent: Scrollable content
 *
 * Note: All UIElements can have components via AddComponent<T>(). Container
 * provides convenience methods for layout and scrolling which are container-specific.
 *
 * @code
 * using namespace engine::ui::elements;
 * using namespace engine::ui::components;
 *
 * auto container = std::make_unique<Container>(0, 0, 300, 400);
 *
 * // Add layout component
 * container->AddComponent<LayoutComponent>(
 *     std::make_unique<VerticalLayout>(8.0f, Alignment::Center)
 * );
 *
 * // Add scroll component
 * auto scroll = container->AddComponent<ScrollComponent>(ScrollDirection::Vertical);
 * scroll->SetContentSize(300, 1000);
 *
 * // Add children
 * container->AddChild(std::make_unique<Button>(0, 0, 100, 30, "Button 1"));
 * @endcode
 */
class Container : public Panel {
public:
	/**
	 * @brief Default constructor for layout-managed containers
	 *
	 * Creates a container with zero position and size. Use when the container
	 * will be sized by a parent layout container.
	 */
	Container() = default;

	Container(const float x, const float y, const float width, const float height) : Panel(x, y, width, height) {}

	// === Scroll Offset for Children ===

	/**
	 * @brief Get content offset from scroll component
	 *
	 * Children of this container will have their positions offset by
	 * the current scroll amount.
	 */
	std::pair<float, float> GetContentOffset() const override {
		if (const auto* scroll = GetComponent<components::ScrollComponent>()) {
			return {scroll->GetState().GetScrollX(), scroll->GetState().GetScrollY()};
		}
		return {0.0f, 0.0f};
	}

	// === Convenience Methods (Container-specific) ===

	/**
	 * @brief Set layout strategy (convenience for LayoutComponent)
	 */
	void SetLayout(std::unique_ptr<components::ILayout> layout) {
		if (auto* comp = GetComponent<components::LayoutComponent>()) {
			comp->SetLayout(std::move(layout));
		}
		else {
			AddComponent<components::LayoutComponent>(std::move(layout));
		}
	}

	/**
	 * @brief Enable scrolling (convenience for ScrollComponent)
	 */
	components::ScrollComponent*
	EnableScrolling(components::ScrollDirection dir = components::ScrollDirection::Vertical) {
		if (auto* existing = GetComponent<components::ScrollComponent>()) {
			existing->GetState().SetDirection(dir);
			return existing;
		}
		return AddComponent<components::ScrollComponent>(dir);
	}

	/**
	 * @brief Update all components
	 *
	 * Call this each frame before rendering if components need updating.
	 * Note: Layout and constraint components auto-update when dirty.
	 */
	void Update(const float delta_time = 0.0f) { UpdateComponents(delta_time); }
};

} // namespace engine::ui::elements
