#include "example_scene.h"
#include "scene_registry.h"
#include "ui_debug_visualizer.h"

#include <imgui.h>
#include <iostream>
#include <memory>
#include <string>

import glm;
import engine;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::components;
using namespace engine::ui::batch_renderer;
using namespace engine::input;

/**
 * @brief Comprehensive UI showcase demonstrating all ported components
 *
 * This scene demonstrates:
 * - UITheme constants usage
 * - All UI components (Button, Checkbox, Slider, Panel, Label, Text, Image, ConfirmationDialog)
 * - New components (ProgressBar, TabContainer, Divider, TooltipComponent)
 * - Composition and layout patterns
 * - Event callbacks and reactive updates
 * - Declarative UI construction
 * - Batch renderer integration
 */
class UIShowcaseScene : public examples::ExampleScene {
private:
	// ========================================================================
	// UI Components
	// ========================================================================

	// Root UI elements
	std::unique_ptr<Panel> root_panel_;
	std::unique_ptr<ConfirmationDialog> confirm_dialog_;

	// Button demonstrations
	Text* button_click_count_text_ = nullptr;
	Button* normal_button_ = nullptr;
	Button* disabled_button_ = nullptr;
	Button* primary_button_ = nullptr;

	// Checkbox demonstrations
	Checkbox* checkbox_1_ = nullptr;
	Checkbox* checkbox_2_ = nullptr;
	Checkbox* checkbox_3_ = nullptr;

	// Slider demonstrations
	Slider* slider_volume_ = nullptr;
	Slider* slider_brightness_ = nullptr;
	Label* slider_value_label_ = nullptr;

	// Progress bar demonstrations
	ProgressBar* progress_bar_ = nullptr;

	// Tab container demonstrations
	TabContainer* tab_container_ = nullptr;
	TabContainer* main_tab_container_ = nullptr;

	// Tooltip demonstrations
	Button* tooltip_button_ = nullptr;

	// Debug visualizer
	UIDebugVisualizer ui_debugger_;

	// ========================================================================
	// State
	// ========================================================================

	int button_click_count_ = 0;
	bool show_confirmation_ = false;
	float volume_value_ = 0.5f;
	float brightness_value_ = 0.75f;
	float progress_value_ = 0.0f;
	float panel_width_ = 700.0f;
	float panel_height_ = 520.0f;

public:
	const char* GetName() const override { return "UI Showcase"; }

	const char* GetDescription() const override {
		return "Comprehensive demonstration of all UI components with UITheme styling";
	}

	void Initialize(engine::Engine& engine) override {
		std::cout << "UIShowcaseScene: Initializing comprehensive UI demo..." << std::endl;

		// Initialize UI systems
		text_renderer::FontManager::Initialize("fonts/Kenney Future.ttf", 16);
		BatchRenderer::Initialize();

		// Build declarative UI structure
		BuildUI();

		std::cout << "UIShowcaseScene: Initialized" << std::endl;
	}

