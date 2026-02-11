module;

#include <flecs.h>
#include <memory>
#include <string>
#include <variant>

export module engine.animation.system;

export import engine.animation.animator;
import engine.ecs;
import engine.components;
import glm;

export namespace engine::animation {

// === ANIMATION SYSTEM ===

// ECS system for updating and applying animations
class AnimationSystem {
public:
	explicit AnimationSystem(flecs::world& world);

	// Update all active animations
	void Update(float dt);

	// Register the animation system with ECS world
	static void Register(const flecs::world& world);

private:
	flecs::world& world_;

	// Apply an animated value to an entity component
	static void ApplyAnimatedValue(flecs::entity entity, const std::string& property_name, const AnimatedValue& value);

	// Process animation transitions
	static void ProcessTransitions(Animator& animator, float dt);
};

// Helper functions for creating common animation clips
namespace animation_helpers {
// Create a simple position animation
std::shared_ptr<AnimationClip> CreatePositionAnimation(
		const std::string& name,
		const glm::vec3& start_pos,
		const glm::vec3& end_pos,
		float duration,
		bool looping = false);

// Create a simple rotation animation
std::shared_ptr<AnimationClip> CreateRotationAnimation(
		const std::string& name,
		const glm::vec3& start_rot,
		const glm::vec3& end_rot,
		float duration,
		bool looping = false);

// Create a simple scale animation
std::shared_ptr<AnimationClip> CreateScaleAnimation(
		const std::string& name,
		const glm::vec3& start_scale,
		const glm::vec3& end_scale,
		float duration,
		bool looping = false);
} // namespace animation_helpers

} // namespace engine::animation
