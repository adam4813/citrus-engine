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
    constexpr float MOVE_SPEED = 100.0f;
}

class Triangle2DScene : public examples::ExampleScene {
public:
    const char* GetName() const override {
        return "2D Triangle";
    }

    const char* GetDescription() const override {
        return "Basic 2D triangle with input controls (orthographic projection)";
    }

    void Initialize(engine::Engine& engine) override {
        std::cout << "Triangle2DScene: Initialize" << std::endl;

        // Create triangle mesh with colored vertices
        engine::rendering::MeshCreateInfo mesh_info;
        mesh_info.vertices = {
            // Position                  Normal              TexCoords  Tangent  Bitangent  Color (RGB)
            {{0.0f, 0.5f, 0.0f},     {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}, {}, {}, {1.0f, 0.0f, 0.0f, 1.0f}}, // Top (Red)
            {{-0.5f, -0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {}, {}, {0.0f, 1.0f, 0.0f, 1.0f}}, // Bottom-left (Green)
            {{0.5f, -0.5f, 0.0f},    {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {}, {}, {0.0f, 0.0f, 1.0f, 1.0f}}  // Bottom-right (Blue)
        };
        mesh_info.indices = {0, 1, 2};
        mesh_info.dynamic = false;

        triangle_mesh_ = engine.renderer->GetMeshManager().CreateMesh(mesh_info);

        // Load the colored 2D shader
        triangle_shader_ = engine.renderer->GetShaderManager().LoadShader(
            "colored_2d",
            "assets/shaders/colored_2d.vert",
            "assets/shaders/colored_2d.frag"
        );

        // Initialize transform
        position_ = glm::vec2(0.0f, 0.0f);
        rotation_ = 0.0f;
        scale_ = 50.0f; // Scale up the triangle for visibility

        std::cout << "Triangle2DScene: Initialized successfully" << std::endl;
    }

    void Shutdown(engine::Engine& engine) override {
        std::cout << "Triangle2DScene: Shutdown" << std::endl;
        
        // Clean up resources
        if (triangle_mesh_ != engine::rendering::INVALID_MESH) {
            engine.renderer->GetMeshManager().DestroyMesh(triangle_mesh_);
        }
    }

    void Update(engine::Engine& engine, float delta_time) override {
        // Handle input for movement
        glm::vec2 movement(0.0f);

        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::W) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::UP)) {
            movement.y += 1.0f;
        }
        if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::S) ||
            engine::input::Input::IsKeyPressed(engine::input::KeyCode::DOWN)) {
            movement.y -= 1.0f;
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
    }

    void Render(engine::Engine& engine) override {
        if (triangle_mesh_ == engine::rendering::INVALID_MESH || 
            triangle_shader_ == engine::rendering::INVALID_SHADER) {
            return;
        }

        // Get window size for orthographic projection
        uint32_t width, height;
        engine.renderer->GetFramebufferSize(width, height);

        // Create orthographic projection matrix (centered on screen)
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        float ortho_height = 400.0f;
        float ortho_width = ortho_height * aspect;
        
        glm::mat4 projection = glm::ortho(
            -ortho_width / 2.0f, ortho_width / 2.0f,
            -ortho_height / 2.0f, ortho_height / 2.0f,
            -1.0f, 1.0f
        );

        // Create model transform
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(position_, 0.0f));
        model = glm::rotate(model, rotation_, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(scale_, scale_, 1.0f));

        // Calculate MVP matrix
        glm::mat4 mvp = projection * model;

        // Bind shader and set uniforms
        auto& shader = engine.renderer->GetShaderManager().GetShader(triangle_shader_);
        shader.Use();
        shader.SetUniform("u_MVP", mvp);

        // Bind mesh and render
        auto* gl_mesh = engine::rendering::GetGLMesh(triangle_mesh_);
        if (gl_mesh && gl_mesh->vao != 0) {
            glBindVertexArray(gl_mesh->vao);
            glDrawElements(GL_TRIANGLES, gl_mesh->index_count, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
    }

    void RenderUI(engine::Engine& engine) override {
        ImGui::Begin("2D Triangle Example");
        
        ImGui::Text("A simple colored triangle demonstration");
        ImGui::Separator();
        
        ImGui::Text("Controls:");
        ImGui::BulletText("W/Up Arrow: Move Up");
        ImGui::BulletText("S/Down Arrow: Move Down");
        ImGui::BulletText("A/Left Arrow: Move Left");
        ImGui::BulletText("D/Right Arrow: Move Right");
        ImGui::Text("(Rotation can be adjusted via slider below)");
        ImGui::Separator();
        
        ImGui::Text("Transform:");
        ImGui::Text("Position: (%.1f, %.1f)", position_.x, position_.y);
        ImGui::SliderFloat("Rotation", &rotation_, -3.14f, 3.14f);
        ImGui::SliderFloat("Scale", &scale_, 10.0f, 200.0f);
        
        if (ImGui::Button("Reset Position")) {
            position_ = glm::vec2(0.0f, 0.0f);
            rotation_ = 0.0f;
        }
        
        ImGui::End();
    }

private:
    engine::rendering::MeshId triangle_mesh_ = engine::rendering::INVALID_MESH;
    engine::rendering::ShaderId triangle_shader_ = engine::rendering::INVALID_SHADER;
    glm::vec2 position_{0.0f, 0.0f};
    float rotation_ = 0.0f;
    float scale_ = 50.0f;
};

// Register the scene
REGISTER_EXAMPLE_SCENE(Triangle2DScene, "2D Triangle", "Basic 2D triangle with input controls (orthographic projection)");
