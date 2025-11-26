module;

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

export module engine.ui:components.layout;

import :ui_element;
import :elements.panel;
import engine.ui.batch_renderer;

export namespace engine::ui::components {

/**
 * @brief Alignment options for layout positioning
 */
enum class Alignment : uint8_t {
	Start, ///< Align to start (left/top)
	Center, ///< Align to center
	End, ///< Align to end (right/bottom)
	Stretch ///< Stretch to fill available space
};

/**
 * @brief Base interface for layout strategies (Strategy pattern)
 *
 * Layout strategies determine how children are positioned within a container.
 * Layouts receive the container element and extract properties as needed.
 * 
 * Padding behavior:
 * - Primary axis: padding insets start position and reduces available space
 * - Cross axis: Start/End respect padding, Center uses full dimension
 *
 * @see UI_DEVELOPMENT_BIBLE.md §2.4 for Strategy pattern usage
 */
class ILayout {
public:
	virtual ~ILayout() = default;

	/**
	 * @brief Apply layout to children within the container
	 *
	 * @param children Vector of child elements to position
	 * @param container The container element (used to get bounds, padding, etc.)
	 */
	virtual void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) = 0;

protected:
	/**
	 * @brief Helper to get padding from container (0 if not a Panel)
	 */
	static float GetContainerPadding(UIElement* container) {
		if (auto* panel = dynamic_cast<elements::Panel*>(container)) {
			return panel->GetPadding();
		}
		return 0.0f;
	}
};

/**
 * @brief Vertical layout - stacks children top to bottom
 *
 * Children are positioned vertically with configurable gap and alignment.
 * Primary axis (Y): padding insets start position and reduces available space
 * Cross axis (X): Start/End respect padding, Center uses full width
 *
 * @code
 * auto layout = std::make_unique<VerticalLayout>(8.0f, Alignment::Center);
 * container->SetLayout(std::move(layout));
 * @endcode
 */
class VerticalLayout : public ILayout {
public:
	/**
	 * @brief Construct a vertical layout
	 * @param gap Space between children in pixels
	 * @param horizontal_align Horizontal alignment of children within container
	 */
	explicit VerticalLayout(const float gap = 0.0f, const Alignment horizontal_align = Alignment::Start) :
			gap_(gap), horizontal_align_(horizontal_align) {}

	void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) override {
		const auto bounds = container->GetRelativeBounds();
		const float padding = GetContainerPadding(container);

		// Primary axis (Y): start at padding, available height reduced by 2*padding
		float y = padding;

		// Cross axis dimensions depend on alignment
		const float content_width = bounds.width - padding * 2.0f;

		for (auto& child : children) {
			if (!child || !child->IsVisible()) {
				continue;
			}

			float x = 0.0f;
			const float child_width = child->GetWidth();

			switch (horizontal_align_) {
			case Alignment::Start:
				// Respect padding on cross axis
				x = padding;
				break;
			case Alignment::Center:
				// Center uses full width (no padding)
				x = (bounds.width - child_width) / 2.0f;
				break;
			case Alignment::End:
				// Respect padding on cross axis
				x = bounds.width - padding - child_width;
				break;
			case Alignment::Stretch:
				// Stretch within padded area
				x = padding;
				child->SetSize(content_width, child->GetHeight());
				break;
			}

			child->SetRelativePosition(x, y);
			y += child->GetHeight() + gap_;
		}
	}

	void SetGap(const float gap) { gap_ = gap >= 0.0f ? gap : 0.0f; }
	float GetGap() const { return gap_; }

	void SetAlignment(const Alignment align) { horizontal_align_ = align; }
	Alignment GetAlignment() const { return horizontal_align_; }

private:
	float gap_{0.0f};
	Alignment horizontal_align_{Alignment::Start};
};

/**
 * @brief Horizontal layout - arranges children left to right
 *
 * Children are positioned horizontally with configurable gap and alignment.
 * Primary axis (X): padding insets start position and reduces available space
 * Cross axis (Y): Start/End respect padding, Center uses full height
 *
 * @code
 * auto layout = std::make_unique<HorizontalLayout>(8.0f, Alignment::Center);
 * container->SetLayout(std::move(layout));
 * @endcode
 */
