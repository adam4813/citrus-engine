module;

#include <cstdint>
#include <string>

export module engine.audio.clip;

export namespace engine::audio {

// Audio clip metadata (no actual audio data loaded yet - stub implementation)
struct AudioClip {
	uint32_t id{0};           // Unique identifier for this clip
	std::string file_path;    // Path to the audio file
	float duration{0.0f};     // Duration in seconds
	uint32_t sample_rate{0};  // Sample rate (e.g., 44100)
	uint32_t channels{0};     // Number of channels (1=mono, 2=stereo)
	bool is_loaded{false};    // Whether the clip metadata has been loaded

	AudioClip() = default;
	
	explicit AudioClip(std::string path) 
		: file_path(std::move(path)) {}
	
	AudioClip(uint32_t clip_id, std::string path) 
		: id(clip_id), file_path(std::move(path)) {}
};

} // namespace engine::audio
