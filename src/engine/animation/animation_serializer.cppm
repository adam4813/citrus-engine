module;

#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <string>

export module engine.animation.serializer;

export import engine.animation.clip;

import engine.platform;

export namespace engine::animation {

// === ANIMATION SERIALIZATION ===

class AnimationSerializer {
public:
	// Serialize an AnimationClip to JSON
	static nlohmann::json ToJson(const AnimationClip& clip);
	
	// Deserialize an AnimationClip from JSON
	static std::shared_ptr<AnimationClip> FromJson(const nlohmann::json& json);
	
	// Save an AnimationClip to a file
	static bool SaveToFile(const AnimationClip& clip, const platform::fs::Path& path);
	
	// Load an AnimationClip from a file
	static std::shared_ptr<AnimationClip> LoadFromFile(const platform::fs::Path& path);
	
private:
	// Helper: Serialize a Keyframe
	static nlohmann::json KeyframeToJson(const Keyframe& keyframe);
	
	// Helper: Deserialize a Keyframe
	static Keyframe KeyframeFromJson(const nlohmann::json& json);
	
	// Helper: Serialize an AnimationTrack
	static nlohmann::json TrackToJson(const AnimationTrack& track);
	
	// Helper: Deserialize an AnimationTrack
	static AnimationTrack TrackFromJson(const nlohmann::json& json);
	
	// Helper: Serialize AnimatedValue
	static nlohmann::json AnimatedValueToJson(const AnimatedValue& value);
	
	// Helper: Deserialize AnimatedValue
	static AnimatedValue AnimatedValueFromJson(const nlohmann::json& json);
};

} // namespace engine::animation
