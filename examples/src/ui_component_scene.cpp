#include "example_scene.h"
#include "scene_registry.h"
#include "ui_debug_visualizer.h"
#include "ui_element_inspector.h"

#include <imgui.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

import glm;
import engine;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::components;
using namespace engine::ui::batch_renderer;
using namespace engine::input;

/**
 * @brief Demonstrates the UI Component System
 *
 * This scene shows:
 * - Container element with component support
 * - Layout strategies (vertical, horizontal, grid, center, justify, stack)
 * - Constraint system (anchors and size constraints)
 * - Scroll component
 * - ContainerBuilder fluent API
 *
 * ImGui controls allow real-time manipulation of components.
 */
class UIComponentScene : public examples::ExampleScene {
private:
	// Main demonstration container
	std::unique_ptr<Container> demo_container_;

	// Child panels for layout demonstration
	std::vector<std::unique_ptr<Panel>> child_panels_;

	// Debug visualizer
	UIDebugVisualizer ui_debugger_;

	// Element inspector
	UIElementInspector inspector_;

	// Currently selected element for inspector
	UIElement* selected_element_ = nullptr;

	// State for ImGui controls
	int layout_type_ = 0; // 0=Vertical, 1=Horizontal, 2=Grid, 3=Stack, 4=Justify
	float layout_gap_ = 8.0f;
	int alignment_ = 1; // 0=Start, 1=Center, 2=End, 3=Stretch
	int stack_h_align_ = 1; // Horizontal alignment for Stack layout
	int stack_v_align_ = 1; // Vertical alignment for Stack layout
	int grid_columns_ = 3;

	// Constraint settings
	bool use_constraints_ = false;
	int anchor_preset_ = 0; // 0=None, 1=TopLeft, 2=Center, 3=Fill, 4=StretchH, 5=StretchV
	float anchor_margin_ = 20.0f;

	// Scroll settings
	bool enable_scroll_ = false;
	int scroll_direction_ = 0; // 0=Vertical, 1=Horizontal, 2=Both
	float content_height_ = 600.0f;

	// Container settings
	float container_x_ = 50.0f;
	float container_y_ = 50.0f;
	float container_width_ = 400.0f;
	float container_height_ = 400.0f;
	float container_padding_ = 10.0f;

	// Child panel settings
	int child_count_ = 5;
	float child_width_ = 80.0f;
	float child_height_ = 60.0f;

	// Parent panel for constraint demo
	std::unique_ptr<Panel> parent_panel_;

public:
	const char* GetName() const override { return "UI Components"; }

	const char* GetDescription() const override {
		return "Demonstrates layout, constraint, and scroll components with interactive controls";
	}

	void Initialize(engine::Engine& engine) override {
		std::cout << "UIComponentScene: Initializing..." << std::endl;

		text_renderer::FontManager::Initialize("fonts/Kenney Future.ttf", 16);
		BatchRenderer::Initialize();

		// Create parent panel for constraint demonstrations
		parent_panel_ = std::make_unique<Panel>(20, 20, 800, 600);
		parent_panel_->SetBackgroundColor(Color{0.15f, 0.15f, 0.15f, 0.5f});
		parent_panel_->SetBorderColor(UITheme::Border::DEFAULT);
		parent_panel_->SetBorderWidth(1.0f);

		RebuildUI();

		std::cout << "UIComponentScene: Initialized" << std::endl;
	}

