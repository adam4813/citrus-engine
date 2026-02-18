module;

#include <cstdint>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

#ifdef __EMSCRIPTEN__
#define MA_ENABLE_AUDIO_WORKLETS
#define MA_NO_RUNTIME_LINKING
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <spdlog/spdlog.h>

module engine.audio.system;

import engine.platform;

namespace engine::audio {

// Internal sound instance tracked per play handle
struct SoundInstance {
	ma_sound sound;
	uint32_t clip_id{0};
	bool valid{false};
	bool paused{false};
	ma_uint64 pause_cursor{0};
};

// PIMPL backend holding the miniaudio engine and active sound instances
struct AudioBackend {
	ma_engine engine{};
	bool engine_initialized{false};
	std::unordered_map<uint32_t, std::unique_ptr<SoundInstance>> sounds;

	~AudioBackend() {
		// Stop and uninitialize all sounds before engine shutdown
		for (const auto& instance : sounds | std::views::values) {
			if (instance && instance->valid) {
				ma_sound_uninit(&instance->sound);
			}
		}
		sounds.clear();

		if (engine_initialized) {
			ma_engine_uninit(&engine);
		}
	}
};

// === AudioSystem implementation ===

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() { Shutdown(); }

AudioSystem& AudioSystem::Get() {
	static AudioSystem instance;
	return instance;
}

bool AudioSystem::IsInitialized() const { return initialized_; }

const AudioClip* AudioSystem::GetClip(const uint32_t clip_id) const {
	const auto it = clips_.find(clip_id);
	return it != clips_.end() ? &it->second : nullptr;
}

bool AudioSystem::Initialize() {
	if (initialized_) {
		return true;
	}

	backend_ = std::make_unique<AudioBackend>();

	ma_engine_config config = ma_engine_config_init();
	config.channels = 2;
	config.sampleRate = 0; // Use device default
#ifdef __EMSCRIPTEN__
	config.noDevice = MA_TRUE; // Emscripten uses AudioWorklets; avoid native device init
#endif

	const ma_result result = ma_engine_init(&config, &backend_->engine);
	if (result != MA_SUCCESS) {
		spdlog::error("[Audio] Failed to initialize miniaudio engine (error: {})", static_cast<int>(result));
		backend_.reset();
		return false;
	}

	backend_->engine_initialized = true;
	initialized_ = true;
	spdlog::info("[Audio] Audio system initialized (miniaudio)");
	return true;
}

void AudioSystem::Shutdown() {
	if (!initialized_) {
		return;
	}

	spdlog::info("[Audio] Shutting down audio system");
	backend_.reset();
	clips_.clear();
	initialized_ = false;
}

void AudioSystem::Update([[maybe_unused]] float dt) {
	if (!initialized_ || !backend_) {
		return;
	}

	// Clean up finished (non-looping) sound instances
	for (auto it = backend_->sounds.begin(); it != backend_->sounds.end();) {
		const auto& instance = it->second;
		if (instance && instance->valid && ma_sound_at_end(&instance->sound)) {
			ma_sound_uninit(&instance->sound);
			it = backend_->sounds.erase(it);
		}
		else {
			++it;
		}
	}
}

uint32_t AudioSystem::LoadClip(const std::string& file_path) {
	if (!initialized_ || !backend_) {
		spdlog::warn("[Audio] Cannot load clip: audio system not initialized");
		return 0;
	}

	// Resolve asset-relative paths through the platform file system
	const auto resolved_path = (platform::fs::GetAssetsDirectory() / file_path).string();

	// Try resolved path first, fall back to raw path (may be absolute already)
	ma_decoder decoder;
	const ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 0, 0);
	std::string actual_path = resolved_path;
	if (ma_result result = ma_decoder_init_file(resolved_path.c_str(), &decoder_config, &decoder);
		result != MA_SUCCESS) {
		actual_path = file_path;
		result = ma_decoder_init_file(file_path.c_str(), &decoder_config, &decoder);
		if (result != MA_SUCCESS) {
			spdlog::error("[Audio] Failed to load audio file '{}' (error: {})", file_path, static_cast<int>(result));
			return 0;
		}
	}

	uint32_t clip_id = next_clip_id_++;

