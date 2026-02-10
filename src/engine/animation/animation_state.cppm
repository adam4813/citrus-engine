module;

#include <memory>
#include <string>
#include <vector>

export module engine.animation.state;

export import engine.animation.clip;

export namespace engine::animation {

// === ANIMATION PLAYBACK STATE ===

// Tracks the current playback state of an animation
class AnimationState {
public:
	AnimationState() = default;
	explicit AnimationState(std::shared_ptr<AnimationClip> clip);

	// Playback control
	void Play();
	void Pause();
	void Stop();
	void Reset();

	// Update the animation state
	void Update(float dt);

	// Evaluate the current animation state
	void Evaluate(std::vector<std::pair<std::string, AnimatedValue>>& out_values) const;

	// Getters/Setters
	void SetClip(std::shared_ptr<AnimationClip> clip);
	[[nodiscard]] std::shared_ptr<AnimationClip> GetClip() const { return clip_; }

	void SetSpeed(float speed) { speed_ = speed; }
	[[nodiscard]] float GetSpeed() const { return speed_; }

	void SetLooping(bool looping) { is_looping_ = looping; }
	[[nodiscard]] bool IsLooping() const { return is_looping_; }

	void SetTime(float time);
	[[nodiscard]] float GetTime() const { return current_time_; }

	[[nodiscard]] bool IsPlaying() const { return is_playing_; }
	[[nodiscard]] bool HasFinished() const;

	// Get normalized playback position [0, 1]
	[[nodiscard]] float GetNormalizedTime() const;

private:
	std::shared_ptr<AnimationClip> clip_;
	float current_time_{0.0f};
	float speed_{1.0f};
	bool is_playing_{false};
	bool is_looping_{false};
};

} // namespace engine::animation
