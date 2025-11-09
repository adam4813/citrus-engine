module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.ui.batch_renderer;

import engine.rendering;
import engine.platform;
import engine.ui;
import glm;

namespace engine::ui::batch_renderer {
constexpr float PI = 3.14159265358979323846f;
constexpr float MIN_LINE_LENGTH = 0.001f; // Minimum line length to avoid degenerate geometry
constexpr float MIN_CORNER_RADIUS = 0.1f; // Minimum corner radius for rounded rectangles

struct BatchRenderer::BatchState {
	// Current batch buffers
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::unordered_map<uint32_t, int> texture_slots;

	// Scissor stack
	std::vector<ScissorRect> scissor_stack;
	ScissorRect current_scissor;

	// Stats
	size_t draw_call_count = 0;
	bool initialized = false;
	bool in_frame = false;

	// White pixel texture for untextured draws
	rendering::TextureId white_texture_id = 0;

	// Rendering resources
	rendering::ShaderId ui_shader = 0;
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	glm::mat4 projection;

	// Screen dimensions
	uint32_t screen_width = 0;
	uint32_t screen_height = 0;

	BatchState() {
		vertices.reserve(INITIAL_VERTEX_CAPACITY);
		indices.reserve(INITIAL_INDEX_CAPACITY);
	}
};

std::unique_ptr<BatchRenderer::BatchState> BatchRenderer::state_ = nullptr;

void BatchRenderer::Initialize() {
	if (!state_) {
		state_ = std::make_unique<BatchState>();

		auto& renderer = rendering::GetRenderer();
		auto& shader_mgr = renderer.GetShaderManager();
		auto& texture_mgr = renderer.GetTextureManager();

		// Load UI batch shader
		platform::fs::Path shader_dir = "shaders";
		state_->ui_shader =
				shader_mgr.LoadShader("ui_batch", shader_dir / "ui_batch.vert", shader_dir / "ui_batch.frag");

		// Create 1x1 white texture for untextured draws
		std::vector<uint8_t> white_pixel = {255, 255, 255, 255};
		rendering::TextureCreateInfo tex_info{};
		tex_info.width = 1;
		tex_info.height = 1;
		tex_info.format = rendering::TextureFormat::RGBA8;
		tex_info.data = white_pixel.data();
		tex_info.parameters = {.generate_mipmaps = false};

		state_->white_texture_id = texture_mgr.CreateTexture("ui_white_pixel", tex_info);

		// Create OpenGL vertex array and buffers
		glGenVertexArrays(1, &state_->vao);
		glGenBuffers(1, &state_->vbo);
		glGenBuffers(1, &state_->ebo);

		glBindVertexArray(state_->vao);

		// Allocate buffers
		glBindBuffer(GL_ARRAY_BUFFER, state_->vbo);
		glBufferData(GL_ARRAY_BUFFER, INITIAL_VERTEX_CAPACITY * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state_->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, INITIAL_INDEX_CAPACITY * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);

		// Setup vertex attributes
		// Position (x, y)
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));

		// TexCoord (u, v)
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u)));

		// Color (vec4)
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));

		// TexIndex (float)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(
				3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_index)));

		glBindVertexArray(0);

		state_->initialized = true;
	}
}

void BatchRenderer::Shutdown() {
	if (state_ && state_->initialized) {
		// Clean up OpenGL resources
		if (state_->vao != 0) {
			glDeleteVertexArrays(1, &state_->vao);
		}
		if (state_->vbo != 0) {
			glDeleteBuffers(1, &state_->vbo);
		}
		if (state_->ebo != 0) {
			glDeleteBuffers(1, &state_->ebo);
		}

		// Clean up texture
		if (state_->white_texture_id != 0) {
			auto& renderer = rendering::GetRenderer();
			auto& texture_mgr = renderer.GetTextureManager();
			texture_mgr.DestroyTexture(state_->white_texture_id);
		}

		state_.reset();
	}
}

