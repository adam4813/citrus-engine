module;

#include <memory>
#include <variant>

export module engine.ui:factory;

import :ui_element;
import :descriptor;
import :elements.button;
import :elements.panel;
import :elements.container;
import :elements.label;
import :elements.slider;
import :elements.checkbox;
import :elements.divider;
import :elements.progress_bar;
import :elements.image;
import engine.ui.batch_renderer;
import engine.rendering;

export namespace engine::ui {

/**
 * @brief Factory for creating UI elements from declarative descriptors
 *
 * UIFactory provides static methods to create UIElement instances from
 * descriptor structs, enabling declarative UI construction with designated
 * initializers.
 *
 * ## Architecture Note
 *
 * Factory functions are kept separate from descriptors (in descriptor.cppm) because:
 * - They need to import element implementations (Button, Panel, etc.)
 * - This avoids circular dependencies
 * - Descriptors remain pure data structures with JSON serialization
 *
 * When adding a new element type, you need to:
 * 1. Define the descriptor and JSON functions in descriptor.cppm
 * 2. Add a Create() overload here with the appropriate element import
 * 3. Optionally register with UIFactoryRegistry for JSON-based creation
 *
 * See the documentation in descriptor.cppm for complete step-by-step guide.
 *
 * ## Usage
 *
 * @code
 * using namespace engine::ui;
 * using namespace engine::ui::descriptor;
 *
 * // Create a button from descriptor
 * auto button = UIFactory::Create(ButtonDescriptor{
 *     .bounds = {10, 10, 120, 40},
 *     .label = "Click Me",
 *     .on_click = [](const MouseEvent&) { return true; }
 * });
 *
 * // Create a container with children
 * auto container = UIFactory::Create(ContainerDescriptor{
 *     .bounds = {0, 0, 400, 300},
 *     .padding = 10.0f,
 *     .children = {
 *         LabelDescriptor{.bounds = {0, 0, 200, 24}, .text = "Title"},
 *         ButtonDescriptor{.bounds = {0, 40, 100, 30}, .label = "OK"}
 *     }
 * });
 * @endcode
 *
 * The factory supports backwards compatibility - existing code using
 * direct constructors continues to work unchanged.
 */
class UIFactory {
public:
	// ========================================================================
	// Button Creation
	// ========================================================================

	/**
	 * @brief Create a Button from descriptor
	 *
	 * @param desc Button descriptor with all configuration
	 * @return Unique pointer to created Button
	 */
	static std::unique_ptr<elements::Button> Create(const descriptor::ButtonDescriptor& desc) {
		auto button =
			std::make_unique<elements::Button>(desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height,
											   desc.label, desc.text_style.font_size);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			button->SetId(desc.id);
		}

		// Apply colors
		button->SetNormalColor(desc.normal_color);
		button->SetHoverColor(desc.hover_color);
		button->SetPressedColor(desc.pressed_color);
		button->SetDisabledColor(desc.disabled_color);
		button->SetTextColor(desc.text_style.color);

		// Apply border
		button->SetBorderWidth(desc.border.width);
		button->SetBorderColor(desc.border.color);

		// Apply state
		button->SetEnabled(desc.enabled);
		button->SetVisible(desc.visible);

		// Wire callback
		if (desc.on_click) {
			button->SetClickCallback(desc.on_click);
		}

		return button;
	}

	// ========================================================================
	// Panel Creation
	// ========================================================================

	/**
	 * @brief Create a Panel from descriptor
	 *
	 * @param desc Panel descriptor with all configuration
	 * @return Unique pointer to created Panel
	 */
	static std::unique_ptr<elements::Panel> Create(const descriptor::PanelDescriptor& desc) {
		auto panel = std::make_unique<elements::Panel>(desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			panel->SetId(desc.id);
		}

		// Apply styling
		panel->SetBackgroundColor(desc.background);
		panel->SetBorderWidth(desc.border.width);
		panel->SetBorderColor(desc.border.color);
		panel->SetPadding(desc.padding);
		panel->SetOpacity(desc.opacity);
		panel->SetClipChildren(desc.clip_children);

		// Apply state
		panel->SetVisible(desc.visible);

		return panel;
	}

	// ========================================================================
	// Label Creation
	// ========================================================================