class HorizontalLayout : public ILayout {
public:
	/**
	 * @brief Construct a horizontal layout
	 * @param gap Space between children in pixels
	 * @param vertical_align Vertical alignment of children within container
	 */
	explicit HorizontalLayout(const float gap = 0.0f, const Alignment vertical_align = Alignment::Start) :
			gap_(gap), vertical_align_(vertical_align) {}

	void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) override {
		const auto bounds = container->GetRelativeBounds();
		const float padding = GetContainerPadding(container);

		// Primary axis (X): start at padding, available width reduced by 2*padding
		float x = padding;

		// Cross axis dimensions depend on alignment
		const float content_height = bounds.height - padding * 2.0f;

		for (auto& child : children) {
			if (!child || !child->IsVisible()) {
				continue;
			}

			float y = 0.0f;
			const float child_height = child->GetHeight();

			switch (vertical_align_) {
			case Alignment::Start:
				// Respect padding on cross axis
				y = padding;
				break;
			case Alignment::Center:
				// Center uses full height (no padding)
				y = (bounds.height - child_height) / 2.0f;
				break;
			case Alignment::End:
				// Respect padding on cross axis
				y = bounds.height - padding - child_height;
				break;
			case Alignment::Stretch:
				// Stretch within padded area
				y = padding;
				child->SetSize(child->GetWidth(), content_height);
				break;
			}

			child->SetRelativePosition(x, y);
			x += child->GetWidth() + gap_;
		}
	}

	void SetGap(const float gap) { gap_ = gap >= 0.0f ? gap : 0.0f; }
	float GetGap() const { return gap_; }

	void SetAlignment(const Alignment align) { vertical_align_ = align; }
	Alignment GetAlignment() const { return vertical_align_; }

private:
	float gap_{0.0f};
	Alignment vertical_align_{Alignment::Start};
};

/**
 * @brief Grid layout - arranges children in a grid pattern
 *
 * Children are positioned in a grid with configurable columns, gaps, and alignment.
 * Row count is determined automatically based on child count and column count.
 * Padding is applied to all edges (grid starts at padding offset).
 *
 * @code
 * auto layout = std::make_unique<GridLayout>(3, 8.0f, 8.0f);  // 3 columns
 * container->SetLayout(std::move(layout));
 * @endcode
 */
class GridLayout : public ILayout {
public:
	/**
	 * @brief Construct a grid layout
	 * @param columns Number of columns (0 = auto-calculate)
	 * @param horizontal_gap Horizontal space between cells
	 * @param vertical_gap Vertical space between rows
	 */
	explicit GridLayout(
			const uint32_t columns = 3, const float horizontal_gap = 0.0f, const float vertical_gap = 0.0f) :
			columns_(columns > 0 ? columns : 1), horizontal_gap_(horizontal_gap), vertical_gap_(vertical_gap) {}

	void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) override {
		if (children.empty()) {
			return;
		}

		const auto bounds = container->GetRelativeBounds();
		const float padding = GetContainerPadding(container);

		// Content area with padding applied
		const float content_width = bounds.width - padding * 2.0f;

		// Calculate cell size based on available space
		const float cell_width =
				(content_width - (static_cast<float>(columns_ - 1) * horizontal_gap_)) / static_cast<float>(columns_);

		uint32_t col = 0;
		float row_y = padding;
		float row_height = 0.0f;

		for (auto& child : children) {
			if (!child || !child->IsVisible()) {
				continue;
			}

			// Position child
			const float x = padding + static_cast<float>(col) * (cell_width + horizontal_gap_);
			child->SetRelativePosition(x, row_y);

			// Track maximum row height
			row_height = std::max(row_height, child->GetHeight());

			// Move to next column
			col++;
			if (col >= columns_) {
				col = 0;
				row_y += row_height + vertical_gap_;
				row_height = 0.0f;
			}
		}
	}

	void SetColumns(const uint32_t columns) { columns_ = columns > 0 ? columns : 1; }
	uint32_t GetColumns() const { return columns_; }

	void SetGap(const float horizontal, const float vertical) {
		horizontal_gap_ = horizontal >= 0.0f ? horizontal : 0.0f;
		vertical_gap_ = vertical >= 0.0f ? vertical : 0.0f;
	}