void BatchRenderer::BeginFrame() {
	if (!state_ || !state_->initialized) {
		Initialize();
	}

	state_->vertices.clear();
	state_->indices.clear();
	state_->texture_slots.clear();
	state_->scissor_stack.clear();
	state_->draw_call_count = 0;
	state_->in_frame = true;

	// Get current screen dimensions from renderer
	auto& renderer = rendering::GetRenderer();
	// Query the current framebuffer size from the renderer
	uint32_t width = 1920, height = 1080;
	renderer.GetFramebufferSize(width, height);
	state_->screen_width = width;
	state_->screen_height = height;

	// Setup orthographic projection for screen-space rendering
	state_->projection = glm::ortho(
			0.0f,
			static_cast<float>(state_->screen_width),
			static_cast<float>(state_->screen_height),
			0.0f,
			-1.0f,
			1.0f);

	// Initialize scissor to full screen
	state_->current_scissor =
			ScissorRect(0, 0, static_cast<float>(state_->screen_width), static_cast<float>(state_->screen_height));
}

void BatchRenderer::EndFrame() {
	if (!state_ || !state_->in_frame) {
		return;
	}

	// Flush any remaining batched draws
	if (!state_->vertices.empty()) {
		FlushBatch();
	}

	state_->in_frame = false;
}

void BatchRenderer::PushScissor(const ScissorRect& scissor) {
	if (!state_) {
		return;
	}

	// Intersect with current scissor
	ScissorRect new_scissor = state_->current_scissor.Intersect(scissor);

	// If scissor changed, flush current batch
	if (new_scissor != state_->current_scissor && !state_->vertices.empty()) {
		FlushBatch();
	}

	state_->scissor_stack.push_back(state_->current_scissor);
	state_->current_scissor = new_scissor;
}

void BatchRenderer::PopScissor() {
	if (!state_ || state_->scissor_stack.empty()) {
		return;
	}

	ScissorRect previous = state_->scissor_stack.back();
	state_->scissor_stack.pop_back();

	// If scissor changed, flush current batch
	if (previous != state_->current_scissor && !state_->vertices.empty()) {
		FlushBatch();
	}

	state_->current_scissor = previous;
}

ScissorRect BatchRenderer::GetCurrentScissor() { return state_ ? state_->current_scissor : ScissorRect(); }

void BatchRenderer::SubmitQuad(
		const Rectangle& rect, const Color& color, const std::optional<Rectangle>& uv_coords, uint32_t texture_id) {
	if (!state_) {
		return;
	}

	// Use white texture if none specified
	if (texture_id == 0) {
		texture_id = state_->white_texture_id;
	}

	// Check if we need to flush
	if (ShouldFlush(texture_id)) {
		FlushBatch();
	}

	// Get texture slot
	const int tex_slot = GetOrAddTextureSlot(texture_id);

	// UV coordinates (default to full texture)
	float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
	if (uv_coords.has_value()) {
		const Rectangle& uv = uv_coords.value();
		u0 = uv.x;
		v0 = uv.y;
		u1 = uv.x + uv.width;
		v1 = uv.y + uv.height;
	}

	// Quad vertices (top-left origin)

	const float x0 = rect.x;
	const float y0 = rect.y + rect.height; // Note: y-axis inverted
	const float x1 = rect.x + rect.width;
	const float y1 = rect.y;

	const float tex_slot_f = static_cast<float>(tex_slot);

	// clang-format off
	PushQuadVertices(
			Vertex(x0, y0, u0, v0, color, tex_slot_f), // Top-left
			Vertex(x1, y0, u1, v0, color, tex_slot_f), // Top-right
			Vertex(x1, y1, u1, v1, color, tex_slot_f), // Bottom-right
			Vertex(x0, y1, u0, v1, color, tex_slot_f)  // Bottom-left
	);
	// clang-format on

	const uint32_t base = static_cast<uint32_t>(state_->vertices.size()) - 4;
	PushQuadIndices(base);
}

