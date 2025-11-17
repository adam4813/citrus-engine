#include "example_scene.h"
#include "scene_registry.h"

#include <iostream>
#include <memory>
#include <string>

#include <imgui.h>

import glm;
import engine;

/**
 * @brief Comprehensive UI showcase demonstrating all ported components
 *
 * This scene demonstrates:
 * - UITheme constants usage
 * - All UI components (Button, Checkbox, Slider, Panel, Label, Text, Image, ConfirmationDialog)
 * - Composition and layout patterns
 * - Event callbacks and reactive updates
 * - Declarative UI construction
 * - Batch renderer integration
 */
class UIShowcaseScene : public examples::ExampleScene {
private:
	using namespace engine::ui;
	using namespace engine::ui::elements;
	using namespace engine::ui::batch_renderer;
	
	// ========================================================================
	// UI Components
	// ========================================================================
	
	// Main container panel
	std::unique_ptr<Panel> root_panel_;
	
	// Button demonstrations
	std::unique_ptr<Panel> button_section_;
	Button* normal_button_ = nullptr;
	Button* disabled_button_ = nullptr;
	Button* primary_button_ = nullptr;
	
	// Checkbox demonstrations
	std::unique_ptr<Panel> checkbox_section_;
	Checkbox* checkbox_1_ = nullptr;
	Checkbox* checkbox_2_ = nullptr;
	Checkbox* checkbox_3_ = nullptr;
	
	// Slider demonstrations
	std::unique_ptr<Panel> slider_section_;
	Slider* slider_volume_ = nullptr;
	Slider* slider_brightness_ = nullptr;
	Label* slider_value_label_ = nullptr;
	
	// Label and Text demonstrations
	std::unique_ptr<Panel> text_section_;
	
	// Panel nesting demonstration
	std::unique_ptr<Panel> nested_panel_section_;
	
	// Confirmation dialog
	std::unique_ptr<ConfirmationDialog> confirm_dialog_;
	
	// ========================================================================
	// State
	// ========================================================================
	
	int button_click_count_ = 0;
	bool show_confirmation_ = false;
	float volume_value_ = 0.5f;
	float brightness_value_ = 0.75f;
	
public:
	const char* GetName() const override { 
		return "UI Showcase"; 
	}
	
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
		
		root_panel_ = std::make_unique<Panel>(
			UITheme::Spacing::LARGE,
			UITheme::Spacing::LARGE,
			700.0f,
			600.0f
		);
		root_panel_->SetBackgroundColor(UITheme::Background::PANEL);
		root_panel_->SetBorderColor(UITheme::Border::DEFAULT);
		root_panel_->SetPadding(
			UITheme::Padding::PANEL_HORIZONTAL,
			UITheme::Padding::PANEL_VERTICAL
		);
		
		// ====================================================================
		// Title Section
		// ====================================================================
		
		auto title = std::make_unique<Text>(
			UITheme::Padding::PANEL_HORIZONTAL,
			UITheme::Padding::PANEL_VERTICAL,
			"UI Component Showcase",
			UITheme::FontSize::HEADING_1,
			UITheme::Text::ACCENT
		);
		root_panel_->AddChild(std::move(title));
		
		auto subtitle = std::make_unique<Text>(
			UITheme::Padding::PANEL_HORIZONTAL,
			UITheme::FontSize::HEADING_1 + UITheme::Spacing::MEDIUM,
			"Demonstrating all components with UITheme styling",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SECONDARY
		);
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
		
		// ====================================================================
		// Confirmation Dialog (hidden initially)
		// ====================================================================
		
		confirm_dialog_ = std::make_unique<ConfirmationDialog>(
			200.0f, 200.0f, 300.0f, 150.0f,
			"Confirm Action",
			"Are you sure you want to proceed?"
		);
		
		confirm_dialog_->SetConfirmCallback([this](const MouseEvent& event) {
			std::cout << "User confirmed action" << std::endl;
			show_confirmation_ = false;
			confirm_dialog_->Hide();
			return true;
		});
		
		confirm_dialog_->SetCancelCallback([this](const MouseEvent& event) {
			std::cout << "User canceled action" << std::endl;
			show_confirmation_ = false;
			confirm_dialog_->Hide();
			return true;
		});
		