	AudioClip clip(clip_id, actual_path);
	clip.sample_rate = decoder.outputSampleRate;
	clip.channels = decoder.outputChannels;
	clip.is_loaded = true;

	// Compute duration from total frame count
	ma_uint64 total_frames = 0;
	if (ma_decoder_get_length_in_pcm_frames(&decoder, &total_frames) == MA_SUCCESS && total_frames > 0
		&& clip.sample_rate > 0) {
		clip.duration = static_cast<float>(total_frames) / static_cast<float>(clip.sample_rate);
	}
	else {
		clip.duration = 0.0f;
	}

	ma_decoder_uninit(&decoder);

	clips_[clip_id] = clip;
	spdlog::info(
			"[Audio] Loaded clip '{}' (ID: {}, {}Hz, {}ch, {:.2f}s)",
			file_path,
			clip_id,
			clip.sample_rate,
			clip.channels,
			clip.duration);
	return clip_id;
}

uint32_t AudioSystem::LoadClipNamed(const std::string& name, const std::string& file_path) {
	// Return existing clip if already loaded by this name
	if (const auto it = named_clips_.find(name); it != named_clips_.end()) {
		return it->second;
	}

	const uint32_t clip_id = LoadClip(file_path);
	if (clip_id != 0) {
		named_clips_[name] = clip_id;
	}
	return clip_id;
}

uint32_t AudioSystem::FindClipByName(const std::string& name) const {
	if (const auto it = named_clips_.find(name); it != named_clips_.end()) {
		return it->second;
	}
	return 0;
}

void AudioSystem::UnloadClip(uint32_t clip_id) {
	if (clip_id == 0) {
		return;
	}

	// Stop and remove any active sound instances using this clip
	if (backend_) {
		for (auto it = backend_->sounds.begin(); it != backend_->sounds.end();) {
			if (it->second && it->second->clip_id == clip_id) {
				if (it->second->valid) {
					ma_sound_stop(&it->second->sound);
					ma_sound_uninit(&it->second->sound);
				}
				it = backend_->sounds.erase(it);
			}
			else {
				++it;
			}
		}
	}

	if (const auto it = clips_.find(clip_id); it != clips_.end()) {
		spdlog::debug("[Audio] Unloaded clip '{}' (ID: {})", it->second.file_path, clip_id);
		// Remove from named clips map
		std::erase_if(named_clips_, [clip_id](const auto& pair) { return pair.second == clip_id; });
		clips_.erase(it);
	}
}

uint32_t AudioSystem::PlaySoundClip(uint32_t clip_id, float volume, bool looping) {
	if (!initialized_ || !backend_) {
		spdlog::warn("[Audio] Cannot play sound: audio system not initialized");
		return 0;
	}

	const auto clip_it = clips_.find(clip_id);
	if (clip_it == clips_.end()) {
		spdlog::warn("[Audio] Cannot play sound: clip ID {} not found", clip_id);
		return 0;
	}

	uint32_t handle = next_play_handle_++;
	auto instance = std::make_unique<SoundInstance>();
	instance->clip_id = clip_id;

	ma_result result = ma_sound_init_from_file(
			&backend_->engine,
			clip_it->second.file_path.c_str(),
			MA_SOUND_FLAG_DECODE,
			nullptr,
			nullptr,
			&instance->sound);
	if (result != MA_SUCCESS) {
		spdlog::error(
				"[Audio] Failed to create sound from clip '{}' (error: {})",
				clip_it->second.file_path,
				static_cast<int>(result));
		return 0;
	}

	instance->valid = true;
	ma_sound_set_volume(&instance->sound, volume);
	ma_sound_set_looping(&instance->sound, looping ? MA_TRUE : MA_FALSE);

	result = ma_sound_start(&instance->sound);
	if (result != MA_SUCCESS) {
		spdlog::error("[Audio] Failed to start sound (handle: {}, error: {})", handle, static_cast<int>(result));
		ma_sound_uninit(&instance->sound);
		return 0;
	}

	backend_->sounds[handle] = std::move(instance);
	spdlog::debug(
			"[Audio] Playing clip '{}' (ID: {}, handle: {}, volume: {:.2f}, looping: {})",
			clip_it->second.file_path,
			clip_id,
			handle,
			volume,
			looping);
	return handle;
}

