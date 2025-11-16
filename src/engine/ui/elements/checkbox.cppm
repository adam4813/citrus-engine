module;

#include <functional>
#include <memory>
#include <string>

export module engine.ui:elements.checkbox;

import :ui_element;
import :elements.text;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
     * @brief Checkbox UI element for boolean selection
     *
     * Checkbox provides toggle interaction with visual checked/unchecked states:
     * - Unchecked: Empty box
     * - Checked: Box with checkmark
     * - Hover: Visual feedback
     * - Focus: Indicator for keyboard navigation
     *
     * **Usage Pattern (Declarative + Observer):**
     * @code
     * // Create checkbox once (declarative)
     * auto checkbox = std::make_unique<Checkbox>(10, 10, "Enable Sound");
     *
     * // Wire callback (observer pattern)
     * checkbox->SetToggleCallback([this](bool checked) {
     *     sound_enabled_ = checked;
     * });
     *
     * // Optional: Set initial state
     * checkbox->SetChecked(true);
     *
     * // Render many times
     * checkbox->Render();
     * @endcode
     *
     * **Features:**
     * - Toggle on click
     * - Optional text label
     * - Focus indicator for keyboard navigation
     * - Customizable colors for box, checkmark, and label
     * - Checked/unchecked state management
     *
     * @see UI_DEVELOPMENT_BIBLE.md §2.2 for Observer pattern
     * @see UI_DEVELOPMENT_BIBLE.md §6 for event handling
     */
class Checkbox : public UIElement {
public:
	/**
         * @brief Toggle callback type
         *
         * Called when checkbox is toggled via user interaction.
         *
         * @param checked New checked state
         */
	using ToggleCallback = std::function<void(bool checked)>;

	/**
         * @brief Construct checkbox with optional label
         *
         * Checkbox size is fixed (based on font size), but width extends
         * to accommodate label text.
         *
         * **Design Note**: The label Text element is not added as a child via AddChild().
         * Instead, it's managed internally and rendered directly in Render(). This design
         * allows the label to be positioned relative to the checkbox without participating
         * in the normal UI tree traversal, simplifying layout calculations.
         *
         * @param x X position relative to parent
         * @param y Y position relative to parent
         * @param label Optional text label (empty for no label)
         * @param font_size Font size for label (default: 16)
         * @param initial_checked Initial checked state (default: false)
         *
         * @code
         * auto checkbox = std::make_unique<Checkbox>(10, 10, "Enable");
         * auto checkbox2 = std::make_unique<Checkbox>(10, 40, "Large", 20);
         * auto unlabeled = std::make_unique<Checkbox>(10, 70, "");
         * @endcode
         */
	Checkbox(
			const float x,
			const float y,
			const std::string& label = "",
			const float font_size = 16.0f,
			const bool initial_checked = false) :
			UIElement(x, y, font_size * 1.5f, font_size * 1.5f), // Square box
			is_checked_(initial_checked), box_size_(font_size * 1.5f), label_text_(label), font_size_(font_size) {

		// Create label element if text provided
		// Note: Not added as a child - rendered directly in Render() method
		if (!label.empty()) {
			label_element_ = std::make_unique<Text>(box_size_ + label_spacing_, 0, label, font_size_, label_color_);

			// Extend width to include label
			width_ = box_size_ + label_spacing_ + label_element_->GetWidth();

			// Center label vertically with box
			const float label_y = (box_size_ - label_element_->GetHeight()) * 0.5f;
			label_element_->SetRelativePosition(box_size_ + label_spacing_, label_y);
		}
	}

	~Checkbox() override = default;

	// === State Management ===

	/**
         * @brief Set checked state
         *
         * Does NOT trigger toggle callback.
         *
         * @param checked True for checked, false for unchecked
         *
         * @code
         * checkbox->SetChecked(true);
         * checkbox->SetChecked(settings.sound_enabled);
         * @endcode
         */
	void SetChecked(const bool checked) { is_checked_ = checked; }

