module;

#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>

export module engine.ui:elements.slider;

import :ui_element;
import :text;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
     * @brief Draggable slider UI element for value selection
     *
     * Slider provides interactive value selection within a min-max range:
     * - Draggable thumb for precise control
     * - Visual bar showing current value
     * - Optional label and value display
     * - Click-to-set interaction
     *
     * **Usage Pattern (Declarative + Observer):**
     * @code
     * // Create slider once (declarative)
     * auto slider = std::make_unique<Slider>(10, 10, 200, 30, 0.0f, 100.0f);
     * slider->SetValue(50.0f);
     *
     * // Wire callback (observer pattern)
     * slider->SetValueChangedCallback([this](float value) {
     *     volume_ = value;
     * });
     *
     * // Optional: Add label
     * slider->SetLabel("Volume");
     * slider->SetShowValue(true);
     *
     * // Render many times
     * slider->Render();
     * @endcode
     *
     * **Features:**
     * - Draggable thumb for value selection
     * - Click anywhere on track to jump to value
     * - Min/max range configuration
     * - Optional label and value display
     * - Customizable colors for track, fill, and thumb
     *
     * @see UI_DEVELOPMENT_BIBLE.md §2.2 for Observer pattern
     * @see UI_DEVELOPMENT_BIBLE.md §2.6 for Command pattern (for undo support)
     */
class Slider : public UIElement {
public:
	/**
         * @brief Value changed callback type
         *
         * Called when slider value changes via user interaction.
         *
         * @param value New slider value
         */
	using ValueChangedCallback = std::function<void(float value)>;

	/**
         * @brief Construct slider with range
         *
         * @param x X position relative to parent
         * @param y Y position relative to parent
         * @param width Slider width in pixels
         * @param height Slider height in pixels
         * @param min_value Minimum value (left side)
         * @param max_value Maximum value (right side)
         * @param initial_value Initial value (default: min_value)
         *
         * @code
         * auto slider = std::make_unique<Slider>(10, 10, 200, 30, 0.0f, 1.0f);
         * auto volume = std::make_unique<Slider>(10, 50, 200, 30, 0.0f, 100.0f, 75.0f);
         * @endcode
         */
	Slider(float x, float y, float width, float height, float min_value, float max_value, float initial_value = 0.0f) :
			UIElement(x, y, width, height), min_value_(min_value), max_value_(max_value),
			current_value_(std::clamp(initial_value, min_value, max_value)),
			track_color_(batch_renderer::Colors::DARK_GRAY), fill_color_(batch_renderer::Colors::GOLD),
			thumb_color_(batch_renderer::Colors::WHITE), label_color_(batch_renderer::Colors::WHITE),
			is_dragging_(false), show_value_(false), label_font_size_(14.0f), value_font_size_(12.0f),
			label_element_(nullptr), value_element_(nullptr), value_changed_callback_(nullptr) {

		// Thumb size is proportional to slider height
		thumb_radius_ = height * 0.5f;
	}

	virtual ~Slider() = default;

	// === Value Configuration ===

	/**
         * @brief Set slider value
         *
         * Clamps value to [min_value, max_value] range.
         * Does NOT trigger value changed callback.
         *
         * @param value New value
         *
         * @code
         * slider->SetValue(0.5f);
         * slider->SetValue(volume_);  // Sync from external state
         * @endcode
         */
	void SetValue(float value) {
		const float new_value = std::clamp(value, min_value_, max_value_);
		if (new_value != current_value_) {
			current_value_ = new_value;
			UpdateValueDisplay();
		}
	}

	/**
         * @brief Get current slider value
         * @return Current value
         */
	float GetValue() const { return current_value_; }

	/**
         * @brief Set minimum value
         * @param min_value Minimum value (left side)
         */
	void SetMinValue(float min_value) {
		min_value_ = min_value;
		current_value_ = std::clamp(current_value_, min_value_, max_value_);
	}

	/**
         * @brief Get minimum value
         * @return Minimum value
         */
	float GetMinValue() const { return min_value_; }

