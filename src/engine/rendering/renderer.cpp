// Rendering renderer implementation stub
module;

#include <memory>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import :mesh;
import engine.components;

namespace engine::rendering {
    // Renderer implementation
    struct Renderer::Impl {
        bool initialized = false;
        //Camera camera;
        Color clear_color = colors::black;
        uint32_t window_width = 800;
        uint32_t window_height = 600;

        // Resource managers
        TextureManager texture_manager;
        ShaderManager shader_manager;
        MeshManager mesh_manager;
        MaterialManager material_manager;

        // Statistics
        uint32_t draw_call_count = 0;
        uint32_t triangle_count = 0;
    };

    Renderer::Renderer() : pimpl_(std::make_unique<Impl>()) {
    }

    Renderer::~Renderer() = default;

    bool Renderer::Initialize(const uint32_t window_width, const uint32_t window_height) const {
        pimpl_->window_width = window_width;
        pimpl_->window_height = window_height;

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Enable face culling (optional, but recommended for performance)
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // Set viewport
        glViewport(0, 0, window_width, window_height);

        pimpl_->initialized = true;
        return true;
    }

    void Renderer::Shutdown() const {
        pimpl_->initialized = false;
    }

    void Renderer::BeginFrame() const {
        pimpl_->draw_call_count = 0;
        pimpl_->triangle_count = 0;
        glClearColor(pimpl_->clear_color.r, pimpl_->clear_color.g, pimpl_->clear_color.b, pimpl_->clear_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::EndFrame() {
        // TODO: Implement frame end logic
    }

    void Renderer::SubmitRenderCommand(const RenderCommand &command) const {
        // Look up mesh OpenGL handles
        const GLMesh *gl_mesh = GetGLMesh(command.mesh);
        if (!gl_mesh) {
            return;
        }

        const Material &material = pimpl_->material_manager.GetMaterial(command.material);

        // Look up shader
        const Shader &shader = pimpl_->shader_manager.GetShader(material.GetShader());
        if (!shader.IsValid()) {
            return;
        }

        shader.Use();
        shader.SetUniform("u_MVP", command.transform);
        material.Apply(shader);

        glBindVertexArray(gl_mesh->vao);
        glDrawElements(GL_TRIANGLES, gl_mesh->index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        pimpl_->draw_call_count++;
        pimpl_->triangle_count += gl_mesh->index_count / 3;
    }

    void Renderer::SubmitSprite(const SpriteRenderCommand &command) const {
        // TODO: Implement sprite rendering
        pimpl_->draw_call_count++;
    }

    void Renderer::DrawLine(const Vec3 &start, const Vec3 &end, const Color &color) {
        // TODO: Implement debug line drawing
    }

    void Renderer::DrawWireCube(const Vec3 &center, const Vec3 &size, const Color &color) {
        // TODO: Implement debug cube drawing
    }

    void Renderer::DrawWireSphere(const Vec3 &center, float radius, const Color &color) {
        // TODO: Implement debug sphere drawing
    }

    void Renderer::SetCamera(const components::Camera &camera) {
        //pimpl_->camera = camera;
    }

    /*const Camera& Renderer::GetCamera() const {
        return pimpl_->camera;
    }*/

    void Renderer::SetClearColor(const Color &color) const {
        pimpl_->clear_color = color;
    }

    void Renderer::SetViewport(int x, int y, uint32_t width, uint32_t height) {
        // TODO: Implement viewport setting
    }

    uint32_t Renderer::GetDrawCallCount() const {
        return pimpl_->draw_call_count;
    }

    uint32_t Renderer::GetTriangleCount() const {
        return pimpl_->triangle_count;
    }

    void Renderer::ResetStatistics() const {
        pimpl_->draw_call_count = 0;
        pimpl_->triangle_count = 0;
    }

    TextureManager &Renderer::GetTextureManager() const {
        return pimpl_->texture_manager;
    }

    ShaderManager &Renderer::GetShaderManager() const {
        return pimpl_->shader_manager;
    }

    MeshManager &Renderer::GetMeshManager() const {
        return pimpl_->mesh_manager;
    }

    MaterialManager &Renderer::GetMaterialManager() const {
        return pimpl_->material_manager;
    }

    // Global renderer instance
    static std::unique_ptr<Renderer> g_renderer;

    Renderer &GetRenderer() {
        if (!g_renderer) {
            g_renderer = std::make_unique<Renderer>();
        }
        return *g_renderer;
    }

    bool InitializeRenderer(const uint32_t window_width, const uint32_t window_height) {
        return GetRenderer().Initialize(window_width, window_height);
    }

    void ShutdownRenderer() {
        if (g_renderer) {
            g_renderer->Shutdown();
            g_renderer.reset();
        }
    }
} // namespace engine::rendering
