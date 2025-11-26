// Rendering renderer implementation stub
module;

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import glm;
import :mesh;
import engine.components;

namespace engine::rendering {
// GL error checking helper
static void CheckGLError(const char* context) {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		const char* error_str = "Unknown";
		switch (err) {
		case GL_INVALID_ENUM: error_str = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE: error_str = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: error_str = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY: error_str = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		spdlog::error("[GL ERROR] {}: {} (0x{:x})", context, error_str, err);
	}
}
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

	// Built-in shaders
	ShaderId sprite_shader_id = INVALID_SHADER;

	// Statistics
	uint32_t draw_call_count = 0;
	uint32_t triangle_count = 0;
};

Renderer::Renderer() : pimpl_(std::make_unique<Impl>()) {}

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

	// Create built-in sprite shader
	const std::string vertex_shader_source = R"(#version 300 es

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coords;

uniform mat4 u_MVP;
uniform vec2 u_TexOffset;
uniform vec2 u_TexScale;

out vec2 v_TexCoord;

void main() {
    gl_Position = u_MVP * vec4(position, 1.0);
    v_TexCoord = tex_coords * u_TexScale + u_TexOffset;
}
)";

	const std::string fragment_shader_source = R"(#version 300 es
precision mediump float;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform vec4 u_Color;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);
    FragColor = texColor * u_Color;
}
)";

	// Create the sprite shader
	pimpl_->sprite_shader_id =
			pimpl_->shader_manager.LoadShaderFromString("sprite_shader", vertex_shader_source, fragment_shader_source);

	if (pimpl_->sprite_shader_id == INVALID_SHADER) {
		// Log error - sprite shader creation failed
		return false;
	}

	pimpl_->initialized = true;
	return true;
}

void Renderer::Shutdown() const { pimpl_->initialized = false; }