private:
	uint32_t columns_{3};
	float horizontal_gap_{0.0f};
	float vertical_gap_{0.0f};
};

/**
 * @brief Justify layout - distributes children evenly along primary axis
 *
 * Similar to CSS flexbox justify-content. Distributes children with equal
 * spacing between them (first child at start, last child at end).
 * Padding is applied on primary axis edges, cross axis follows alignment rules.
 */
enum class JustifyDirection : uint8_t {
	Horizontal, ///< Distribute horizontally (left to right)
	Vertical ///< Distribute vertically (top to bottom)
};

class JustifyLayout : public ILayout {
public:
	/**
	 * @brief Construct a justify layout
	 * @param direction Distribution direction
	 * @param cross_align Alignment on the cross axis
	 */
	explicit JustifyLayout(
			const JustifyDirection direction = JustifyDirection::Horizontal,
			const Alignment cross_align = Alignment::Center) : direction_(direction), cross_align_(cross_align) {}

	void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) override {
		const auto bounds = container->GetRelativeBounds();
		const float padding = GetContainerPadding(container);

		// Count visible children
		std::vector<UIElement*> visible;
		for (auto& child : children) {
			if (child && child->IsVisible()) {
				visible.push_back(child.get());
			}
		}

		if (visible.empty()) {
			return;
		}

		if (visible.size() == 1) {
			// Single child: center it (using full dimensions for centering)
			const float x = (bounds.width - visible[0]->GetWidth()) / 2.0f;
			const float y = (bounds.height - visible[0]->GetHeight()) / 2.0f;
			visible[0]->SetRelativePosition(x, y);
			return;
		}

		if (direction_ == JustifyDirection::Horizontal) {
			ApplyHorizontal(visible, bounds, padding);
		}
		else {
			ApplyVertical(visible, bounds, padding);
		}
	}

private:
	void ApplyHorizontal(std::vector<UIElement*>& visible, const batch_renderer::Rectangle& bounds, float padding) {
		// Primary axis: padding reduces available width
		const float content_width = bounds.width - padding * 2.0f;

		// Calculate total child width
		float total_width = 0.0f;
		for (auto* child : visible) {
			total_width += child->GetWidth();
		}

		// Calculate gap between children
		const float gap = (content_width - total_width) / static_cast<float>(visible.size() - 1);
		float x = padding;

		for (auto* child : visible) {
			float y = 0.0f;
			const float child_height = child->GetHeight();

			switch (cross_align_) {
			case Alignment::Start: y = padding; break;
			case Alignment::Center:
				// Center uses full height
				y = (bounds.height - child_height) / 2.0f;
				break;
			case Alignment::End: y = bounds.height - padding - child_height; break;
			case Alignment::Stretch:
				y = padding;
				child->SetSize(child->GetWidth(), bounds.height - padding * 2.0f);
				break;
			}

			child->SetRelativePosition(x, y);
			x += child->GetWidth() + gap;
		}
	}

	void ApplyVertical(std::vector<UIElement*>& visible, const batch_renderer::Rectangle& bounds, float padding) {
		// Primary axis: padding reduces available height
		const float content_height = bounds.height - padding * 2.0f;

		// Calculate total child height
		float total_height = 0.0f;
		for (auto* child : visible) {
			total_height += child->GetHeight();
		}

		// Calculate gap between children
		const float gap = (content_height - total_height) / static_cast<float>(visible.size() - 1);
		float y = padding;

		for (auto* child : visible) {
			float x = 0.0f;
			const float child_width = child->GetWidth();

			switch (cross_align_) {
			case Alignment::Start: x = padding; break;
			case Alignment::Center:
				// Center uses full width
				x = (bounds.width - child_width) / 2.0f;
				break;
			case Alignment::End: x = bounds.width - padding - child_width; break;
			case Alignment::Stretch:
				x = padding;
				child->SetSize(bounds.width - padding * 2.0f, child->GetHeight());
				break;
			}

			child->SetRelativePosition(x, y);
			y += child->GetHeight() + gap;
		}
	}

	JustifyDirection direction_{JustifyDirection::Horizontal};
	Alignment cross_align_{Alignment::Center};
};

