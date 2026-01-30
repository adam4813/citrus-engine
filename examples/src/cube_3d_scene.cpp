#include "example_scene.h"
#include "scene_registry.h"

#include <flecs.h>
#include <imgui.h>
#include <iostream>

import glm;
import engine;

using namespace engine::components;
using namespace engine::rendering;

namespace {
constexpr float MOVE_SPEED = 3.0f;
constexpr float AUTO_ROTATE_SPEED_Y = 0.5f;
constexpr float AUTO_ROTATE_SPEED_X = 0.3f;
} // namespace

class Cube3DScene : public examples::ExampleScene {
private:
	ShaderId cube_shader_id_{INVALID_SHADER};
	//MeshId cube_mesh_id_{INVALID_MESH};
	flecs::entity cube_entity_;
	glm::vec3 light_dir_{0.2f, -1.0f, -0.3f};

public:
	const char* GetName() const override { return "3D Cube"; }

	const char* GetDescription() const override { return "Basic 3D cube with input controls (perspective projection)"; }

	void Initialize(engine::Engine& engine) override {
		std::cout << "Cube3DScene: Initialize" << std::endl;

		engine::scene::GetSceneManager().LoadSceneFromFile("assets/scenes/cube-3d.json");
		cube_shader_id_ = engine.renderer->GetShaderManager().FindShader("colored_3d");
		cube_entity_ = engine.ecs.FindEntityByName("Cube");
		std::cout << "Cube mesh ID " << cube_entity_.get_mut<Renderable>().mesh;

		std::cout << "Cube3DScene: Initialized successfully" << std::endl;
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "Cube3DScene: Shutdown" << std::endl;

		const auto renderer = engine.renderer;
		const auto& shader_manager = renderer->GetShaderManager();
		shader_manager.DestroyShader(cube_shader_id_);
		//const auto& mesh_manager = renderer->GetMeshManager();
		//mesh_manager.DestroyMesh(cube_mesh_id_);
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

		// Auto-rotate the cube for demonstration
		angular.y = AUTO_ROTATE_SPEED_Y;
		angular.x = AUTO_ROTATE_SPEED_X;

		const auto camera_pos = engine.ecs.GetActiveCamera().get<Transform>();
		const auto& shader = engine.renderer->GetShaderManager().GetShader(cube_shader_id_);
		shader.Use();
		shader.SetUniform("u_LightDir", light_dir_);
		shader.SetUniform("u_ViewPos", camera_pos.position);
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
		ImGui::SliderFloat("Scale", &scale_, 0.5f, 3.0f);
		cube_pos.scale = glm::vec3(scale_); // Update uniform scale

		if (ImGui::Button("Reset Position")) {
			position_ = glm::vec3(0.0f, 0.0f, -5.0f);
			rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		ImGui::Separator();
		ImGui::Text("Lighting:");
		ImGui::SliderFloat3("Light Direction", &light_dir_.x, -1.0f, 1.0f);

		ImGui::End();
	}
};

// Register the scene
REGISTER_EXAMPLE_SCENE(Cube3DScene, "3D Cube", "Basic 3D cube with input controls (perspective projection)");
