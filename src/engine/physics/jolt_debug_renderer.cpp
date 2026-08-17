module;

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Jolt includes — order matters.
// clang-format off
#include <Jolt/Jolt.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Core/Color.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif // JPH_DEBUG_RENDERER
// clang-format on

module engine.physics;

import glm;

#ifdef JPH_DEBUG_RENDERER

namespace engine::physics {

namespace {

inline glm::vec3 ToGlm(JPH::RVec3Arg v) { return {v.GetX(), v.GetY(), v.GetZ()}; }

inline glm::vec3 ToGlmColor(JPH::ColorArg c) { return {c.r / 255.0F, c.g / 255.0F, c.b / 255.0F}; }

inline float ToGlmAlpha(JPH::ColorArg c) { return c.a / 255.0F; }

} // namespace

// Adapter that inherits Jolt's DebugRendererSimple and forwards draw calls
// to our engine's IPhysicsDebugRenderer interface.
class JoltDebugRendererAdapter : public JPH::DebugRendererSimple {
public:
	explicit JoltDebugRendererAdapter(IPhysicsDebugRenderer& renderer) : renderer_(renderer) { Initialize(); }

	void DrawLine(JPH::RVec3Arg in_from, JPH::RVec3Arg in_to, JPH::ColorArg inColor) override {
		renderer_.DrawLine(ToGlm(in_from), ToGlm(in_to), ToGlmColor(inColor));
	}

	void DrawTriangle(
		JPH::RVec3Arg in_v1,
		JPH::RVec3Arg in_v2,
		JPH::RVec3Arg in_v3,
		JPH::ColorArg inColor,
		ECastShadow /*inCastShadow*/
	) override {
		renderer_.DrawTriangle(ToGlm(in_v1), ToGlm(in_v2), ToGlm(in_v3), ToGlmColor(inColor), ToGlmAlpha(inColor));
	}

	void DrawText3D(
		JPH::RVec3Arg in_position,
		const std::string_view& inString,
		JPH::ColorArg /*inColor*/,
		float /*inHeight*/
	) override {
		renderer_.DrawText(ToGlm(in_position), std::string(inString));
	}

private:
	IPhysicsDebugRenderer& renderer_;
};

} // namespace engine::physics

#endif // JPH_DEBUG_RENDERER