void BatchRenderer::SubmitLine(
		const float x0,
		const float y0,
		const float x1,
		const float y1,
		const float thickness,
		const Color& color,
		uint32_t texture_id) {
	if (!state_) {
		return;
	}

	// Tessellate line as quad (2 triangles)
	const float dx = x1 - x0;
	const float dy = y1 - y0;
	const float len = std::sqrt(dx * dx + dy * dy);

	if (len < MIN_LINE_LENGTH) {
		return; // Degenerate line
	}

	// Perpendicular vector (normalized, scaled by half thickness)
	const float nx = -dy / len * (thickness * 0.5f);
	const float ny = dx / len * (thickness * 0.5f);

	// Quad corners
	const float xa = x0 + nx;
	const float ya = y0 + ny;
	const float xb = x0 - nx;
	const float yb = y0 - ny;
	const float xc = x1 - nx;
	const float yc = y1 - ny;
	const float xd = x1 + nx;
	const float yd = y1 + ny;

	// Use white texture
	if (texture_id == 0) {
		texture_id = state_->white_texture_id;
	}

	if (ShouldFlush(texture_id)) {
		FlushBatch();
	}

	const int tex_slot = GetOrAddTextureSlot(texture_id);
	const float tex_slot_f = static_cast<float>(tex_slot);

	PushQuadVertices(
			Vertex(xa, ya, 0.0f, 0.0f, color, tex_slot_f),
			Vertex(xd, yd, 1.0f, 0.0f, color, tex_slot_f),
			Vertex(xc, yc, 1.0f, 1.0f, color, tex_slot_f),
			Vertex(xb, yb, 0.0f, 1.0f, color, tex_slot_f));

	const uint32_t base = static_cast<uint32_t>(state_->vertices.size()) - 4;
	PushQuadIndices(base);
}

void BatchRenderer::SubmitCircle(
		float center_x, float center_y, const float radius, const Color& color, const int segments) {
	if (!state_ || segments < 3) {
		return;
	}

	const uint32_t texture_id = state_->white_texture_id;

	if (ShouldFlush(texture_id)) {
		FlushBatch();
	}

	const int tex_slot = GetOrAddTextureSlot(texture_id);
	const auto tex_index = static_cast<float>(tex_slot);

	// Center vertex
	const auto center_idx = static_cast<uint32_t>(state_->vertices.size());
	state_->vertices.emplace_back(center_x, center_y, 0.5f, 0.5f, color, tex_index);

	// Perimeter vertices (triangle fan)
	const float angle_step = 2.0f * PI / static_cast<float>(segments);
	for (int i = 0; i <= segments; ++i) {
		const float angle = static_cast<float>(i) * angle_step;
		const float x = center_x + std::cos(angle) * radius;
		const float y = center_y + std::sin(angle) * radius;
		state_->vertices.emplace_back(x, y, 0.5f, 0.5f, color, tex_index);
	}

	// Indices (triangle fan)
	for (int i = 0; i < segments; ++i) {
		state_->indices.push_back(center_idx);
		state_->indices.push_back(center_idx + 1 + i);
		state_->indices.push_back(center_idx + 1 + i + 1);
	}
}

void BatchRenderer::SubmitRoundedRect(
		const Rectangle& rect, float corner_radius, const Color& color, const int corner_segments) {
	if (!state_ || corner_segments < 1) {
		return;
	}

	// Clamp corner radius
	const float max_radius = std::min(rect.width, rect.height) * 0.5f;
	corner_radius = std::min(corner_radius, max_radius);

	if (corner_radius < MIN_CORNER_RADIUS) {
		// Degenerate to normal quad
		SubmitQuad(rect, color);
		return;
	}

	const uint32_t texture_id = state_->white_texture_id;

	if (ShouldFlush(texture_id)) {
		FlushBatch();
	}

	const int tex_slot = GetOrAddTextureSlot(texture_id);
	const auto tex_index = static_cast<float>(tex_slot);

	// Center rectangle (non-rounded part)
	const float inner_x = rect.x + corner_radius;
	const float inner_y = rect.y + corner_radius;
	const float inner_w = rect.width - 2 * corner_radius;
	const float inner_h = rect.height - 2 * corner_radius;

	// Center quad
	if (inner_w > 0 && inner_h > 0) {
		SubmitQuad(Rectangle{inner_x, inner_y, inner_w, inner_h}, color);
	}

	// Four edge rectangles
	SubmitQuad(Rectangle{inner_x, rect.y, inner_w, corner_radius}, color); // Top
	SubmitQuad(Rectangle{inner_x, rect.y + rect.height - corner_radius, inner_w, corner_radius}, color); // Bottom
	SubmitQuad(Rectangle{rect.x, inner_y, corner_radius, inner_h}, color); // Left
	SubmitQuad(Rectangle{rect.x + rect.width - corner_radius, inner_y, corner_radius, inner_h}, color); // Right

	// Four rounded corners (quarter circles)
	const Vector2 corners[4] = {
			{inner_x, inner_y}, // Top-left
			{inner_x + inner_w, inner_y}, // Top-right
			{inner_x + inner_w, inner_y + inner_h}, // Bottom-right
			{inner_x, inner_y + inner_h} // Bottom-left
	};

	constexpr float angle_offsets[4] = {PI, PI * 0.5f, 0.0f, PI * 1.5f};

	for (int c = 0; c < 4; ++c) {
		const auto center_idx = static_cast<uint32_t>(state_->vertices.size());
		state_->vertices.emplace_back(corners[c].x, corners[c].y, 0.5f, 0.5f, color, tex_index);

		const float angle_start = angle_offsets[c];
		const float angle_step = (PI * 0.5f) / static_cast<float>(corner_segments);

		for (int i = 0; i <= corner_segments; ++i) {
			const float angle = angle_start + static_cast<float>(i) * angle_step;
			const float x = corners[c].x + std::cos(angle) * corner_radius;
			const float y = corners[c].y + std::sin(angle) * corner_radius;
			state_->vertices.emplace_back(x, y, 0.5f, 0.5f, color, tex_index);
		}

		for (int i = 0; i < corner_segments; ++i) {
			state_->indices.push_back(center_idx);
			state_->indices.push_back(center_idx + 1 + i);
			state_->indices.push_back(center_idx + 1 + i + 1);
		}
	}
}