	void BuildUI() {
		// ====================================================================
		// Root Panel - Main Container
		// ====================================================================

		root_panel_ =
				std::make_unique<Panel>(UITheme::Spacing::LARGE, UITheme::Spacing::LARGE, panel_width_, panel_height_);
		root_panel_->SetRelativePosition(25, 50);
		root_panel_->SetBackgroundColor(UITheme::Background::PANEL);
		root_panel_->SetBorderColor(UITheme::Border::DEFAULT);
		root_panel_->SetPadding(UITheme::Padding::PANEL_HORIZONTAL);

		// ====================================================================
		// Title Section
		// ====================================================================

		auto title = std::make_unique<Text>(
				UITheme::Padding::PANEL_HORIZONTAL,
				UITheme::Padding::PANEL_VERTICAL,
				"UI Component Showcase",
				UITheme::FontSize::HEADING_1,
				UITheme::Text::ACCENT);
		root_panel_->AddChild(std::move(title));

		auto subtitle = std::make_unique<Text>(
				UITheme::Padding::PANEL_HORIZONTAL,
				UITheme::FontSize::HEADING_1 + UITheme::Spacing::MEDIUM,
				"Demonstrating all components with UITheme styling",
				UITheme::FontSize::NORMAL,
				UITheme::Text::SECONDARY);
		root_panel_->AddChild(std::move(subtitle));

		// ====================================================================
		// Main Tab Container - Groups all component demos
		// ====================================================================

		float tab_y = UITheme::FontSize::HEADING_1 + UITheme::Spacing::XXL + UITheme::Spacing::LARGE;
		auto main_tabs = std::make_unique<TabContainer>(UITheme::Padding::PANEL_HORIZONTAL, tab_y, 660.0f, 400.0f);

		// Tab 1: Basic Controls (Buttons, Checkboxes, Sliders)
		auto controls_panel = BuildControlsTab();
		main_tabs->AddTab("Controls", std::move(controls_panel));

		// Tab 2: Text & Labels
		auto text_panel = BuildTextTab();
		main_tabs->AddTab("Text", std::move(text_panel));

		// Tab 3: Progress & Feedback
		auto feedback_panel = BuildFeedbackTab();
		main_tabs->AddTab("Feedback", std::move(feedback_panel));

		// Tab 4: Layout & Composition
		auto layout_panel = BuildLayoutTab();
		main_tabs->AddTab("Layout", std::move(layout_panel));

		main_tabs->SetTabChangedCallback([](size_t index, const std::string& label) {
			std::cout << "Main tab changed to: " << label << std::endl;
		});

		main_tab_container_ = main_tabs.get();
		root_panel_->AddChild(std::move(main_tabs));

		// ====================================================================
		// Confirmation Dialog (hidden initially)
		// ====================================================================

		confirm_dialog_ = std::make_unique<ConfirmationDialog>("Confirm Action", "Are you sure you want to proceed?");

		confirm_dialog_->SetConfirmCallback([this] {
			std::cout << "User confirmed action" << std::endl;
			show_confirmation_ = false;
			confirm_dialog_->Hide();
			return true;
		});

		confirm_dialog_->SetCancelCallback([this] {
			std::cout << "User canceled action" << std::endl;
			show_confirmation_ = false;
			confirm_dialog_->Hide();
			return true;
		});

		confirm_dialog_->Hide();
	}

	// ========================================================================
	// Tab Content Builders
	// ========================================================================

