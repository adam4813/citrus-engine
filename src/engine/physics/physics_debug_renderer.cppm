module;

#include <cmath>
#include <string>

export module engine.physics:debug_renderer;

import engine.rendering;
import glm;

export namespace engine::physics {

// Abstract interface for physics debug rendering.
// Backend-specific adapters (Jolt JPH::DebugRenderer, Bullet3 btIDebugDraw)
// implement this to bridge their draw calls to the engine renderer.
class IPhysicsDebugRenderer {
public:
	virtual ~IPhysicsDebugRenderer() = default;

	// Core drawing primitives
	virtual void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) = 0;
	virtual void DrawTriangle(
			const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& color,
			float alpha = 1.0F) = 0;

	// Convenience shapes (default implementations using DrawLine)
	virtual void DrawWireBox(const glm::mat4& transform, const glm::vec3& half_extents, const glm::vec3& color);
	virtual void DrawWireSphere(const glm::vec3& center, float radius, const glm::vec3& color);
	virtual void DrawText(const glm::vec3& position, const std::string& text) { (void)position; (void)text; }
};

// Concrete adapter that forwards IPhysicsDebugRenderer calls to engine::rendering::Renderer
class RendererDebugAdapter : public IPhysicsDebugRenderer {
public:
	explicit RendererDebugAdapter(const engine::rendering::Renderer& renderer)
			: renderer_(renderer) {}

	void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override {
		renderer_.DrawLine(from, to, {color.r, color.g, color.b, 1.0F});
	}

	void DrawTriangle(
			const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& color,
			float alpha) override {
		// Wireframe triangle via three line segments
		const engine::rendering::Color line_color{color.r, color.g, color.b, alpha};
		renderer_.DrawLine(v1, v2, line_color);
		renderer_.DrawLine(v2, v3, line_color);
		renderer_.DrawLine(v3, v1, line_color);
	}

	void DrawWireBox(
			const glm::mat4& transform, const glm::vec3& half_extents, const glm::vec3& color) override {
		const glm::vec3 center{transform[3]};
		const glm::vec3 size = half_extents * 2.0F;
		renderer_.DrawWireCube(center, size, {color.r, color.g, color.b, 1.0F});
	}

	void DrawWireSphere(const glm::vec3& center, float radius, const glm::vec3& color) override {
		renderer_.DrawWireSphere(center, radius, {color.r, color.g, color.b, 1.0F});
	}

private:
	const engine::rendering::Renderer& renderer_;
};

// Default implementations for convenience shapes

inline void IPhysicsDebugRenderer::DrawWireBox(
		const glm::mat4& transform, const glm::vec3& half_extents, const glm::vec3& color) {
	// 8 corners of the box in local space
	const glm::vec3 h = half_extents;
	const glm::vec3 corners[8] = {
			{-h.x, -h.y, -h.z}, { h.x, -h.y, -h.z},
			{ h.x,  h.y, -h.z}, {-h.x,  h.y, -h.z},
			{-h.x, -h.y,  h.z}, { h.x, -h.y,  h.z},
			{ h.x,  h.y,  h.z}, {-h.x,  h.y,  h.z},
	};

	// Transform corners to world space
	glm::vec3 world[8];
	for (int i = 0; i < 8; ++i) {
		const glm::vec4 w = transform * glm::vec4(corners[i], 1.0F);
		world[i] = glm::vec3(w);
	}

	// 12 edges of the box
	DrawLine(world[0], world[1], color);
	DrawLine(world[1], world[2], color);
	DrawLine(world[2], world[3], color);
	DrawLine(world[3], world[0], color);
	DrawLine(world[4], world[5], color);
	DrawLine(world[5], world[6], color);
	DrawLine(world[6], world[7], color);
	DrawLine(world[7], world[4], color);
	DrawLine(world[0], world[4], color);
	DrawLine(world[1], world[5], color);
	DrawLine(world[2], world[6], color);
	DrawLine(world[3], world[7], color);
}

inline void IPhysicsDebugRenderer::DrawWireSphere(
		const glm::vec3& center, float radius, const glm::vec3& color) {
	constexpr int kSegments = 16;
	constexpr float kStep = 2.0F * 3.14159265358979F / static_cast<float>(kSegments);

	// Three orthogonal circles
	for (int i = 0; i < kSegments; ++i) {
		const float a0 = static_cast<float>(i) * kStep;
		const float a1 = static_cast<float>(i + 1) * kStep;
		const float c0 = std::cos(a0) * radius;
		const float s0 = std::sin(a0) * radius;
		const float c1 = std::cos(a1) * radius;
		const float s1 = std::sin(a1) * radius;

		// XY circle
		DrawLine(center + glm::vec3(c0, s0, 0.0F), center + glm::vec3(c1, s1, 0.0F), color);
		// XZ circle
		DrawLine(center + glm::vec3(c0, 0.0F, s0), center + glm::vec3(c1, 0.0F, s1), color);
		// YZ circle
		DrawLine(center + glm::vec3(0.0F, c0, s0), center + glm::vec3(0.0F, c1, s1), color);
	}
}

} // namespace engine::physics
