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
	float panel_height_ = 850.0f;

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
		// Button Section
		// ====================================================================

		float current_y = UITheme::FontSize::HEADING_1 + UITheme::Spacing::XXL + UITheme::Spacing::LARGE;

		BuildButtonSection(current_y);
		current_y += 120.0f;

		// ====================================================================
		// Checkbox Section
		// ====================================================================

		BuildCheckboxSection(current_y);
		current_y += 120.0f;

		// ====================================================================
		// Slider Section
		// ====================================================================

		BuildSliderSection(current_y);
		current_y += 120.0f;

		// ====================================================================
		// Text and Label Section
		// ====================================================================

		BuildTextSection(current_y);
		current_y += 80.0f;

		// ====================================================================
		// Nested Panel Section
		// ====================================================================

		BuildNestedPanelSection(current_y);
		current_y += 100.0f;

		// ====================================================================
		// Progress Bar Section
		// ====================================================================

		BuildProgressBarSection(current_y);
		current_y += 80.0f;

		// ====================================================================
		// Tab Container Section
		// ====================================================================

		BuildTabContainerSection(current_y);

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

	void BuildButtonSection(float y) {
		auto button_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 100.0f);
		button_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		button_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title = std::make_unique<Text>(0, 0, "Buttons", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		button_section->AddChild(std::move(section_title));

		// Normal button
		auto normal_button = std::make_unique<Button>(
				0,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				120.0f,
				UITheme::Button::DEFAULT_HEIGHT,
				"Click Me");
		normal_button->SetClickCallback([this](const MouseEvent& event) {
			button_click_count_++;
			std::cout << "Button clicked! Count: " << button_click_count_ << std::endl;
			UpdateButtonClickLabel();
			return true;
		});

		// Add tooltip to normal button
		auto tooltip_content = std::make_unique<Panel>(0, 0, 160.0f, 35.0f);
		tooltip_content->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		tooltip_content->SetBorderWidth(1.0f);
		tooltip_content->SetBorderColor(UITheme::Border::DEFAULT);
		auto tooltip_label = std::make_unique<Label>(8, 8, "Click to increment counter", UITheme::FontSize::SMALL);
		tooltip_content->AddChild(std::move(tooltip_label));
		auto* tooltip = normal_button->AddComponent<TooltipComponent>(std::move(tooltip_content));
		tooltip->SetOffset(15.0f, 20.0f);

		normal_button_ = normal_button.get();
		tooltip_button_ = normal_button.get();
		button_section->AddChild(std::move(normal_button));

		// Primary button (styled with accent color)
		auto primary_button = std::make_unique<Button>(
				140.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				120.0f,
				UITheme::Button::DEFAULT_HEIGHT,
				"Primary");
		primary_button->SetNormalColor(UITheme::Primary::NORMAL);
		primary_button->SetHoverColor(UITheme::Primary::HOVER);
		primary_button->SetPressedColor(UITheme::Primary::ACTIVE);
		primary_button->SetClickCallback([this](const MouseEvent& event) {
			show_confirmation_ = true;
			confirm_dialog_->Show();
			return true;
		});
		primary_button_ = primary_button.get();
		button_section->AddChild(std::move(primary_button));

		// Disabled button
		auto disabled_button = std::make_unique<Button>(
				280.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				120.0f,
				UITheme::Button::DEFAULT_HEIGHT,
				"Disabled");
		disabled_button->SetEnabled(false);
		disabled_button_ = disabled_button.get();
		button_section->AddChild(std::move(disabled_button));

		// Click counter label
		auto click_label = std::make_unique<Text>(
				420.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 8.0f,
				"Clicks: 0",
				UITheme::FontSize::NORMAL,
				UITheme::Text::SECONDARY);
		button_click_count_text_ = click_label.get();
		button_section->AddChild(std::move(click_label));

		root_panel_->AddChild(std::move(button_section));
	}

	void BuildCheckboxSection(float y) {
		auto checkbox_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 100.0f);
		checkbox_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		checkbox_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title =
				std::make_unique<Text>(0, 0, "Checkboxes", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		checkbox_section->AddChild(std::move(section_title));

		// Checkbox 1
		auto checkbox1 =
				std::make_unique<Checkbox>(0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, "Enable Feature A");
		checkbox1->SetToggleCallback(
				[](bool checked) { std::cout << "Feature A: " << (checked ? "Enabled" : "Disabled") << std::endl; });
		checkbox_1_ = checkbox1.get();
		checkbox_section->AddChild(std::move(checkbox1));

		// Checkbox 2 (initially checked)
		auto checkbox2 = std::make_unique<Checkbox>(
				0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 30.0f, "Enable Feature B");
		checkbox2->SetChecked(true);
		checkbox2->SetToggleCallback(
				[](bool checked) { std::cout << "Feature B: " << (checked ? "Enabled" : "Disabled") << std::endl; });
		checkbox_2_ = checkbox2.get();
		checkbox_section->AddChild(std::move(checkbox2));

		// Checkbox 3
		auto checkbox3 = std::make_unique<Checkbox>(
				250.0f, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, "Show Advanced Options");
		checkbox3->SetToggleCallback(
				[](bool checked) { std::cout << "Advanced Options: " << (checked ? "Shown" : "Hidden") << std::endl; });
		checkbox_3_ = checkbox3.get();
		checkbox_section->AddChild(std::move(checkbox3));

		root_panel_->AddChild(std::move(checkbox_section));
	}

	void BuildSliderSection(float y) {
		auto slider_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 100.0f);
		slider_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		slider_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title = std::make_unique<Text>(0, 0, "Sliders", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		slider_section->AddChild(std::move(section_title));

		// Volume slider
		auto volume_slider = std::make_unique<Slider>(
				0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 280.0f, 20.0f, 0.0f, 1.0f);
		volume_slider->SetValue(0.5f);
		volume_slider->SetLabel("Volume");
		volume_slider->SetValueChangedCallback([this](float value) {
			volume_value_ = value;
			std::cout << "Volume: " << (value * 100.0f) << "%" << std::endl;
			UpdateSliderValueLabel();
		});
		slider_volume_ = volume_slider.get();
		slider_section->AddChild(std::move(volume_slider));

		// Brightness slider
		auto brightness_slider = std::make_unique<Slider>(
				300.0f, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 280.0f, 20.0f, 0.0f, 1.0f);
		brightness_slider->SetValue(0.75f);
		brightness_slider->SetValueChangedCallback([this](float value) {
			brightness_value_ = value;
			std::cout << "Brightness: " << (value * 100.0f) << "%" << std::endl;
			UpdateSliderValueLabel();
		});
		brightness_slider->SetLabel("Brightness");
		slider_brightness_ = brightness_slider.get();
		slider_section->AddChild(std::move(brightness_slider));

		// Value display label
		auto value_label = std::make_unique<Label>(
				0,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 30.0f,
				"Volume: 50% | Brightness: 75%",
				UITheme::FontSize::SMALL);
		slider_value_label_ = value_label.get();
		slider_section->AddChild(std::move(value_label));

		root_panel_->AddChild(std::move(slider_section));
	}

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

	void BuildTextSection(float y) {
		auto text_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 60.0f);
		text_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		text_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title =
				std::make_unique<Text>(0, 0, "Text & Labels", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		text_section->AddChild(std::move(section_title));

		// Various text styles
		auto normal_text = std::make_unique<Text>(
				0,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				"Normal Text",
				UITheme::FontSize::NORMAL,
				UITheme::Text::PRIMARY);
		text_section->AddChild(std::move(normal_text));

		auto secondary_text = std::make_unique<Text>(
				120.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				"Secondary Text",
				UITheme::FontSize::NORMAL,
				UITheme::Text::SECONDARY);
		text_section->AddChild(std::move(secondary_text));

		auto accent_text = std::make_unique<Text>(
				260.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				"Accent Text",
				UITheme::FontSize::NORMAL,
				UITheme::Text::ACCENT);
		text_section->AddChild(std::move(accent_text));

		auto error_text = std::make_unique<Text>(
				380.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				"Error",
				UITheme::FontSize::NORMAL,
				UITheme::Text::ERROR);
		text_section->AddChild(std::move(error_text));

		auto success_text = std::make_unique<Text>(
				460.0f,
				UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
				"Success",
				UITheme::FontSize::NORMAL,
				UITheme::Text::SUCCESS);
		text_section->AddChild(std::move(success_text));

		root_panel_->AddChild(std::move(text_section));
	}

	void BuildNestedPanelSection(float y) {
		auto nested_panel_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 80.0f);
		nested_panel_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		nested_panel_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title = std::make_unique<Text>(
				0, 0, "Nested Panels (Composition)", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		nested_panel_section->AddChild(std::move(section_title));

		// Create nested panels
		auto nested_panel_1 =
				std::make_unique<Panel>(0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 150.0f, 40.0f);
		nested_panel_1->SetBackgroundColor(UITheme::Primary::NORMAL);
		nested_panel_1->SetPadding(UITheme::Padding::SMALL);

		auto nested_label_1 = std::make_unique<Label>(0, 0, "Nested Panel 1", UITheme::FontSize::SMALL);
		nested_panel_1->AddChild(std::move(nested_label_1));
		nested_panel_section->AddChild(std::move(nested_panel_1));

		auto nested_panel_2 =
				std::make_unique<Panel>(170.0f, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 150.0f, 40.0f);
		nested_panel_2->SetBackgroundColor(Color::Alpha(UITheme::Background::BUTTON, 0.8f));
		nested_panel_2->SetPadding(UITheme::Padding::SMALL);

		auto nested_label_2 = std::make_unique<Label>(0, 0, "Nested Panel 2", UITheme::FontSize::SMALL);
		nested_panel_2->AddChild(std::move(nested_label_2));
		nested_panel_section->AddChild(std::move(nested_panel_2));

		root_panel_->AddChild(std::move(nested_panel_section));
	}

	void BuildProgressBarSection(float y) {
		auto progress_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 60.0f);
		progress_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		progress_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title =
				std::make_unique<Text>(0, 0, "Progress Bar", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		progress_section->AddChild(std::move(section_title));

		// Progress bar with label and percentage
		auto progress_bar = std::make_unique<ProgressBar>(
				0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 400.0f, 20.0f, 0.0f);
		progress_bar->SetLabel("Loading");
		progress_bar->SetShowPercentage(true);
		progress_bar->SetFillColor(UITheme::Primary::NORMAL);
		progress_bar->SetBorderWidth(1.0f);
		progress_bar_ = progress_bar.get();
		progress_section->AddChild(std::move(progress_bar));

		// Button to increment progress
		auto increment_button = std::make_unique<Button>(
				500.0f, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM - 5.0f, 80.0f, 30.0f, "+10%");
		increment_button->SetClickCallback([this](const MouseEvent& event) {
			progress_value_ = std::min(1.0f, progress_value_ + 0.1f);
			if (progress_bar_) {
				progress_bar_->SetProgress(progress_value_);
			}
			return true;
		});
		progress_section->AddChild(std::move(increment_button));

		// Reset button
		auto reset_button = std::make_unique<Button>(
				590.0f, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM - 5.0f, 60.0f, 30.0f, "Reset");
		reset_button->SetClickCallback([this](const MouseEvent& event) {
			progress_value_ = 0.0f;
			if (progress_bar_) {
				progress_bar_->SetProgress(progress_value_);
			}
			return true;
		});
		progress_section->AddChild(std::move(reset_button));

		root_panel_->AddChild(std::move(progress_section));
	}

	void BuildTabContainerSection(float y) {
		auto tab_section = std::make_unique<Panel>(UITheme::Padding::PANEL_HORIZONTAL, y, 660.0f, 150.0f);
		tab_section->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		tab_section->SetPadding(UITheme::Padding::MEDIUM);

		// Section title
		auto section_title =
				std::make_unique<Text>(0, 0, "Tab Container", UITheme::FontSize::LARGE, UITheme::Text::PRIMARY);
		tab_section->AddChild(std::move(section_title));

		// Create tab container
		auto tab_container =
				std::make_unique<TabContainer>(0, UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM, 640.0f, 100.0f);

		// Tab 1: General settings
		auto general_panel = std::make_unique<Panel>(0, 0, 620.0f, 60.0f);
		general_panel->SetBackgroundColor(UITheme::Background::PANEL);
		auto general_label =
				std::make_unique<Label>(10, 10, "General settings content goes here.", UITheme::FontSize::NORMAL);
		general_panel->AddChild(std::move(general_label));
		tab_container->AddTab("General", std::move(general_panel));

		// Tab 2: Audio settings
		auto audio_panel = std::make_unique<Panel>(0, 0, 620.0f, 60.0f);
		audio_panel->SetBackgroundColor(UITheme::Background::PANEL);
		auto audio_label =
				std::make_unique<Label>(10, 10, "Audio settings: Volume, Effects, Music", UITheme::FontSize::NORMAL);
		audio_panel->AddChild(std::move(audio_label));
		tab_container->AddTab("Audio", std::move(audio_panel));

		// Tab 3: Video settings
		auto video_panel = std::make_unique<Panel>(0, 0, 620.0f, 60.0f);
		video_panel->SetBackgroundColor(UITheme::Background::PANEL);
		auto video_label = std::make_unique<Label>(
				10, 10, "Video settings: Resolution, Quality, VSync", UITheme::FontSize::NORMAL);
		video_panel->AddChild(std::move(video_label));
		tab_container->AddTab("Video", std::move(video_panel));

		tab_container->SetTabChangedCallback([](size_t index, const std::string& label) {
			std::cout << "Tab changed to: " << label << " (index " << index << ")" << std::endl;
		});

		tab_container_ = tab_container.get();
		tab_section->AddChild(std::move(tab_container));

		root_panel_->AddChild(std::move(tab_section));
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
