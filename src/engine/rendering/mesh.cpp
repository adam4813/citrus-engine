// Mesh implementation stub
module;

#include <cmath>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>
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

GLMesh* GetGLMesh(const MeshId id) {
	const auto it = g_mesh_gl.find(id);
	return it != g_mesh_gl.end() ? &it->second : nullptr;
}

MeshManager::MeshManager() : pimpl_(std::make_unique<Impl>()) {}

MeshManager::~MeshManager() = default;

MeshId MeshManager::CreateMesh(const MeshCreateInfo& info) const {
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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.indices.size() * sizeof(uint32_t), info.indices.data(), GL_STATIC_DRAW);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	// TexCoords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
	glEnableVertexAttribArray(2);
	// color
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
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
			{{-width / 2, height / 2, 0}, {0, 0, 1}, {0, 1}}};

	const std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

	MeshCreateInfo info;
	info.vertices = vertices;
	info.indices = indices;

	return CreateMesh(info);
}

MeshId MeshManager::CreateCube(const float size) const { return CreateCube(size, size, size); }

MeshId MeshManager::CreateCube(const float width, const float height, const float depth) const {
	// Cube mesh with separate dimensions (24 vertices, 36 indices, unique color per face)
	const float hw = width * 0.5f; // half width (X)
	const float hh = height * 0.5f; // half height (Y)
	const float hd = depth * 0.5f; // half depth (Z)
	// ReSharper disable once CppVariableCanBeMadeConstexpr - constexpr not supported for color
	const Color face_colors[6] = {
			colors::red, // Front
			colors::green, // Back
			colors::blue, // Left
			colors::yellow, // Right
			colors::magenta, // Top
			colors::cyan // Bottom
	};
	// clang-format off
	const std::vector<Vertex> vertices = {
		// Front face (red) - Z+
		{{-hw, -hh, hd}, {0, 0, 1}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
		{{hw, -hh, hd}, {0, 0, 1}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
		{{hw, hh, hd}, {0, 0, 1}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
		{{-hw, hh, hd}, {0, 0, 1}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[0]},
		// Back face (green) - Z-
		{{hw, -hh, -hd}, {0, 0, -1}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
		{{-hw, -hh, -hd}, {0, 0, -1}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
		{{-hw, hh, -hd}, {0, 0, -1}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
		{{hw, hh, -hd}, {0, 0, -1}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[1]},
		// Left face (blue) - X-
		{{-hw, -hh, -hd}, {-1, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
		{{-hw, -hh, hd}, {-1, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
		{{-hw, hh, hd}, {-1, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
		{{-hw, hh, -hd}, {-1, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[2]},
		// Right face (yellow) - X+
		{{hw, -hh, hd}, {1, 0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
		{{hw, -hh, -hd}, {1, 0, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
		{{hw, hh, -hd}, {1, 0, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
		{{hw, hh, hd}, {1, 0, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[3]},
		// Top face (magenta) - Y+
		{{-hw, hh, hd}, {0, 1, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
		{{hw, hh, hd}, {0, 1, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
		{{hw, hh, -hd}, {0, 1, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
		{{-hw, hh, -hd}, {0, 1, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[4]},
		// Bottom face (cyan) - Y-
		{{-hw, -hh, -hd}, {0, -1, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
		{{hw, -hh, -hd}, {0, -1, 0}, {1, 0}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
		{{hw, -hh, hd}, {0, -1, 0}, {1, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
		{{-hw, -hh, hd}, {0, -1, 0}, {0, 1}, {0, 0, 0}, {0, 0, 0}, face_colors[5]},
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
	// clang-format on
	MeshCreateInfo info;
	info.vertices = vertices;
	info.indices = indices;
	return CreateMesh(info);
}

MeshId MeshManager::CreateSphere(float radius, uint32_t segments) {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	const uint32_t rings = segments;
	const uint32_t sectors = segments;

	constexpr float PI = 3.14159265359f;

	// Top pole - single vertex
	Vertex top_pole;
	top_pole.position = Vec3(0, radius, 0);
	top_pole.normal = Vec3(0, 1, 0);
	top_pole.tex_coords = Vec2(0.5f, 0.0f);
	vertices.push_back(top_pole);

	// Generate middle rings (exclude poles)
	for (uint32_t ring = 1; ring < rings; ++ring) {
		const float phi = PI * static_cast<float>(ring) / static_cast<float>(rings);
		const float y = radius * std::cos(phi);
		const float ring_radius = radius * std::sin(phi);

		for (uint32_t sector = 0; sector <= sectors; ++sector) {
			const float theta = 2.0f * PI * static_cast<float>(sector) / static_cast<float>(sectors);
			const float x = ring_radius * std::cos(theta);
			const float z = ring_radius * std::sin(theta);

			Vertex vertex;
			vertex.position = Vec3(x, y, z);
			vertex.normal = glm::normalize(Vec3(x, y, z));
			vertex.tex_coords =
					Vec2(static_cast<float>(sector) / static_cast<float>(sectors),
						 static_cast<float>(ring) / static_cast<float>(rings));

			vertices.push_back(vertex);
		}
	}

	// Bottom pole - single vertex
	Vertex bottom_pole;
	bottom_pole.position = Vec3(0, -radius, 0);
	bottom_pole.normal = Vec3(0, -1, 0);
	bottom_pole.tex_coords = Vec2(0.5f, 1.0f);
	vertices.push_back(bottom_pole);

	// Generate indices for top cap (triangle fan from top pole)
	for (uint32_t sector = 0; sector < sectors; ++sector) {
		indices.push_back(0); // top pole
		indices.push_back(1 + sector + 1);
		indices.push_back(1 + sector);
	}

	// Generate indices for middle rings
	for (uint32_t ring = 0; ring < rings - 2; ++ring) {
		for (uint32_t sector = 0; sector < sectors; ++sector) {
			const uint32_t current = 1 + ring * (sectors + 1) + sector;
			const uint32_t next = current + sectors + 1;

			indices.push_back(current);
			indices.push_back(current + 1);
			indices.push_back(next);

			indices.push_back(current + 1);
			indices.push_back(next + 1);
			indices.push_back(next);
		}
	}

	// Generate indices for bottom cap (triangle fan from bottom pole)
	const uint32_t bottom_pole_index = static_cast<uint32_t>(vertices.size()) - 1;
	const uint32_t last_ring_start = 1 + (rings - 2) * (sectors + 1);
	for (uint32_t sector = 0; sector < sectors; ++sector) {
		indices.push_back(bottom_pole_index);
		indices.push_back(last_ring_start + sector);
		indices.push_back(last_ring_start + sector + 1);
	}

	MeshCreateInfo info;
	info.vertices = vertices;
	info.indices = indices;

	return CreateMesh(info);
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

void MeshManager::DestroyMesh(const MeshId id) const { pimpl_->meshes.erase(id); }

bool MeshManager::IsValid(const MeshId id) const { return pimpl_->meshes.contains(id); }

void MeshManager::Clear() const { pimpl_->meshes.clear(); }
} // namespace engine::rendering
