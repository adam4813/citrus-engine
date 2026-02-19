module;

#include <string>

#include <spdlog/spdlog.h>

#include <btBulletDynamicsCommon.h>

module engine.physics;

import glm;

namespace engine::physics {

namespace {

auto ToGlm(const btVector3& v) -> glm::vec3 { return {v.getX(), v.getY(), v.getZ()}; }

} // namespace

// Adapter that implements Bullet3's btIDebugDraw and forwards draw calls
// to the engine's IPhysicsDebugRenderer.
class Bullet3DebugDrawAdapter : public btIDebugDraw {
public:
	explicit Bullet3DebugDrawAdapter(IPhysicsDebugRenderer* renderer) : renderer_(renderer) {}

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {
		if (renderer_) {
			renderer_->DrawLine(ToGlm(from), ToGlm(to), ToGlm(color));
		}
	}

	void drawContactPoint(
			const btVector3& point_on_b,
			const btVector3& normal_on_b,
			btScalar distance,
			int /*life_time*/,
			const btVector3& color) override {
		if (renderer_) {
			const btVector3 to = point_on_b + normal_on_b * distance;
			renderer_->DrawLine(ToGlm(point_on_b), ToGlm(to), ToGlm(color));
		}
	}

	void reportErrorWarning(const char* warning_string) override {
		spdlog::warn("[Bullet3 Debug] {}", warning_string);
	}

	void draw3dText(const btVector3& location, const char* text_string) override {
		if (renderer_) {
			renderer_->DrawText(ToGlm(location), std::string(text_string));
		}
	}

	void setDebugMode(int debug_mode) override { debug_mode_ = debug_mode; }

	[[nodiscard]] int getDebugMode() const override { return debug_mode_; }

private:
	IPhysicsDebugRenderer* renderer_{nullptr};
	int debug_mode_{DBG_DrawWireframe | DBG_DrawConstraints | DBG_DrawContactPoints};
};

} // namespace engine::physics
