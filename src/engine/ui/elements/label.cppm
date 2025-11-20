module;

#include <memory>
#include <string>

export module engine.ui:elements.label;

import :ui_element;
import :elements.text;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
     * @brief Text label UI element with alignment and auto-sizing
     *
     * Label wraps the Text component to provide a UIElement-compatible
     * text display with alignment options and automatic sizing.
     *
     * **Usage Pattern (Declarative):**
     * @code
     * // Create label once (declarative)
     * auto label = std::make_unique<Label>(10, 10, "Hello World", 16);
     *
     * // Optional: Set alignment
     * label->SetAlignment(Label::Alignment::Center);
     *
     * // Optional: Set fixed width for wrapping
     * label->SetMaxWidth(200.0f);
     *
     * // Render many times
     * label->Render();
     *
     * // React to changes
     * label->SetText("New Text");  // Updates and resizes
     * @endcode
     *
     * **Features:**
     * - Text alignment (left, center, right)
     * - Auto-sizing to fit text content
     * - Optional max width for text wrapping
     * - Font scaling support
     * - Pre-computed text rendering (efficient)
     *
     * @see UI_DEVELOPMENT_BIBLE.md §1.3 for Text component optimization
     */
class Label : public UIElement {
public:
	/**
         * @brief Text alignment options
         */
	enum class Alignment {
		Left, ///< Left-aligned text (default)
		Center, ///< Center-aligned text
		Right ///< Right-aligned text
	};

	/**
         * @brief Construct label with text
         *
         * Label auto-sizes to fit text content. Use SetMaxWidth() to constrain.
         *
         * @param x X position relative to parent
         * @param y Y position relative to parent
         * @param text UTF-8 encoded text string
         * @param font_size Font size in pixels (default: 16)
         * @param color Text color (default: white)
         *
         * @code
         * auto label = std::make_unique<Label>(10, 10, "Score: 100");
         * auto title = std::make_unique<Label>(0, 0, "Title", 24, Colors::GOLD);
         * @endcode
         */
	Label(const float x,
		  const float y,
		  const std::string& text,
		  const float font_size = 16.0f,
		  const batch_renderer::Color& color = batch_renderer::Colors::WHITE) :
			UIElement(x, y, 0, 0), // Width/height set after text creation
			text_content_(text), font_size_(font_size), text_color_(color) {

		// Create text element (pre-computed rendering)
		text_element_ = std::make_unique<Text>(0, 0, text, font_size_, color);

		// Auto-size to text bounds
		UpdateSize();
	}

	~Label() override = default;

	// === Text Configuration ===

	/**
         * @brief Set label text
         *
         * Updates text and auto-resizes label to fit.
         *
         * @param text New text content
         *
         * @code
         * label->SetText("Updated Text");
         * label->SetText("Score: " + std::to_string(score));
         * @endcode
         */
	void SetText(const std::string& text) {
		text_content_ = text;
		if (text_element_) {
			text_element_->SetText(text);
			UpdateSize();
			UpdateTextPosition();
		}
	}

	/**
         * @brief Get current text content
         * @return Text string
         */
	const std::string& GetText() const { return text_content_; }

	/**
         * @brief Set font size
         *
         * Updates font size and resizes label to fit.
         *
         * @param font_size Font size in pixels
         *
         * @code
         * label->SetFontSize(20.0f);
         * @endcode
         */
	void SetFontSize(const float font_size) {
		font_size_ = font_size;
		if (text_element_) {
			text_element_->SetFontSize(font_size_);
			UpdateSize();
			UpdateTextPosition();
		}
	}

	/**
         * @brief Get current font size
         * @return Font size in pixels
         */
	float GetFontSize() const { return font_size_; }

