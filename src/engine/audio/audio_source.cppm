module;

#include <cstdint>
#include <string>

export module engine.audio.source;

import glm;

export namespace engine::audio {

// Play state for audio sources
enum class PlayState { Stopped, Playing, Paused };

// AudioSource component for ECS - represents a sound emitter
struct AudioSource {
	uint32_t clip_id{0}; // Runtime: resolved audio clip handle (managed by SoundRef observer)
	float volume{1.0f}; // Volume multiplier (0.0 to 1.0+)
	float pitch{1.0f}; // Pitch multiplier (0.5 = half speed, 2.0 = double speed)
	bool looping{false}; // Whether the sound should loop
	bool spatial{false}; // Whether this is a 3D positional sound
	glm::vec3 position{0.0f}; // 3D position (only used if spatial=true)
	PlayState state{PlayState::Stopped}; // Current playback state
	uint32_t play_handle{0}; // Runtime: handle for tracking this playing instance

	AudioSource() = default;

	explicit AudioSource(const uint32_t audio_clip_id) : clip_id(audio_clip_id) {}

	AudioSource(const uint32_t audio_clip_id, const float vol, const bool loop = false) :
			clip_id(audio_clip_id), volume(vol), looping(loop) {}
};

// Asset reference component for sound - stores sound asset name for serialization
// An observer resolves the name to a clip_id and updates AudioSource::clip_id
struct SoundRef {
	std::string name;
};

} // namespace engine::audio