	/**
         * @brief Get checked state
         * @return True if checked, false if unchecked
         */
	bool IsChecked() const { return is_checked_; }

	/**
         * @brief Toggle checked state
         *
         * Flips between checked and unchecked.
         * Triggers toggle callback.
         *
         * @code
         * checkbox->Toggle();  // If checked, becomes unchecked (and vice versa)
         * @endcode
         */
	void Toggle() {
		is_checked_ = !is_checked_;

		if (toggle_callback_) {
			toggle_callback_(is_checked_);
		}
	}

	// === Label Configuration ===

	/**
         * @brief Set label text
         *
         * @param label New label text (empty to hide label)
         *
         * @code
         * checkbox->SetLabel("Enable Sound");
         * checkbox->SetLabel("");  // Remove label
         * @endcode
         */
	void SetLabel(const std::string& label) {
		label_text_ = label;

		if (!label.empty()) {
			if (!label_element_) {
				label_element_ = std::make_unique<Text>(box_size_ + label_spacing_, 0, label, font_size_, label_color_);
			}
			else {
				label_element_->SetText(label);
			}

			// Update width
			width_ = box_size_ + label_spacing_ + label_element_->GetWidth();

			// Center label vertically
			const float label_y = (box_size_ - label_element_->GetHeight()) * 0.5f;
			label_element_->SetRelativePosition(box_size_ + label_spacing_, label_y);
		}
		else {
			label_element_.reset();
			width_ = box_size_;
		}
	}

	/**
         * @brief Get label text
         * @return Label text
         */
	const std::string& GetLabel() const { return label_text_; }

	// === Appearance Configuration ===

	/**
         * @brief Set box (outline) color
         * @param color Box color
         */
	void SetBoxColor(const batch_renderer::Color& color) { box_color_ = color; }

	/**
         * @brief Set checkmark color
         * @param color Checkmark color
         */
	void SetCheckmarkColor(const batch_renderer::Color& color) { checkmark_color_ = color; }

	/**
         * @brief Set label text color
         * @param color Label color
         */
	void SetLabelColor(const batch_renderer::Color& color) {
		label_color_ = color;
		if (label_element_) {
			label_element_->SetColor(color);
		}
	}

	/**
         * @brief Set focus indicator color
         * @param color Focus color
         */
	void SetFocusColor(const batch_renderer::Color& color) { focus_color_ = color; }

	// === Event Callbacks ===

	/**
         * @brief Set toggle callback
         *
         * Called when checkbox is toggled via user interaction.
         *
         * @param callback Function to call when toggled
         *
         * @code
         * checkbox->SetToggleCallback([this](bool checked) {
         *     settings.sound_enabled = checked;
         *     SaveSettings();
         * });
         * @endcode
         */
	void SetToggleCallback(ToggleCallback callback) { toggle_callback_ = std::move(callback); }

	// === Event Handlers ===

	/**
         * @brief Handle mouse click events
         *
         * Toggles checkbox on click.
         *
         * @param event Mouse event data
         * @return True if event was consumed
         */
	bool OnClick(const MouseEvent& event) override {
		if (!Contains(event.x, event.y)) {
			return false;
		}

		if (event.left_pressed) {
			Toggle();
			return true;
		}

		return true;
	}

	// === Rendering ===

	/**
         * @brief Render checkbox box, checkmark, label, and focus indicator
         *
         * Rendering order:
         * 1. Box outline (square)
         * 2. Checkmark (if checked)
         * 3. Focus indicator (if focused)
         * 4. Label (if set)
         *
         * @code
         * checkbox->Render();
         * @endcode
         */
	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		// Box rectangle (square)
		const Rectangle box_rect{bounds.x, bounds.y, box_size_, box_size_};

