module;

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

export module engine.ui:elements.tab_container;

import :ui_element;
import :elements.panel;
import :elements.button;
import :elements.text;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
/**
 * @brief Tab container UI element for switching between content panels
 *
 * TabContainer provides tabbed navigation with multiple content panels:
 * - Horizontal tab bar at top for navigation
 * - Content panel that shows active tab's content
 * - Support for adding/removing tabs dynamically
 * - Tab selection callback for external notification
 *
 * **Usage Pattern (Declarative + Observer):**
 * @code
 * // Create tab container once (declarative)
 * auto tabs = std::make_unique<TabContainer>(10, 10, 400, 300);
 *
 * // Add tabs with content
 * auto general_panel = std::make_unique<Panel>(0, 0, 380, 250);
 * // ... add children to general_panel
 * tabs->AddTab("General", std::move(general_panel));
 *
 * auto audio_panel = std::make_unique<Panel>(0, 0, 380, 250);
 * // ... add children to audio_panel
 * tabs->AddTab("Audio", std::move(audio_panel));
 *
 * // Wire callback (observer pattern)
 * tabs->SetTabChangedCallback([this](size_t index, const std::string& label) {
 *     OnTabChanged(index, label);
 * });
 *
 * // Render many times
 * tabs->Render();
 * @endcode
 *
 * **Features:**
 * - Multiple tabs with labels
 * - Content panel per tab
 * - Active tab highlighting
 * - Tab selection callback
 * - Dynamic tab management (add/remove)
 *
 * @see UI_DEVELOPMENT_BIBLE.md for complete patterns and best practices
 */
class TabContainer : public UIElement {
public:
	/**
	 * @brief Tab changed callback type
	 *
	 * Called when the active tab changes.
	 *
	 * @param index Index of newly selected tab
	 * @param label Label of newly selected tab
	 */
	using TabChangedCallback = std::function<void(size_t index, const std::string& label)>;

	/**
	 * @brief Construct tab container for layout (position determined by layout)
	 *
	 * Creates a tab container with zero position (will be set by parent layout).
	 *
	 * @param width Container width in pixels
	 * @param height Container height in pixels
	 *
	 * @code
	 * auto tabs = std::make_unique<TabContainer>(400, 300);
	 * container->AddChild(std::move(tabs));  // Layout sets position
	 * @endcode
	 */
	TabContainer(const float width, const float height) : TabContainer(0, 0, width, height) {}

	/**
	 * @brief Construct tab container with position and size
	 *
	 * @param x X position relative to parent
	 * @param y Y position relative to parent
	 * @param width Container width in pixels
	 * @param height Container height in pixels
	 *
	 * @code
	 * auto tabs = std::make_unique<TabContainer>(10, 10, 400, 300);
	 * @endcode
	 */
	TabContainer(const float x, const float y, const float width, const float height) :
			UIElement(x, y, width, height), content_height_(height - tab_bar_height_) {
		// Create content panel (sized to fill below tab bar)
		content_panel_ = std::make_unique<Panel>(0, tab_bar_height_, width, content_height_);
		content_panel_->SetParent(this);
		content_panel_->SetBackgroundColor(batch_renderer::UITheme::Background::PANEL);
		content_panel_->SetClipChildren(true);
	}

	~TabContainer() override = default;

	// === Tab Management ===

	/**
	 * @brief Add a new tab with label and content
	 *
	 * @param label Tab label displayed in tab bar
	 * @param content Content panel shown when tab is active
	 * @return Index of newly added tab
	 *
	 * @code
	 * auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	 * tabs->AddTab("Settings", std::move(panel));
	 * @endcode
	 */
	size_t AddTab(const std::string& label, std::unique_ptr<UIElement> content) {
		TabInfo tab;
		tab.label = label;
		tab.content = std::move(content);

		// Set content parent for proper coordinate system
		if (tab.content) {
			tab.content->SetParent(content_panel_.get());
			tab.content->SetVisible(tabs_.empty()); // First tab is visible
		}

		tabs_.push_back(std::move(tab));

		// Auto-select first tab
		if (tabs_.size() == 1) {
			active_tab_index_ = 0;
		}

		UpdateTabButtons();
		return tabs_.size() - 1;
	}

