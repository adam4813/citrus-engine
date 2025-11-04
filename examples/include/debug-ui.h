#pragma once

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>

class DebugUi {
public:
    void Init(GLFWwindow *window) {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Example: Load custom fonts using AssetManager (currently disabled)
        // To load custom fonts, uncomment the following code and ensure font files exist in assets/fonts/
        /*
        import engine.assets;
        
        // Load font data using AssetManager instead of direct file access
        std::vector<uint8_t> font_data = engine::assets::AssetManager::LoadBinaryFile("fonts/Roboto-Regular.ttf");
        if (!font_data.empty()) {
            // Store font data to keep it alive (ImGui doesn't copy the data)
            font_buffers_.push_back(std::move(font_data));
            
            // Add font from memory - ImGui will use the pointer, so data must remain valid
            io.Fonts->AddFontFromMemoryTTF(
                font_buffers_.back().data(),
                static_cast<int>(font_buffers_.back().size()),
                16.0f
            );
            
            // Build font atlas
            io.Fonts->Build();
        }
        */

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

private:
    // Font data must be kept alive for ImGui's lifetime
    // ImGui's AddFontFromMemoryTTF doesn't copy the data, so we store it here
    std::vector<std::vector<uint8_t>> font_buffers_;
};
