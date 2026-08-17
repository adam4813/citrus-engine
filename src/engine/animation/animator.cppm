module;

#include <memory>
#include <queue>
#include <string>

export module engine.animation.animator;

export import engine.animation.state;

export namespace engine::animation {

// === ANIMATION TRANSITION ===

// Represents a queued transition to a new animation
struct AnimationTransition {
	std::shared_ptr<AnimationClip> target_clip;
	float blend_duration{0.0F};   // Duration of blend/crossfade
	float delay{0.0F};            // Delay before starting transition
	bool interrupt_current{true}; // Whether to interrupt current animation

	AnimationTransition() = default;
	AnimationTransition(std::shared_ptr<AnimationClip> clip, float blend = 0.0F, float d = 0.0F) :
			target_clip(std::move(clip)), blend_duration(blend), delay(d) {}
};

// === ANIMATOR COMPONENT ===

// ECS component for controlling entity animations
struct Animator {
	AnimationState current_state;                     // Current animation state
	std::queue<AnimationTransition> transition_queue; // Queued transitions

	// Blending parameters (for future use)
	float blend_weight{1.0F};   // Weight for current animation [0, 1]
	float blend_time{0.0F};     // Current blend time
	float blend_duration{0.0F}; // Total blend duration

	// Queue a transition to a new animation
	void QueueTransition(const AnimationTransition& transition) { transition_queue.push(transition); }

	// Queue a simple animation change
	void QueueAnimation(std::shared_ptr<AnimationClip> clip, float blend_duration = 0.0F) {
		transition_queue.emplace(std::move(clip), blend_duration, 0.0F);
	}

	// Clear all queued transitions
	void ClearQueue() {
		while (!transition_queue.empty()) {
			transition_queue.pop();
		}
	}

	// Check if there are pending transitions
	[[nodiscard]] bool HasPendingTransitions() const { return !transition_queue.empty(); }
};

} // namespace engine::animation
