#include "example_scene.h"
#include "scene_registry.h"
#include "ui_debug_visualizer.h"

#include <iostream>
#include <memory>
#include <string>

#include <imgui.h>

import glm;
import engine;

/**
 * @brief Example demonstrating the Image UI element
 *
 * Shows how to use the Image class to render sprites within the UI system.
 * Demonstrates:
 * - Creating Image elements with bounds
 * - Loading and setting sprites
 * - Composing Images with other UI elements (children)
 * - Reactive updates (changing sprites)
 */
class UIImageScene : public examples::ExampleScene {
private:
	// UI System
	std::unique_ptr<engine::ui::elements::Image> logo_image_;

	// Sprite data
	std::shared_ptr<engine::rendering::Sprite> logo_sprite_;
	std::shared_ptr<engine::rendering::Sprite> icon_sprite_;

	// Texture IDs (would normally be loaded from asset manager)
	uint32_t logo_texture_id_ = 0;
	uint32_t icon_texture_id_ = 0;

	// Interactive demo state
	float image_x_ = 100.0f;
	float image_y_ = 100.0f;
	float image_width_ = 256.0f;
	float image_height_ = 256.0f;
	bool show_logo_ = true;

	// Debug visualizer
	UIDebugVisualizer ui_debugger_;

public:
	const char* GetName() const override { return "UI Image Element"; }

	const char* GetDescription() const override { return "Demonstrates the Image UI element for rendering sprites"; }

	void Initialize(engine::Engine& engine) override {
		std::cout << "UIImageScene: Initializing..." << std::endl;

		// Initialize UI systems
		engine::ui::text_renderer::FontManager::Initialize("fonts/Kenney Future.ttf", 16);
		engine::ui::batch_renderer::BatchRenderer::Initialize();

		// For this example, we'll use the font texture as our "image"
		// In a real application, you would load actual image textures
		auto* default_font = engine::ui::text_renderer::FontManager::GetDefaultFont();
		logo_texture_id_ = default_font->GetTextureId();
		icon_texture_id_ = logo_texture_id_; // Same texture for demo

		// Create sprite data (declarative - set once)
		logo_sprite_ = std::make_shared<engine::rendering::Sprite>();
		logo_sprite_->texture = logo_texture_id_;
		logo_sprite_->color = engine::rendering::Color{1.0f, 1.0f, 1.0f, 1.0f}; // White
		logo_sprite_->texture_offset = glm::vec2(0.0f, 0.0f);
		logo_sprite_->texture_scale = glm::vec2(1.0f, 1.0f);
		logo_sprite_->layer = 0;

		icon_sprite_ = std::make_shared<engine::rendering::Sprite>();
		icon_sprite_->texture = icon_texture_id_;
		icon_sprite_->color = engine::rendering::Color{0.2f, 0.8f, 1.0f, 1.0f}; // Cyan tint
		icon_sprite_->texture_offset = glm::vec2(0.0f, 0.0f);
		icon_sprite_->texture_scale = glm::vec2(0.5f, 0.5f); // Smaller UV coords
		icon_sprite_->layer = 1;

		// Create Image UI elements (declarative - build once)
		logo_image_ = std::make_unique<engine::ui::elements::Image>(image_x_, image_y_, image_width_, image_height_);
		logo_image_->SetSprite(logo_sprite_);

		// Create a child icon in the corner of the logo
		std::unique_ptr<engine::ui::elements::Image> icon_image_ = std::make_unique<engine::ui::elements::Image>(
				image_width_ - 64.0f, // Position relative to parent
				0.0f,
				64.0f,
				64.0f);
		icon_image_->SetSprite(icon_sprite_);

		// Add icon as child of logo (demonstrates composition)
		logo_image_->AddChild(std::move(icon_image_));

		// Create text label as another child
		std::unique_ptr<engine::ui::elements::Text> label_text_ = std::make_unique<engine::ui::elements::Text>(
				10.0f, // Relative to logo image
				image_height_ + 10.0f,
				"Logo Image",
				16.0f,
				engine::ui::batch_renderer::Colors::WHITE);

		// Add text as child (will move with the image)
		logo_image_->AddChild(std::move(label_text_));

		std::cout << "UIImageScene: Initialized successfully" << std::endl;
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "UIImageScene: Shutting down..." << std::endl;

		// Clean up (RAII handles destruction)
		logo_image_.reset();
		logo_sprite_.reset();
		icon_sprite_.reset();

		// Shutdown UI systems
		engine::ui::batch_renderer::BatchRenderer::Shutdown();
		engine::ui::text_renderer::FontManager::Shutdown();

		std::cout << "UIImageScene: Shutdown complete" << std::endl;
	}

