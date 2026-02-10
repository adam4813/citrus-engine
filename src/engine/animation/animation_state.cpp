#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

import engine.animation;

namespace engine::animation {

AnimationState::AnimationState(std::shared_ptr<AnimationClip> clip) : clip_(std::move(clip)) {
	if (clip_) {
		is_looping_ = clip_->looping;
	}
}

void AnimationState::Play() { is_playing_ = true; }

void AnimationState::Pause() { is_playing_ = false; }

void AnimationState::Stop() {
	is_playing_ = false;
	current_time_ = 0.0f;
}

void AnimationState::Reset() { current_time_ = 0.0f; }

void AnimationState::Update(float dt) {
	if (!is_playing_ || !clip_) {
		return;
	}

	current_time_ += dt * speed_;

	if (clip_->duration > 0.0f) {
		if (is_looping_) {
			// Wrap around for looping animations
			current_time_ = std::fmod(current_time_, clip_->duration);
			if (current_time_ < 0.0f) {
				current_time_ += clip_->duration;
			}
		}
		else {
			// Clamp for non-looping animations
			if (current_time_ >= clip_->duration) {
				current_time_ = clip_->duration;
				is_playing_ = false;
			}
			else if (current_time_ < 0.0f) {
				current_time_ = 0.0f;
				is_playing_ = false;
			}
		}
	}
}

void AnimationState::Evaluate(std::vector<std::pair<std::string, AnimatedValue>>& out_values) const {
	if (!clip_) {
		out_values.clear();
		return;
	}

	clip_->EvaluateAll(current_time_, out_values);
}

void AnimationState::SetClip(std::shared_ptr<AnimationClip> clip) {
	clip_ = std::move(clip);
	current_time_ = 0.0f;

	if (clip_) {
		is_looping_ = clip_->looping;
	}
}

void AnimationState::SetTime(float time) {
	current_time_ = time;

	if (clip_ && clip_->duration > 0.0f) {
		if (is_looping_) {
			current_time_ = std::fmod(current_time_, clip_->duration);
			if (current_time_ < 0.0f) {
				current_time_ += clip_->duration;
			}
		}
		else {
			current_time_ = std::clamp(current_time_, 0.0f, clip_->duration);
		}
	}
}

bool AnimationState::HasFinished() const {
	if (!clip_ || is_looping_) {
		return false;
	}

	return !is_playing_ && current_time_ >= clip_->duration;
}

float AnimationState::GetNormalizedTime() const {
	if (!clip_ || clip_->duration <= 0.0f) {
		return 0.0f;
	}

	return current_time_ / clip_->duration;
}

} // namespace engine::animation
