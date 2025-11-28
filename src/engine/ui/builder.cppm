module;

#include <memory>
#include <utility>

export module engine.ui:builder;

import :ui_element;
import :elements.panel;
import :elements.container;
import :components.layout;
import :components.constraints;
import :components.scroll;
import engine.ui.batch_renderer;

export namespace engine::ui {

/**
 * @brief Fluent builder for constructing Container elements with components
 *
 * Provides a clean, chainable API for creating UI containers with layout,
 * constraints, scrolling, and styling in a single expression.
 *
 * @code
 * using namespace engine::ui;
 * using namespace engine::ui::components;
 *
 * auto menu = ContainerBuilder()
 *     .Position(100, 50)
 *     .Size(300, 400)
 *     .Padding(10)
 *     .Layout<VerticalLayout>(8.0f, Alignment::Center)
 *     .Scrollable(ScrollDirection::Vertical)
 *     .Background(UITheme::Background::PANEL)
 *     .Border(1.0f, UITheme::Border::DEFAULT)
 *     .ClipChildren()
 *     .Build();
 * @endcode
 */
class ContainerBuilder {
public:
	ContainerBuilder() = default;

	// === Position and Size ===

	ContainerBuilder& Position(const float x, const float y) {
		x_ = x;
		y_ = y;
		return *this;
	}

	ContainerBuilder& Size(const float width, const float height) {
		width_ = width;
		height_ = height;
		return *this;
	}

	ContainerBuilder& Bounds(const float x, const float y, const float width, const float height) {
		x_ = x;
		y_ = y;
		width_ = width;
		height_ = height;
		return *this;
	}

	// === Panel Styling ===

	ContainerBuilder& Padding(const float padding) {
		padding_ = padding;
		return *this;
	}

	ContainerBuilder& Background(const batch_renderer::Color& color) {
		background_ = color;
		return *this;
	}

	ContainerBuilder& Border(const float width, const batch_renderer::Color& color = batch_renderer::Colors::GRAY) {
		border_width_ = width;
		border_color_ = color;
		return *this;
	}

	ContainerBuilder& Opacity(const float opacity) {
		opacity_ = opacity;
		return *this;
	}

	ContainerBuilder& ClipChildren(const bool clip = true) {
		clip_children_ = clip;
		return *this;
	}

	// === Layout Component ===

	template <typename LayoutType, typename... Args> ContainerBuilder& Layout(Args&&... args) {
		layout_ = std::make_unique<LayoutType>(std::forward<Args>(args)...);
		return *this;
	}

	ContainerBuilder& Layout(std::unique_ptr<components::ILayout> layout) {
		layout_ = std::move(layout);
		return *this;
	}

	// === Constraint Component ===

	ContainerBuilder& Anchor(const components::Anchor& anchor) {
		anchor_ = anchor;
		has_constraints_ = true;
		return *this;
	}

	ContainerBuilder& SizeConstraints(const components::SizeConstraints& constraints) {
		size_constraints_ = constraints;
		has_constraints_ = true;
		return *this;
	}

	ContainerBuilder& Fill(const float margin = 0.0f) {
		anchor_ = components::Anchor::Fill(margin);
		has_constraints_ = true;
		return *this;
	}

	ContainerBuilder& StretchHorizontal(const float left = 0.0f, const float right = 0.0f) {
		anchor_ = components::Anchor::StretchHorizontal(left, right);
		has_constraints_ = true;
		return *this;
	}

	ContainerBuilder& StretchVertical(const float top = 0.0f, const float bottom = 0.0f) {
		anchor_ = components::Anchor::StretchVertical(top, bottom);
		has_constraints_ = true;
		return *this;
	}

	// === Scroll Component ===

	ContainerBuilder& Scrollable(const components::ScrollDirection direction = components::ScrollDirection::Vertical) {
		scroll_direction_ = direction;
		scrollable_ = true;
		return *this;
	}

	ContainerBuilder& ScrollStyle(const components::ScrollbarStyle& style) {
		scroll_style_ = style;
		return *this;
	}

	// === Build ===

	/**
	 * @brief Build and return the configured container
	 */
	std::unique_ptr<elements::Container> Build() {
		auto container = std::make_unique<elements::Container>(x_, y_, width_, height_);

		// Apply panel properties
		container->SetPadding(padding_);
		container->SetBackgroundColor(background_);
		container->SetBorderWidth(border_width_);
		container->SetBorderColor(border_color_);
		container->SetOpacity(opacity_);
		container->SetClipChildren(clip_children_);

		// Add layout component
		if (layout_) {
			container->AddComponent<components::LayoutComponent>(std::move(layout_));
		}

		// Add constraint component
		if (has_constraints_) {
			container->AddComponent<components::ConstraintComponent>(anchor_, size_constraints_);
		}

		// Add scroll component
		if (scrollable_) {
			auto* scroll = container->AddComponent<components::ScrollComponent>(scroll_direction_);
			if (scroll) {
				scroll->SetStyle(scroll_style_);
			}
		}

		return container;
	}

private:
	// Position/Size
	float x_{0.0f};
	float y_{0.0f};
	float width_{100.0f};
	float height_{100.0f};

	// Panel styling
	float padding_{0.0f};
	batch_renderer::Color background_{batch_renderer::Colors::DARK_GRAY};
	float border_width_{0.0f};
	batch_renderer::Color border_color_{batch_renderer::Colors::GRAY};
	float opacity_{1.0f};
	bool clip_children_{false};

	// Layout
	std::unique_ptr<components::ILayout> layout_;

	// Constraints
	bool has_constraints_{false};
	components::Anchor anchor_;
	components::SizeConstraints size_constraints_;

	// Scroll
	bool scrollable_{false};
	components::ScrollDirection scroll_direction_{components::ScrollDirection::Vertical};
	components::ScrollbarStyle scroll_style_;
};

} // namespace engine::ui