		confirm_dialog_->Hide();
	}
	
	void BuildButtonSection(float y) {
		button_section_ = std::make_unique<Panel>(
			UITheme::Padding::PANEL_HORIZONTAL,
			y,
			660.0f,
			100.0f
		);
		button_section_->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		button_section_->SetPadding(UITheme::Padding::MEDIUM, UITheme::Padding::MEDIUM);
		
		// Section title
		auto section_title = std::make_unique<Text>(
			0, 0,
			"Buttons",
			UITheme::FontSize::LARGE,
			UITheme::Text::PRIMARY
		);
		button_section_->AddChild(std::move(section_title));
		
		// Normal button
		auto normal_button = std::make_unique<Button>(
			0, 
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			120.0f,
			UITheme::Button::DEFAULT_HEIGHT,
			"Click Me"
		);
		normal_button->SetClickCallback([this](const MouseEvent& event) {
			button_click_count_++;
			std::cout << "Button clicked! Count: " << button_click_count_ << std::endl;
			return true;
		});
		normal_button_ = normal_button.get();
		button_section_->AddChild(std::move(normal_button));
		
		// Primary button (styled with accent color)
		auto primary_button = std::make_unique<Button>(
			140.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			120.0f,
			UITheme::Button::DEFAULT_HEIGHT,
			"Primary"
		);
		primary_button->SetBackgroundColor(UITheme::Primary::NORMAL);
		primary_button->SetHoverColor(UITheme::Primary::HOVER);
		primary_button->SetClickCallback([this](const MouseEvent& event) {
			show_confirmation_ = true;
			confirm_dialog_->Show();
			return true;
		});
		primary_button_ = primary_button.get();
		button_section_->AddChild(std::move(primary_button));
		
		// Disabled button
		auto disabled_button = std::make_unique<Button>(
			280.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			120.0f,
			UITheme::Button::DEFAULT_HEIGHT,
			"Disabled"
		);
		disabled_button->SetEnabled(false);
		disabled_button_ = disabled_button.get();
		button_section_->AddChild(std::move(disabled_button));
		
		// Click counter label
		auto click_label = std::make_unique<Text>(
			420.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 8.0f,
			"Clicks: 0",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SECONDARY
		);
		button_section_->AddChild(std::move(click_label));
		
		root_panel_->AddChild(std::move(button_section_));
	}
	
	void BuildCheckboxSection(float y) {
		checkbox_section_ = std::make_unique<Panel>(
			UITheme::Padding::PANEL_HORIZONTAL,
			y,
			660.0f,
			100.0f
		);
		checkbox_section_->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		checkbox_section_->SetPadding(UITheme::Padding::MEDIUM, UITheme::Padding::MEDIUM);
		
		// Section title
		auto section_title = std::make_unique<Text>(
			0, 0,
			"Checkboxes",
			UITheme::FontSize::LARGE,
			UITheme::Text::PRIMARY
		);
		checkbox_section_->AddChild(std::move(section_title));
		
		// Checkbox 1
		auto checkbox1 = std::make_unique<Checkbox>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Enable Feature A"
		);
		checkbox1->SetCheckedCallback([](bool checked) {
			std::cout << "Feature A: " << (checked ? "Enabled" : "Disabled") << std::endl;
		});
		checkbox_1_ = checkbox1.get();
		checkbox_section_->AddChild(std::move(checkbox1));
		
		// Checkbox 2 (initially checked)
		auto checkbox2 = std::make_unique<Checkbox>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 30.0f,
			"Enable Feature B"
		);
		checkbox2->SetChecked(true);
		checkbox2->SetCheckedCallback([](bool checked) {
			std::cout << "Feature B: " << (checked ? "Enabled" : "Disabled") << std::endl;
		});
		checkbox_2_ = checkbox2.get();
		checkbox_section_->AddChild(std::move(checkbox2));
		
		// Checkbox 3
		auto checkbox3 = std::make_unique<Checkbox>(
			250.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Show Advanced Options"
		);
		checkbox3->SetCheckedCallback([](bool checked) {
			std::cout << "Advanced Options: " << (checked ? "Shown" : "Hidden") << std::endl;
		});
		checkbox_3_ = checkbox3.get();
		checkbox_section_->AddChild(std::move(checkbox3));
		
		root_panel_->AddChild(std::move(checkbox_section_));
	}
	
	void BuildSliderSection(float y) {
		slider_section_ = std::make_unique<Panel>(
			UITheme::Padding::PANEL_HORIZONTAL,
			y,
			660.0f,
			100.0f
		);
		slider_section_->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		slider_section_->SetPadding(UITheme::Padding::MEDIUM, UITheme::Padding::MEDIUM);
		
		// Section title
		auto section_title = std::make_unique<Text>(
			0, 0,
			"Sliders",
			UITheme::FontSize::LARGE,
			UITheme::Text::PRIMARY
		);
		slider_section_->AddChild(std::move(section_title));
		
		// Volume slider label
		auto volume_label = std::make_unique<Text>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Volume",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SECONDARY
		);
		slider_section_->AddChild(std::move(volume_label));
		
		// Volume slider
		auto volume_slider = std::make_unique<Slider>(
			80.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			200.0f,
			20.0f,
			0.0f, 1.0f
		);
		volume_slider->SetValue(0.5f);
		volume_slider->SetValueChangedCallback([this](float value) {
			volume_value_ = value;
			std::cout << "Volume: " << (value * 100.0f) << "%" << std::endl;
		});
		slider_volume_ = volume_slider.get();
		slider_section_->AddChild(std::move(volume_slider));
		
		// Brightness slider label
		auto brightness_label = std::make_unique<Text>(
			320.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Brightness",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SECONDARY
		);
		slider_section_->AddChild(std::move(brightness_label));
		
		// Brightness slider
		auto brightness_slider = std::make_unique<Slider>(
			420.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			200.0f,
			20.0f,
			0.0f, 1.0f
		);
		brightness_slider->SetValue(0.75f);
		brightness_slider->SetValueChangedCallback([this](float value) {
			brightness_value_ = value;
			std::cout << "Brightness: " << (value * 100.0f) << "%" << std::endl;
		});
		slider_brightness_ = brightness_slider.get();
		slider_section_->AddChild(std::move(brightness_slider));
		
		// Value display label
		auto value_label = std::make_unique<Label>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM + 30.0f,
			"Volume: 50% | Brightness: 75%",
			UITheme::FontSize::SMALL
		);
		slider_value_label_ = value_label.get();
		slider_section_->AddChild(std::move(value_label));
		
		root_panel_->AddChild(std::move(slider_section_));
	}
	
	void BuildTextSection(float y) {
		text_section_ = std::make_unique<Panel>(
			UITheme::Padding::PANEL_HORIZONTAL,
			y,
			660.0f,
			60.0f
		);
		text_section_->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		text_section_->SetPadding(UITheme::Padding::MEDIUM, UITheme::Padding::MEDIUM);
		
		// Section title
		auto section_title = std::make_unique<Text>(
			0, 0,
			"Text & Labels",
			UITheme::FontSize::LARGE,
			UITheme::Text::PRIMARY
		);
		text_section_->AddChild(std::move(section_title));
		
		// Various text styles
		auto normal_text = std::make_unique<Text>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Normal Text",
			UITheme::FontSize::NORMAL,
			UITheme::Text::PRIMARY
		);
		text_section_->AddChild(std::move(normal_text));
		
		auto secondary_text = std::make_unique<Text>(
			120.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Secondary Text",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SECONDARY
		);
		text_section_->AddChild(std::move(secondary_text));
		
		auto accent_text = std::make_unique<Text>(
			260.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Accent Text",
			UITheme::FontSize::NORMAL,
			UITheme::Text::ACCENT
		);
		text_section_->AddChild(std::move(accent_text));
		
		auto error_text = std::make_unique<Text>(
			380.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Error",
			UITheme::FontSize::NORMAL,
			UITheme::Text::ERROR
		);
		text_section_->AddChild(std::move(error_text));
		
		auto success_text = std::make_unique<Text>(
			460.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			"Success",
			UITheme::FontSize::NORMAL,
			UITheme::Text::SUCCESS
		);
		text_section_->AddChild(std::move(success_text));
		
		root_panel_->AddChild(std::move(text_section_));
	}
	
	void BuildNestedPanelSection(float y) {
		nested_panel_section_ = std::make_unique<Panel>(
			UITheme::Padding::PANEL_HORIZONTAL,
			y,
			660.0f,
			80.0f
		);
		nested_panel_section_->SetBackgroundColor(UITheme::Background::PANEL_DARK);
		nested_panel_section_->SetPadding(UITheme::Padding::MEDIUM, UITheme::Padding::MEDIUM);
		
		// Section title
		auto section_title = std::make_unique<Text>(
			0, 0,
			"Nested Panels (Composition)",
			UITheme::FontSize::LARGE,
			UITheme::Text::PRIMARY
		);
		nested_panel_section_->AddChild(std::move(section_title));
		
		// Create nested panels
		auto nested_panel_1 = std::make_unique<Panel>(
			0,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			150.0f,
			40.0f
		);
		nested_panel_1->SetBackgroundColor(UITheme::Primary::NORMAL);
		nested_panel_1->SetPadding(UITheme::Padding::SMALL, UITheme::Padding::SMALL);
		
		auto nested_label_1 = std::make_unique<Label>(
			0, 0,
			"Nested Panel 1",
			UITheme::FontSize::SMALL
		);
		nested_panel_1->AddChild(std::move(nested_label_1));
		nested_panel_section_->AddChild(std::move(nested_panel_1));
		
		auto nested_panel_2 = std::make_unique<Panel>(
			170.0f,
			UITheme::FontSize::LARGE + UITheme::Spacing::MEDIUM,
			150.0f,
			40.0f
		);
		nested_panel_2->SetBackgroundColor(Color::Alpha(UITheme::Background::BUTTON, 0.8f));
		nested_panel_2->SetPadding(UITheme::Padding::SMALL, UITheme::Padding::SMALL);
		
		auto nested_label_2 = std::make_unique<Label>(
			0, 0,
			"Nested Panel 2",
			UITheme::FontSize::SMALL
		);
		nested_panel_2->AddChild(std::move(nested_label_2));
		nested_panel_section_->AddChild(std::move(nested_panel_2));
		
		root_panel_->AddChild(std::move(nested_panel_section_));
	}
	
	void Update(float delta_time) override {
		// Update slider value labels reactively
		if (slider_value_label_) {
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "Volume: %.0f%% | Brightness: %.0f%%",
					 volume_value_ * 100.0f, brightness_value_ * 100.0f);
			slider_value_label_->SetText(buffer);
		}
		
		// Update button click count label
		if (button_section_) {
			// Find the click label (last text child)
			auto& children = button_section_->GetChildren();
			for (auto& child : children) {
				if (auto* text = dynamic_cast<Text*>(child.get())) {
					if (text->GetRelativeBounds().x > 400.0f) {
						char buffer[32];
						snprintf(buffer, sizeof(buffer), "Clicks: %d", button_click_count_);
						text->SetText(buffer);
						break;
					}
				}
			}
		}
	}
	
	void Render() override {
		using namespace engine::ui::batch_renderer;
		
		BatchRenderer::BeginFrame();
		
		// Render main UI
		if (root_panel_) {
			root_panel_->Render();
		}
		
		// Render confirmation dialog on top
		if (show_confirmation_ && confirm_dialog_) {
			confirm_dialog_->Render();
		}
		
		BatchRenderer::EndFrame();
	}
	
	void HandleInput(const engine::input::InputState& input) override {
		using namespace engine::ui;
		
		MouseEvent mouse_event{
			static_cast<float>(input.mouse_x),
			static_cast<float>(input.mouse_y),
			input.mouse_left_down,
			input.mouse_right_down,
			input.mouse_middle_down,
			input.mouse_left_pressed,
			input.mouse_right_pressed,
			input.mouse_middle_pressed,
			input.mouse_left_released,
			input.mouse_right_released,
			input.mouse_middle_released,
			input.mouse_scroll_delta
		};
		
		// Process events (confirmation dialog has priority if visible)
		bool event_handled = false;
		
		if (show_confirmation_ && confirm_dialog_) {
			event_handled = confirm_dialog_->ProcessMouseEvent(mouse_event);
		}
		
		if (!event_handled && root_panel_) {
			root_panel_->ProcessMouseEvent(mouse_event);
		}
	}
	
	void Shutdown() override {
		std::cout << "UIShowcaseScene: Shutting down..." << std::endl;
		
		// Cleanup UI
		root_panel_.reset();
		button_section_.reset();
		checkbox_section_.reset();
		slider_section_.reset();
		text_section_.reset();
		nested_panel_section_.reset();
		confirm_dialog_.reset();
		
		// Shutdown renderer
		engine::ui::batch_renderer::BatchRenderer::Shutdown();
		
		std::cout << "UIShowcaseScene: Shutdown complete" << std::endl;
	}
};

// Register scene
REGISTER_SCENE(UIShowcaseScene);
