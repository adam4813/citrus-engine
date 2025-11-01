#include "example_scene.h"
#include "scene_registry.h"

import engine;
import glm;

#include <imgui.h>
#include <iostream>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

namespace {
    constexpr float MOVE_SPEED = 3.0f;
    constexpr float AUTO_ROTATE_SPEED_Y = 0.5f;
    constexpr float AUTO_ROTATE_SPEED_X = 0.3f;
}

class Cube3DScene : public examples::ExampleScene {
public:
    const char* GetName() const override {
        return "3D Cube";
    }

    const char* GetDescription() const override {
        return "Basic 3D cube with input controls (perspective projection)";
    }

    void Initialize(engine::Engine& engine) override {
        std::cout << "Cube3DScene: Initialize" << std::endl;

        // Create cube mesh using MeshManager
        cube_mesh_ = engine.renderer->GetMeshManager().CreateCube(1.0f);

        // Load the colored 3D shader
        cube_shader_ = engine.renderer->GetShaderManager().LoadShader(
            "colored_3d",
            "assets/shaders/colored_3d.vert",
            "assets/shaders/colored_3d.frag"
        );

        // Initialize transform
        position_ = glm::vec3(0.0f, 0.0f, -5.0f);
        rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
        scale_ = 1.0f;

        // Camera setup
        camera_position_ = glm::vec3(0.0f, 0.0f, 0.0f);
        
        // Light direction
        light_dir_ = glm::vec3(0.2f, -1.0f, -0.3f);

        std::cout << "Cube3DScene: Initialized successfully" << std::endl;
    }

    void Shutdown(engine::Engine& engine) override {
        std::cout << "Cube3DScene: Shutdown" << std::endl;
        
        // Clean up resources
        if (cube_mesh_ != engine::rendering::INVALID_MESH) {
            engine.renderer->GetMeshManager().DestroyMesh(cube_mesh_);
        }
    }

    void Update(engine::Engine& engine, float delta_time) override {
        // Handle input for movement
        glm::vec3 movement(0.0f);

        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::W) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::UP)) {
            movement.z -= 1.0f; // Move forward (into screen)
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::S) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::DOWN)) {
            movement.z += 1.0f; // Move backward (out of screen)
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::A) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT)) {
            movement.x -= 1.0f;
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::D) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT)) {
            movement.x += 1.0f;
        }

        // Normalize diagonal movement
        if (glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement) * MOVE_SPEED * delta_time;
            position_ += movement;
        }

        // Auto-rotate the cube for demonstration
        rotation_.y += AUTO_ROTATE_SPEED_Y * delta_time;
        rotation_.x += AUTO_ROTATE_SPEED_X * delta_time;
    }

    void Render(engine::Engine& engine) override {
        if (cube_mesh_ == engine::rendering::INVALID_MESH || 
            cube_shader_ == engine::rendering::INVALID_SHADER) {
            return;
        }

        // Get window size for perspective projection
        uint32_t width, height;
        engine.renderer->GetFramebufferSize(width, height);

        // Create perspective projection matrix
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

        // Create view matrix
        glm::mat4 view = glm::lookAt(
            camera_position_,
            camera_position_ + glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Create model transform
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position_);
        model = glm::rotate(model, rotation_.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation_.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation_.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(scale_));

        // Calculate MVP matrix
        glm::mat4 mvp = projection * view * model;

        // Bind shader and set uniforms
        auto& shader = engine.renderer->GetShaderManager().GetShader(cube_shader_);
        shader.Use();
        shader.SetUniform("u_MVP", mvp);
        shader.SetUniform("u_Model", model);
        shader.SetUniform("u_LightDir", light_dir_);
        shader.SetUniform("u_ViewPos", camera_position_);

        // Enable depth test for 3D rendering
        glEnable(GL_DEPTH_TEST);

        // Bind mesh and render
        auto* gl_mesh = engine::rendering::GetGLMesh(cube_mesh_);
        if (gl_mesh && gl_mesh->vao != 0) {
            glBindVertexArray(gl_mesh->vao);
            glDrawElements(GL_TRIANGLES, gl_mesh->index_count, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glDisable(GL_DEPTH_TEST);
    }

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
        
        ImGui::Text("Transform:");
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", position_.x, position_.y, position_.z);
        ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", rotation_.x, rotation_.y, rotation_.z);
        ImGui::SliderFloat("Scale", &scale_, 0.5f, 3.0f);
        
        if (ImGui::Button("Reset Position")) {
            position_ = glm::vec3(0.0f, 0.0f, -5.0f);
            rotation_ = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        ImGui::Separator();
        ImGui::Text("Lighting:");
        ImGui::SliderFloat3("Light Direction", &light_dir_.x, -1.0f, 1.0f);
        
        ImGui::End();
    }

private:
    engine::rendering::MeshId cube_mesh_ = engine::rendering::INVALID_MESH;
    engine::rendering::ShaderId cube_shader_ = engine::rendering::INVALID_SHADER;
    glm::vec3 position_{0.0f, 0.0f, -5.0f};
    glm::vec3 rotation_{0.0f, 0.0f, 0.0f};
    float scale_ = 1.0f;
    glm::vec3 camera_position_{0.0f, 0.0f, 0.0f};
    glm::vec3 light_dir_{0.2f, -1.0f, -0.3f};
};

// Register the scene
REGISTER_EXAMPLE_SCENE(Cube3DScene, "3D Cube", "Basic 3D cube with input controls (perspective projection)");
