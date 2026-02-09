#include "example_scene.h"
#include "scene_registry.h"

#include <flecs.h>
#include <imgui.h>
#include <iostream>

import glm;
import engine;

using namespace engine::components;
using namespace engine::rendering;
using namespace engine::scene;

namespace {
constexpr float MOVE_SPEED = 3.0f;
} // namespace

class Cube3DScene : public examples::ExampleScene {
private:
	SceneId scene_id_{INVALID_SCENE};
	flecs::entity cube_entity_;
	flecs::entity light_entity_;
	glm::vec3 light_dir_{0.2f, -1.0f, -0.3f};

public:
	const char* GetName() const override { return "3D Cube"; }

	const char* GetDescription() const override { return "Basic 3D cube with input controls (perspective projection)"; }

	void Initialize(engine::Engine& engine) override {
		std::cout << "Cube3DScene: Initialize" << std::endl;

		scene_id_ = GetSceneManager().LoadSceneFromFile("assets/scenes/cube-3d.json");
		cube_entity_ = engine.ecs.FindEntityByName("Cube");
		light_entity_ = engine.ecs.FindEntityByName("Light");

		std::cout << "Cube3DScene: Initialized successfully" << std::endl;
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "Cube3DScene: Shutdown" << std::endl;
		GetSceneManager().DestroyScene(scene_id_);
	}

	void Update(engine::Engine& engine, float delta_time) override {
		using namespace engine::components;
		// Handle input for movement
		glm::vec3 movement(0.0f);

		if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::W)
			|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::UP)) {
			movement.z -= 1.0f; // Move forward (into screen)
		}
		if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::S)
			|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::DOWN)) {
			movement.z += 1.0f; // Move backward (out of screen)
		}
		if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::A)
			|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT)) {
			movement.x -= 1.0f;
		}
		if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::D)
			|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT)) {
			movement.x += 1.0f;
		}

		auto& [linear, angular] = cube_entity_.get_mut<Velocity>();

		// Normalize diagonal movement
		if (glm::length(movement) > 0.0f) {
			movement = glm::normalize(movement) * MOVE_SPEED;
			linear = movement;
		}
		else {
			linear = glm::vec3(0.0f);
		}
	}

	void Render(engine::Engine& engine) override {}

	void RenderUI(engine::Engine& engine) override {
		ImGui::Begin("3D Cube Example");

		ImGui::Text("A simple colored cube demonstration");
		ImGui::Separator();

		ImGui::Text("Controls:");
		ImGui::BulletText("W/Up Arrow: Move Forward");
		ImGui::BulletText("S/Down Arrow: Move Backward");
		ImGui::BulletText("A/Left Arrow: Move Left");
		ImGui::BulletText("D/Right Arrow: Move Right");
		ImGui::Separator();

		auto& cube_pos = cube_entity_.get_mut<Transform>();
		auto& position_ = cube_pos.position;
		auto& rotation_ = cube_pos.rotation;
		float scale_ = cube_pos.scale.x; // Uniform scale

		ImGui::Text("Transform:");
		ImGui::Text("Position: (%.1f, %.1f, %.1f)", position_.x, position_.y, position_.z);
		ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", rotation_.x, rotation_.y, rotation_.z);
		if (ImGui::SliderFloat("Scale", &scale_, 0.5f, 3.0f)) {
			cube_pos.scale = glm::vec3(scale_); // Update uniform scale
		}

		if (ImGui::Button("Reset Position")) {
			position_ = glm::vec3(0.0f, 0.0f, -5.0f);
			rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		ImGui::Separator();
		ImGui::Text("Lighting:");
		if (ImGui::SliderFloat3("Light Direction", &light_dir_.x, -1.0f, 1.0f)) {
			auto& light_comp = light_entity_.get_mut<Light>();
			light_comp.direction = light_dir_;
		}

		ImGui::End();
	}
};

// Register the scene
REGISTER_EXAMPLE_SCENE(Cube3DScene, "3D Cube", "Basic 3D cube with input controls (perspective projection)");