	/**
	 * @brief Remove a tab by index
	 *
	 * @param index Index of tab to remove
	 * @return True if tab was removed, false if index out of range
	 *
	 * @code
	 * tabs->RemoveTab(1);  // Remove second tab
	 * @endcode
	 */
	bool RemoveTab(const size_t index) {
		if (index >= tabs_.size()) {
			return false;
		}

		tabs_.erase(tabs_.begin() + static_cast<ptrdiff_t>(index));

		// Adjust active tab if necessary
		if (active_tab_index_ >= tabs_.size() && !tabs_.empty()) {
			active_tab_index_ = tabs_.size() - 1;
		}

		UpdateTabButtons();
		UpdateContentVisibility();
		return true;
	}

	/**
	 * @brief Get number of tabs
	 * @return Tab count
	 */
	size_t GetTabCount() const { return tabs_.size(); }

	/**
	 * @brief Get label of a tab
	 *
	 * @param index Tab index
	 * @return Tab label, or empty string if index out of range
	 */
	std::string GetTabLabel(const size_t index) const {
		if (index >= tabs_.size()) {
			return "";
		}
		return tabs_[index].label;
	}

	// === Tab Selection ===

	/**
	 * @brief Set active tab by index
	 *
	 * @param index Index of tab to activate
	 *
	 * @code
	 * tabs->SetActiveTab(0);  // Select first tab
	 * @endcode
	 */
	void SetActiveTab(const size_t index) {
		if (index >= tabs_.size() || index == active_tab_index_) {
			return;
		}

		const size_t old_index = active_tab_index_;
		active_tab_index_ = index;

		UpdateContentVisibility();
		UpdateTabButtonStyles();

		// Notify callback
		if (tab_changed_callback_ && active_tab_index_ < tabs_.size()) {
			tab_changed_callback_(active_tab_index_, tabs_[active_tab_index_].label);
		}
	}

	/**
	 * @brief Get active tab index
	 * @return Index of currently active tab
	 */
	size_t GetActiveTab() const { return active_tab_index_; }

	// === Event Callbacks ===

	/**
	 * @brief Set tab changed callback
	 *
	 * Called when active tab changes via user interaction or SetActiveTab().
	 *
	 * @param callback Function to call when tab changes
	 *
	 * @code
	 * tabs->SetTabChangedCallback([this](size_t index, const std::string& label) {
	 *     std::cout << "Tab changed to: " << label << std::endl;
	 * });
	 * @endcode
	 */
	void SetTabChangedCallback(TabChangedCallback callback) { tab_changed_callback_ = std::move(callback); }

	// === Appearance Configuration ===

	/**
	 * @brief Set tab bar height
	 * @param height Tab bar height in pixels
	 */
	void SetTabBarHeight(const float height) {
		tab_bar_height_ = height;
		content_height_ = height_ - tab_bar_height_;
		content_panel_->SetRelativePosition(0, tab_bar_height_);
		content_panel_->SetSize(width_, content_height_);
		UpdateTabButtons();
	}

	/**
	 * @brief Set tab bar background color
	 * @param color Background color
	 */
	void SetTabBarColor(const batch_renderer::Color& color) { tab_bar_color_ = color; }

	/**
	 * @brief Set active tab color
	 * @param color Active tab background color
	 */
	void SetActiveTabColor(const batch_renderer::Color& color) { active_tab_color_ = color; }

	/**
	 * @brief Set inactive tab color
	 * @param color Inactive tab background color
	 */
	void SetInactiveTabColor(const batch_renderer::Color& color) { inactive_tab_color_ = color; }

	/**
	 * @brief Set tab text color
	 * @param color Text color for tab labels
	 */
	void SetTabTextColor(const batch_renderer::Color& color) { tab_text_color_ = color; }

	/**
	 * @brief Set content panel background color
	 * @param color Content background color
	 */
	void SetContentBackgroundColor(const batch_renderer::Color& color) {
		content_background_color_ = color;
		if (content_panel_) {
			content_panel_->SetBackgroundColor(color);
		}
	}

	// === Event Handlers ===

	/**
	 * @brief Process mouse event
	 *
	 * Routes events to tab buttons and content panel.
	 */
	bool ProcessMouseEvent(const MouseEvent& event) override {
		if (!IsVisible()) {
			return false;
		}

		// First, let content panel handle if it contains the event
		if (content_panel_ && content_panel_->ProcessMouseEvent(event)) {
			return true;
		}

		// Check tab buttons
		const batch_renderer::Rectangle bounds = GetAbsoluteBounds();
		const float tab_bar_y = bounds.y;

		// Check if click is in tab bar area
		if (event.y >= tab_bar_y && event.y < tab_bar_y + tab_bar_height_) {
			if (event.left_pressed) {
				// Calculate which tab was clicked
				const float relative_x = event.x - bounds.x;
				const size_t clicked_tab = static_cast<size_t>(relative_x / tab_width_);

				if (clicked_tab < tabs_.size()) {
					SetActiveTab(clicked_tab);
					return true;
				}
			}
			return true; // Consume event in tab bar area
		}

		return UIElement::ProcessMouseEvent(event);
	}

