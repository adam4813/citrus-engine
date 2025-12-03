module;

#include <algorithm>
#include <format>
#include <functional>
#include <memory>
#include <string>

export module engine.ui:elements.progress_bar;

import :ui_element;
import :elements.text;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
 * @brief Progress bar UI element for displaying completion percentage
 *
 * ProgressBar displays a horizontal bar showing progress from 0% to 100%:
 * - Configurable track and fill colors
 * - Optional label display (left side or centered)
 * - Optional percentage text display
 * - Smooth visual representation of progress
 *
 * **Usage Pattern (Declarative + Reactive):**
 * @code
 * // Create progress bar once (declarative)
 * auto progress = std::make_unique<ProgressBar>(10, 10, 200, 20);
 * progress->SetProgress(0.75f);  // 75% complete
 *
 * // Optional: Add label and show percentage
 * progress->SetLabel("Loading...");
 * progress->SetShowPercentage(true);
 *
 * // Customize colors
 * progress->SetTrackColor(Colors::DARK_GRAY);
 * progress->SetFillColor(Colors::GOLD);
 *
 * // Render many times
 * progress->Render();
 *
 * // React to changes
 * progress->SetProgress(0.90f);  // Update progress
 * @endcode
 *
 * **Features:**
 * - Progress value from 0.0 to 1.0
 * - Optional label text (left of bar)
 * - Optional percentage text display
 * - Customizable track and fill colors
 * - Border support
 *
 * @see UI_DEVELOPMENT_BIBLE.md for complete patterns and best practices
 */
class ProgressBar : public UIElement {
public:
	/**
	 * @brief Construct progress bar for layout container (position determined by layout)
	 *
	 * Creates a progress bar with zero position (will be set by parent layout).
	 *
	 * @param width Progress bar width in pixels
	 * @param height Progress bar height in pixels
	 * @param initial_progress Initial progress value (0.0 - 1.0, default: 0.0)
	 *
	 * @code
	 * auto progress = std::make_unique<ProgressBar>(200, 20);
	 * container->AddChild(std::move(progress));  // Layout sets position
	 * @endcode
	 */
	ProgressBar(const float width, const float height, const float initial_progress = 0.0f) :
			ProgressBar(0, 0, width, height, initial_progress) {}

	/**
	 * @brief Construct progress bar with position and size
	 *
	 * @param x X position relative to parent
	 * @param y Y position relative to parent
	 * @param width Progress bar width in pixels
	 * @param height Progress bar height in pixels
	 * @param initial_progress Initial progress value (0.0 - 1.0, default: 0.0)
	 *
	 * @code
	 * auto progress = std::make_unique<ProgressBar>(10, 10, 200, 20, 0.5f);
	 * @endcode
	 */
	ProgressBar(const float x,
				const float y,
				const float width,
				const float height,
				const float initial_progress = 0.0f) :
			UIElement(x, y, width, height), progress_(std::clamp(initial_progress, 0.0f, 1.0f)), bar_width_(width),
			bar_x_offset_(0.0f) {}

	~ProgressBar() override = default;

	// === Progress Configuration ===

	/**
	 * @brief Set progress value
	 *
	 * Clamps value to [0.0, 1.0] range.
	 *
	 * @param progress Progress value (0.0 = empty, 1.0 = full)
	 *
	 * @code
	 * progress->SetProgress(0.75f);  // 75% complete
	 * @endcode
	 */
	void SetProgress(const float progress) {
		progress_ = std::clamp(progress, 0.0f, 1.0f);
		UpdatePercentageDisplay();
	}

	/**
	 * @brief Get current progress value
	 * @return Progress value (0.0 - 1.0)
	 */
	float GetProgress() const { return progress_; }

	// === Display Configuration ===

	/**
	 * @brief Set label text
	 *
	 * Label is displayed to the left of the progress bar.
	 *
	 * @param label Label text (empty to hide)
	 *
	 * @code
	 * progress->SetLabel("Loading...");
	 * @endcode
	 */
	void SetLabel(const std::string& label) {
		label_text_ = label;

		if (!label.empty() && !label_element_) {
			label_element_ = std::make_unique<Text>(0, 0, label, label_font_size_, label_color_);
			label_element_->SetParent(this);
			UpdateLabelPosition();
			UpdateBarWidth();
		}
		else if (label.empty() && label_element_) {
			label_element_.reset();
			UpdateBarWidth();
		}
		else if (label_element_) {
			label_element_->SetText(label);
			UpdateLabelPosition();
			UpdateBarWidth();
		}
	}

	/**
	 * @brief Get label text
	 * @return Label text
	 */
	const std::string& GetLabel() const { return label_text_; }

	/**
	 * @brief Enable/disable percentage display
	 *
	 * When enabled, percentage is shown to the right of the bar.
	 *
	 * @param show True to show percentage, false to hide
	 *
	 * @code
	 * progress->SetShowPercentage(true);
	 * progress->SetProgress(0.75f);  // Shows "75%" next to bar
	 * @endcode
	 */
	void SetShowPercentage(const bool show) {
		show_percentage_ = show;
		if (show && !percentage_element_) {
			UpdatePercentageDisplay();
			UpdateBarWidth();
		}
		else if (!show && percentage_element_) {
			percentage_element_.reset();
			UpdateBarWidth();
		}
	}

	/**
	 * @brief Check if percentage display is enabled
	 * @return True if showing percentage
	 */
	bool GetShowPercentage() const { return show_percentage_; }

	// === Appearance Configuration ===

