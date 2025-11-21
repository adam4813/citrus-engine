module;

#include <functional>
#include <memory>
#include <string>

export module engine.ui:elements.button;

import :ui_element;
import :elements.text;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
     * @brief Interactive button UI element with text label
     *
     * Button provides clickable interaction with visual feedback states:
     * - Normal: Default appearance
     * - Hovered: Mouse over button
     * - Pressed: Mouse button held down
     * - Disabled: Non-interactive state
     *
     * **Usage Pattern (Declarative + Observer):**
     * @code
     * // Create button once (declarative)
     * auto button = std::make_unique<Button>(10, 10, 120, 40, "Click Me");
     *
     * // Wire callback (observer pattern)
     * button->SetClickCallback([this](const MouseEvent& event) {
     *     HandleButtonClick();
     *     return true;  // Event consumed
     * });
     *
     * // Optional: Customize appearance (reactive)
     * button->SetNormalColor(Colors::DARK_GRAY);
     * button->SetHoverColor(Colors::GRAY);
     * button->SetPressedColor(Colors::LIGHT_GRAY);
     *
     * // Render many times
     * button->Render();
     * @endcode
     *
     * **Features:**
     * - State-based colors (normal, hover, pressed, disabled)
     * - Text label with automatic centering
     * - Text truncation if label exceeds button width
     * - Enabled/disabled state management
     * - Mouse event handling with callbacks
     *
     * @see UI_DEVELOPMENT_BIBLE.md §2.2 for Observer pattern details
     * @see UI_DEVELOPMENT_BIBLE.md §6 for event handling best practices
     */
class Button : public UIElement {
public:
	/**
         * @brief Construct button with label
         *
         * @param x X position relative to parent
         * @param y Y position relative to parent
         * @param width Button width in pixels
         * @param height Button height in pixels
         * @param label Text label displayed on button
         * @param font_size Font size for label (default: 16)
         *
         * @code
         * auto button = std::make_unique<Button>(10, 10, 100, 30, "OK");
         * auto large_button = std::make_unique<Button>(10, 50, 150, 50, "Large", 24);
         * @endcode
         */
	Button(const float x,
		   const float y,
		   const float width,
		   const float height,
		   const std::string& label,
		   const float font_size = 16.0f) : UIElement(x, y, width, height), label_(label), font_size_(font_size) {

		// Create text element as child (composition pattern)
		auto text = std::make_unique<Text>(0, 0, label_, font_size_, text_color_);
		text_element_ = text.get(); // Store raw pointer for updates
		AddChild(std::move(text));
		
		UpdateTextPosition();
	}

	~Button() override = default;

	// === Label Configuration ===

	/**
         * @brief Set button label text
         *
         * Updates the text and recenters it within the button.
         *
         * @param label New label text
         *
         * @code
         * button->SetLabel("Save");
         * button->SetLabel("Save (Ctrl+S)");
         * @endcode
         */
	void SetLabel(const std::string& label) {
		label_ = label;
		if (text_element_) {
			text_element_->SetText(label_);
			UpdateTextPosition();
		}
	}

	/**
         * @brief Get current label text
         * @return Label string
         */
	const std::string& GetLabel() const { return label_; }

	/**
         * @brief Set label font size
         *
         * @param font_size Font size in pixels
         *
         * @code
         * button->SetFontSize(20.0f);
         * @endcode
         */
	void SetFontSize(const float font_size) {
		font_size_ = font_size;
		if (text_element_) {
			text_element_->SetFontSize(font_size_);
			UpdateTextPosition();
		}
	}

	/**
         * @brief Get current font size
         * @return Font size in pixels
         */
	float GetFontSize() const { return font_size_; }

	// === Appearance Configuration ===

	/**
         * @brief Set normal (default) background color
         * @param color Normal state color
         */
	void SetNormalColor(const batch_renderer::Color& color) { normal_color_ = color; }

	/**
         * @brief Set hover background color
         * @param color Hover state color
         */
	void SetHoverColor(const batch_renderer::Color& color) { hover_color_ = color; }

	/**
         * @brief Set pressed background color
         * @param color Pressed state color
         */
	void SetPressedColor(const batch_renderer::Color& color) { pressed_color_ = color; }

	/**
         * @brief Set disabled background color
         * @param color Disabled state color
         */
	void SetDisabledColor(const batch_renderer::Color& color) { disabled_color_ = color; }

	/**
         * @brief Set text color
         * @param color Text color for label
         */
	void SetTextColor(const batch_renderer::Color& color) {
		text_color_ = color;
		if (text_element_) {
			text_element_->SetColor(color);
		}
	}

	/**
         * @brief Set border color
         * @param color Border color
         */
	void SetBorderColor(const batch_renderer::Color& color) { border_color_ = color; }

	/**
         * @brief Set border width
         * @param width Border width in pixels (0 = no border)
         */
	void SetBorderWidth(const float width) { border_width_ = width >= 0.0f ? width : 0.0f; }

	// === State Management ===