/**
 * @brief Stack layout - overlays all children at specified position
 *
 * All children are positioned at the same location within the container,
 * stacking on top of each other (z-order determined by child order).
 * Center alignment uses full dimensions, Start/End respect padding.
 */
class StackLayout : public ILayout {
public:
	/**
	 * @brief Construct a stack layout
	 * @param horizontal_align Horizontal alignment of stacked children
	 * @param vertical_align Vertical alignment of stacked children
	 */
	explicit StackLayout(
			const Alignment horizontal_align = Alignment::Center, const Alignment vertical_align = Alignment::Center) :
			horizontal_align_(horizontal_align), vertical_align_(vertical_align) {}

	void Apply(std::vector<std::unique_ptr<UIElement>>& children, UIElement* container) override {
		const auto bounds = container->GetRelativeBounds();
		const float padding = GetContainerPadding(container);

		for (auto& child : children) {
			if (!child || !child->IsVisible()) {
				continue;
			}

			float x = 0.0f;
			float y = 0.0f;
			const float child_width = child->GetWidth();
			const float child_height = child->GetHeight();

			switch (horizontal_align_) {
			case Alignment::Start: x = padding; break;
			case Alignment::Center:
				// Center uses full width
				x = (bounds.width - child_width) / 2.0f;
				break;
			case Alignment::End: x = bounds.width - padding - child_width; break;
			case Alignment::Stretch:
				x = padding;
				child->SetSize(bounds.width - padding * 2.0f, child_height);
				break;
			}

			switch (vertical_align_) {
			case Alignment::Start: y = padding; break;
			case Alignment::Center:
				// Center uses full height
				y = (bounds.height - child_height) / 2.0f;
				break;
			case Alignment::End: y = bounds.height - padding - child_height; break;
			case Alignment::Stretch:
				y = padding;
				child->SetSize(child_width, bounds.height - padding * 2.0f);
				break;
			}

			child->SetRelativePosition(x, y);
		}
	}

private:
	Alignment horizontal_align_{Alignment::Center};
	Alignment vertical_align_{Alignment::Center};
};

/**
 * @brief Layout component - manages child positioning via strategy pattern
 *
 * Wraps a layout strategy and applies it to the owner element's children.
 * Automatically marks layout as dirty when children change.
 *
 * @code
 * element->AddComponent<LayoutComponent>(
 *     std::make_unique<VerticalLayout>(8.0f, Alignment::Center)
 * );
 * @endcode
 */
class LayoutComponent : public IUIComponent {
public:
	explicit LayoutComponent(std::unique_ptr<ILayout> layout = nullptr) : layout_(std::move(layout)) {}

	/**
	 * @brief Set the layout strategy
	 */
	void SetLayout(std::unique_ptr<ILayout> layout) {
		layout_ = std::move(layout);
		dirty_ = true;
	}

	/**
	 * @brief Get the current layout strategy
	 */
	ILayout* GetLayout() const { return layout_.get(); }

	/**
	 * @brief Mark layout as needing recalculation
	 */
	void OnInvalidate() override { dirty_ = true; }

	/**
	 * @brief Check if layout needs recalculation
	 */
	bool IsDirty() const { return dirty_; }

	/**
	 * @brief Apply layout to owner's children
	 */
	void ApplyLayout() {
		if (!layout_ || !owner_) {
			return;
		}
		auto& children = const_cast<std::vector<std::unique_ptr<UIElement>>&>(owner_->GetChildren());

		// Layout receives the container and extracts bounds/padding as needed
		layout_->Apply(children, owner_);
		dirty_ = false;
	}

	void OnUpdate([[maybe_unused]] float delta_time) override {
		if (dirty_) {
			ApplyLayout();
		}
	}

private:
	std::unique_ptr<ILayout> layout_;
	bool dirty_{true};
};

} // namespace engine::ui::components