		// Render box outline
		constexpr float border_width = 2.0f;
		const Color box_outline = is_hovered_ ? Color::Brightness(box_color_, 0.2f) : box_color_;

		// Top edge
		BatchRenderer::SubmitLine(
				box_rect.x, box_rect.y, box_rect.x + box_rect.width, box_rect.y, border_width, box_outline);
		// Right edge
		BatchRenderer::SubmitLine(
				box_rect.x + box_rect.width,
				box_rect.y,
				box_rect.x + box_rect.width,
				box_rect.y + box_rect.height,
				border_width,
				box_outline);
		// Bottom edge
		BatchRenderer::SubmitLine(
				box_rect.x + box_rect.width,
				box_rect.y + box_rect.height,
				box_rect.x,
				box_rect.y + box_rect.height,
				border_width,
				box_outline);
		// Left edge
		BatchRenderer::SubmitLine(
				box_rect.x, box_rect.y + box_rect.height, box_rect.x, box_rect.y, border_width, box_outline);

		// Render checkmark if checked
		if (is_checked_) {
			// Draw checkmark as two lines forming a check shape
			const float check_padding = box_size_ * 0.25f;
			const float x1 = box_rect.x + check_padding;
			const float y1 = box_rect.y + box_size_ * 0.5f;
			const float x2 = box_rect.x + box_size_ * 0.4f;
			const float y2 = box_rect.y + box_size_ - check_padding;
			const float x3 = box_rect.x + box_size_ - check_padding;
			const float y3 = box_rect.y + check_padding;

			// First line (short part of check)
			BatchRenderer::SubmitLine(x1, y1, x2, y2, border_width * 1.5f, checkmark_color_);
			// Second line (long part of check)
			BatchRenderer::SubmitLine(x2, y2, x3, y3, border_width * 1.5f, checkmark_color_);
		}

		// Render focus indicator if focused
		if (is_focused_) {
			constexpr float focus_padding = -4.0f; // Outside box
			const Rectangle focus_rect{
					box_rect.x + focus_padding,
					box_rect.y + focus_padding,
					box_rect.width - focus_padding * 2.0f,
					box_rect.height - focus_padding * 2.0f};

			constexpr float focus_width = 1.0f;
			// Top edge
			BatchRenderer::SubmitLine(
					focus_rect.x,
					focus_rect.y,
					focus_rect.x + focus_rect.width,
					focus_rect.y,
					focus_width,
					focus_color_);
			// Right edge
			BatchRenderer::SubmitLine(
					focus_rect.x + focus_rect.width,
					focus_rect.y,
					focus_rect.x + focus_rect.width,
					focus_rect.y + focus_rect.height,
					focus_width,
					focus_color_);
			// Bottom edge
			BatchRenderer::SubmitLine(
					focus_rect.x + focus_rect.width,
					focus_rect.y + focus_rect.height,
					focus_rect.x,
					focus_rect.y + focus_rect.height,
					focus_width,
					focus_color_);
			// Left edge
			BatchRenderer::SubmitLine(
					focus_rect.x,
					focus_rect.y + focus_rect.height,
					focus_rect.x,
					focus_rect.y,
					focus_width,
					focus_color_);
		}

		// Render label
		if (label_element_) {
			label_element_->Render();
		}

		// Render children
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	bool is_checked_;
	float box_size_;
	std::string label_text_;
	float font_size_;

	// Appearance
	batch_renderer::Color box_color_{batch_renderer::Colors::LIGHT_GRAY};
	batch_renderer::Color checkmark_color_{batch_renderer::Colors::GOLD};
	batch_renderer::Color label_color_{batch_renderer::Colors::WHITE};
	batch_renderer::Color focus_color_{batch_renderer::Colors::GOLD};
	float label_spacing_{8.0f};

	// Text element (composition)
	std::unique_ptr<Text> label_element_{nullptr};

	// Callback
	ToggleCallback toggle_callback_{nullptr};
};

} // namespace engine::ui::elements
