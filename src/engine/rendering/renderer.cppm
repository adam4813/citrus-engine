module;

#include <cstdint>
#include <memory>
#include <functional>

export module engine.rendering:renderer;

import engine.components;
import :types;
import :texture;
import :shader;
import :mesh;
import :material;

export namespace engine::rendering {
    // Forward declarations for managers
    class TextureManager;
    class ShaderManager;
    class MeshManager;
    class MaterialManager;

    // Render statistics
    struct RenderStats {
        uint32_t draw_calls = 0;
        uint32_t triangles = 0;
        uint32_t vertices = 0;
        uint32_t textures_bound = 0;
        uint32_t shader_switches = 0;
    };

    // Main renderer class
    class Renderer {
    public:
        Renderer();

        ~Renderer();

        // Initialization
        bool Initialize(uint32_t window_width, uint32_t window_height) const;

        void Shutdown() const;

        // Frame management
        void BeginFrame() const;

        void EndFrame();

        // Camera management
        void SetCamera(const components::Camera &camera);

        //const Camera& GetCamera() const;

        // Resource manager access
        TextureManager &GetTextureManager() const;

        ShaderManager &GetShaderManager() const;

        MeshManager &GetMeshManager() const;

        MaterialManager &GetMaterialManager() const;

        // Rendering operations
        void SubmitRenderCommand(const RenderCommand &command) const;

        void SubmitSprite(const SpriteRenderCommand &command) const;

        void SubmitUIBatch(const UIBatchRenderCommand &command) const;

        // Immediate mode rendering (for debugging)
        void DrawLine(const Vec3 &start, const Vec3 &end, const Color &color = colors::white);

        void DrawWireCube(const Vec3 &center, const Vec3 &size, const Color &color = colors::white);

        void DrawWireSphere(const Vec3 &center, float radius, const Color &color = colors::white);

        // Render settings
        void SetClearColor(const Color &color) const;

        void SetViewport(int x, int y, uint32_t width, uint32_t height);

        void GetFramebufferSize(uint32_t &width, uint32_t &height) const;

        // Statistics
        uint32_t GetDrawCallCount() const;

        uint32_t GetTriangleCount() const;

        void ResetStatistics() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };

    Renderer &GetRenderer();
}
