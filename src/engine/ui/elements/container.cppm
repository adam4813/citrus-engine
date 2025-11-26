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
	Container(const float x, const float y, const float width, const float height) : Panel(x, y, width, height) {}

	// === Convenience Methods (Container-specific) ===

	/**
	 * @brief Set layout strategy (convenience for LayoutComponent)
	 */
	void SetLayout(std::unique_ptr<components::ILayout> layout) {
		auto* comp = GetComponent<components::LayoutComponent>();
		if (comp) {
			comp->SetLayout(std::move(layout));
		} else {
			AddComponent<components::LayoutComponent>(std::move(layout));
		}
	}

	/**
	 * @brief Enable scrolling (convenience for ScrollComponent)
	 */
	components::ScrollComponent* EnableScrolling(components::ScrollDirection dir = components::ScrollDirection::Vertical) {
		auto* existing = GetComponent<components::ScrollComponent>();
		if (existing) {
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
	void Update(float delta_time = 0.0f) { UpdateComponents(delta_time); }

	// === Rendering ===

	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		// Get absolute bounds for rendering
		const Rectangle absolute_bounds = GetAbsoluteBounds();

		// Render background
		const Color bg_color = Color::Alpha(GetBackgroundColor(), GetOpacity());
		BatchRenderer::SubmitQuad(absolute_bounds, bg_color);

		// Render border if width > 0
		if (GetBorderWidth() > 0.0f) {
			const Color border_color = Color::Alpha(GetBorderColor(), GetOpacity());
			const float x = absolute_bounds.x;
			const float y = absolute_bounds.y;
			const float w = absolute_bounds.width;
			const float h = absolute_bounds.height;

			BatchRenderer::SubmitLine(x, y, x + w, y, GetBorderWidth(), border_color);
			BatchRenderer::SubmitLine(x + w, y, x + w, y + h, GetBorderWidth(), border_color);
			BatchRenderer::SubmitLine(x + w, y + h, x, y + h, GetBorderWidth(), border_color);
			BatchRenderer::SubmitLine(x, y + h, x, y, GetBorderWidth(), border_color);
		}

		// Push scissor for children
		if (GetClipChildren()) {
			BatchRenderer::PushScissor(
					ScissorRect{absolute_bounds.x, absolute_bounds.y, absolute_bounds.width, absolute_bounds.height});
		}

		// Render children
		for (const auto& child : GetChildren()) {
			if (child) {
				child->Render();
			}
		}

		// Pop scissor
		if (GetClipChildren()) {
			BatchRenderer::PopScissor();
		}

		// Render component visuals (e.g., scrollbars)
		RenderComponents();
	}
};

} // namespace engine::ui::elements