	/**
	 * @brief Set track (background) color
	 * @param color Track color
	 */
	void SetTrackColor(const batch_renderer::Color& color) { track_color_ = color; }

	/**
	 * @brief Get track color
	 * @return Track color
	 */
	batch_renderer::Color GetTrackColor() const { return track_color_; }

	/**
	 * @brief Set fill (progress) color
	 * @param color Fill color
	 */
	void SetFillColor(const batch_renderer::Color& color) { fill_color_ = color; }

	/**
	 * @brief Get fill color
	 * @return Fill color
	 */
	batch_renderer::Color GetFillColor() const { return fill_color_; }

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

	/**
	 * @brief Set label and percentage text color
	 * @param color Text color
	 */
	void SetTextColor(const batch_renderer::Color& color) {
		label_color_ = color;
		if (label_element_) {
			label_element_->SetColor(color);
		}
		if (percentage_element_) {
			percentage_element_->SetColor(color);
		}
	}

	// === Rendering ===

	/**
	 * @brief Render progress bar track, fill, border, label, and percentage
	 *
	 * Rendering order:
	 * 1. Track (background bar)
	 * 2. Fill (progress from left)
	 * 3. Border (if width > 0)
	 * 4. Label (left of bar)
	 * 5. Percentage (right of bar, if enabled)
	 *
	 * @code
	 * progress->Render();
	 * @endcode
	 */
	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		// Calculate bar dimensions
		const float bar_x = bounds.x + bar_x_offset_;
		const Rectangle track_rect{bar_x, bounds.y, bar_width_, height_};

		// Render track (background)
		BatchRenderer::SubmitQuad(track_rect, track_color_);

		// Render fill (progress)
		const float fill_width = bar_width_ * progress_;
		if (fill_width > 0.0f) {
			const Rectangle fill_rect{bar_x, bounds.y, fill_width, height_};
			BatchRenderer::SubmitQuad(fill_rect, fill_color_);
		}

		// Render border if width > 0
		if (border_width_ > 0.0f) {
			const float x = bar_x;
			const float y = bounds.y;
			const float w = bar_width_;
			const float h = height_;

			// Top edge
			BatchRenderer::SubmitLine(x, y, x + w, y, border_width_, border_color_);
			// Right edge
			BatchRenderer::SubmitLine(x + w, y, x + w, y + h, border_width_, border_color_);
			// Bottom edge
			BatchRenderer::SubmitLine(x + w, y + h, x, y + h, border_width_, border_color_);
			// Left edge
			BatchRenderer::SubmitLine(x, y + h, x, y, border_width_, border_color_);
		}

		// Render label (if set)
		if (label_element_) {
			label_element_->Render();
		}

		// Render percentage (if enabled)
		if (percentage_element_ && show_percentage_) {
			percentage_element_->Render();
		}

		// Render children
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
	 * @brief Update percentage display text
	 */
	void UpdatePercentageDisplay() {
		if (!show_percentage_) {
			return;
		}

		// Format percentage as string
		const std::string percentage_str = std::format("{:.0f}%", progress_ * 100.0f);

		if (!percentage_element_) {
			percentage_element_ = std::make_unique<Text>(0, 0, percentage_str, percentage_font_size_, label_color_);
			percentage_element_->SetParent(this);
		}
		else {
			percentage_element_->SetText(percentage_str);
		}

		UpdatePercentagePosition();
		UpdateBarWidth();
	}

	/**
	 * @brief Update label position (left side, vertically centered)
	 */
	void UpdateLabelPosition() const {
		if (!label_element_) {
			return;
		}

		const float label_height = label_element_->GetHeight();
		const float label_y = (height_ - label_height) * 0.5f;

		label_element_->SetRelativePosition(0.0f, label_y);
	}

	/**
	 * @brief Update percentage position (right side, vertically centered)
	 */
	void UpdatePercentagePosition() const {
		if (!percentage_element_) {
			return;
		}

		const float percentage_height = percentage_element_->GetHeight();
		const float percentage_y = (height_ - percentage_height) * 0.5f;
		const float percentage_x = width_ - percentage_element_->GetWidth();

		percentage_element_->SetRelativePosition(percentage_x, percentage_y);
	}

	/**
	 * @brief Update bar width and offset based on label/percentage widths
	 */
	void UpdateBarWidth() {
		constexpr float padding = 10.0f;

		float left_width = 0.0f;
		float right_width = 0.0f;

		if (label_element_) {
			left_width = label_element_->GetWidth() + padding;
		}

		if (percentage_element_ && show_percentage_) {
			right_width = percentage_element_->GetWidth() + padding;
		}

		bar_x_offset_ = left_width;
		bar_width_ = width_ - left_width - right_width;
	}

	float progress_;

	// Appearance
	batch_renderer::Color track_color_{batch_renderer::Colors::DARK_GRAY};
	batch_renderer::Color fill_color_{batch_renderer::Colors::GOLD};
	batch_renderer::Color border_color_{batch_renderer::Colors::LIGHT_GRAY};
	batch_renderer::Color label_color_{batch_renderer::Colors::WHITE};
	float border_width_{0.0f};

	// Display options
	bool show_percentage_{false};
	std::string label_text_;
	float label_font_size_{14.0f};
	float percentage_font_size_{12.0f};

	// Text elements (composition)
	std::unique_ptr<Text> label_element_{nullptr};
	std::unique_ptr<Text> percentage_element_{nullptr};

	// Bar layout
	float bar_width_;
	float bar_x_offset_;
};

} // namespace engine::ui::elements
