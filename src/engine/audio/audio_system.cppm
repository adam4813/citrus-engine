module;

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <spdlog/spdlog.h>

export module engine.audio.system;

export import engine.audio.clip;
export import engine.audio.source;
export import engine.audio.listener;

export namespace engine::audio {

// Audio system - stub implementation that logs actions
// Real audio backend (OpenAL, Web Audio) will be plugged in later
class AudioSystem {
public:
	AudioSystem() = default;
	~AudioSystem() = default;

	// Disable copy/move
	AudioSystem(const AudioSystem&) = delete;
	AudioSystem& operator=(const AudioSystem&) = delete;
	AudioSystem(AudioSystem&&) = delete;
	AudioSystem& operator=(AudioSystem&&) = delete;

	// Initialize the audio system
	bool Initialize() {
		spdlog::info("[Audio] Initializing audio system (stub)");
		initialized_ = true;
		return true;
	}

	// Shutdown the audio system
	void Shutdown() {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Shutting down audio system");
		clips_.clear();
		initialized_ = false;
	}

	// Update the audio system (called every frame)
	void Update(float dt) {
		if (!initialized_) {
			return;
		}
		// In a real implementation, this would update streaming, 3D audio, etc.
		// For now, just a stub
	}

	// Load an audio clip from a file path
	// Returns the clip ID on success, 0 on failure
	uint32_t LoadClip(const std::string& file_path) {
		if (!initialized_) {
			spdlog::warn("[Audio] Cannot load clip: audio system not initialized");
			return 0;
		}

		// Generate a new ID
		uint32_t clip_id = next_clip_id_++;
		
		// Create a stub clip with metadata
		AudioClip clip(clip_id, file_path);
		clip.is_loaded = true;
		clip.sample_rate = 44100;  // Stub values
		clip.channels = 2;
		clip.duration = 1.0f;

		clips_[clip_id] = clip;
		
		spdlog::info("[Audio] Loaded clip '{}' with ID {}", file_path, clip_id);
		return clip_id;
	}

	// Play a sound clip
	// Returns a handle for controlling the sound instance, 0 on failure
	uint32_t PlaySound(uint32_t clip_id, float volume = 1.0f, bool looping = false) {
		if (!initialized_) {
			spdlog::warn("[Audio] Cannot play sound: audio system not initialized");
			return 0;
		}

		auto it = clips_.find(clip_id);
		if (it == clips_.end()) {
			spdlog::warn("[Audio] Cannot play sound: clip ID {} not found", clip_id);
			return 0;
		}

		uint32_t handle = next_play_handle_++;
		spdlog::info("[Audio] Playing clip '{}' (ID: {}, handle: {}, volume: {:.2f}, looping: {})", 
		             it->second.file_path, clip_id, handle, volume, looping);
		
		return handle;
	}

	// Stop a playing sound
	void StopSound(uint32_t handle) {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Stopping sound with handle {}", handle);
	}

	// Set volume for a playing sound
	void SetVolume(uint32_t handle, float volume) {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Setting volume for handle {} to {:.2f}", handle, volume);
	}

	// Set pitch for a playing sound
	void SetPitch(uint32_t handle, float pitch) {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Setting pitch for handle {} to {:.2f}", handle, pitch);
	}

	// Pause a playing sound
	void PauseSound(uint32_t handle) {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Pausing sound with handle {}", handle);
	}

	// Resume a paused sound
	void ResumeSound(uint32_t handle) {
		if (!initialized_) {
			return;
		}
		spdlog::info("[Audio] Resuming sound with handle {}", handle);
	}

	// Get a loaded clip by ID
	const AudioClip* GetClip(uint32_t clip_id) const {
		auto it = clips_.find(clip_id);
		return it != clips_.end() ? &it->second : nullptr;
	}

	// Check if the audio system is initialized
	bool IsInitialized() const { return initialized_; }

	// Global accessor (singleton-like pattern used by other engine systems)
	static AudioSystem& Get() {
		static AudioSystem instance;
		return instance;
	}

private:
	bool initialized_{false};
	uint32_t next_clip_id_{1};
	uint32_t next_play_handle_{1};
	std::unordered_map<uint32_t, AudioClip> clips_;
};

} // namespace engine::audio