void Renderer::BeginFrame() const {
	pimpl_->draw_call_count = 0;
	pimpl_->triangle_count = 0;
	glClearColor(pimpl_->clear_color.r, pimpl_->clear_color.g, pimpl_->clear_color.b, pimpl_->clear_color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame() {
	// TODO: Implement frame end logic
}

void Renderer::SubmitRenderCommand(const RenderCommand& command) const {
	// Look up mesh OpenGL handles
	const GLMesh* gl_mesh = GetGLMesh(command.mesh);
	if (!gl_mesh) {
		return;
	}

	// Determine which shader to use: custom shader if material is invalid, otherwise material's shader
	const Shader* shader = nullptr;
	if (command.material == INVALID_MATERIAL && command.shader != INVALID_SHADER) {
		// Use custom shader
		shader = &pimpl_->shader_manager.GetShader(command.shader);
	}
	else if (command.material != INVALID_MATERIAL) {
		// Use material's shader
		const Material& material = pimpl_->material_manager.GetMaterial(command.material);
		shader = &pimpl_->shader_manager.GetShader(material.GetShader());
	}

	if (!shader || !shader->IsValid()) {
		return;
	}

	shader->Use();

	shader->SetUniform("u_Model", command.transform);

	// Create perspective projection matrix
	const float aspect = static_cast<float>(pimpl_->window_width) / static_cast<float>(pimpl_->window_height);
	const glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 1000.0f);

	const glm::mat4 mvp = projection * command.camera_view * command.transform;
	shader->SetUniform("u_MVP", mvp);

	// Apply material properties if using a material
	if (command.material != INVALID_MATERIAL) {
		const Material& material = pimpl_->material_manager.GetMaterial(command.material);
		material.Apply(*shader);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& [enable_flags, disable_flags] : command.render_state_stack) {
		for (auto& flag : enable_flags) {
			switch (flag) {
			case RenderFlag::DepthTest: glEnable(GL_DEPTH_TEST); break;
			case RenderFlag::Blend: glEnable(GL_BLEND); break;
			case RenderFlag::CullFace: glEnable(GL_CULL_FACE); break;
			case RenderFlag::Wireframe:
#ifdef __EMSCRIPTEN__
// Not available in WebGL/GLES3, skip or find an alternative
#else
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
				break;
			case RenderFlag::DepthMask: glDepthMask(GL_TRUE); break;
			default: break;
			}
		}
		for (auto& flag : disable_flags) {
			switch (flag) {
			case RenderFlag::DepthTest: glDisable(GL_DEPTH_TEST); break;
			case RenderFlag::Blend: glDisable(GL_BLEND); break;
			case RenderFlag::CullFace: glDisable(GL_CULL_FACE); break;
			case RenderFlag::Wireframe:
#ifdef __EMSCRIPTEN__
// Not available in WebGL/GLES3, skip or find an alternative
#else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
				break;
			case RenderFlag::DepthMask: glDepthMask(GL_FALSE); break;
			default: break;
			}
		}
	}

	glBindVertexArray(gl_mesh->vao);
	glDrawElements(GL_TRIANGLES, gl_mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	for (const auto& [enable_flags, disable_flags] : command.render_state_stack) {
		// Restore render states
		for (auto& flag : disable_flags) {
			switch (flag) {
			case RenderFlag::DepthTest: glEnable(GL_DEPTH_TEST); break;
			case RenderFlag::Blend: glEnable(GL_BLEND); break;
			case RenderFlag::CullFace: glEnable(GL_CULL_FACE); break;
			case RenderFlag::Wireframe:
#ifdef __EMSCRIPTEN__
// Not available in WebGL/GLES3, skip or find an alternative
#else
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
				break;
			case RenderFlag::DepthMask: glDepthMask(GL_TRUE); break;
			default: break;
			}
		}
		for (auto& flag : enable_flags) {
			switch (flag) {
			case RenderFlag::DepthTest: glDisable(GL_DEPTH_TEST); break;
			case RenderFlag::Blend: glDisable(GL_BLEND); break;
			case RenderFlag::CullFace: glDisable(GL_CULL_FACE); break;
			case RenderFlag::Wireframe:
#ifdef __EMSCRIPTEN__
// Not available in WebGL/GLES3, skip or find an alternative
#else
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
				break;
			case RenderFlag::DepthMask: glDepthMask(GL_FALSE); break;
			default: break;
			}
		}
	}

	pimpl_->draw_call_count++;
	pimpl_->triangle_count += gl_mesh->index_count / 3;
}

void Renderer::SubmitSprite(const SpriteRenderCommand& command) const {
	// Get or create a unit quad mesh for sprite rendering
	static MeshId sprite_quad = 0;
	if (sprite_quad == 0) {
		sprite_quad = pimpl_->mesh_manager.CreateQuad(1.0f, 1.0f); // Unit quad
	}

	// Get the quad mesh
	const GLMesh* gl_mesh = GetGLMesh(sprite_quad);
	if (!gl_mesh) {
		return;
	}

	// Get sprite texture
	if (command.texture == INVALID_TEXTURE) {
		return;
	}

	// Use the sprite shader
	const Shader& sprite_shader = pimpl_->shader_manager.GetShader(pimpl_->sprite_shader_id);
	if (!sprite_shader.IsValid()) {
		return;
	}

	// Create transformation matrix for the sprite
	Mat4 transform = glm::mat4(1.0f);

	// Apply translation
	transform = glm::translate(transform, Vec3(command.position.x, command.position.y, 0.0f));

	// Apply rotation around Z-axis
	if (command.rotation != 0.0f) {
		transform = glm::rotate(transform, command.rotation, Vec3(0, 0, 1));
	}

	// Apply scale
	transform = glm::scale(transform, Vec3(command.size.x, command.size.y, 1.0f));

	// Set up orthographic projection for 2D rendering (screen coordinates)
	const float left = 0.0f;
	const float right = static_cast<float>(pimpl_->window_width);
	const float bottom = 0.0f;
	const float top = static_cast<float>(pimpl_->window_height);
	const float near = -1.0f;
	const float far = 1.0f;

	const Mat4 projection = glm::ortho(left, right, bottom, top, near, far);
	const Mat4 mvp = projection * transform;

	// Use sprite shader and set uniforms
	sprite_shader.Use();
	sprite_shader.SetUniform("u_MVP", mvp);
	sprite_shader.SetUniform("u_Texture", 0);
	sprite_shader.SetUniform("u_Color", command.color);
	sprite_shader.SetUniform("u_TexOffset", command.texture_offset);
	sprite_shader.SetUniform("u_TexScale", command.texture_scale);

	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GetGLTexture(command.texture)->handle);

	// Enable alpha blending for sprites
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Disable depth testing for 2D sprites
	glDisable(GL_DEPTH_TEST);

	// Render the quad
	glBindVertexArray(gl_mesh->vao);
	glDrawElements(GL_TRIANGLES, gl_mesh->index_count, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// Restore OpenGL state
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);

	pimpl_->draw_call_count++;
	pimpl_->triangle_count += gl_mesh->index_count / 3;
}

void Renderer::SubmitUIBatch(const UIBatchRenderCommand& command) const {
	// Get shader
	const Shader& shader = pimpl_->shader_manager.GetShader(command.shader);
	if (!shader.IsValid()) {
		spdlog::error("[UI Batch] Invalid shader");
		return;
	}

	// Save GL state
	GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
	GLboolean cull_face_enabled = glIsEnabled(GL_CULL_FACE);

	// Disable depth test and face culling for UI rendering
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Use the UI batch shader
	shader.Use();
	CheckGLError("After shader.Use()");

	// Set projection matrix
	shader.SetUniform("u_Projection", command.projection);
	CheckGLError("After setting u_Projection");

	// Bind textures to their slots
	// Important: In WebGL/GLES, all sampler uniforms must have valid textures bound
	int tex_samplers[UI_BATCH_MAX_TEXTURE_SLOTS] = {0, 1, 2, 3, 4, 5, 6, 7};

	// First, bind all requested textures
	for (size_t i = 0; i < command.texture_count; ++i) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));

		// Get the GL texture handle from our texture manager
		auto* gl_tex = GetGLTexture(command.texture_ids[i]);
		if (gl_tex) {
			glBindTexture(GL_TEXTURE_2D, gl_tex->handle);
		}
		else {
			spdlog::error("[UI Batch] Invalid texture ID: {} at slot {}", command.texture_ids[i], i);
			// Bind white texture as fallback
			if (i == 0 && command.texture_count > 0) {
				auto* first_tex = GetGLTexture(command.texture_ids[0]);
				if (first_tex) {
					glBindTexture(GL_TEXTURE_2D, first_tex->handle);
				}
			}
		}
	}

	// For unused texture slots, bind the first texture to prevent WebGL errors
	// (WebGL requires all sampler uniforms to have valid textures)
	if (command.texture_count > 0) {
		auto* first_tex = GetGLTexture(command.texture_ids[0]);
		if (first_tex) {
			for (size_t i = command.texture_count; i < UI_BATCH_MAX_TEXTURE_SLOTS; ++i) {
				glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
				glBindTexture(GL_TEXTURE_2D, first_tex->handle);
			}
		}
	}

	CheckGLError("After binding textures");

	// Set texture sampler array uniform
	shader.SetUniformArray("u_Textures", tex_samplers, UI_BATCH_MAX_TEXTURE_SLOTS);
	CheckGLError("After setting u_Textures array");

	// Apply scissor if active
	if (command.enable_scissor) {
		// OpenGL scissor uses bottom-left origin, but UI uses top-left origin
		glEnable(GL_SCISSOR_TEST);
		glScissor(command.scissor_x,
				  static_cast<int>(pimpl_->window_height) - command.scissor_y - command.scissor_height, // Flip Y
				  command.scissor_width,
				  command.scissor_height);
		CheckGLError("After setting scissor");
	}

	// Upload vertex and index data
	glBindVertexArray(command.vao);
	CheckGLError("After binding VAO");

	glBindBuffer(GL_ARRAY_BUFFER, command.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, command.vertex_data_size, command.vertex_data);
	CheckGLError("After uploading vertex data");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, command.ebo);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, command.index_data_size, command.index_data);
	CheckGLError("After uploading index data");

	// Enable blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	CheckGLError("After setting blend mode");

	// Draw the batch
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(command.index_count), GL_UNSIGNED_INT, nullptr);
	CheckGLError("After glDrawElements");

	// Disable blending
	glDisable(GL_BLEND);

	// Disable scissor
	if (command.enable_scissor) {
		glDisable(GL_SCISSOR_TEST);
	}

	glBindVertexArray(0);

	// Restore GL state
	if (depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
	}
	if (cull_face_enabled) {
		glEnable(GL_CULL_FACE);
	}

	pimpl_->draw_call_count++;
	pimpl_->triangle_count += command.index_count / 3;
}