	/**
         * @brief Set maximum value
         * @param max_value Maximum value (right side)
         */
	void SetMaxValue(float max_value) {
		max_value_ = max_value;
		current_value_ = std::clamp(current_value_, min_value_, max_value_);
	}

	/**
         * @brief Get maximum value
         * @return Maximum value
         */
	float GetMaxValue() const { return max_value_; }

	// === Display Configuration ===

	/**
         * @brief Set label text
         *
         * Label is displayed to the left of the slider.
         *
         * @param label Label text (empty to hide)
         *
         * @code
         * slider->SetLabel("Volume");
         * slider->SetLabel("Master Volume:");
         * @endcode
         */
	void SetLabel(const std::string& label) {
		label_text_ = label;

		if (!label.empty() && !label_element_) {
			label_element_ = std::make_unique<Text>(0, 0, label, label_font_size_, label_color_);
			UpdateLabelPosition();
		}
		else if (label.empty() && label_element_) {
			label_element_.reset();
		}
		else if (label_element_) {
			label_element_->SetText(label);
			UpdateLabelPosition();
		}
	}

	/**
         * @brief Get label text
         * @return Label text
         */
	const std::string& GetLabel() const { return label_text_; }

	/**
         * @brief Enable/disable value display
         *
         * When enabled, current value is shown to the right of slider.
         *
         * @param show True to show value, false to hide
         *
         * @code
         * slider->SetShowValue(true);
         * slider->SetValue(75.0f);  // Shows "75.0" next to slider
         * @endcode
         */
	void SetShowValue(bool show) {
		show_value_ = show;
		if (show && !value_element_) {
			UpdateValueDisplay();
		}
		else if (!show && value_element_) {
			value_element_.reset();
		}
	}

	/**
         * @brief Check if value display is enabled
         * @return True if showing value
         */
	bool GetShowValue() const { return show_value_; }

	// === Appearance Configuration ===

	/**
         * @brief Set track (background) color
         * @param color Track color
         */
	void SetTrackColor(const batch_renderer::Color& color) { track_color_ = color; }

	/**
         * @brief Set fill (progress) color
         * @param color Fill color
         */
	void SetFillColor(const batch_renderer::Color& color) { fill_color_ = color; }

	/**
         * @brief Set thumb (handle) color
         * @param color Thumb color
         */
	void SetThumbColor(const batch_renderer::Color& color) { thumb_color_ = color; }

	// === Event Callbacks ===

	/**
         * @brief Set value changed callback
         *
         * Called when value changes via user interaction (drag or click).
         * NOT called when SetValue() is used programmatically.
         *
         * @param callback Function to call when value changes
         *
         * @code
         * slider->SetValueChangedCallback([this](float value) {
         *     audio_->SetVolume(value / 100.0f);
         * });
         * @endcode
         */
	void SetValueChangedCallback(ValueChangedCallback callback) { value_changed_callback_ = std::move(callback); }

	// === Event Handlers ===

	/**
         * @brief Handle mouse click events
         *
         * Click anywhere on track to jump to that value.
         *
         * @param event Mouse event data
         * @return True if event was consumed
         */
	bool OnClick(const MouseEvent& event) override {
		if (!Contains(event.x, event.y)) {
			return false;
		}

		if (event.left_pressed) {
			is_dragging_ = true;
			UpdateValueFromMouse(event.x);
			return true;
		}

		if (event.left_released) {
			is_dragging_ = false;
			return true;
		}

		return true;
	}

	/**
         * @brief Handle mouse drag events
         *
         * Updates slider value while dragging.
         *
         * @param event Mouse event data
         * @return True if event was consumed
         */
	bool OnDrag(const MouseEvent& event) override {
		if (!is_dragging_) {
			return false;
		}

		UpdateValueFromMouse(event.x);
		return true;
	}

	// === Rendering ===