void BatchRenderer::SubmitText(
		const std::string& text, const float x, const float y, const int font_size, const Color& color) {
	if (!state_ || text.empty()) {
		return;
	}

	// Get font from font manager - if font_size matches default, use default font
	// Otherwise try to get the default font path with the requested size
	auto* default_font = text_renderer::FontManager::GetDefaultFont();
	if (!default_font || !default_font->IsValid()) {
		return; // Font manager not initialized
	}

	text_renderer::FontAtlas* font = default_font;

	// If a different size is requested, try to load it
	// Note: This only works if the default font is already loaded
	// For more flexibility, users should use FontManager directly
	if (font_size > 0 && font_size != default_font->GetFontSize()) {
		// We can't get the font path from the default font, so just use default size
		// This parameter is kept for API compatibility but currently limited
		font = default_font;
	}

	// Layout text (simple single-line layout)
	text_renderer::LayoutOptions options;
	options.h_align = text_renderer::HorizontalAlign::Left;
	options.v_align = text_renderer::VerticalAlign::Top;
	options.max_width = 0.0f; // No wrapping

	auto glyphs = text_renderer::TextLayout::Layout(text, *font, options);

	// Submit each glyph as a textured quad
	uint32_t texture_id = font->GetTextureId();

	for (const auto& pg : glyphs) {
		if (pg.metrics->size.x == 0 || pg.metrics->size.y == 0) {
			continue; // Skip empty glyphs (like space)
		}

		Rectangle screen_rect(x + pg.position.x, y + pg.position.y, pg.metrics->size.x, pg.metrics->size.y);

		// Submit quad with glyph's UV coordinates
		SubmitQuad(screen_rect, color, pg.metrics->atlas_rect, texture_id);
	}
}

void BatchRenderer::SubmitTextRect(
		const Rectangle& rect, const std::string& text, const int font_size, const Color& color) {
	if (!state_ || text.empty()) {
		return;
	}

	// Get default font from font manager
	auto* font = text_renderer::FontManager::GetDefaultFont();
	if (!font || !font->IsValid()) {
		return; // Font not loaded
	}

	// Layout text with wrapping and alignment
	text_renderer::LayoutOptions options;
	options.h_align = text_renderer::HorizontalAlign::Left;
	options.v_align = text_renderer::VerticalAlign::Top;
	options.max_width = rect.width; // Enable wrapping

	auto glyphs = text_renderer::TextLayout::Layout(text, *font, options);

	// Push scissor for clipping
	PushScissor(ScissorRect(rect.x, rect.y, rect.width, rect.height));

	// Submit each glyph as a textured quad
	uint32_t texture_id = font->GetTextureId();

	for (const auto& pg : glyphs) {
		if (pg.metrics->size.x == 0 || pg.metrics->size.y == 0) {
			continue; // Skip empty glyphs
		}

		Rectangle screen_rect(rect.x + pg.position.x, rect.y + pg.position.y, pg.metrics->size.x, pg.metrics->size.y);

		// Submit quad with glyph's UV coordinates
		SubmitQuad(screen_rect, color, pg.metrics->atlas_rect, texture_id);
	}

	// Pop scissor
	PopScissor();
}