void Renderer::DrawLine(const Vec3& start, const Vec3& end, const Color& color) {
	// TODO: Implement debug line drawing
}

void Renderer::DrawWireCube(const Vec3& center, const Vec3& size, const Color& color) {
	// TODO: Implement debug cube drawing
}

void Renderer::DrawWireSphere(const Vec3& center, float radius, const Color& color) {
	// TODO: Implement debug sphere drawing
}

void Renderer::SetCamera(const components::Camera& camera) {
	//pimpl_->camera = camera;
}

/*const Camera& Renderer::GetCamera() const {
        return pimpl_->camera;
    }*/

void Renderer::SetClearColor(const Color& color) const { pimpl_->clear_color = color; }

void Renderer::SetViewport(const int x, const int y, const uint32_t width, const uint32_t height) {
	glViewport(x, y, width, height);
}

void Renderer::GetFramebufferSize(uint32_t& width, uint32_t& height) const {
	width = pimpl_->window_width;
	height = pimpl_->window_height;
}

void Renderer::SetWindowSize(const uint32_t width, const uint32_t height) const {
	pimpl_->window_width = width;
	pimpl_->window_height = height;
	SetViewport(0, 0, width, height);
}

uint32_t Renderer::GetDrawCallCount() const { return pimpl_->draw_call_count; }

uint32_t Renderer::GetTriangleCount() const { return pimpl_->triangle_count; }

void Renderer::ResetStatistics() const {
	pimpl_->draw_call_count = 0;
	pimpl_->triangle_count = 0;
}

TextureManager& Renderer::GetTextureManager() const { return pimpl_->texture_manager; }

ShaderManager& Renderer::GetShaderManager() const { return pimpl_->shader_manager; }

MeshManager& Renderer::GetMeshManager() const { return pimpl_->mesh_manager; }

MaterialManager& Renderer::GetMaterialManager() const { return pimpl_->material_manager; }

// Global renderer instance
static std::unique_ptr<Renderer> g_renderer;

Renderer& GetRenderer() {
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
