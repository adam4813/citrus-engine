#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

namespace editor {

/**
 * @brief Sound effect synthesis editor panel (sfxr/bfxr-style)
 * 
 * Features:
 * - Waveform synthesis parameters (oscillator, envelope, effects)
 * - Preset randomization buttons (Pickup, Laser, Explosion, etc.)
 * - Waveform visualizer (oscilloscope-like preview)
 * - Save/Load sound presets to .sfx.json
 * - Export to WAV (future)
 */
class SoundEditorPanel {
public:
	SoundEditorPanel();
	~SoundEditorPanel();

	/**
	 * @brief Render the sound editor panel
	 */
	void Render();

	/**
	 * @brief Check if panel is visible
	 */
	[[nodiscard]] bool IsVisible() const { return is_visible_; }

	/**
	 * @brief Set panel visibility
	 */
	void SetVisible(bool visible) { is_visible_ = visible; }

	/**
	 * @brief Get mutable reference to visibility (for ImGui::MenuItem binding)
	 */
	bool& VisibleRef() { return is_visible_; }

	/**
	 * @brief Create a new empty sound preset
	 */
	void NewSound();

	/**
	 * @brief Open a sound preset from file
	 */
	bool OpenSound(const std::string& path);

	/**
	 * @brief Save the current sound preset to file
	 */
	bool SaveSound(const std::string& path);

private:
	// ========================================================================
	// Rendering Methods
	// ========================================================================
	void RenderMenuBar();
	void RenderWaveformSelector();
	void RenderFrequencyControls();
	void RenderEnvelopeControls();
	void RenderEffectControls();
	void RenderVolumeControls();
	void RenderPresetButtons();
	void RenderTransportControls();
	void RenderWaveformVisualizer();

	// ========================================================================
	// Preset Randomization
	// ========================================================================
	void RandomizePickup();
	void RandomizeLaser();
	void RandomizeExplosion();
	void RandomizeJump();
	void RandomizeHit();
	void RandomizeAll();

	// ========================================================================
	// Waveform Generation (for visualization)
	// ========================================================================
	void GenerateWaveform(std::vector<float>& samples, int sample_count);
	float GenerateOscillatorSample(float phase) const;

	// ========================================================================
	// File I/O
	// ========================================================================
	bool LoadPresetFromJson(const std::string& path);
	bool SavePresetToJson(const std::string& path);

	// ========================================================================
	// Sound Synthesis Parameters
	// ========================================================================

	enum class WaveformType { Sine = 0, Square, Saw, Triangle, Noise };

	struct SoundPreset {
		// Oscillator
		WaveformType waveform = WaveformType::Square;
		float base_frequency = 440.0f; // Hz
		float frequency_min = 100.0f; // Hz (for slide range)
		float frequency_max = 1000.0f; // Hz (for slide range)
		float frequency_slide = 0.0f; // -1.0 to 1.0 (slide speed)

		// Envelope
		float attack_time = 0.0f; // seconds
		float sustain_time = 0.3f; // seconds
		float sustain_level = 1.0f; // 0.0 to 1.0
		float decay_time = 0.0f; // seconds

		// Vibrato
		float vibrato_depth = 0.0f; // 0.0 to 1.0
		float vibrato_speed = 0.0f; // Hz

		// Phaser
		float phaser_offset = 0.0f; // 0.0 to 1.0
		float phaser_sweep = 0.0f; // -1.0 to 1.0

		// Filter
		float lowpass_cutoff = 1.0f; // 0.0 to 1.0 (normalized)
		float lowpass_sweep = 0.0f; // -1.0 to 1.0
		float highpass_cutoff = 0.0f; // 0.0 to 1.0 (normalized)

		// Volume
		float master_volume = 0.5f; // 0.0 to 1.0
		float gain = 1.0f; // 0.0 to 2.0
	};

	// ========================================================================
	// State
	// ========================================================================
	bool is_visible_ = true;
	SoundPreset preset_;
	std::string preset_name_ = "Untitled";
	std::string current_file_path_;

	// Transport state
	bool is_playing_ = false;

	// Waveform visualization
	static constexpr int WAVEFORM_SAMPLE_COUNT = 512;
	std::vector<float> waveform_samples_;
};

} // namespace editor
