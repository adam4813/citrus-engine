#include <algorithm>
#include <cmath>
#include <string>
#include <variant>
#include <vector>

import engine.animation;
import glm;

namespace engine::animation {

// === ANIMATION TRACK IMPLEMENTATION ===

void AnimationTrack::AddKeyframe(float time, AnimatedValue value) {
	Keyframe kf{time, std::move(value)};

	// Insert in sorted order by time
	auto it = std::lower_bound(keyframes.begin(), keyframes.end(), kf, [](const Keyframe& a, const Keyframe& b) {
		return a.time < b.time;
	});

	keyframes.insert(it, kf);
}

float AnimationTrack::GetDuration() const {
	if (keyframes.empty()) {
		return 0.0f;
	}
	return keyframes.back().time;
}

// Helper to interpolate between two values based on type
namespace {
template <typename T> T Lerp(const T& a, const T& b, float t) { return a + (b - a) * t; }

// Cubic interpolation (Hermite spline)
template <typename T> T CubicInterpolate(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
	const float t2 = t * t;
	const float t3 = t2 * t;

	// Catmull-Rom spline coefficients
	const float a = -0.5f * t3 + t2 - 0.5f * t;
	const float b = 1.5f * t3 - 2.5f * t2 + 1.0f;
	const float c = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
	const float d = 0.5f * t3 - 0.5f * t2;

	return p0 * a + p1 * b + p2 * c + p3 * d;
}

AnimatedValue InterpolateValues(const AnimatedValue& v1, const AnimatedValue& v2, float t, InterpolationMode mode) {

	if (mode == InterpolationMode::Step) {
		return v1;
	}

	// Linear interpolation for all types
	return std::visit(
			[t](auto&& a, auto&& b) -> AnimatedValue {
				using T1 = std::decay_t<decltype(a)>;
				using T2 = std::decay_t<decltype(b)>;

				if constexpr (std::is_same_v<T1, T2>) {
					if constexpr (std::is_same_v<T1, glm::quat>) {
						// Use slerp for quaternions
						return glm::slerp(a, b, t);
					}
					else {
						// Linear interpolation for scalars and vectors
						return Lerp(a, b, t);
					}
				}
				else {
					// Type mismatch, return first value
					return a;
				}
			},
			v1,
			v2);
}
} // namespace

AnimatedValue AnimationTrack::Evaluate(float time) const {
	if (keyframes.empty()) {
		return 0.0f; // Default value
	}

	if (keyframes.size() == 1 || time <= keyframes.front().time) {
		return keyframes.front().value;
	}

	if (time >= keyframes.back().time) {
		return keyframes.back().value;
	}

	// Find the two keyframes to interpolate between
	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		const auto& kf1 = keyframes[i];
		const auto& kf2 = keyframes[i + 1];

		if (time >= kf1.time && time <= kf2.time) {
			// Calculate interpolation factor
			const float duration = kf2.time - kf1.time;
			const float t = (duration > 0.0f) ? (time - kf1.time) / duration : 0.0f;

			if (interpolation == InterpolationMode::Cubic && keyframes.size() >= 4) {
				// Use cubic interpolation with neighboring keyframes
				const auto& p0 = (i > 0) ? keyframes[i - 1].value : kf1.value;
				const auto& p1 = kf1.value;
				const auto& p2 = kf2.value;
				const auto& p3 = (i + 2 < keyframes.size()) ? keyframes[i + 2].value : kf2.value;

				// For now, fall back to linear for cubic (full cubic requires same type)
				// TODO: Implement proper cubic interpolation per type
				return InterpolateValues(p1, p2, t, InterpolationMode::Linear);
			}
			else {
				return InterpolateValues(kf1.value, kf2.value, t, interpolation);
			}
		}
	}

	return keyframes.back().value;
}

// === ANIMATION CLIP IMPLEMENTATION ===

void AnimationClip::AddTrack(AnimationTrack track) {
	tracks.push_back(std::move(track));
	UpdateDuration();
}

AnimationTrack* AnimationClip::FindTrack(const std::string& property_name) {
	for (auto& track : tracks) {
		if (track.target_property == property_name) {
			return &track;
		}
	}
	return nullptr;
}

const AnimationTrack* AnimationClip::FindTrack(const std::string& property_name) const {
	for (const auto& track : tracks) {
		if (track.target_property == property_name) {
			return &track;
		}
	}
	return nullptr;
}

void AnimationClip::EvaluateAll(float time, std::vector<std::pair<std::string, AnimatedValue>>& out_values) const {

	out_values.clear();
	out_values.reserve(tracks.size());

	for (const auto& track : tracks) {
		out_values.emplace_back(track.target_property, track.Evaluate(time));
	}
}

void AnimationClip::UpdateDuration() {
	duration = 0.0f;
	for (const auto& track : tracks) {
		const float track_duration = track.GetDuration();
		if (track_duration > duration) {
			duration = track_duration;
		}
	}
}

} // namespace engine::animation
