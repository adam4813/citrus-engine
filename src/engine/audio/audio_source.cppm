module;

#include <cstdint>

export module engine.audio.source;

import glm;

export namespace engine::audio {

// Play state for audio sources
enum class PlayState {
	Stopped,
	Playing,
	Paused
};

// AudioSource component for ECS - represents a sound emitter
struct AudioSource {
	uint32_t clip_id{0};       // ID of the audio clip to play
	float volume{1.0f};        // Volume multiplier (0.0 to 1.0+)
	float pitch{1.0f};         // Pitch multiplier (0.5 = half speed, 2.0 = double speed)
	bool looping{false};       // Whether the sound should loop
	bool spatial{false};       // Whether this is a 3D positional sound
	glm::vec3 position{0.0f};  // 3D position (only used if spatial=true)
	PlayState state{PlayState::Stopped};  // Current playback state
	uint32_t play_handle{0};   // Handle for tracking this playing instance

	AudioSource() = default;

	explicit AudioSource(uint32_t audio_clip_id) 
		: clip_id(audio_clip_id) {}

	AudioSource(uint32_t audio_clip_id, float vol, bool loop = false)
		: clip_id(audio_clip_id), volume(vol), looping(loop) {}
};

} // namespace engine::audio