	/**
         * @brief Enable or disable button interaction
         *
         * Disabled buttons don't respond to mouse events and use disabled_color_.
         *
         * @param enabled True to enable, false to disable
         *
         * @code
         * button->SetEnabled(false);  // Disable
         * button->SetEnabled(true);   // Enable
         * @endcode
         */
	void SetEnabled(const bool enabled) {
		is_enabled_ = enabled;
		if (!enabled) {
			is_pressed_ = false;
			is_hovered_ = false;
		}
	}

	/**
         * @brief Check if button is enabled
         * @return True if enabled, false if disabled
         */
	bool IsEnabled() const { return is_enabled_; }

	/**
         * @brief Check if button is currently pressed
         * @return True if pressed, false otherwise
         */
	bool IsPressed() const { return is_pressed_; }

	// === Event Handlers ===

	/**
         * @brief Handle mouse click events
         *
         * Triggers click callback if button is enabled and clicked.
         * Updates pressed state based on mouse button state.
         *
         * @param event Mouse event data
         * @return True if event was consumed, false otherwise
         */
	bool OnClick(const MouseEvent& event) override {
		if (!is_enabled_) {
			return false;
		}

		if (!Contains(event.x, event.y)) {
			is_pressed_ = false;
			return false;
		}

		// Update pressed state
		is_pressed_ = event.left_down;

		// Fire callback on button press (not release)
		if (event.left_pressed && click_callback_) {
			return click_callback_(event);
		}

		return true; // Consume event if within bounds
	}

	/**
         * @brief Handle mouse hover events
         *
         * Updates hover state for visual feedback.
         *
         * @param event Mouse event data
         * @return True if event was consumed, false otherwise
         */
	bool OnHover(const MouseEvent& event) override {
		if (!is_enabled_) {
			return false;
		}

		const bool was_hovered = is_hovered_;
		is_hovered_ = Contains(event.x, event.y);

		// Call hover callback if state changed
		if (is_hovered_ != was_hovered && hover_callback_) {
			return hover_callback_(event);
		}

		return is_hovered_;
	}

	// === Rendering ===

	/**
         * @brief Render button background, border, and label
         *
         * Background color is determined by current state:
         * - Disabled: disabled_color_
         * - Pressed: pressed_color_
         * - Hovered: hover_color_
         * - Normal: normal_color_
         *
         * Label is centered within button bounds.
         *
         * @code
         * button->Render();  // Renders button with current state
         * @endcode
         */
	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		// Determine background color based on state
		Color bg_color;
		if (!is_enabled_) {
			bg_color = disabled_color_;
		}
		else if (is_pressed_) {
			bg_color = pressed_color_;
		}
		else if (is_hovered_) {
			bg_color = hover_color_;
		}
		else {
			bg_color = normal_color_;
		}

		// Render background
		BatchRenderer::SubmitQuad(bounds, bg_color);

		// Render border if width > 0
		if (border_width_ > 0.0f) {
			const float x = bounds.x;
			const float y = bounds.y;
			const float w = bounds.width;
			const float h = bounds.height;

			// Top edge
			BatchRenderer::SubmitLine(x, y, x + w, y, border_width_, border_color_);
			// Right edge
			BatchRenderer::SubmitLine(x + w, y, x + w, y + h, border_width_, border_color_);
			// Bottom edge
			BatchRenderer::SubmitLine(x + w, y + h, x, y + h, border_width_, border_color_);
			// Left edge
			BatchRenderer::SubmitLine(x, y + h, x, y, border_width_, border_color_);
		}

		// Update text position before children render
		UpdateTextPosition();

		// Render children (text element renders itself)
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
         * @brief Update text position to center it in button
         *
         * Called when label or button size changes.
         * Centers text horizontally and vertically within button bounds.
         */
	void UpdateTextPosition() const {
		using namespace batch_renderer;

		if (!text_element_) {
			return;
		}

		// Center text within button (relative positioning)
		const float text_width = text_element_->GetWidth();
		const float text_height = text_element_->GetHeight();

		const float text_x = (width_ - text_width) * 0.5f;
		const float text_y = (height_ - text_height) * 0.5f;

		text_element_->SetRelativePosition(text_x, text_y);
	}

	std::string label_;
	float font_size_;

	// State colors
	batch_renderer::Color normal_color_{batch_renderer::Colors::DARK_GRAY};
	batch_renderer::Color hover_color_{batch_renderer::Color::Brightness(batch_renderer::Colors::DARK_GRAY, 0.2f)};
	batch_renderer::Color pressed_color_{batch_renderer::Color::Brightness(batch_renderer::Colors::DARK_GRAY, -0.2f)};
	batch_renderer::Color disabled_color_{batch_renderer::Color::Alpha(batch_renderer::Colors::DARK_GRAY, 0.5f)};
	batch_renderer::Color text_color_{batch_renderer::Colors::WHITE};
	batch_renderer::Color border_color_{batch_renderer::Colors::LIGHT_GRAY};
	float border_width_{1.0f};

	// State flags
	bool is_pressed_{false};
	bool is_enabled_{true};

	// Text child (raw pointer for updates, owned by children_ vector)
	Text* text_element_{nullptr};
};

} // namespace engine::ui::elements