	void RebuildUI() {
		// Clear existing
		demo_container_.reset();
		child_panels_.clear();

		// Build container using builder
		auto& builder = ContainerBuilder()
								.Position(container_x_, container_y_)
								.Size(container_width_, container_height_)
								.Padding(container_padding_)
								.Background(UITheme::Background::PANEL)
								.Border(2.0f, UITheme::Border::FOCUS)
								.ClipChildren(true);

		// Add layout component based on selection
		Alignment align = static_cast<Alignment>(alignment_);

		switch (layout_type_) {
		case 0: // Vertical
			builder.Layout<VerticalLayout>(layout_gap_, align);
			break;
		case 1: // Horizontal
			builder.Layout<HorizontalLayout>(layout_gap_, align);
			break;
		case 2: // Grid
			builder.Layout<GridLayout>(grid_columns_, layout_gap_, layout_gap_);
			break;
		case 3: // Stack
			builder.Layout<StackLayout>(static_cast<Alignment>(stack_h_align_), static_cast<Alignment>(stack_v_align_));
			break;
		case 4: // Justify
			builder.Layout<JustifyLayout>(JustifyDirection::Horizontal, align);
			break;
		}

		// Add scroll if enabled
		if (enable_scroll_) {
			ScrollDirection dir = static_cast<ScrollDirection>(scroll_direction_);
			builder.Scrollable(dir);
		}

		demo_container_ = builder.Build();

		// Apply constraints if using them
		if (use_constraints_ && anchor_preset_ > 0) {
			Anchor anchor;
			switch (anchor_preset_) {
			case 1: anchor = Anchor::TopLeft(anchor_margin_); break;
			case 2:
			{
				// Center - manually set position
				float parent_w = parent_panel_->GetWidth();
				float parent_h = parent_panel_->GetHeight();
				demo_container_->SetRelativePosition(
						(parent_w - container_width_) / 2, (parent_h - container_height_) / 2);
				break;
			}
			case 3: anchor = Anchor::Fill(anchor_margin_); break;
			case 4:
				anchor = Anchor::StretchHorizontal(anchor_margin_, anchor_margin_);
				anchor.SetTop(anchor_margin_);
				break;
			case 5:
				anchor = Anchor::StretchVertical(anchor_margin_, anchor_margin_);
				anchor.SetLeft(anchor_margin_);
				break;
			}

			if (anchor_preset_ != 2) {
				demo_container_->AddComponent<ConstraintComponent>(anchor);
			}
		}

		// Add as child of parent panel if using constraints
		if (use_constraints_) {
			parent_panel_->AddChild(std::move(demo_container_));
			// Get pointer back for adding children
			demo_container_.reset();
		}

		// Create child panels
		CreateChildPanels();

		// Update layout
		if (use_constraints_) {
			// Container is now owned by parent_panel_, find it
			if (!parent_panel_->GetChildren().empty()) {
				auto* container = static_cast<Container*>(parent_panel_->GetChildren().back().get());
				container->Update();

				// Set content size for scroll
				if (enable_scroll_) {
					auto* scroll = container->GetComponent<ScrollComponent>();
					if (scroll) {
						scroll->CalculateContentSizeFromChildren();
					}
				}
			}
			// Setup click-to-select on parent panel hierarchy
			ui_debugger_.SetupClickToSelect(parent_panel_.get());
		}
		else if (demo_container_) {
			demo_container_->Update();

			if (enable_scroll_) {
				auto* scroll = demo_container_->GetComponent<ScrollComponent>();
				if (scroll) {
					scroll->CalculateContentSizeFromChildren();
				}
			}
			// Setup click-to-select on demo container hierarchy
			ui_debugger_.SetupClickToSelect(demo_container_.get());
		}
	}

	void CreateChildPanels() {
		Container* target = nullptr;

		if (use_constraints_ && !parent_panel_->GetChildren().empty()) {
			target = static_cast<Container*>(parent_panel_->GetChildren().back().get());
		}
		else if (demo_container_) {
			target = demo_container_.get();
		}

		if (!target)
			return;

		// Create colored child panels with varying sizes for Stack layout
		const Color colors[] = {
				{0.8f, 0.2f, 0.2f, 0.9f}, // Red
				{0.2f, 0.8f, 0.2f, 0.9f}, // Green
				{0.2f, 0.2f, 0.8f, 0.9f}, // Blue
				{0.8f, 0.8f, 0.2f, 0.9f}, // Yellow
				{0.8f, 0.2f, 0.8f, 0.9f}, // Magenta
				{0.2f, 0.8f, 0.8f, 0.9f}, // Cyan
				{0.8f, 0.5f, 0.2f, 0.9f}, // Orange
				{0.5f, 0.2f, 0.8f, 0.9f}, // Purple
		};

		// For Stack layout, use decreasing sizes to visualize layering
		const bool is_stack = (layout_type_ == 3);

		for (int i = 0; i < child_count_; i++) {
			float w = child_width_;
			float h = child_height_;

			if (is_stack) {
				// Decreasing sizes: largest first (background), smallest last (foreground)
				float scale = 1.0f - (static_cast<float>(i) * 0.15f);
				w = child_width_ * scale;
				h = child_height_ * scale;
			}

			auto panel = std::make_unique<Panel>(0, 0, w, h);
			panel->SetBackgroundColor(colors[i % 8]);
			panel->SetBorderColor(Colors::WHITE);
			panel->SetBorderWidth(1.0f);

			// Add a label
			auto label = std::make_unique<Text>(5, 5, std::to_string(i + 1), 14, Colors::WHITE);
			panel->AddChild(std::move(label));

			// Add a nested button to the first panel for constraint testing
			// This button is NOT managed by the parent layout, so constraints work independently
			if (i == 0) {
				auto nested_btn = std::make_unique<Button>(0, 0, 30, 20, "X");
				nested_btn->SetNormalColor(Color{0.9f, 0.3f, 0.3f, 1.0f});
				// Add constraint to anchor to top-right corner
				nested_btn->AddComponent<ConstraintComponent>(Anchor::TopRight(5.0f));
				panel->AddChild(std::move(nested_btn));
			}

			target->AddChild(std::move(panel));
		}
	}