	/**
	 * @brief Build the Controls tab (Buttons, Checkboxes, Sliders)
	 */
	std::unique_ptr<Panel> BuildControlsTab() {
		auto panel = std::make_unique<Panel>(0, 0, 640.0f, 360.0f);
		panel->SetBackgroundColor(UITheme::Background::PANEL);
		panel->SetPadding(UITheme::Padding::MEDIUM);

		float y = 0.0f;

		// === Buttons Section ===
		auto button_title = std::make_unique<Text>(0, y, "Buttons", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(button_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		// Normal button with tooltip
		auto normal_button = std::make_unique<Button>(0, y, 120.0f, UITheme::Button::DEFAULT_HEIGHT, "Click Me");
		normal_button->SetClickCallback([this](const MouseEvent& event) {
			button_click_count_++;
			std::cout << "Button clicked! Count: " << button_click_count_ << std::endl;
			UpdateButtonClickLabel();
			return true;
		});

		// Add tooltip
		auto tooltip_label = std::make_unique<Label>(8, 8, "Click to increment counter", UITheme::FontSize::SMALL);
		auto tooltip_content =
				std::make_unique<Panel>(0, 0, tooltip_label->GetWidth() + 16, tooltip_label->GetHeight() + 16);
		tooltip_content->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		tooltip_content->SetBorderWidth(1.0f);
		tooltip_content->SetBorderColor(UITheme::Border::DEFAULT);
		tooltip_content->AddChild(std::move(tooltip_label));
		auto* tooltip = normal_button->AddComponent<TooltipComponent>(std::move(tooltip_content));
		tooltip->SetOffset(15.0f, 20.0f);

		normal_button_ = normal_button.get();
		tooltip_button_ = normal_button.get();
		panel->AddChild(std::move(normal_button));

		// Primary button
		auto primary_button = std::make_unique<Button>(140.0f, y, 120.0f, UITheme::Button::DEFAULT_HEIGHT, "Primary");
		primary_button->SetNormalColor(UITheme::Primary::NORMAL);
		primary_button->SetHoverColor(UITheme::Primary::HOVER);
		primary_button->SetPressedColor(UITheme::Primary::ACTIVE);
		primary_button->SetClickCallback([this](const MouseEvent& event) {
			show_confirmation_ = true;
			confirm_dialog_->Show();
			return true;
		});
		primary_button_ = primary_button.get();
		panel->AddChild(std::move(primary_button));

		// Disabled button
		auto disabled_button = std::make_unique<Button>(280.0f, y, 120.0f, UITheme::Button::DEFAULT_HEIGHT, "Disabled");
		disabled_button->SetEnabled(false);
		disabled_button_ = disabled_button.get();
		panel->AddChild(std::move(disabled_button));

		// Click counter
		auto click_label = std::make_unique<Text>(
				420.0f, y + 8.0f, "Clicks: 0", UITheme::FontSize::NORMAL, UITheme::Text::SECONDARY);
		button_click_count_text_ = click_label.get();
		panel->AddChild(std::move(click_label));

		y += UITheme::Button::DEFAULT_HEIGHT + UITheme::Spacing::LARGE;

		// === Divider ===
		auto divider1 = std::make_unique<Divider>();
		divider1->SetRelativePosition(0, y);
		divider1->SetSize(620.0f, 2.0f);
		panel->AddChild(std::move(divider1));
		y += UITheme::Spacing::LARGE;

		// === Checkboxes Section ===
		auto checkbox_title =
				std::make_unique<Text>(0, y, "Checkboxes", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(checkbox_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto checkbox1 = std::make_unique<Checkbox>(0.0f, y, "Enable Feature A");
		checkbox1->SetToggleCallback(
				[](bool checked) { std::cout << "Feature A: " << (checked ? "Enabled" : "Disabled") << std::endl; });
		checkbox_1_ = checkbox1.get();
		panel->AddChild(std::move(checkbox1));

		auto checkbox2 = std::make_unique<Checkbox>(200.0f, y, "Enable Feature B");
		checkbox2->SetChecked(true);
		checkbox2->SetToggleCallback(
				[](bool checked) { std::cout << "Feature B: " << (checked ? "Enabled" : "Disabled") << std::endl; });
		checkbox_2_ = checkbox2.get();
		panel->AddChild(std::move(checkbox2));

		auto checkbox3 = std::make_unique<Checkbox>(400.0f, y, "Advanced");
		checkbox3->SetToggleCallback(
				[](bool checked) { std::cout << "Advanced: " << (checked ? "Shown" : "Hidden") << std::endl; });
		checkbox_3_ = checkbox3.get();
		panel->AddChild(std::move(checkbox3));

		y += 30.0f + UITheme::Spacing::LARGE;

		// === Divider ===
		auto divider2 = std::make_unique<Divider>();
		divider2->SetRelativePosition(0, y);
		divider2->SetSize(620.0f, 2.0f);
		panel->AddChild(std::move(divider2));
		y += UITheme::Spacing::LARGE;

		// === Sliders Section ===
		auto slider_title = std::make_unique<Text>(0, y, "Sliders", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(slider_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto volume_slider = std::make_unique<Slider>(0, y, 280.0f, 20.0f, 0.0f, 1.0f);
		volume_slider->SetValue(0.5f);
		volume_slider->SetLabel("Volume");
		volume_slider->SetValueChangedCallback([this](float value) {
			volume_value_ = value;
			UpdateSliderValueLabel();
		});
		slider_volume_ = volume_slider.get();
		panel->AddChild(std::move(volume_slider));

		auto brightness_slider = std::make_unique<Slider>(320.0f, y, 280.0f, 20.0f, 0.0f, 1.0f);
		brightness_slider->SetValue(0.75f);
		brightness_slider->SetLabel("Brightness");
		brightness_slider->SetValueChangedCallback([this](float value) {
			brightness_value_ = value;
			UpdateSliderValueLabel();
		});
		slider_brightness_ = brightness_slider.get();
		panel->AddChild(std::move(brightness_slider));

		y += 30.0f;

		auto value_label = std::make_unique<Label>(0, y, "Volume: 50% | Brightness: 75%", UITheme::FontSize::SMALL);
		slider_value_label_ = value_label.get();
		panel->AddChild(std::move(value_label));

		return panel;
	}

	/**
	 * @brief Build the Text tab (Text styles and labels)
	 */
	std::unique_ptr<Panel> BuildTextTab() {
		auto panel = std::make_unique<Panel>(0, 0, 640.0f, 360.0f);
		panel->SetBackgroundColor(UITheme::Background::PANEL);
		panel->SetPadding(UITheme::Padding::MEDIUM);

		float y = 0.0f;

		// === Font Sizes ===
		auto size_title = std::make_unique<Text>(0, y, "Font Sizes", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(size_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto heading1 = std::make_unique<Text>(0, y, "Heading 1", UITheme::FontSize::HEADING_1, UITheme::Text::PRIMARY);
		panel->AddChild(std::move(heading1));
		y += UITheme::FontSize::HEADING_1 + UITheme::Spacing::SMALL;

		auto heading2 = std::make_unique<Text>(0, y, "Heading 2", UITheme::FontSize::HEADING_2, UITheme::Text::PRIMARY);
		panel->AddChild(std::move(heading2));
		y += UITheme::FontSize::HEADING_2 + UITheme::Spacing::SMALL;

		auto normal =
				std::make_unique<Text>(0, y, "Normal text (14px)", UITheme::FontSize::NORMAL, UITheme::Text::PRIMARY);
		panel->AddChild(std::move(normal));
		y += UITheme::FontSize::NORMAL + UITheme::Spacing::LARGE;

		// === Divider ===
		auto divider = std::make_unique<Divider>();
		divider->SetRelativePosition(0, y);
		divider->SetSize(620.0f, 2.0f);
		panel->AddChild(std::move(divider));
		y += UITheme::Spacing::LARGE;

		// === Text Colors ===
		auto color_title = std::make_unique<Text>(0, y, "Text Colors", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(color_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		float x = 0.0f;
		auto primary_text = std::make_unique<Text>(x, y, "Primary", UITheme::FontSize::NORMAL, UITheme::Text::PRIMARY);
		panel->AddChild(std::move(primary_text));
		x += 80.0f;

		auto secondary_text =
				std::make_unique<Text>(x, y, "Secondary", UITheme::FontSize::NORMAL, UITheme::Text::SECONDARY);
		panel->AddChild(std::move(secondary_text));
		x += 100.0f;

		auto accent_text = std::make_unique<Text>(x, y, "Accent", UITheme::FontSize::NORMAL, UITheme::Text::ACCENT);
		panel->AddChild(std::move(accent_text));
		x += 80.0f;

		auto error_text = std::make_unique<Text>(x, y, "Error", UITheme::FontSize::NORMAL, UITheme::Text::ERROR);
		panel->AddChild(std::move(error_text));
		x += 60.0f;

		auto success_text = std::make_unique<Text>(x, y, "Success", UITheme::FontSize::NORMAL, UITheme::Text::SUCCESS);
		panel->AddChild(std::move(success_text));
		x += 80.0f;

		auto warning_text = std::make_unique<Text>(x, y, "Warning", UITheme::FontSize::NORMAL, UITheme::Text::WARNING);
		panel->AddChild(std::move(warning_text));

		return panel;
	}

	/**
	 * @brief Build the Feedback tab (Progress bars, dividers)
	 */
	std::unique_ptr<Panel> BuildFeedbackTab() {
		auto panel = std::make_unique<Panel>(0, 0, 640.0f, 360.0f);
		panel->SetBackgroundColor(UITheme::Background::PANEL);
		panel->SetPadding(UITheme::Padding::MEDIUM);

		float y = 0.0f;

		// === Progress Bar Section ===
		auto progress_title =
				std::make_unique<Text>(0, y, "Progress Bar", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(progress_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto progress_bar = std::make_unique<ProgressBar>(0, y, 450.0f, 20.0f, 0.0f);
		progress_bar->SetLabel("Loading");
		progress_bar->SetShowPercentage(true);
		progress_bar->SetFillColor(UITheme::Primary::NORMAL);
		progress_bar->SetBorderWidth(1.0f);
		progress_bar_ = progress_bar.get();
		panel->AddChild(std::move(progress_bar));

		auto increment_button = std::make_unique<Button>(470.0f, y - 5.0f, 70.0f, 30.0f, "+10%");
		increment_button->SetClickCallback([this](const MouseEvent& event) {
			progress_value_ = std::min(1.0f, progress_value_ + 0.1f);
			if (progress_bar_)
				progress_bar_->SetProgress(progress_value_);
			return true;
		});
		panel->AddChild(std::move(increment_button));

		auto reset_button = std::make_unique<Button>(550.0f, y - 5.0f, 60.0f, 30.0f, "Reset");
		reset_button->SetClickCallback([this](const MouseEvent& event) {
			progress_value_ = 0.0f;
			if (progress_bar_)
				progress_bar_->SetProgress(progress_value_);
			return true;
		});
		panel->AddChild(std::move(reset_button));

		y += 40.0f + UITheme::Spacing::LARGE;

		// === Divider Styles ===
		auto divider_title = std::make_unique<Text>(0, y, "Dividers", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(divider_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto divider1 = std::make_unique<Divider>();
		divider1->SetRelativePosition(0, y);
		divider1->SetSize(620.0f, 2.0f);
		panel->AddChild(std::move(divider1));
		y += UITheme::Spacing::MEDIUM;

		auto label1 = std::make_unique<Label>(0, y, "Default divider (above)", UITheme::FontSize::SMALL);
		panel->AddChild(std::move(label1));
		y += UITheme::FontSize::SMALL + UITheme::Spacing::LARGE;

		auto divider2 = std::make_unique<Divider>(4.0f);
		divider2->SetRelativePosition(0, y);
		divider2->SetSize(620.0f, 4.0f);
		divider2->SetColor(UITheme::Primary::NORMAL);
		panel->AddChild(std::move(divider2));
		y += UITheme::Spacing::MEDIUM;

		auto label2 = std::make_unique<Label>(0, y, "Thick accent divider (above)", UITheme::FontSize::SMALL);
		panel->AddChild(std::move(label2));

		return panel;
	}

	/**
	 * @brief Build the Layout tab (Nested panels, tab container demo)
	 */
	std::unique_ptr<Panel> BuildLayoutTab() {
		auto panel = std::make_unique<Panel>(0, 0, 640.0f, 360.0f);
		panel->SetBackgroundColor(UITheme::Background::PANEL);
		panel->SetPadding(UITheme::Padding::MEDIUM);

		float y = 0.0f;

		// === Nested Panels ===
		auto nested_title =
				std::make_unique<Text>(0, y, "Nested Panels", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(nested_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto nested1 = std::make_unique<Panel>(0, y, 150.0f, 40.0f);
		nested1->SetBackgroundColor(UITheme::Primary::NORMAL);
		nested1->SetPadding(UITheme::Padding::SMALL);
		auto nested1_label = std::make_unique<Label>(0, 0, "Panel 1", UITheme::FontSize::SMALL);
		nested1->AddChild(std::move(nested1_label));
		panel->AddChild(std::move(nested1));

		auto nested2 = std::make_unique<Panel>(170.0f, y, 150.0f, 40.0f);
		nested2->SetBackgroundColor(Color::Alpha(UITheme::Background::BUTTON, 0.8f));
		nested2->SetPadding(UITheme::Padding::SMALL);
		auto nested2_label = std::make_unique<Label>(0, 0, "Panel 2", UITheme::FontSize::SMALL);
		nested2->AddChild(std::move(nested2_label));
		panel->AddChild(std::move(nested2));

		auto nested3 = std::make_unique<Panel>(340.0f, y, 150.0f, 40.0f);
		nested3->SetBackgroundColor(UITheme::Text::SUCCESS);
		nested3->SetPadding(UITheme::Padding::SMALL);
		auto nested3_label = std::make_unique<Label>(0, 0, "Panel 3", UITheme::FontSize::SMALL);
		nested3->AddChild(std::move(nested3_label));
		panel->AddChild(std::move(nested3));

		y += 50.0f + UITheme::Spacing::LARGE;

		// === Divider ===
		auto divider = std::make_unique<Divider>();
		divider->SetRelativePosition(0, y);
		divider->SetSize(620.0f, 2.0f);
		panel->AddChild(std::move(divider));
		y += UITheme::Spacing::LARGE;

		// === Nested Tab Container Demo ===
		auto tab_title =
				std::make_unique<Text>(0, y, "Nested Tab Container", UITheme::FontSize::LARGE, UITheme::Text::ACCENT);
		panel->AddChild(std::move(tab_title));
		y += UITheme::FontSize::LARGE + UITheme::Spacing::SMALL;

		auto nested_tabs = std::make_unique<TabContainer>(0, y, 620.0f, 120.0f);

		auto tab1_content = std::make_unique<Panel>(0, 0, 600.0f, 80.0f);
		tab1_content->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		auto tab1_label = std::make_unique<Label>(10, 10, "General settings go here", UITheme::FontSize::NORMAL);
		tab1_content->AddChild(std::move(tab1_label));
		nested_tabs->AddTab("General", std::move(tab1_content));

		auto tab2_content = std::make_unique<Panel>(0, 0, 600.0f, 80.0f);
		tab2_content->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		auto tab2_label = std::make_unique<Label>(10, 10, "Audio settings go here", UITheme::FontSize::NORMAL);
		tab2_content->AddChild(std::move(tab2_label));
		nested_tabs->AddTab("Audio", std::move(tab2_content));

		auto tab3_content = std::make_unique<Panel>(0, 0, 600.0f, 80.0f);
		tab3_content->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		auto tab3_label = std::make_unique<Label>(10, 10, "Video settings go here", UITheme::FontSize::NORMAL);
		tab3_content->AddChild(std::move(tab3_label));
		nested_tabs->AddTab("Video", std::move(tab3_content));

		tab_container_ = nested_tabs.get();
		panel->AddChild(std::move(nested_tabs));

		return panel;
	}

	// ========================================================================
	// Helper Methods
	// ========================================================================

	void UpdateButtonClickLabel() const {
		if (button_click_count_text_) {
			char buffer[32];
			snprintf(buffer, sizeof(buffer), "Clicks: %d", button_click_count_);
			button_click_count_text_->SetText(buffer);
		}
	}

	void UpdateSliderValueLabel() const {
		if (slider_value_label_) {
			char buffer[128];
			snprintf(
					buffer,
					sizeof(buffer),
					"Volume: %.0f%% | Brightness: %.0f%%",
					volume_value_ * 100.0f,
					brightness_value_ * 100.0f);
			slider_value_label_->SetText(buffer);
		}
	}

	void Update(engine::Engine& engine, float delta_time) override {
		std::uint32_t screen_width;
		std::uint32_t screen_height;
		engine.renderer->GetFramebufferSize(screen_width, screen_height);
		const MouseEvent mouse_event(Input::GetMouseState());

		if (show_confirmation_ && confirm_dialog_) {
			confirm_dialog_->ProcessMouseEvent(mouse_event);
			confirm_dialog_->UpdateComponentsRecursive(delta_time);
			return;
		}

		if (root_panel_) {
			root_panel_->SetRelativePosition(
					screen_width * 0.5f - panel_width_ / 2.0F, screen_height * 0.5f - panel_height_ / 2.0F);
			root_panel_->ProcessMouseEvent(mouse_event);
			root_panel_->UpdateComponentsRecursive(delta_time);
		}
	}

	void Render(engine::Engine& engine) override {
		using namespace engine::ui::batch_renderer;

		BatchRenderer::BeginFrame();

		// Render main UI
		if (root_panel_) {
			root_panel_->Render();
			root_panel_->RenderComponentsRecursive();
			ui_debugger_.RenderDebugOverlay(root_panel_.get());
		}

		// Render confirmation dialog on top
		if (show_confirmation_ && confirm_dialog_) {
			confirm_dialog_->Render();
			confirm_dialog_->RenderComponentsRecursive();
			ui_debugger_.RenderDebugOverlay(confirm_dialog_.get());
		}

		BatchRenderer::EndFrame();
	}

	void RenderUI(engine::Engine& engine) override {
		ImGui::Begin("UI Showcase Controls");

		ImGui::Text("Interactive UI Demonstration");
		ImGui::Separator();

		ImGui::Text("Features:");
		ImGui::BulletText("UITheme styling constants");
		ImGui::BulletText("All UI components (Button, Checkbox, Slider, etc.)");
		ImGui::BulletText("New: ProgressBar, TabContainer, TooltipComponent");
		ImGui::BulletText("Event callbacks and reactive updates");
		ImGui::BulletText("Composition and layout patterns");

		ImGui::Separator();

		ImGui::Text("Button clicks: %d", button_click_count_);
		ImGui::Text("Volume: %.0f%%", volume_value_ * 100.0f);
		ImGui::Text("Brightness: %.0f%%", brightness_value_ * 100.0f);
		ImGui::Text("Progress: %.0f%%", progress_value_ * 100.0f);

		if (ImGui::Button("Reset Counters")) {
			button_click_count_ = 0;
			progress_value_ = 0.0f;
			UpdateButtonClickLabel();
			if (progress_bar_) {
				progress_bar_->SetProgress(0.0f);
			}
		}

		if (ImGui::Button("Show Confirmation Dialog")) {
			show_confirmation_ = true;
			confirm_dialog_->Show();
		}

		ImGui::Separator();
		ImGui::Text("Debug Visualizer:");
		ui_debugger_.RenderImGuiControls();

		ImGui::End();
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "UIShowcaseScene: Shutting down..." << std::endl;

		// Cleanup UI
		root_panel_.reset();
		confirm_dialog_.reset();

		// Shutdown renderer
		engine::ui::batch_renderer::BatchRenderer::Shutdown();
		engine::ui::text_renderer::FontManager::Shutdown();

		std::cout << "UIShowcaseScene: Shutdown complete" << std::endl;
	}
};

// Register scene
REGISTER_EXAMPLE_SCENE(
		UIShowcaseScene, "UI Showcase", "Comprehensive demonstration of all UI components with UITheme styling");