	/**
         * @brief Set text color
         *
         * @param color New text color
         *
         * @code
         * label->SetColor(Colors::GOLD);
         * label->SetColor(Color::Alpha(Colors::WHITE, 0.5f));
         * @endcode
         */
	void SetColor(const batch_renderer::Color& color) {
		text_color_ = color;
		if (text_element_) {
			text_element_->SetColor(color);
		}
	}

	/**
         * @brief Get current text color
         * @return Text color
         */
	batch_renderer::Color GetColor() const { return text_color_; }

	// === Alignment Configuration ===

	/**
         * @brief Set text alignment
         *
         * Alignment is applied within the label's width.
         * For auto-sized labels (no max width), alignment has no visible effect
         * unless label width is manually set larger than text.
         *
         * @param alignment Alignment mode (Left, Center, Right)
         *
         * @code
         * label->SetAlignment(Label::Alignment::Center);
         * label->SetMaxWidth(300.0f);  // Now centering is visible
         * @endcode
         */
	void SetAlignment(const Alignment alignment) {
		alignment_ = alignment;
		UpdateTextPosition();
	}

	/**
         * @brief Get current text alignment
         * @return Alignment mode
         */
	Alignment GetAlignment() const { return alignment_; }

	/**
         * @brief Set maximum width for text wrapping
         *
         * If text exceeds max width, it will wrap (future: word wrap support).
         * Set to 0 to disable constraint (auto-size to text).
         *
         * @param max_width Maximum width in pixels (0 = no constraint)
         *
         * @code
         * label->SetMaxWidth(200.0f);  // Constrain to 200px width
         * label->SetMaxWidth(0.0f);    // Auto-size to text
         * @endcode
         */
	void SetMaxWidth(const float max_width) {
		max_width_ = max_width >= 0.0f ? max_width : 0.0f;
		UpdateSize();
		UpdateTextPosition();
	}

	/**
         * @brief Get maximum width constraint
         * @return Max width in pixels (0 = no constraint)
         */
	float GetMaxWidth() const { return max_width_; }

	// === Rendering ===

	/**
         * @brief Render label text
         *
         * Uses pre-computed text rendering for efficiency.
         * Text is positioned according to alignment setting.
         *
         * @code
         * label->Render();  // Efficient: just submits pre-computed vertices
         * @endcode
         */
	void Render() const override {
		if (!IsVisible()) {
			return;
		}

		// Render text (pre-computed, efficient)
		if (text_element_) {
			UpdateTextPosition();
			text_element_->Render();
		}

		// Render children (if any)
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
         * @brief Update label size based on text dimensions
         *
         * Called when text or font size changes.
         * Respects max_width constraint if set.
         */
	void UpdateSize() {
		if (!text_element_) {
			return;
		}

		const float text_width = text_element_->GetWidth();
		const float text_height = text_element_->GetHeight();

		// Apply max width constraint if set
		if (max_width_ > 0.0f) {
			width_ = std::min(text_width, max_width_);
		}
		else {
			width_ = text_width;
		}

		height_ = text_height;
	}

	/**
         * @brief Update text position based on alignment
         *
         * Called when alignment or size changes.
         * Aligns text within label bounds.
         */
	void UpdateTextPosition() const {
		using namespace batch_renderer;
		if (!text_element_) {
			return;
		}

		const float text_width = text_element_->GetWidth();
		float text_x = 0.0f;

		// Calculate X offset based on alignment
		switch (alignment_) {
		case Alignment::Left: text_x = 0.0f; break;
		case Alignment::Center: text_x = (width_ - text_width) * 0.5f; break;
		case Alignment::Right: text_x = width_ - text_width; break;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		text_element_->SetRelativePosition(bounds.x + text_x, bounds.y + 0.0f);
	}

	std::string text_content_;
	float font_size_;
	batch_renderer::Color text_color_;
	Alignment alignment_{Alignment::Left};
	float max_width_{0.0f};

	// Text rendering (composition pattern)
	std::unique_ptr<Text> text_element_{nullptr};
};

} // namespace engine::ui::elements