	void Update(engine::Engine& engine, float delta_time) override {
		// Handle mouse events for scrolling and click-to-select
		const MouseEvent event{Input::GetMouseState()};

		if (use_constraints_) {
			if (event.left_pressed || event.scroll_delta != 0.0f) {
				parent_panel_->ProcessMouseEvent(event);
			}
			// Update all components recursively (including constraints on nested elements)
			parent_panel_->UpdateComponentsRecursive(delta_time);
		}
		else if (demo_container_) {
			if (event.left_pressed || event.scroll_delta != 0.0f) {
				demo_container_->ProcessMouseEvent(event);
			}
			// Update all components recursively (including constraints on nested elements)
			demo_container_->UpdateComponentsRecursive(delta_time);
		}

		// Sync selection from debug visualizer
		selected_element_ = const_cast<UIElement*>(ui_debugger_.GetSelectedElement());
	}

	void Render(engine::Engine& engine) override {
		BatchRenderer::BeginFrame();

		if (use_constraints_) {
			parent_panel_->Render();
			ui_debugger_.RenderDebugOverlay(parent_panel_.get());
		}
		else if (demo_container_) {
			demo_container_->Render();
			ui_debugger_.RenderDebugOverlay(demo_container_.get());
		}

		BatchRenderer::EndFrame();
	}

