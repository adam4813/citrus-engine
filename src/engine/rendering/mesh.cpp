// Mesh implementation stub
module;

#include <memory>
#include <unordered_map>
#include <vector>
#include <span>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import glm;

import :mesh;
import :types;

namespace engine::rendering {
    // MeshManager implementation
    struct MeshManager::Impl {
        std::unordered_map<MeshId, MeshCreateInfo> meshes;
        MeshId next_id = 1;
    };

    namespace {
        std::unordered_map<MeshId, GLMesh> g_mesh_gl;
    }

    GLMesh *GetGLMesh(const MeshId id) {
        const auto it = g_mesh_gl.find(id);
        return it != g_mesh_gl.end() ? &it->second : nullptr;
    }

    MeshManager::MeshManager() : pimpl_(std::make_unique<Impl>()) {
    }

    MeshManager::~MeshManager() = default;

    MeshId MeshManager::CreateMesh(const MeshCreateInfo &info) const {
        const MeshId id = pimpl_->next_id++;
        pimpl_->meshes[id] = info;

        // OpenGL buffer setup
        GLMesh gl_mesh;
        glGenVertexArrays(1, &gl_mesh.vao);
        glGenBuffers(1, &gl_mesh.vbo);
        glGenBuffers(1, &gl_mesh.ebo);
        gl_mesh.index_count = static_cast<uint32_t>(info.indices.size());

        glBindVertexArray(gl_mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, gl_mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, info.vertices.size() * sizeof(Vertex), info.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.indices.size() * sizeof(uint32_t), info.indices.data(),
                     GL_STATIC_DRAW);

        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        // TexCoords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tex_coords));
        glEnableVertexAttribArray(2);
        // color
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, color));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);

        g_mesh_gl[id] = gl_mesh;

        return id;
    }

    MeshId MeshManager::CreateQuad(const float width, const float height) const {
        // Create hardcoded quad mesh for MVP
        const std::vector<Vertex> vertices = {
            {{-width / 2, -height / 2, 0}, {0, 0, 1}, {0, 0}},
            {{width / 2, -height / 2, 0}, {0, 0, 1}, {1, 0}},
            {{width / 2, height / 2, 0}, {0, 0, 1}, {1, 1}},
            {{-width / 2, height / 2, 0}, {0, 0, 1}, {0, 1}}
        };

        const std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

        MeshCreateInfo info;
        info.vertices = vertices;
        info.indices = indices;

        return CreateMesh(info);
    }

    MeshId MeshManager::CreateCube(const float size) const {
        // Hardcoded cube mesh (24 vertices, 36 indices, unique color per face)
        const float h = size * 0.5f;
        // ReSharper disable once CppVariableCanBeMadeConstexpr - constexpr not supported for color
        const Color face_colors[6] = {
            colors::red, // Front
            colors::green, // Back
            colors::blue, // Left
            colors::yellow, // Right
            colors::magenta, // Top
            colors::cyan // Bottom
        };
        const std::vector<Vertex> vertices = {
            // Front face (red)
            {{-h, -h, h}, {0, 0, 1}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
            {{h, -h, h}, {0, 0, 1}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
            {{h, h, h}, {0, 0, 1}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
            {{-h, h, h}, {0, 0, 1}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
            // Back face (green)
            {{h, -h, -h}, {0, 0, -1}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
            {{-h, -h, -h}, {0, 0, -1}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
            {{-h, h, -h}, {0, 0, -1}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
            {{h, h, -h}, {0, 0, -1}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
            // Left face (blue)
            {{-h, -h, -h}, {-1, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
            {{-h, -h, h}, {-1, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
            {{-h, h, h}, {-1, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
            {{-h, h, -h}, {-1, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
            // Right face (yellow)
            {{h, -h, h}, {1, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
            {{h, -h, -h}, {1, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
            {{h, h, -h}, {1, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
            {{h, h, h}, {1, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
            // Top face (magenta)
            {{-h, h, h}, {0, 1, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
            {{h, h, h}, {0, 1, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
            {{h, h, -h}, {0, 1, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
            {{-h, h, -h}, {0, 1, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
            // Bottom face (cyan)
            {{-h, -h, -h}, {0, -1, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
            {{h, -h, -h}, {0, -1, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
            {{h, -h, h}, {0, -1, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
            {{-h, -h, h}, {0, -1, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
        };
        const std::vector<uint32_t> indices = {
            // Front face
            0, 1, 2, 0, 2, 3,
            // Back face
            4, 5, 6, 4, 6, 7,
            // Left face
            8, 9, 10, 8, 10, 11,
            // Right face
            12, 13, 14, 12, 14, 15,
            // Top face
            16, 17, 18, 16, 18, 19,
            // Bottom face
            20, 21, 22, 20, 22, 23
        };
        MeshCreateInfo info;
        info.vertices = vertices;
        info.indices = indices;
        return CreateMesh(info);
    }

    MeshId MeshManager::CreateSphere(float radius, uint32_t segments) {
        // TODO: Create procedural sphere mesh
        return INVALID_MESH;
    }

    void MeshManager::UpdateMesh(MeshId id, std::span<const Vertex> vertices, std::span<const uint32_t> indices) {
        // TODO: Update mesh data
    }

    void MeshManager::UpdateVertices(MeshId id, std::span<const Vertex> vertices) {
        // TODO: Update vertex data
    }

    void MeshManager::UpdateIndices(MeshId id, std::span<const uint32_t> indices) {
        // TODO: Update index data
    }

    uint32_t MeshManager::GetVertexCount(const MeshId id) const {
        const auto it = pimpl_->meshes.find(id);
        return it != pimpl_->meshes.end() ? static_cast<uint32_t>(it->second.vertices.size()) : 0;
    }

    uint32_t MeshManager::GetIndexCount(const MeshId id) const {
        const auto it = pimpl_->meshes.find(id);
        return it != pimpl_->meshes.end() ? static_cast<uint32_t>(it->second.indices.size()) : 0;
    }

    void MeshManager::DestroyMesh(const MeshId id) const {
        pimpl_->meshes.erase(id);
    }

    bool MeshManager::IsValid(const MeshId id) const {
        return pimpl_->meshes.contains(id);
    }

    void MeshManager::Clear() const {
        pimpl_->meshes.clear();
    }
} // namespace engine::rendering
