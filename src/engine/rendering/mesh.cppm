module;

#include <memory>
#include <vector>
#include <span>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

export module engine.rendering:mesh;

import :types;

export namespace engine::rendering {
    struct GLMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        uint32_t index_count = 0;
    };

    GLMesh *GetGLMesh(MeshId id);

    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Vec2 tex_coords;

        // Additional vertex attributes can be added here
        Vec3 tangent{0.0f};
        Vec3 bitangent{0.0f};
        Color color{1.0f};
    };

    struct MeshCreateInfo {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        bool dynamic = false; // Can be updated after creation
    };

    class MeshManager {
    public:
        MeshManager();

        ~MeshManager();

        // Mesh creation
        MeshId CreateMesh(const MeshCreateInfo &info) const;

        MeshId CreateQuad(float width = 1.0f, float height = 1.0f) const;

        MeshId CreateCube(float size = 1.0f) const;

        MeshId CreateSphere(float radius = 1.0f, uint32_t segments = 32);

        // Mesh modification
        void UpdateMesh(MeshId id, std::span<const Vertex> vertices, std::span<const uint32_t> indices);

        void UpdateVertices(MeshId id, std::span<const Vertex> vertices);

        void UpdateIndices(MeshId id, std::span<const uint32_t> indices);

        // Mesh info
        uint32_t GetVertexCount(MeshId id) const;

        uint32_t GetIndexCount(MeshId id) const;

        // Resource management
        void DestroyMesh(MeshId id) const;

        bool IsValid(MeshId id) const;

        void Clear() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> pimpl_;
    };
}