void BatchRenderer::Flush() {
	if (!state_ || state_->vertices.empty()) {
		return;
	}
	FlushBatch();
}

size_t BatchRenderer::GetPendingVertexCount() { return state_ ? state_->vertices.size() : 0; }

size_t BatchRenderer::GetPendingIndexCount() { return state_ ? state_->indices.size() : 0; }

size_t BatchRenderer::GetDrawCallCount() { return state_ ? state_->draw_call_count : 0; }

void BatchRenderer::ResetDrawCallCount() {
	if (state_) {
		state_->draw_call_count = 0;
	}
}

// Private helper implementations

void BatchRenderer::PushQuadVertices(const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
	state_->vertices.push_back(v0);
	state_->vertices.push_back(v1);
	state_->vertices.push_back(v2);
	state_->vertices.push_back(v3);
}

void BatchRenderer::PushQuadIndices(const uint32_t base_vertex) {
	// Two triangles: 0-1-2, 2-3-0
	state_->indices.push_back(base_vertex + 0);
	state_->indices.push_back(base_vertex + 1);
	state_->indices.push_back(base_vertex + 2);

	state_->indices.push_back(base_vertex + 2);
	state_->indices.push_back(base_vertex + 3);
	state_->indices.push_back(base_vertex + 0);
}

bool BatchRenderer::ShouldFlush(const uint32_t texture_id) {
	if (!state_) {
		return false;
	}

	// Check if adding this texture would exceed limit
	if (state_->texture_slots.find(texture_id) == state_->texture_slots.end()) {
		if (state_->texture_slots.size() >= MAX_TEXTURE_SLOTS) {
			return true;
		}
	}

	return false;
}

int BatchRenderer::GetOrAddTextureSlot(const uint32_t texture_id) {
	auto it = state_->texture_slots.find(texture_id);
	if (it != state_->texture_slots.end()) {
		return it->second;
	}

	const int slot = static_cast<int>(state_->texture_slots.size());
	state_->texture_slots[texture_id] = slot;
	return slot;
}

void BatchRenderer::FlushBatch() {
	if (!state_ || state_->vertices.empty()) {
		return;
	}

	// Get renderer
	auto& renderer = rendering::GetRenderer();

	// Prepare texture IDs array for the command
	uint32_t texture_ids[MAX_TEXTURE_SLOTS] = {0};
	for (const auto& [texture_id, slot] : state_->texture_slots) {
		texture_ids[slot] = texture_id;
	}

	// Apply scissor if active
	const bool use_scissor = state_->current_scissor.IsValid();

	// Create UI batch render command
	rendering::UIBatchRenderCommand command{};
	command.shader = state_->ui_shader;
	command.projection = state_->projection;
	command.vao = state_->vao;
	command.vbo = state_->vbo;
	command.ebo = state_->ebo;
	command.vertex_data = state_->vertices.data();
	command.vertex_data_size = state_->vertices.size() * sizeof(Vertex);
	command.index_data = state_->indices.data();
	command.index_data_size = state_->indices.size() * sizeof(uint32_t);
	command.index_count = state_->indices.size();
	command.texture_ids = texture_ids;
	command.texture_count = state_->texture_slots.size();
	command.enable_scissor = use_scissor;
	if (use_scissor) {
		command.scissor_x = static_cast<int>(state_->current_scissor.x);
		command.scissor_y = static_cast<int>(state_->current_scissor.y);
		command.scissor_width = static_cast<int>(state_->current_scissor.width);
		command.scissor_height = static_cast<int>(state_->current_scissor.height);
	}

	// Submit the batch command to the renderer
	renderer.SubmitUIBatch(command);

	state_->draw_call_count++;

	// Clear batch for next submission
	StartNewBatch();
}

void BatchRenderer::StartNewBatch() {
	if (!state_) {
		return;
	}

	state_->vertices.clear();
	state_->indices.clear();
	state_->texture_slots.clear();
}
} // namespace engine::ui::batch_renderer
