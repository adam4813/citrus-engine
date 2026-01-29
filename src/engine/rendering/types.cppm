module;

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

export module engine.rendering:types;

import glm; // Ensure glm is available in the module context

export namespace engine::rendering {
// Core math types
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Color = glm::vec4;

// Common colors
namespace colors {
constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color black{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color red{1.0f, 0.0f, 0.0f, 1.0f};
constexpr Color green{0.0f, 1.0f, 0.0f, 1.0f};
constexpr Color blue{0.0f, 0.0f, 1.0f, 1.0f};
constexpr Color yellow{1.0f, 1.0f, 0.0f, 1.0f};
constexpr Color magenta{1.0f, 0.0f, 1.0f, 1.0f};
constexpr Color cyan{0.0f, 1.0f, 1.0f, 1.0f};
constexpr Color transparent{0.0f, 0.0f, 0.0f, 0.0f};
} // namespace colors

// Resource handles
using TextureId = uint32_t;
using ShaderId = uint32_t;
using MeshId = uint32_t;
using MaterialId = uint32_t;
using RenderTargetId = uint32_t;

constexpr TextureId INVALID_TEXTURE = 0;
constexpr ShaderId INVALID_SHADER = 0;
constexpr MeshId INVALID_MESH = 0;
constexpr MaterialId INVALID_MATERIAL = 0;
constexpr RenderTargetId INVALID_RENDER_TARGET = 0;

enum class RenderFlag : uint32_t {
	DepthTest = 1 << 0,
	Blend = 1 << 1,
	CullFace = 1 << 2,
	Wireframe = 1 << 3,
	StencilTest = 1 << 4,
	ScissorTest = 1 << 5,
	DepthMask = 1 << 6
};

struct RenderFlagStackEntry {
	std::vector<RenderFlag> enable_flags;
	std::vector<RenderFlag> disable_flags;
};

struct RenderCommand {
	MeshId mesh;
	ShaderId shader;
	MaterialId material;
	Mat4 transform;
	Color tint = colors::white;
	std::vector<RenderFlagStackEntry> render_state_stack;
	int layer = 0;
	glm::mat4 camera_view;
};

struct SpriteRenderCommand {
	TextureId texture;
	Vec2 position;
	Vec2 size;
	float rotation = 0.0f;
	Color color = colors::white;
	Vec2 texture_offset{0.0f, 0.0f};
	Vec2 texture_scale{1.0f, 1.0f};
	int layer = 0;
};

// UI Batch render command for batch renderer
// Maximum texture slots supported by UI batch renderer
constexpr size_t UI_BATCH_MAX_TEXTURE_SLOTS = 8;

struct UIBatchRenderCommand {
	ShaderId shader;
	Mat4 projection;
	uint32_t vao;
	uint32_t vbo;
	uint32_t ebo;
	const void* vertex_data;
	size_t vertex_data_size;
	const void* index_data;
	size_t index_data_size;
	size_t index_count;
	const uint32_t* texture_ids;
	size_t texture_count;
	bool enable_scissor = false;
	int scissor_x = 0;
	int scissor_y = 0;
	int scissor_width = 0;
	int scissor_height = 0;
};

struct Renderable {
	MeshId mesh{0};
	MaterialId material{0};
	ShaderId shader{0};
	bool visible{true};
	uint32_t render_layer{0};
	std::vector<RenderFlagStackEntry> render_state_stack;
	float alpha{1.0F};
};

// Asset reference component for shader - stores shader name for serialization
// An observer resolves the name to ShaderId and updates Renderable::shader
struct ShaderRef {
	std::string name;
};

// Asset reference component for mesh - stores mesh asset name for serialization
// An observer resolves the name to MeshId and updates Renderable::mesh
struct MeshRef {
	std::string name;
};

// Light component for lighting calculations
struct Light {
	enum class Type : int { Directional = 0, Point = 1, Spot = 2 };

	Type type{Type::Directional};
	Color color{1.0F, 1.0F, 1.0F, 1.0F};
	float intensity{1.0F};
	// Point/Spot light parameters
	float range{10.0F};
	float attenuation{1.0F};
	// Spot light parameters
	float spot_angle{45.0F};
	float spot_falloff{1.0F};
	// Directional light direction
	glm::vec3 direction{0.0F, -1.0F, 0.0F};
};

// Animation component for animated meshes/sprites
struct Animation {
	char current_animation[64] = ""; // Fixed-size string for POD compatibility
	float animation_time{0.0F};
	float animation_speed{1.0F};
	bool looping{true};
	bool playing{false};
};

// Particle system component
struct ParticleSystem {
	uint32_t max_particles{100};
	uint32_t active_particles{0};
	float emission_rate{10.0F};
	float particle_lifetime{1.0F};
	glm::vec3 velocity{0.0F};
	glm::vec3 gravity{0.0F, -9.81F, 0.0F};
	Color start_color{1.0F, 1.0F, 1.0F, 1.0F};
	Color end_color{1.0F, 1.0F, 1.0F, 1.0F};
	float start_size{1.0F};
	float end_size{0.0F};
};

struct Sprite {
	TextureId texture{0};
	glm::vec2 position{0.0F}; // Screen coordinates
	glm::vec2 size{1.0F};
	float rotation{0.0F};
	Color color{1.0F, 1.0F, 1.0F, 1.0F};
	glm::vec2 texture_offset{0.0F, 0.0F};
	glm::vec2 texture_scale{1.0F, 1.0F};
	int layer{0};
	// UI-specific properties
	glm::vec2 pivot{0.5F}; // 0,0 = bottom-left, 1,1 = top-right
	bool flip_x{false};
	bool flip_y{false};
};
} // namespace engine::rendering
