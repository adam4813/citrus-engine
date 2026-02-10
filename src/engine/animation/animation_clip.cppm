module;

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

export module engine.animation.clip;

import glm;

export namespace engine::animation {

// === KEYFRAME TYPES ===

// Supported animated value types
using AnimatedValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, glm::quat>;

// Interpolation mode for keyframes
enum class InterpolationMode : uint8_t {
	Step,   // No interpolation, step to next value
	Linear, // Linear interpolation between keyframes
	Cubic   // Cubic interpolation (smooth)
};

// Single keyframe in an animation track
struct Keyframe {
	float time{0.0f};          // Time in seconds
	AnimatedValue value;       // Value at this keyframe
	
	Keyframe() = default;
	Keyframe(float t, AnimatedValue v) : time(t), value(std::move(v)) {}
};

// === ANIMATION TRACK ===

// A single animated property (e.g., "position.x", "rotation.z", "scale")
struct AnimationTrack {
	std::string target_property;           // Name of the property to animate
	std::vector<Keyframe> keyframes;       // Keyframes sorted by time
	InterpolationMode interpolation{InterpolationMode::Linear};
	
	// Add a keyframe (maintains sorted order)
	void AddKeyframe(float time, AnimatedValue value);
	
	// Evaluate the track at a specific time
	AnimatedValue Evaluate(float time) const;
	
	// Get the duration of this track
	[[nodiscard]] float GetDuration() const;
	
	// Clear all keyframes
	void Clear() { keyframes.clear(); }
	
	// Get number of keyframes
	[[nodiscard]] size_t GetKeyframeCount() const { return keyframes.size(); }
};

// === ANIMATION CLIP ===

// Complete animation clip containing multiple property tracks
struct AnimationClip {
	std::string name;                      // Name of the animation
	float duration{0.0f};                  // Total duration in seconds
	std::vector<AnimationTrack> tracks;    // All animated property tracks
	bool looping{false};                   // Whether to loop the animation
	
	// Add a track to the animation
	void AddTrack(AnimationTrack track);
	
	// Find a track by property name
	AnimationTrack* FindTrack(const std::string& property_name);
	const AnimationTrack* FindTrack(const std::string& property_name) const;
	
	// Evaluate all tracks at a specific time
	void EvaluateAll(float time, std::vector<std::pair<std::string, AnimatedValue>>& out_values) const;
	
	// Clear all tracks
	void Clear() { tracks.clear(); }
	
	// Get number of tracks
	[[nodiscard]] size_t GetTrackCount() const { return tracks.size(); }
	
	// Update duration based on tracks
	void UpdateDuration();
};

} // namespace engine::animation
