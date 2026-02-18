module;

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

export module engine.audio.system;

export import engine.audio.clip;
export import engine.audio.source;
export import engine.audio.listener;

export namespace engine::audio {

// Forward declaration for the miniaudio backend (PIMPL)
struct AudioBackend;

// Audio system backed by miniaudio for cross-platform audio playback
class AudioSystem {
public:
	AudioSystem();
	~AudioSystem();

	// Disable copy/move
	AudioSystem(const AudioSystem&) = delete;
	AudioSystem& operator=(const AudioSystem&) = delete;
	AudioSystem(AudioSystem&&) = delete;
	AudioSystem& operator=(AudioSystem&&) = delete;

	// Initialize the audio system
	bool Initialize();

	// Shutdown the audio system
	void Shutdown();

	// Update the audio system (called every frame for 3D audio sync)
	void Update(float dt);

	// Load an audio clip from a file path
	// Returns the clip ID on success, 0 on failure
	uint32_t LoadClip(const std::string& file_path);

	// Load an audio clip and associate it with a logical name (for asset reference resolution)
	// If the name is already loaded, returns the existing clip ID
	uint32_t LoadClipNamed(const std::string& name, const std::string& file_path);

	// Find a clip by its logical name (set via LoadClipNamed)
	// Returns the clip ID, or 0 if not found
	uint32_t FindClipByName(const std::string& name) const;

	// Unload a previously loaded audio clip by ID
	void UnloadClip(uint32_t clip_id);

	// Play a sound clip
	// Returns a handle for controlling the sound instance, 0 on failure
	uint32_t PlaySoundClip(uint32_t clip_id, float volume = 1.0f, bool looping = false);

	// Stop a playing sound
	void StopSound(uint32_t handle);

	// Stop all currently playing sounds
	void StopAllSounds();

	// Set volume for a playing sound
	void SetVolume(uint32_t handle, float volume);

	// Set pitch for a playing sound
	void SetPitch(uint32_t handle, float pitch);

	// Pause a playing sound
	void PauseSound(uint32_t handle);

	// Resume a paused sound
	void ResumeSound(uint32_t handle);

	// Check if a sound is still playing
	bool IsSoundPlaying(uint32_t handle) const;

	// Update 3D listener position and orientation
	void SetListenerPosition(const AudioListener& listener);

	// Update 3D source position for a playing sound
	void SetSourcePosition(uint32_t handle, float x, float y, float z);

	// Get a loaded clip by ID
	const AudioClip* GetClip(uint32_t clip_id) const;

	// Check if the audio system is initialized
	bool IsInitialized() const;

	// Global accessor (singleton-like pattern used by other engine systems)
	static AudioSystem& Get();

private:
	bool initialized_{false};
	uint32_t next_clip_id_{1};
	uint32_t next_play_handle_{1};
	std::unordered_map<uint32_t, AudioClip> clips_;
	std::unordered_map<std::string, uint32_t> named_clips_; // name → clip_id
	std::unique_ptr<AudioBackend> backend_;
};

} // namespace engine::audio
