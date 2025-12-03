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
 * - Horizontal tab bar with Button elements for navigation
 * - Content panel that owns and clips the active tab's content
 * - Support for adding/removing tabs dynamically
 * - Tab selection callback for external notification
 *
 * **Architecture:**
 * - Tab buttons are children of a tab_bar_panel_ (proper event routing)
 * - Tab content is owned by content_panel_->children_ (proper scissor clipping)
 * - TabInfo stores raw pointers to button/content for visibility control
 * - Switching tabs toggles visibility, preserving state
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
 * - Content panel per tab with proper clipping
 * - Active tab highlighting via Button states
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
	TabContainer(const float x, const float y, const float width, const float height) : UIElement(x, y, width, height) {
		// Create tab bar panel for button layout - added as child for proper hierarchy
		auto tab_bar = std::make_unique<Panel>(0, 0, width, tab_bar_height_);
		tab_bar->SetBackgroundColor(batch_renderer::UITheme::Background::PANEL_DARK);
		tab_bar_panel_ = tab_bar.get();
		AddChild(std::move(tab_bar));

		// Create content panel (sized to fill below tab bar) - added as child for proper hierarchy
		auto content = std::make_unique<Panel>(0, tab_bar_height_, width, GetContentHeight());
		content->SetBackgroundColor(batch_renderer::UITheme::Background::PANEL);
		content->SetClipChildren(true);
		content_panel_ = content.get();
		AddChild(std::move(content));
	}

	~TabContainer() override = default;

	// === Tab Management ===

	/**
	 * @brief Add a new tab with label and content
	 *
	 * Content ownership is transferred to content_panel_. A raw pointer is stored
	 * for visibility control. Tab buttons are created as children of tab_bar_panel_.
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

		// Transfer content ownership to content_panel_, store raw pointer
		if (content) {
			tab.content = content.get();
			tab.content->SetVisible(tabs_.empty()); // First tab is visible
			content_panel_->AddChild(std::move(content));
		}

		tabs_.push_back(std::move(tab));

		// Auto-select first tab and invoke callback
		if (tabs_.size() == 1) {
			active_tab_index_ = 0;
			if (tab_changed_callback_) {
				tab_changed_callback_(active_tab_index_, tabs_[active_tab_index_].label);
			}
		}

		RebuildTabButtons();
		return tabs_.size() - 1;
	}

	/**
	 * @brief Remove a tab by index
	 *
	 * Removes the tab button and content. Content is destroyed since it's owned
	 * by content_panel_.
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

		// Remove content from content_panel_ (destroys it)
		if (tabs_[index].content) {
			content_panel_->RemoveChild(tabs_[index].content);
		}

		tabs_.erase(tabs_.begin() + static_cast<ptrdiff_t>(index));

		// Adjust active tab if necessary
		if (active_tab_index_ >= tabs_.size() && !tabs_.empty()) {
			active_tab_index_ = tabs_.size() - 1;
		}

		RebuildTabButtons();
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
	 * If the requested tab is already active, the callback is not invoked unless `force` is true.
	 *
	 * @param index Index of tab to activate
	 * @param force If true, triggers callback and updates tab even if already active (default: false)
	 *
	 * @code
	 * tabs->SetActiveTab(0);         // Select first tab
	 * tabs->SetActiveTab(0, true);   // Force callback for first tab
	 * @endcode
	 */
	void SetActiveTab(const size_t index, const bool force = false) {
		if (index >= tabs_.size() || (index == active_tab_index_ && !force)) {
			return;
		}

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
	 * @brief Get content area height (computed dynamically from container height minus tab bar)
	 * @return Content height in pixels
	 */
	float GetContentHeight() const { return height_ - tab_bar_height_; }

	/**
	 * @brief Set tab bar height
	 * @param height Tab bar height in pixels
	 */
	void SetTabBarHeight(const float height) {
		tab_bar_height_ = height;
		tab_bar_panel_->SetSize(width_, tab_bar_height_);
		content_panel_->SetRelativePosition(0, tab_bar_height_);
		content_panel_->SetSize(width_, GetContentHeight());
		RebuildTabButtons();
	}

	/**
	 * @brief Set tab bar background color
	 * @param color Background color
	 */
	void SetTabBarColor(const batch_renderer::Color& color) {
		tab_bar_color_ = color;
		if (tab_bar_panel_) {
			tab_bar_panel_->SetBackgroundColor(color);
		}
	}

	/**
	 * @brief Set active tab color
	 * @param color Active tab background color
	 */
	void SetActiveTabColor(const batch_renderer::Color& color) {
		active_tab_color_ = color;
		UpdateTabButtonStyles();
	}

	/**
	 * @brief Set inactive tab color
	 * @param color Inactive tab background color
	 */
	void SetInactiveTabColor(const batch_renderer::Color& color) {
		inactive_tab_color_ = color;
		UpdateTabButtonStyles();
	}

	/**
	 * @brief Set tab text color
	 * @param color Text color for tab labels
	 */
	void SetTabTextColor(const batch_renderer::Color& color) {
		tab_text_color_ = color;
		UpdateTabButtonStyles();
	}

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

	// === Rendering ===

	/**
	 * @brief Render tab bar panel with buttons, and content panel with active content
	 *
	 * Rendering is handled by the standard UIElement child traversal since
	 * tab_bar_panel_ and content_panel_ are proper children.
	 *
	 * @code
	 * tabs->Render();
	 * @endcode
	 */
	void Render() const override {
		if (!IsVisible()) {
			return;
		}

		// Render all children (tab_bar_panel_ and content_panel_ are children)
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	/**
	 * @brief Tab information structure
	 *
	 * Stores raw pointers to button and content. Ownership:
	 * - button: owned by tab_bar_panel_->children_
	 * - content: owned by content_panel_->children_
	 */
	struct TabInfo {
		std::string label;
		Button* button{nullptr};    // Raw pointer - tab_bar_panel_ owns
		UIElement* content{nullptr}; // Raw pointer - content_panel_ owns
	};

	/**
	 * @brief Rebuild all tab buttons
	 *
	 * Called when tabs are added/removed. Clears existing buttons and creates
	 * new ones with proper sizing and callbacks.
	 */
	void RebuildTabButtons() {
		// Clear existing buttons from tab bar
		tab_bar_panel_->ClearChildren();

		if (tabs_.empty()) {
			return;
		}

		// Calculate tab width (evenly distributed)
		const float tab_width = width_ / static_cast<float>(tabs_.size());

		// Create buttons for each tab
		float x = 0;
		for (size_t i = 0; i < tabs_.size(); ++i) {
			auto button = std::make_unique<Button>(x, 0, tab_width, tab_bar_height_, tabs_[i].label, tab_font_size_);

			// Style the button
			const bool is_active = (i == active_tab_index_);
			button->SetNormalColor(is_active ? active_tab_color_ : inactive_tab_color_);
			button->SetHoverColor(batch_renderer::Color::Brightness(inactive_tab_color_, 0.1f));
			button->SetPressedColor(active_tab_color_);
			button->SetTextColor(tab_text_color_);
			button->SetBorderWidth(0.0f);

			// Capture index for callback
			const size_t tab_index = i;
			button->SetClickCallback([this, tab_index](const MouseEvent&) {
				SetActiveTab(tab_index);
				return true;
			});

			// Store raw pointer and transfer ownership
			tabs_[i].button = button.get();
			tab_bar_panel_->AddChild(std::move(button));

			x += tab_width;
		}
	}

	/**
	 * @brief Update content visibility based on active tab
	 */
	void UpdateContentVisibility() const {
		for (size_t i = 0; i < tabs_.size(); ++i) {
			if (tabs_[i].content) {
				tabs_[i].content->SetVisible(i == active_tab_index_);
			}
		}
	}

	/**
	 * @brief Update tab button styles when active tab or colors change
	 */
	void UpdateTabButtonStyles() const {
		for (size_t i = 0; i < tabs_.size(); ++i) {
			if (tabs_[i].button) {
				const bool is_active = (i == active_tab_index_);
				tabs_[i].button->SetNormalColor(is_active ? active_tab_color_ : inactive_tab_color_);
				tabs_[i].button->SetTextColor(tab_text_color_);
			}
		}
	}

	std::vector<TabInfo> tabs_;
	size_t active_tab_index_{0};

	// Panel hierarchy (raw pointers - owned by children_ via AddChild)
	Panel* tab_bar_panel_{nullptr};   // Contains tab Button elements
	Panel* content_panel_{nullptr};   // Contains active tab content

	// Appearance
	float tab_bar_height_{30.0f};
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