	void RenderUI(engine::Engine& engine) override {
		ImGui::Begin("Component Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		bool needs_rebuild = false;

		// Layout section
		if (ImGui::CollapsingHeader("Layout Component", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* layout_names[] = {"Vertical", "Horizontal", "Grid", "Stack", "Justify"};
			if (ImGui::Combo("Layout Type", &layout_type_, layout_names, IM_ARRAYSIZE(layout_names))) {
				needs_rebuild = true;
			}

			if (layout_type_ != 3 && layout_type_ != 4) { // Not Stack or Justify
				if (ImGui::SliderFloat("Gap", &layout_gap_, 0.0f, 30.0f)) {
					needs_rebuild = true;
				}
			}

			if (layout_type_ != 3 && layout_type_ != 2) { // Not Stack or Grid
				const char* align_names[] = {"Start", "Center", "End", "Stretch"};
				if (ImGui::Combo("Alignment", &alignment_, align_names, IM_ARRAYSIZE(align_names))) {
					needs_rebuild = true;
				}
			}

			if (layout_type_ == 3) { // Stack - separate H/V alignment
				const char* align_names[] = {"Start", "Center", "End", "Stretch"};
				if (ImGui::Combo("H Align", &stack_h_align_, align_names, IM_ARRAYSIZE(align_names))) {
					needs_rebuild = true;
				}
				if (ImGui::Combo("V Align", &stack_v_align_, align_names, IM_ARRAYSIZE(align_names))) {
					needs_rebuild = true;
				}
			}

			if (layout_type_ == 2) { // Grid
				if (ImGui::SliderInt("Columns", &grid_columns_, 1, 6)) {
					needs_rebuild = true;
				}
			}
		}

		// Constraint section
		if (ImGui::CollapsingHeader("Constraint Component")) {
			if (ImGui::Checkbox("Use Constraints", &use_constraints_)) {
				needs_rebuild = true;
			}

			if (use_constraints_) {
				const char* anchor_names[] = {"None", "TopLeft", "Center", "Fill", "StretchH", "StretchV"};
				if (ImGui::Combo("Anchor Preset", &anchor_preset_, anchor_names, IM_ARRAYSIZE(anchor_names))) {
					needs_rebuild = true;
				}

				if (anchor_preset_ > 0) {
					if (ImGui::SliderFloat("Margin", &anchor_margin_, 0.0f, 100.0f)) {
						needs_rebuild = true;
					}
				}
			}
		}

		// Scroll section
		if (ImGui::CollapsingHeader("Scroll Component")) {
			if (ImGui::Checkbox("Enable Scroll", &enable_scroll_)) {
				needs_rebuild = true;
			}

			if (enable_scroll_) {
				const char* dir_names[] = {"Vertical", "Horizontal", "Both"};
				if (ImGui::Combo("Direction", &scroll_direction_, dir_names, IM_ARRAYSIZE(dir_names))) {
					needs_rebuild = true;
				}

				ImGui::Text("Scroll with mouse wheel when hovering");
			}
		}

		// Container settings
		if (ImGui::CollapsingHeader("Container Settings")) {
			if (!use_constraints_) {
				if (ImGui::SliderFloat("X", &container_x_, 0.0f, 400.0f)) {
					needs_rebuild = true;
				}
				if (ImGui::SliderFloat("Y", &container_y_, 0.0f, 300.0f)) {
					needs_rebuild = true;
				}
			}
			ImGui::PushID("container_width");
			if (ImGui::SliderFloat("Width", &container_width_, 100.0f, 700.0f)) {
				needs_rebuild = true;
			}
			ImGui::PopID();
			ImGui::PushID("container_height");
			if (ImGui::SliderFloat("Height", &container_height_, 100.0f, 600.0f)) {
				needs_rebuild = true;
			}
			ImGui::PopID();
			if (ImGui::SliderFloat("Padding", &container_padding_, 0.0f, 30.0f)) {
				needs_rebuild = true;
			}
		}

		// Child settings
		if (ImGui::CollapsingHeader("Child Panels")) {
			if (ImGui::SliderInt("Count", &child_count_, 1, 12)) {
				needs_rebuild = true;
			}
			if (ImGui::SliderFloat("Width", &child_width_, 30.0f, 150.0f)) {
				needs_rebuild = true;
			}
			if (ImGui::SliderFloat("Height", &child_height_, 30.0f, 100.0f)) {
				needs_rebuild = true;
			}
		}

		ImGui::Separator();

		if (ImGui::Button("Rebuild UI")) {
			needs_rebuild = true;
		}

		if (needs_rebuild) {
			// Clear parent panel children first
			while (!parent_panel_->GetChildren().empty()) {
				parent_panel_->RemoveChild(parent_panel_->GetChildren().front().get());
			}
			RebuildUI();
		}

		ImGui::Separator();
		ImGui::TextWrapped(
				"This demo shows how layout, constraint, and scroll components "
				"can be combined to create flexible UI layouts.");

		ImGui::Separator();
		ImGui::Text("Debug Visualizer:");
		ui_debugger_.RenderImGuiControls();

		ImGui::End();

		// Element Inspector window
		ImGui::Begin("Element Inspector");

		// Note about click-to-select
		ImGui::TextDisabled("Click on any element to select it");

		ImGui::Separator();

		// Sync selection with debug visualizer
		ui_debugger_.SetSelectedElement(selected_element_);

		// Get target container for update callback
		Container* target_container = nullptr;
		if (use_constraints_ && !parent_panel_->GetChildren().empty()) {
			target_container = static_cast<Container*>(parent_panel_->GetChildren().back().get());
		}
		else if (demo_container_) {
			target_container = demo_container_.get();
		}

		// Render inspector for selected element
		if (inspector_.Render(selected_element_)) {
			// Element was modified, trigger update
			if (target_container) {
				target_container->InvalidateComponents();
			}
		}

		ImGui::End();
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "UIComponentScene: Shutting down..." << std::endl;

		demo_container_.reset();
		parent_panel_.reset();
		child_panels_.clear();

		BatchRenderer::Shutdown();
		text_renderer::FontManager::Shutdown();

		std::cout << "UIComponentScene: Shutdown complete" << std::endl;
	}
};

REGISTER_EXAMPLE_SCENE(
		UIComponentScene,
		"UI Components",
		"Demonstrates layout, constraint, and scroll components with interactive controls");
