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

inline glm::vec3 ToGlm(JPH::RVec3Arg v) {
	return {static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ())};
}

inline glm::vec3 ToGlmColor(JPH::ColorArg c) {
	return {c.r / 255.0F, c.g / 255.0F, c.b / 255.0F};
}

inline float ToGlmAlpha(JPH::ColorArg c) { return c.a / 255.0F; }

} // namespace

// Adapter that inherits Jolt's DebugRendererSimple and forwards draw calls
// to our engine's IPhysicsDebugRenderer interface.
class JoltDebugRendererAdapter : public JPH::DebugRendererSimple {
public:
	explicit JoltDebugRendererAdapter(IPhysicsDebugRenderer& renderer) : renderer_(renderer) { Initialize(); }

	void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
		renderer_.DrawLine(ToGlm(inFrom), ToGlm(inTo), ToGlmColor(inColor));
	}

	void DrawTriangle(
			JPH::RVec3Arg inV1,
			JPH::RVec3Arg inV2,
			JPH::RVec3Arg inV3,
			JPH::ColorArg inColor,
			ECastShadow /*inCastShadow*/) override {
		renderer_.DrawTriangle(ToGlm(inV1), ToGlm(inV2), ToGlm(inV3), ToGlmColor(inColor), ToGlmAlpha(inColor));
	}

	void DrawText3D(
			JPH::RVec3Arg inPosition,
			const std::string_view& inString,
			JPH::ColorArg /*inColor*/,
			float /*inHeight*/) override {
		renderer_.DrawText(ToGlm(inPosition), std::string(inString));
	}

private:
	IPhysicsDebugRenderer& renderer_;
};

} // namespace engine::physics

#endif // JPH_DEBUG_RENDERER