	/**
	 * @brief Create a Label from descriptor
	 *
	 * @param desc Label descriptor with all configuration
	 * @return Unique pointer to created Label
	 */
	static std::unique_ptr<elements::Label> Create(const descriptor::LabelDescriptor& desc) {
		auto label = std::make_unique<elements::Label>(desc.bounds.x, desc.bounds.y, desc.text, desc.style.font_size);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			label->SetId(desc.id);
		}

		// Apply styling
		label->SetColor(desc.style.color);

		// Apply state
		label->SetVisible(desc.visible);

		return label;
	}

	// ========================================================================
	// Slider Creation
	// ========================================================================

	/**
	 * @brief Create a Slider from descriptor
	 *
	 * @param desc Slider descriptor with all configuration
	 * @return Unique pointer to created Slider
	 */
	static std::unique_ptr<elements::Slider> Create(const descriptor::SliderDescriptor& desc) {
		auto slider = std::make_unique<elements::Slider>(desc.bounds.x, desc.bounds.y, desc.bounds.width,
														 desc.bounds.height, desc.min_value, desc.max_value);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			slider->SetId(desc.id);
		}

		// Apply initial value
		slider->SetValue(desc.initial_value);

		// Apply styling
		slider->SetTrackColor(desc.track_color);
		slider->SetFillColor(desc.fill_color);
		slider->SetThumbColor(desc.thumb_color);

		// Apply label if provided
		if (!desc.label.empty()) {
			slider->SetLabel(desc.label);
		}

		// Apply state
		slider->SetVisible(desc.visible);

		// Wire callback
		if (desc.on_value_changed) {
			slider->SetValueChangedCallback(desc.on_value_changed);
		}

		return slider;
	}

	// ========================================================================
	// Checkbox Creation
	// ========================================================================

	/**
	 * @brief Create a Checkbox from descriptor
	 *
	 * @param desc Checkbox descriptor with all configuration
	 * @return Unique pointer to created Checkbox
	 */
	static std::unique_ptr<elements::Checkbox> Create(const descriptor::CheckboxDescriptor& desc) {
		auto checkbox = std::make_unique<elements::Checkbox>(desc.bounds.x, desc.bounds.y, desc.label,
															 desc.text_style.font_size, desc.initial_checked);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			checkbox->SetId(desc.id);
		}

		// Apply styling
		checkbox->SetBoxColor(desc.unchecked_color);
		checkbox->SetCheckmarkColor(desc.checked_color);
		checkbox->SetLabelColor(desc.text_style.color);

		// Apply state
		checkbox->SetVisible(desc.visible);

		// Wire callback
		if (desc.on_toggled) {
			checkbox->SetToggleCallback(desc.on_toggled);
		}

		return checkbox;
	}

	// ========================================================================
	// Divider Creation
	// ========================================================================

	/**
	 * @brief Create a Divider from descriptor
	 *
	 * @param desc Divider descriptor with all configuration
	 * @return Unique pointer to created Divider
	 */
	static std::unique_ptr<elements::Divider> Create(const descriptor::DividerDescriptor& desc) {
		// Determine thickness from bounds (width for vertical, height for horizontal)
		const float thickness = desc.horizontal ? desc.bounds.height : desc.bounds.width;
		const elements::Orientation orientation =
			desc.horizontal ? elements::Orientation::Horizontal : elements::Orientation::Vertical;

		auto divider = std::make_unique<elements::Divider>(orientation, thickness > 0.0f ? thickness : 2.0f);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			divider->SetId(desc.id);
		}

		// Set position
		divider->SetRelativePosition(desc.bounds.x, desc.bounds.y);

		// Set the appropriate dimension based on orientation
		if (desc.horizontal) {
			divider->SetWidth(desc.bounds.width);
		}
		else {
			divider->SetHeight(desc.bounds.height);
		}

		// Apply styling
		divider->SetColor(desc.color);

		// Apply state
		divider->SetVisible(desc.visible);

		return divider;
	}

	// ========================================================================
	// ProgressBar Creation
	// ========================================================================

	/**
	 * @brief Create a ProgressBar from descriptor
	 *
	 * @param desc ProgressBar descriptor with all configuration
	 * @return Unique pointer to created ProgressBar
	 */
	static std::unique_ptr<elements::ProgressBar> Create(const descriptor::ProgressBarDescriptor& desc) {
		auto progress_bar = std::make_unique<elements::ProgressBar>(
			desc.bounds.x, desc.bounds.y, desc.bounds.width, desc.bounds.height, desc.initial_progress);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			progress_bar->SetId(desc.id);
		}

		// Apply styling
		progress_bar->SetTrackColor(desc.track_color);
		progress_bar->SetFillColor(desc.fill_color);
		progress_bar->SetBorderWidth(desc.border.width);

		// Apply label if provided
		if (!desc.label.empty()) {
			progress_bar->SetLabel(desc.label);
		}

		// Apply percentage display
		progress_bar->SetShowPercentage(desc.show_percentage);

		// Apply state
		progress_bar->SetVisible(desc.visible);

		return progress_bar;
	}

	// ========================================================================
	// Image Creation
	// ========================================================================

	/**
	 * @brief Create an Image from descriptor
	 *
	 * @param desc Image descriptor with all configuration
	 * @return Unique pointer to created Image
	 */
	static std::unique_ptr<elements::Image> Create(const descriptor::ImageDescriptor& desc) {
		auto image = std::make_unique<elements::Image>(desc.bounds.x, desc.bounds.y, desc.bounds.width,
													   desc.bounds.height);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			image->SetId(desc.id);
		}

		// Apply sprite if texture_id provided
		if (desc.texture_id != 0) {
			auto sprite = std::make_shared<rendering::Sprite>();
			sprite->texture = desc.texture_id;
			sprite->color = {desc.tint.r, desc.tint.g, desc.tint.b, desc.tint.a};

			// Apply UV coords if provided
			if (desc.uv_coords) {
				sprite->texture_offset = {desc.uv_coords->u0, desc.uv_coords->v0};
				sprite->texture_scale = {desc.uv_coords->u1 - desc.uv_coords->u0,
										 desc.uv_coords->v1 - desc.uv_coords->v0};
			}

			image->SetSprite(sprite);
		}

		// Apply state
		image->SetVisible(desc.visible);

		return image;
	}

	// ========================================================================
	// Container Creation (with children)
	// ========================================================================

	/**
	 * @brief Create a Container from descriptor with children
	 *
	 * Recursively creates all child elements from their descriptors.
	 *
	 * @param desc Container descriptor with all configuration and children
	 * @return Unique pointer to created Container
	 */
	static std::unique_ptr<elements::Container> Create(const descriptor::ContainerDescriptor& desc) {
		auto container = std::make_unique<elements::Container>(desc.bounds.x, desc.bounds.y, desc.bounds.width,
															   desc.bounds.height);

		// Apply ID for event binding
		if (!desc.id.empty()) {
			container->SetId(desc.id);
		}

		// Apply styling
		container->SetBackgroundColor(desc.background);
		container->SetBorderWidth(desc.border.width);
		container->SetBorderColor(desc.border.color);
		container->SetPadding(desc.padding);
		container->SetOpacity(desc.opacity);
		container->SetClipChildren(desc.clip_children);

		// Apply state
		container->SetVisible(desc.visible);

		// Create and add children
		for (const auto& child_desc : desc.children) {
			auto child = CreateFromVariant(child_desc);
			if (child) {
				container->AddChild(std::move(child));
			}
		}

		return container;
	}

	// ========================================================================
	// Variant Creation (for heterogeneous children)
	// ========================================================================

	/**
	 * @brief Create a UIElement from a descriptor variant
	 *
	 * Used internally for creating children from ContainerDescriptor.
	 *
	 * @param desc Variant containing any supported descriptor type
	 * @return Unique pointer to created UIElement (polymorphic)
	 */
	static std::unique_ptr<UIElement> CreateFromVariant(const descriptor::UIDescriptorVariant& desc) {
		return std::visit(
			[](const auto& specific_desc) -> std::unique_ptr<UIElement> { return Create(specific_desc); }, desc);
	}

	/**
	 * @brief Create a UIElement from a complete descriptor variant
	 *
	 * @param desc Complete variant containing any supported descriptor type (including Container)
	 * @return Unique pointer to created UIElement (polymorphic)
	 */
	static std::unique_ptr<UIElement> CreateFromVariant(const descriptor::CompleteUIDescriptor& desc) {
		return std::visit(
			[](const auto& specific_desc) -> std::unique_ptr<UIElement> { return Create(specific_desc); }, desc);
	}
};

} // namespace engine::ui