	// === Rendering ===

	/**
	 * @brief Render tab bar, tab buttons, and content panel
	 *
	 * Rendering order:
	 * 1. Tab bar background
	 * 2. Tab buttons (inactive, then active on top)
	 * 3. Content panel with active content
	 *
	 * @code
	 * tabs->Render();
	 * @endcode
	 */
	void Render() const override {
		using namespace batch_renderer;

		if (!IsVisible()) {
			return;
		}

		const Rectangle bounds = GetAbsoluteBounds();

		// Render tab bar background
		const Rectangle tab_bar_rect{bounds.x, bounds.y, width_, tab_bar_height_};
		BatchRenderer::SubmitQuad(tab_bar_rect, tab_bar_color_);

		// Render tab buttons
		RenderTabButtons(bounds);

		// Render content panel
		if (content_panel_) {
			content_panel_->Render();

			// Render active content
			if (active_tab_index_ < tabs_.size() && tabs_[active_tab_index_].content) {
				tabs_[active_tab_index_].content->Render();
			}
		}

		// Render children
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
	 * @brief Tab information structure
	 */
	struct TabInfo {
		std::string label;
		std::unique_ptr<UIElement> content;
	};

	/**
	 * @brief Render tab buttons
	 */
	void RenderTabButtons(const batch_renderer::Rectangle& bounds) const {
		using namespace batch_renderer;

		if (tabs_.empty()) {
			return;
		}

		float x = bounds.x;
		const float y = bounds.y;

		for (size_t i = 0; i < tabs_.size(); ++i) {
			const bool is_active = (i == active_tab_index_);
			const Color bg_color = is_active ? active_tab_color_ : inactive_tab_color_;

			// Tab button rectangle
			const Rectangle tab_rect{x, y, tab_width_, tab_bar_height_};
			BatchRenderer::SubmitQuad(tab_rect, bg_color);

			// Tab border (bottom line for inactive tabs)
			if (!is_active) {
				BatchRenderer::SubmitLine(
						x, y + tab_bar_height_, x + tab_width_, y + tab_bar_height_, 1.0f, Colors::GRAY);
			}

			// Tab label (centered)
			const float text_x = x + tab_width_ * 0.5f;
			const float text_y = y + tab_bar_height_ * 0.5f - tab_font_size_ * 0.5f;
			BatchRenderer::SubmitText(tabs_[i].label.c_str(), text_x, text_y, tab_font_size_, tab_text_color_);

			x += tab_width_;
		}
	}

	/**
	 * @brief Update tab buttons layout
	 */
	void UpdateTabButtons() {
		if (tabs_.empty()) {
			tab_width_ = 0;
			return;
		}

		// Calculate tab width (evenly distributed)
		tab_width_ = width_ / static_cast<float>(tabs_.size());
	}

	/**
	 * @brief Update content visibility based on active tab
	 */
	void UpdateContentVisibility() {
		for (size_t i = 0; i < tabs_.size(); ++i) {
			if (tabs_[i].content) {
				tabs_[i].content->SetVisible(i == active_tab_index_);
			}
		}
	}

	/**
	 * @brief Update tab button styles (called when active tab changes)
	 */
	void UpdateTabButtonStyles() {
		// Tab button rendering uses colors directly in Render()
		// No additional state needed here
	}

	std::vector<TabInfo> tabs_;
	size_t active_tab_index_{0};

	// Content panel
	std::unique_ptr<Panel> content_panel_;
	float content_height_;

	// Appearance
	float tab_bar_height_{30.0f};
	float tab_width_{0.0f};
	float tab_font_size_{14.0f};
	batch_renderer::Color tab_bar_color_{batch_renderer::UITheme::Background::PANEL_DARK};
	batch_renderer::Color active_tab_color_{batch_renderer::UITheme::Background::PANEL};
	batch_renderer::Color inactive_tab_color_{batch_renderer::UITheme::Background::PANEL_DARK};
	batch_renderer::Color tab_text_color_{batch_renderer::UITheme::Text::PRIMARY};
	batch_renderer::Color content_background_color_{batch_renderer::UITheme::Background::PANEL};

	// Callbacks
	TabChangedCallback tab_changed_callback_{nullptr};
};

} // namespace engine::ui::elements
