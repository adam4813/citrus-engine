#include <fstream>
#include <nlohmann/json.hpp>
#include <variant>

import engine.animation;
import engine.platform;
import glm;

namespace engine::animation {

using json = nlohmann::json;

// === ANIMATED VALUE SERIALIZATION ===

json AnimationSerializer::AnimatedValueToJson(const AnimatedValue& value) {
	return std::visit(
			[](auto&& val) -> json {
				using T = std::decay_t<decltype(val)>;

				if constexpr (std::is_same_v<T, float>) {
					return json{{"type", "float"}, {"value", val}};
				}
				else if constexpr (std::is_same_v<T, glm::vec2>) {
					return json{{"type", "vec2"}, {"value", {val.x, val.y}}};
				}
				else if constexpr (std::is_same_v<T, glm::vec3>) {
					return json{{"type", "vec3"}, {"value", {val.x, val.y, val.z}}};
				}
				else if constexpr (std::is_same_v<T, glm::vec4>) {
					return json{{"type", "vec4"}, {"value", {val.x, val.y, val.z, val.w}}};
				}
				else if constexpr (std::is_same_v<T, glm::quat>) {
					return json{{"type", "quat"}, {"value", {val.w, val.x, val.y, val.z}}};
				}
				return json{};
			},
			value);
}

AnimatedValue AnimationSerializer::AnimatedValueFromJson(const json& j) {
	const std::string type = j.at("type").get<std::string>();

	if (type == "float") {
		return j.at("value").get<float>();
	}
	else if (type == "vec2") {
		auto arr = j.at("value").get<std::vector<float>>();
		return glm::vec2(arr[0], arr[1]);
	}
	else if (type == "vec3") {
		auto arr = j.at("value").get<std::vector<float>>();
		return glm::vec3(arr[0], arr[1], arr[2]);
	}
	else if (type == "vec4") {
		auto arr = j.at("value").get<std::vector<float>>();
		return glm::vec4(arr[0], arr[1], arr[2], arr[3]);
	}
	else if (type == "quat") {
		auto arr = j.at("value").get<std::vector<float>>();
		return glm::quat(arr[0], arr[1], arr[2], arr[3]);
	}

	return 0.0f; // Default
}

// === KEYFRAME SERIALIZATION ===

json AnimationSerializer::KeyframeToJson(const Keyframe& keyframe) {
	return json{{"time", keyframe.time}, {"value", AnimatedValueToJson(keyframe.value)}};
}

Keyframe AnimationSerializer::KeyframeFromJson(const json& j) {
	Keyframe kf;
	kf.time = j.at("time").get<float>();
	kf.value = AnimatedValueFromJson(j.at("value"));
	return kf;
}

// === TRACK SERIALIZATION ===

json AnimationSerializer::TrackToJson(const AnimationTrack& track) {
	json keyframes_json = json::array();
	for (const auto& kf : track.keyframes) {
		keyframes_json.push_back(KeyframeToJson(kf));
	}

	std::string interp_str;
	switch (track.interpolation) {
	case InterpolationMode::Step: interp_str = "step"; break;
	case InterpolationMode::Linear: interp_str = "linear"; break;
	case InterpolationMode::Cubic: interp_str = "cubic"; break;
	}

	return json{{"property", track.target_property}, {"interpolation", interp_str}, {"keyframes", keyframes_json}};
}

AnimationTrack AnimationSerializer::TrackFromJson(const json& j) {
	AnimationTrack track;
	track.target_property = j.at("property").get<std::string>();

	const std::string interp = j.at("interpolation").get<std::string>();
	if (interp == "step") {
		track.interpolation = InterpolationMode::Step;
	}
	else if (interp == "linear") {
		track.interpolation = InterpolationMode::Linear;
	}
	else if (interp == "cubic") {
		track.interpolation = InterpolationMode::Cubic;
	}

	for (const auto& kf_json : j.at("keyframes")) {
		track.keyframes.push_back(KeyframeFromJson(kf_json));
	}

	return track;
}

// === CLIP SERIALIZATION ===

json AnimationSerializer::ToJson(const AnimationClip& clip) {
	json tracks_json = json::array();
	for (const auto& track : clip.tracks) {
		tracks_json.push_back(TrackToJson(track));
	}

	return json{{"name", clip.name}, {"duration", clip.duration}, {"looping", clip.looping}, {"tracks", tracks_json}};
}

std::shared_ptr<AnimationClip> AnimationSerializer::FromJson(const json& j) {
	auto clip = std::make_shared<AnimationClip>();
	clip->name = j.at("name").get<std::string>();
	clip->duration = j.at("duration").get<float>();
	clip->looping = j.at("looping").get<bool>();

	for (const auto& track_json : j.at("tracks")) {
		clip->tracks.push_back(TrackFromJson(track_json));
	}

	return clip;
}

// === FILE I/O ===

bool AnimationSerializer::SaveToFile(const AnimationClip& clip, const platform::fs::Path& path) {
	try {
		json j = ToJson(clip);

		std::ofstream file(path);
		if (!file.is_open()) {
			return false;
		}

		file << j.dump(2); // Pretty print with 2-space indent
		file.close();

		return true;
	}
	catch (...) {
		return false;
	}
}

std::shared_ptr<AnimationClip> AnimationSerializer::LoadFromFile(const platform::fs::Path& path) {
	try {
		std::ifstream file(path);
		if (!file.is_open()) {
			return nullptr;
		}

		json j;
		file >> j;
		file.close();

		return FromJson(j);
	}
	catch (...) {
		return nullptr;
	}
}

} // namespace engine::animation