void AudioSystem::StopSound(uint32_t handle) {
	if (!initialized_ || !backend_) {
		return;
	}

	const auto it = backend_->sounds.find(handle);
	if (it == backend_->sounds.end() || !it->second || !it->second->valid) {
		return;
	}

	ma_sound_stop(&it->second->sound);
	ma_sound_uninit(&it->second->sound);
	backend_->sounds.erase(it);
	spdlog::debug("[Audio] Stopped sound (handle: {})", handle);
}

void AudioSystem::StopAllSounds() {
	if (!initialized_ || !backend_) {
		return;
	}

	for (const auto& instance : backend_->sounds | std::views::values) {
		if (instance && instance->valid) {
			ma_sound_stop(&instance->sound);
			ma_sound_uninit(&instance->sound);
		}
	}
	backend_->sounds.clear();
	spdlog::debug("[Audio] Stopped all sounds");
}

void AudioSystem::SetVolume(const uint32_t handle, const float volume) {
	if (!initialized_ || !backend_) {
		return;
	}

	if (const auto it = backend_->sounds.find(handle);
		it != backend_->sounds.end() && it->second && it->second->valid) {
		ma_sound_set_volume(&it->second->sound, volume);
	}
}

void AudioSystem::SetPitch(const uint32_t handle, const float pitch) {
	if (!initialized_ || !backend_) {
		return;
	}

	if (const auto it = backend_->sounds.find(handle);
		it != backend_->sounds.end() && it->second && it->second->valid) {
		ma_sound_set_pitch(&it->second->sound, pitch);
	}
}

void AudioSystem::PauseSound(uint32_t handle) {
	if (!initialized_ || !backend_) {
		return;
	}

	if (const auto it = backend_->sounds.find(handle);
		it != backend_->sounds.end() && it->second && it->second->valid) {
		if (!it->second->paused) {
			it->second->pause_cursor = 0;
			ma_sound_get_cursor_in_pcm_frames(&it->second->sound, &it->second->pause_cursor);
			ma_sound_stop(&it->second->sound);
			it->second->paused = true;
			spdlog::debug("[Audio] Paused sound (handle: {})", handle);
		}
	}
}

void AudioSystem::ResumeSound(uint32_t handle) {
	if (!initialized_ || !backend_) {
		return;
	}

	if (const auto it = backend_->sounds.find(handle);
		it != backend_->sounds.end() && it->second && it->second->valid) {
		if (it->second->paused) {
			ma_sound_seek_to_pcm_frame(&it->second->sound, it->second->pause_cursor);
			ma_sound_start(&it->second->sound);
			it->second->paused = false;
			spdlog::debug("[Audio] Resumed sound (handle: {})", handle);
		}
	}
}

bool AudioSystem::IsSoundPlaying(const uint32_t handle) const {
	if (!initialized_ || !backend_ || handle == 0) {
		return false;
	}
	const auto it = backend_->sounds.find(handle);
	if (it == backend_->sounds.end() || !it->second || !it->second->valid) {
		return false;
	}
	return !it->second->paused && !ma_sound_at_end(&it->second->sound);
}

void AudioSystem::SetListenerPosition(const AudioListener& listener) {
	if (!initialized_ || !backend_) {
		return;
	}

	ma_engine_listener_set_position(
			&backend_->engine, 0, listener.position.x, listener.position.y, listener.position.z);
	ma_engine_listener_set_direction(&backend_->engine, 0, listener.forward.x, listener.forward.y, listener.forward.z);
	ma_engine_listener_set_world_up(&backend_->engine, 0, listener.up.x, listener.up.y, listener.up.z);
}

void AudioSystem::SetSourcePosition(const uint32_t handle, const float x, const float y, const float z) {
	if (!initialized_ || !backend_) {
		return;
	}

	if (const auto it = backend_->sounds.find(handle);
		it != backend_->sounds.end() && it->second && it->second->valid) {
		ma_sound_set_spatialization_enabled(&it->second->sound, MA_TRUE);
		ma_sound_set_position(&it->second->sound, x, y, z);
	}
}

} // namespace engine::audio
