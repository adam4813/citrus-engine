#include <flecs.h>
#include <memory>
#include <string>
#include <variant>
#include <vector>

import engine.animation;
import engine.ecs;
import engine.components;
import glm;

namespace engine::animation {

AnimationSystem::AnimationSystem(flecs::world& world) : world_(world) {
	// System is registered via Register() static method
}

void AnimationSystem::Register(const flecs::world& world) {
	// Register the animation update system
	world.system<Animator>("AnimationUpdateSystem")
			.kind(flecs::OnUpdate)
			.each([](flecs::iter itr, std::size_t index, Animator& animator) {
				const float dt = itr.delta_time();

				// Process any pending transitions
				ProcessTransitions(animator, dt);

				// Update current animation state
				animator.current_state.Update(dt);

				// Evaluate and apply animated values
				std::vector<std::pair<std::string, AnimatedValue>> values;
				animator.current_state.Evaluate(values);

				for (const auto& [property, value] : values) {
					ApplyAnimatedValue(itr.entity(index), property, value);
				}
			});
}

void AnimationSystem::Update(float dt) {
	// The system is automatically updated by flecs during world.progress()
	// This method is kept for explicit updates if needed
}

void AnimationSystem::ProcessTransitions(Animator& animator, float dt) {
	if (animator.transition_queue.empty()) {
		return;
	}

	// Handle blend timing
	if (animator.blend_duration > 0.0f) {
		animator.blend_time += dt;
		if (animator.blend_time >= animator.blend_duration) {
			// Blend complete
			animator.blend_weight = 1.0f;
			animator.blend_time = 0.0f;
			animator.blend_duration = 0.0f;
		}
		else {
			// Update blend weight
			animator.blend_weight = animator.blend_time / animator.blend_duration;
		}
		return;
	}

	// Check if current animation has finished (or can be interrupted)
	const bool can_transition = animator.transition_queue.front().interrupt_current
								|| animator.current_state.HasFinished() || !animator.current_state.IsPlaying();

	if (can_transition) {
		auto transition = animator.transition_queue.front();
		animator.transition_queue.pop();

		// Start blend if requested
		if (transition.blend_duration > 0.0f) {
			animator.blend_duration = transition.blend_duration;
			animator.blend_time = 0.0f;
			animator.blend_weight = 0.0f;
		}
		else {
			animator.blend_weight = 1.0f;
		}

		// Set new animation
		animator.current_state.SetClip(transition.target_clip);
		animator.current_state.Reset();
		animator.current_state.Play();
	}
}

void AnimationSystem::ApplyAnimatedValue(
		flecs::entity entity, const std::string& property_name, const AnimatedValue& value) {

	// Get Transform component if it exists
	auto& transform = entity.get_mut<components::Transform>();

	// Apply value based on property name
	std::visit(
			[&](auto&& val) {
				using T = std::decay_t<decltype(val)>;

				if (property_name == "position") {
					if constexpr (std::is_same_v<T, glm::vec3>) {
						transform.position = val;
					}
				}
				else if (property_name == "position.x") {
					if constexpr (std::is_same_v<T, float>) {
						transform.position.x = val;
					}
				}
				else if (property_name == "position.y") {
					if constexpr (std::is_same_v<T, float>) {
						transform.position.y = val;
					}
				}
				else if (property_name == "position.z") {
					if constexpr (std::is_same_v<T, float>) {
						transform.position.z = val;
					}
				}
				else if (property_name == "rotation") {
					if constexpr (std::is_same_v<T, glm::vec3>) {
						transform.rotation = val;
					}
				}
				else if (property_name == "rotation.x") {
					if constexpr (std::is_same_v<T, float>) {
						transform.rotation.x = val;
					}
				}
				else if (property_name == "rotation.y") {
					if constexpr (std::is_same_v<T, float>) {
						transform.rotation.y = val;
					}
				}
				else if (property_name == "rotation.z") {
					if constexpr (std::is_same_v<T, float>) {
						transform.rotation.z = val;
					}
				}
				else if (property_name == "scale") {
					if constexpr (std::is_same_v<T, glm::vec3>) {
						transform.scale = val;
					}
					else if constexpr (std::is_same_v<T, float>) {
						transform.scale = glm::vec3(val);
					}
				}
				else if (property_name == "scale.x") {
					if constexpr (std::is_same_v<T, float>) {
						transform.scale.x = val;
					}
				}
				else if (property_name == "scale.y") {
					if constexpr (std::is_same_v<T, float>) {
						transform.scale.y = val;
					}
				}
				else if (property_name == "scale.z") {
					if constexpr (std::is_same_v<T, float>) {
						transform.scale.z = val;
					}
				}
			},
			value);

	entity.modified<components::Transform>();
}

// === ANIMATION HELPERS ===

namespace animation_helpers {

std::shared_ptr<AnimationClip> CreatePositionAnimation(
		const std::string& name, const glm::vec3& start_pos, const glm::vec3& end_pos, float duration, bool looping) {

	auto clip = std::make_shared<AnimationClip>();
	clip->name = name;
	clip->duration = duration;
	clip->looping = looping;

	AnimationTrack track;
	track.target_property = "position";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, start_pos);
	track.AddKeyframe(duration, end_pos);

	clip->AddTrack(std::move(track));

	return clip;
}

std::shared_ptr<AnimationClip> CreateRotationAnimation(
		const std::string& name, const glm::vec3& start_rot, const glm::vec3& end_rot, float duration, bool looping) {

	auto clip = std::make_shared<AnimationClip>();
	clip->name = name;
	clip->duration = duration;
	clip->looping = looping;

	AnimationTrack track;
	track.target_property = "rotation";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, start_rot);
	track.AddKeyframe(duration, end_rot);

	clip->AddTrack(std::move(track));

	return clip;
}

std::shared_ptr<AnimationClip> CreateScaleAnimation(
		const std::string& name,
		const glm::vec3& start_scale,
		const glm::vec3& end_scale,
		float duration,
		bool looping) {

	auto clip = std::make_shared<AnimationClip>();
	clip->name = name;
	clip->duration = duration;
	clip->looping = looping;

	AnimationTrack track;
	track.target_property = "scale";
	track.interpolation = InterpolationMode::Linear;
	track.AddKeyframe(0.0f, start_scale);
	track.AddKeyframe(duration, end_scale);

	clip->AddTrack(std::move(track));

	return clip;
}

} // namespace animation_helpers

} // namespace engine::animation