	void Update(engine::Engine& engine, float delta_time) override {
		// Reactive updates - only change when needed, not every frame

		// Update image position if changed via UI
		logo_image_->SetRelativePosition(image_x_, image_y_);
		logo_image_->SetSize(image_width_, image_height_);

		// Toggle sprite visibility reactively
		logo_image_->SetVisible(show_logo_);
	}

	void Render(engine::Engine& engine) override {
		using namespace engine::ui::batch_renderer;

		// Begin UI rendering
		BatchRenderer::BeginFrame();

		// Draw background panel to show Image positioning
		BatchRenderer::SubmitQuad(Rectangle{50, 50, 400, 400}, Color::Alpha(Colors::DARK_GRAY, 0.5f));

		// Render the Image element (and all its children)
		// This is the key: Image handles sprite submission + child rendering
		if (logo_image_) {
			logo_image_->Render();
		}

		// Draw UI instructions
		BatchRenderer::SubmitText("Image Element Demo", 10.0f, 10.0f, 24, Colors::GOLD);

		BatchRenderer::SubmitText(
				"The image has two children: an icon and a text label", 10.0f, 40.0f, 16, Colors::WHITE);

		BatchRenderer::SubmitText(
				"Children move with the parent image (relative positioning)", 10.0f, 60.0f, 16, Colors::LIGHT_GRAY);

		// Render debug overlay (after normal UI, so it appears on top)
		if (logo_image_) {
			ui_debugger_.RenderDebugOverlay(logo_image_.get());
		}

		// End UI rendering
		BatchRenderer::EndFrame();
	}

	void RenderUI(engine::Engine& engine) override {
		ImGui::Begin("UI Image Example");

		ImGui::Text("Image Element Demonstration");
		ImGui::Separator();

		ImGui::Text("Key Concepts:");
		ImGui::BulletText("Declarative UI: Create once, render many times");
		ImGui::BulletText("Reactive updates: Change properties when needed");
		ImGui::BulletText("Composition: Images can have children (text, other images)");
		ImGui::BulletText("Coordinate system: Children use relative positioning");

		ImGui::Separator();
		ImGui::Text("Controls:");

		// Interactive controls for demo
		ImGui::Checkbox("Show Logo", &show_logo_);

		ImGui::SliderFloat("X Position", &image_x_, 50.0f, 600.0f);
		ImGui::SliderFloat("Y Position", &image_y_, 50.0f, 400.0f);
		ImGui::SliderFloat("Width", &image_width_, 64.0f, 512.0f);
		ImGui::SliderFloat("Height", &image_height_, 64.0f, 512.0f);

		if (ImGui::Button("Reset")) {
			image_x_ = 100.0f;
			image_y_ = 100.0f;
			image_width_ = 256.0f;
			image_height_ = 256.0f;
			show_logo_ = true;
		}

		ImGui::Separator();
		ImGui::Text("Code Example:");
		ImGui::TextWrapped(
				"auto image = std::make_unique<Image>(x, y, w, h);\n"
				"image->SetSprite(sprite);\n"
				"image->AddChild(std::move(child));\n"
				"image->Render();  // Renders sprite + children");

		ImGui::Separator();
		ImGui::Text("Debug Visualizer:");
		ui_debugger_.RenderImGuiControls();

		ImGui::End();
	}
};

// Register the scene
REGISTER_EXAMPLE_SCENE(UIImageScene, "UI Image Element", "Demonstrates the Image UI element for rendering sprites");