	/**
         * @brief Render slider track, fill, thumb, label, and value
         *
         * Rendering order:
         * 1. Track (background bar)
         * 2. Fill (progress bar from left to thumb)
         * 3. Thumb (draggable circle)
         * 4. Label (left of slider)
         * 5. Value (right of slider, if enabled)
         *
         * @code
         * slider->Render();
         * @endcode
         */
	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		// Calculate track dimensions (centered vertically)
		const float track_height = height_ * 0.2f;
		const float track_y = bounds.y + (height_ - track_height) * 0.5f;
		const Rectangle track_rect{bounds.x, track_y, width_, track_height};

		// Render track (background)
		BatchRenderer::SubmitQuad(track_rect, track_color_);

		// Calculate thumb position
		const float normalized_value = (current_value_ - min_value_) / (max_value_ - min_value_);
		const float thumb_x = bounds.x + normalized_value * width_;
		const float thumb_y = bounds.y + height_ * 0.5f;

		// Render fill (progress from left to thumb)
		const Rectangle fill_rect{bounds.x, track_y, thumb_x - bounds.x, track_height};
		BatchRenderer::SubmitQuad(fill_rect, fill_color_);

		// Render thumb (circle)
		BatchRenderer::SubmitCircle(thumb_x, thumb_y, thumb_radius_, thumb_color_);

		// Render label (if set)
		if (label_element_) {
			label_element_->Render();
		}

		// Render value (if enabled)
		if (value_element_ && show_value_) {
			value_element_->Render();
		}

		// Render children
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
         * @brief Update slider value from mouse X position
         *
         * Called during drag or click events.
         * Triggers value changed callback.
         *
         * @param mouse_x Mouse X coordinate in screen space
         */
	void UpdateValueFromMouse(float mouse_x) {
		const batch_renderer::Rectangle bounds = GetAbsoluteBounds();

		// Calculate normalized position (0.0 - 1.0)
		float normalized = (mouse_x - bounds.x) / width_;
		normalized = std::clamp(normalized, 0.0f, 1.0f);

		// Convert to value in range
		const float new_value = min_value_ + normalized * (max_value_ - min_value_);

		if (new_value != current_value_) {
			current_value_ = new_value;
			UpdateValueDisplay();

			// Notify callback
			if (value_changed_callback_) {
				value_changed_callback_(current_value_);
			}
		}
	}

	/**
         * @brief Update value display text
         *
         * Called when value changes.
         */
	void UpdateValueDisplay() {
		if (!show_value_) {
			return;
		}

		// Format value as string
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.1f", current_value_);
		const std::string value_str = buffer;

		if (!value_element_) {
			value_element_ = std::make_unique<Text>(0, 0, value_str, value_font_size_, label_color_);
		}
		else {
			value_element_->SetText(value_str);
		}

		UpdateValuePosition();
	}

	/**
         * @brief Update label position (left of slider)
         */
	void UpdateLabelPosition() {
		if (!label_element_) {
			return;
		}

		// Position to the left of slider, vertically centered
		const float label_height = label_element_->GetHeight();
		const float label_y = (height_ - label_height) * 0.5f;

		label_element_->SetRelativePosition(-label_element_->GetWidth() - 10.0f, label_y);
	}

	/**
         * @brief Update value position (right of slider)
         */
	void UpdateValuePosition() {
		if (!value_element_) {
			return;
		}

		// Position to the right of slider, vertically centered
		const float value_height = value_element_->GetHeight();
		const float value_y = (height_ - value_height) * 0.5f;

		value_element_->SetRelativePosition(width_ + 10.0f, value_y);
	}

	float min_value_;
	float max_value_;
	float current_value_;

	// Appearance
	batch_renderer::Color track_color_;
	batch_renderer::Color fill_color_;
	batch_renderer::Color thumb_color_;
	batch_renderer::Color label_color_;
	float thumb_radius_;

	// State
	bool is_dragging_;

	// Display options
	bool show_value_;
	std::string label_text_;
	float label_font_size_;
	float value_font_size_;

	// Text elements (composition)
	std::unique_ptr<Text> label_element_;
	std::unique_ptr<Text> value_element_;

	// Callbacks
	ValueChangedCallback value_changed_callback_;
};

} // namespace engine::ui::elements
