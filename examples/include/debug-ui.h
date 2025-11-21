#pragma once

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#else
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

import engine;

class DebugUi {
private:
    bool wireframe_enabled_ = false;

public:
    void Init(GLFWwindow *window) {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    void BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            // Debug menu with wireframe toggle
            if (ImGui::BeginMenu("Debug")) {
                if (ImGui::MenuItem("Wireframe Mode", nullptr, &wireframe_enabled_)) {
#ifndef __EMSCRIPTEN__
                    // Desktop OpenGL supports polygon mode
                    if (wireframe_enabled_) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    }
#else
                    // WebGL doesn't support glPolygonMode - this is a no-op
                    // Would need alternative implementation (geometry shader or manual line drawing)
#endif
                }
                ImGui::EndMenu();
            }
            
            // Capture menu
            if (ImGui::BeginMenu("Capture")) {
                auto& capture = engine::capture::GetCaptureManager();
                
                // Screenshot submenu
                if (ImGui::BeginMenu("Screenshot")) {
                    if (ImGui::MenuItem("Take PNG Screenshot")) {
                        if (capture.Screenshot()) {
                            ImGui::OpenPopup("Screenshot Saved");
                        }
                    }
                    if (ImGui::MenuItem("Take JPEG Screenshot")) {
                        if (capture.Screenshot("screenshot", engine::capture::ImageFormat::JPEG)) {
                            ImGui::OpenPopup("Screenshot Saved");
                        }
                    }
                    if (ImGui::MenuItem("Take BMP Screenshot")) {
                        if (capture.Screenshot("screenshot", engine::capture::ImageFormat::BMP)) {
                            ImGui::OpenPopup("Screenshot Saved");
                        }
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::Separator();
                
                // GIF recording controls
                bool is_recording = capture.IsGifRecording();
                if (!is_recording) {
                    if (ImGui::MenuItem("Start GIF Recording")) {
                        capture.GifStart(30, 0.5f); // 30 FPS, half scale
                    }
                } else {
                    auto status = capture.GifGetStatus();
                    ImGui::Text("Recording: %u frames (%.2f MB)", 
                               status.frame_count, 
                               status.memory_used / 1024.0f / 1024.0f);
                    
                    if (ImGui::MenuItem("Stop & Save GIF")) {
                        capture.GifEnd();
                        if (capture.GifSave()) {
                            ImGui::OpenPopup("GIF Saved");
                        }
                    }
                    if (ImGui::MenuItem("Cancel Recording")) {
                        capture.GifCancel();
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("Output: %s", capture.GetOutputDirectory().c_str());
                
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        // Popup notifications
        if (ImGui::BeginPopupModal("Screenshot Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Screenshot saved successfully!");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopupModal("GIF Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("GIF recording saved successfully!");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
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

    bool IsWireframeEnabled() const { return wireframe_enabled_; }
};
