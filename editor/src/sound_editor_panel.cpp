#include "sound_editor_panel.h"
#include "asset_editor_registry.h"

#include <imgui.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>

import engine;

using json = nlohmann::json;

namespace editor {

SoundEditorPanel::SoundEditorPanel() {
	NewSound();
	waveform_samples_.resize(WAVEFORM_SAMPLE_COUNT);

	open_dialog_.SetCallback([this](const std::string& path) { OpenSound(path); });
	save_dialog_.SetCallback([this](const std::string& path) { SaveSound(path); });
	export_wav_dialog_.SetCallback([this](const std::string& path) { ExportWav(path); });
}

SoundEditorPanel::~SoundEditorPanel() { StopPreview(); }

std::string_view SoundEditorPanel::GetPanelName() const { return "Sound Editor"; }

void SoundEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("sound", [this](const std::string& path) {
		OpenSound(path);
		SetVisible(true);
	});
}

void SoundEditorPanel::Render() {
	if (!BeginPanel(ImGuiWindowFlags_MenuBar)) {
		return;
	}

	RenderMenuBar();

	// Main content area - split into left and right
	ImGui::BeginChild("LeftPane", ImVec2(ImGui::GetContentRegionAvail().x * 0.6f, 0), true);

	// Waveform selector
	RenderWaveformSelector();
	ImGui::Separator();

	// Parameter controls
	RenderFrequencyControls();
	ImGui::Separator();

	RenderEnvelopeControls();
	ImGui::Separator();

	RenderEffectControls();
	ImGui::Separator();

	RenderVolumeControls();

	ImGui::EndChild();

	ImGui::SameLine();

	// Right pane - Visualizer and controls
	ImGui::BeginChild("RightPane", ImVec2(0, 0), true);

	RenderPresetButtons();
	ImGui::Separator();

	RenderTransportControls();
	ImGui::Separator();

	RenderWaveformVisualizer();

	ImGui::EndChild();

	EndPanel();
}

void SoundEditorPanel::RenderMenuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewSound();
			}
			if (ImGui::MenuItem("Open...")) {
				open_dialog_.Open();
			}
			if (ImGui::MenuItem("Save", nullptr, false, !current_file_path_.empty())) {
				SaveSound(current_file_path_);
			}
			if (ImGui::MenuItem("Save As...")) {
				save_dialog_.Open("sound_preset.sfx.json");
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export WAV...")) {
				export_wav_dialog_.Open("sound.wav");
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	open_dialog_.Render();
	save_dialog_.Render();
	export_wav_dialog_.Render();
}

void SoundEditorPanel::RenderWaveformSelector() {
	ImGui::Text("Waveform Type");

	const char* waveform_names[] = {"Sine", "Square", "Saw", "Triangle", "Noise"};
	int current_waveform = static_cast<int>(preset_.waveform);

	if (ImGui::Combo("##Waveform", &current_waveform, waveform_names, IM_ARRAYSIZE(waveform_names))) {
		preset_.waveform = static_cast<WaveformType>(current_waveform);
		SetDirty(true);
	}
}

void SoundEditorPanel::RenderFrequencyControls() {
	ImGui::Text("Frequency");

	if (ImGui::SliderFloat("Base Frequency (Hz)", &preset_.base_frequency, 20.0f, 2000.0f, "%.1f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Min Frequency (Hz)", &preset_.frequency_min, 20.0f, 2000.0f, "%.1f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Max Frequency (Hz)", &preset_.frequency_max, 20.0f, 2000.0f, "%.1f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Frequency Slide", &preset_.frequency_slide, -1.0f, 1.0f, "%.3f"))
		SetDirty(true);
}

void SoundEditorPanel::RenderEnvelopeControls() {
	ImGui::Text("Envelope");

	if (ImGui::SliderFloat("Attack Time (s)", &preset_.attack_time, 0.0f, 1.0f, "%.3f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Sustain Time (s)", &preset_.sustain_time, 0.0f, 2.0f, "%.3f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Sustain Level", &preset_.sustain_level, 0.0f, 1.0f, "%.3f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Decay Time (s)", &preset_.decay_time, 0.0f, 2.0f, "%.3f"))
		SetDirty(true);
}

void SoundEditorPanel::RenderEffectControls() {
	ImGui::Text("Effects");

	if (ImGui::TreeNode("Vibrato")) {
		if (ImGui::SliderFloat("Depth##Vibrato", &preset_.vibrato_depth, 0.0f, 1.0f, "%.3f"))
			SetDirty(true);
		if (ImGui::SliderFloat("Speed (Hz)##Vibrato", &preset_.vibrato_speed, 0.0f, 20.0f, "%.1f"))
			SetDirty(true);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Phaser")) {
		if (ImGui::SliderFloat("Offset##Phaser", &preset_.phaser_offset, 0.0f, 1.0f, "%.3f"))
			SetDirty(true);
		if (ImGui::SliderFloat("Sweep##Phaser", &preset_.phaser_sweep, -1.0f, 1.0f, "%.3f"))
			SetDirty(true);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Filter")) {
		if (ImGui::SliderFloat("Low-pass Cutoff", &preset_.lowpass_cutoff, 0.0f, 1.0f, "%.3f"))
			SetDirty(true);
		if (ImGui::SliderFloat("Low-pass Sweep", &preset_.lowpass_sweep, -1.0f, 1.0f, "%.3f"))
			SetDirty(true);
		if (ImGui::SliderFloat("High-pass Cutoff", &preset_.highpass_cutoff, 0.0f, 1.0f, "%.3f"))
			SetDirty(true);
		ImGui::TreePop();
	}
}

void SoundEditorPanel::RenderVolumeControls() {
	ImGui::Text("Volume");

	if (ImGui::SliderFloat("Master Volume", &preset_.master_volume, 0.0f, 1.0f, "%.3f"))
		SetDirty(true);
	if (ImGui::SliderFloat("Gain", &preset_.gain, 0.0f, 2.0f, "%.3f"))
		SetDirty(true);
}

void SoundEditorPanel::RenderPresetButtons() {
	ImGui::Text("Randomize Presets");

	const float button_width = ImGui::GetContentRegionAvail().x;

	if (ImGui::Button("Pickup/Coin", ImVec2(button_width, 0))) {
		RandomizePickup();
	}
	if (ImGui::Button("Laser/Shoot", ImVec2(button_width, 0))) {
		RandomizeLaser();
	}
	if (ImGui::Button("Explosion", ImVec2(button_width, 0))) {
		RandomizeExplosion();
	}
	if (ImGui::Button("Jump", ImVec2(button_width, 0))) {
		RandomizeJump();
	}
	if (ImGui::Button("Hit/Hurt", ImVec2(button_width, 0))) {
		RandomizeHit();
	}

	ImGui::Separator();

	if (ImGui::Button("Randomize All", ImVec2(button_width, 0))) {
		RandomizeAll();
	}
}

void SoundEditorPanel::RenderTransportControls() {
	ImGui::Text("Transport");

	// Auto-reset playing state when sound finishes naturally
	if (is_playing_ && playback_handle_ != 0) {
		auto& audio = engine::audio::AudioSystem::Get();
		if (!audio.IsSoundPlaying(playback_handle_)) {
			is_playing_ = false;
			playback_handle_ = 0;
		}
	}

	const float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;

	if (!is_playing_) {
		if (ImGui::Button("Play", ImVec2(button_width, 0))) {
			RegeneratePreview();
			PlayPreview();
		}
	}
	else {
		if (ImGui::Button("Stop", ImVec2(button_width, 0))) {
			StopPreview();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Generate", ImVec2(button_width, 0))) {
		RegeneratePreview();
	}

	ImGui::SameLine();

	const bool has_export = !export_wav_path_.empty();
	if (ImGui::Button(has_export ? "Re-export WAV" : "Export WAV...", ImVec2(button_width, 0))) {
		if (has_export) {
			ExportWav(export_wav_path_);
		}
		else {
			export_wav_dialog_.Open("sound.wav");
		}
	}
}

void SoundEditorPanel::RenderWaveformVisualizer() {
	ImGui::Text("Waveform Preview");

	// Generate waveform if not yet generated
	if (waveform_samples_.empty() || waveform_samples_.size() != WAVEFORM_SAMPLE_COUNT) {
		RegeneratePreview();
	}

	// Draw waveform using ImGui draw list
	const ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200.0f);
	const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_p1(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Background
	draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(20, 20, 20, 255));

	// Grid lines
	constexpr int grid_count = 8;
	for (int i = 0; i <= grid_count; ++i) {
		const float y = canvas_p0.y + (canvas_size.y * i) / grid_count;
		draw_list->AddLine(ImVec2(canvas_p0.x, y), ImVec2(canvas_p1.x, y), IM_COL32(40, 40, 40, 255));
	}

	// Center line (zero amplitude)
	const float center_y = canvas_p0.y + canvas_size.y * 0.5f;
	draw_list->AddLine(ImVec2(canvas_p0.x, center_y), ImVec2(canvas_p1.x, center_y), IM_COL32(80, 80, 80, 255), 2.0f);

	// Waveform
	if (!waveform_samples_.empty()) {
		const float x_step = canvas_size.x / static_cast<float>(WAVEFORM_SAMPLE_COUNT - 1);
		const float y_scale = canvas_size.y * 0.4f; // Scale to fit (leaving margins)

		for (int i = 0; i < WAVEFORM_SAMPLE_COUNT - 1; ++i) {
			const float x0 = canvas_p0.x + i * x_step;
			const float x1 = canvas_p0.x + (i + 1) * x_step;
			const float y0 = center_y - waveform_samples_[i] * y_scale;
			const float y1 = center_y - waveform_samples_[i + 1] * y_scale;

			draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 255, 0, 255), 1.5f);
		}
	}

	// Border
	draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 128));

	// Dummy widget to claim space
	ImGui::Dummy(canvas_size);
}

// ========================================================================
// Preset Randomization
// ========================================================================

void SoundEditorPanel::RandomizePickup() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform = WaveformType::Square;
	preset_.base_frequency = std::uniform_real_distribution<float>(600.0f, 1200.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(0.2f, 0.8f)(gen);
	preset_.attack_time = 0.0f;
	preset_.sustain_time = std::uniform_real_distribution<float>(0.1f, 0.3f)(gen);
	preset_.sustain_level = 1.0f;
	preset_.decay_time = std::uniform_real_distribution<float>(0.1f, 0.2f)(gen);
	preset_.master_volume = 0.5f;
	SetDirty(true);

	RegeneratePreview();
}

void SoundEditorPanel::RandomizeLaser() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform = static_cast<WaveformType>(std::uniform_int_distribution<int>(0, 2)(gen)); // Sine, Square, Saw
	preset_.base_frequency = std::uniform_real_distribution<float>(800.0f, 1500.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(-0.8f, -0.3f)(gen); // Downward
	preset_.attack_time = 0.0f;
	preset_.sustain_time = std::uniform_real_distribution<float>(0.2f, 0.5f)(gen);
	preset_.sustain_level = 1.0f;
	preset_.decay_time = std::uniform_real_distribution<float>(0.1f, 0.3f)(gen);
	preset_.master_volume = 0.5f;
	SetDirty(true);

	RegeneratePreview();
}

void SoundEditorPanel::RandomizeExplosion() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform = WaveformType::Noise;
	preset_.base_frequency = std::uniform_real_distribution<float>(50.0f, 150.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(-0.3f, 0.0f)(gen);
	preset_.attack_time = 0.0f;
	preset_.sustain_time = std::uniform_real_distribution<float>(0.3f, 0.8f)(gen);
	preset_.sustain_level = 1.0f;
	preset_.decay_time = std::uniform_real_distribution<float>(0.5f, 1.5f)(gen);
	preset_.lowpass_sweep = std::uniform_real_distribution<float>(-0.5f, -0.2f)(gen);
	preset_.master_volume = 0.6f;
	SetDirty(true);

	RegeneratePreview();
}

void SoundEditorPanel::RandomizeJump() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform = WaveformType::Square;
	preset_.base_frequency = std::uniform_real_distribution<float>(300.0f, 600.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(0.3f, 0.7f)(gen); // Upward
	preset_.attack_time = 0.0f;
	preset_.sustain_time = std::uniform_real_distribution<float>(0.15f, 0.35f)(gen);
	preset_.sustain_level = 1.0f;
	preset_.decay_time = std::uniform_real_distribution<float>(0.1f, 0.2f)(gen);
	preset_.master_volume = 0.5f;
	SetDirty(true);

	RegeneratePreview();
}

void SoundEditorPanel::RandomizeHit() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform =
			static_cast<WaveformType>(std::uniform_int_distribution<int>(1, 3)(gen)); // Square, Saw, Triangle
	preset_.base_frequency = std::uniform_real_distribution<float>(200.0f, 500.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(-0.4f, 0.1f)(gen);
	preset_.attack_time = 0.0f;
	preset_.sustain_time = std::uniform_real_distribution<float>(0.05f, 0.15f)(gen);
	preset_.sustain_level = 1.0f;
	preset_.decay_time = std::uniform_real_distribution<float>(0.05f, 0.15f)(gen);
	preset_.master_volume = 0.5f;
	SetDirty(true);

	RegeneratePreview();
}

void SoundEditorPanel::RandomizeAll() {
	static std::random_device rd;
	static std::mt19937 gen(rd());

	preset_.waveform = static_cast<WaveformType>(std::uniform_int_distribution<int>(0, 4)(gen));
	preset_.base_frequency = std::uniform_real_distribution<float>(50.0f, 2000.0f)(gen);
	preset_.frequency_min = std::uniform_real_distribution<float>(20.0f, 1000.0f)(gen);
	preset_.frequency_max = std::uniform_real_distribution<float>(preset_.frequency_min, 2000.0f)(gen);
	preset_.frequency_slide = std::uniform_real_distribution<float>(-1.0f, 1.0f)(gen);

	preset_.attack_time = std::uniform_real_distribution<float>(0.0f, 0.5f)(gen);
	preset_.sustain_time = std::uniform_real_distribution<float>(0.05f, 1.0f)(gen);
	preset_.sustain_level = std::uniform_real_distribution<float>(0.3f, 1.0f)(gen);
	preset_.decay_time = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);

	preset_.vibrato_depth = std::uniform_real_distribution<float>(0.0f, 0.5f)(gen);
	preset_.vibrato_speed = std::uniform_real_distribution<float>(0.0f, 10.0f)(gen);

	preset_.phaser_offset = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
	preset_.phaser_sweep = std::uniform_real_distribution<float>(-0.5f, 0.5f)(gen);

	preset_.lowpass_cutoff = std::uniform_real_distribution<float>(0.3f, 1.0f)(gen);
	preset_.lowpass_sweep = std::uniform_real_distribution<float>(-0.5f, 0.5f)(gen);
	preset_.highpass_cutoff = std::uniform_real_distribution<float>(0.0f, 0.3f)(gen);

	preset_.master_volume = std::uniform_real_distribution<float>(0.3f, 0.7f)(gen);
	preset_.gain = std::uniform_real_distribution<float>(0.8f, 1.5f)(gen);
	SetDirty(true);

	RegeneratePreview();
}

// ========================================================================
// Waveform Generation
// ========================================================================

void SoundEditorPanel::RegeneratePreview() {
	constexpr int preview_rate = 44100;
	auto full_samples = SynthesizeFullAudio(preview_rate);

	waveform_samples_.resize(WAVEFORM_SAMPLE_COUNT);

	if (full_samples.empty()) {
		std::fill(waveform_samples_.begin(), waveform_samples_.end(), 0.0f);
		return;
	}

	// Downsample the full synthesis to fit the preview display
	const auto full_count = static_cast<int>(full_samples.size());
	for (int i = 0; i < WAVEFORM_SAMPLE_COUNT; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(WAVEFORM_SAMPLE_COUNT - 1);
		const int src_idx = std::min(static_cast<int>(t * static_cast<float>(full_count - 1)), full_count - 1);
		waveform_samples_[i] = full_samples[src_idx];
	}
}

// ========================================================================
// Full Audio Synthesis (sfxr-style)
// ========================================================================

std::vector<float> SoundEditorPanel::SynthesizeFullAudio(const int sample_rate) const {
	constexpr float pi = 3.14159265359f;
	constexpr float two_pi = 2.0f * pi;

	const float total_time = preset_.attack_time + preset_.sustain_time + preset_.decay_time;
	if (total_time <= 0.0f) {
		return {};
	}

	const int total_samples = static_cast<int>(total_time * static_cast<float>(sample_rate));
	if (total_samples <= 0) {
		return {};
	}

	std::vector<float> samples(total_samples);

	const float dt = 1.0f / static_cast<float>(sample_rate);

	// Synthesis state
	float phase = 0.0f;
	float frequency = preset_.base_frequency;

	// Phaser state (delay buffer)
	constexpr int PHASER_BUFFER_SIZE = 1024;
	std::vector<float> phaser_buffer(PHASER_BUFFER_SIZE, 0.0f);
	int phaser_pos = 0;
	float phaser_offset = preset_.phaser_offset * 100.0f; // Convert to sample offset

	// Filter state
	float lp_sample = 0.0f;
	float lp_cutoff = preset_.lowpass_cutoff;
	float hp_sample = 0.0f;
	float prev_sample = 0.0f;

	// Noise RNG
	std::mt19937 noise_gen(42);
	std::uniform_real_distribution<float> noise_dist(-1.0f, 1.0f);

	for (int i = 0; i < total_samples; ++i) {
		const float t = static_cast<float>(i) * dt;

		// --- Envelope ---
		float envelope = 0.0f;
		if (t < preset_.attack_time) {
			// Attack: ramp from 0 to sustain_level
			envelope = (preset_.attack_time > 0.0f) ? preset_.sustain_level * (t / preset_.attack_time)
													: preset_.sustain_level;
		}
		else if (t < preset_.attack_time + preset_.sustain_time) {
			envelope = preset_.sustain_level;
		}
		else {
			// Decay: ramp from sustain_level to 0
			const float decay_t = t - preset_.attack_time - preset_.sustain_time;
			envelope =
					(preset_.decay_time > 0.0f) ? preset_.sustain_level * (1.0f - decay_t / preset_.decay_time) : 0.0f;
		}
		envelope = std::clamp(envelope, 0.0f, 1.0f);

		// --- Frequency slide ---
		frequency += preset_.frequency_slide * 1000.0f * dt;
		frequency = std::clamp(frequency, preset_.frequency_min, preset_.frequency_max);

		// --- Vibrato ---
		float vibrato_mod = 0.0f;
		if (preset_.vibrato_depth > 0.0f && preset_.vibrato_speed > 0.0f) {
			vibrato_mod = preset_.vibrato_depth * frequency * 0.5f * std::sin(two_pi * preset_.vibrato_speed * t);
		}

		const float current_freq = frequency + vibrato_mod;

		// --- Oscillator ---
		float sample = 0.0f;
		switch (preset_.waveform) {
		case WaveformType::Sine: sample = std::sin(phase); break;
		case WaveformType::Square: sample = (std::fmod(phase, two_pi) < pi) ? 1.0f : -1.0f; break;
		case WaveformType::Saw: sample = (std::fmod(phase, two_pi) / pi) - 1.0f; break;
		case WaveformType::Triangle:
		{
			const float norm = std::fmod(phase, two_pi) / two_pi;
			sample = (norm < 0.5f) ? (4.0f * norm - 1.0f) : (3.0f - 4.0f * norm);
			break;
		}
		case WaveformType::Noise: sample = noise_dist(noise_gen); break;
		}

		// Advance phase
		phase += two_pi * current_freq * dt;
		if (phase > two_pi) {
			phase -= two_pi * std::floor(phase / two_pi);
		}

		// --- Low-pass filter ---
		lp_cutoff += preset_.lowpass_sweep * dt;
		lp_cutoff = std::clamp(lp_cutoff, 0.0f, 1.0f);
		if (lp_cutoff < 1.0f) {
			// Simple one-pole low-pass: coefficient from normalized cutoff
			const float lp_coeff = std::clamp(lp_cutoff * lp_cutoff, 0.0001f, 1.0f);
			lp_sample += lp_coeff * (sample - lp_sample);
			sample = lp_sample;
		}

		// --- High-pass filter ---
		if (preset_.highpass_cutoff > 0.0f) {
			const float hp_coeff = std::clamp(1.0f - preset_.highpass_cutoff * preset_.highpass_cutoff, 0.0f, 0.9999f);
			hp_sample = hp_coeff * (hp_sample + sample - prev_sample);
			sample = hp_sample;
		}
		prev_sample = sample;

		// --- Phaser ---
		if (std::abs(preset_.phaser_offset) > 0.001f) {
			phaser_offset += preset_.phaser_sweep * dt * 100.0f;
			const int offset = std::clamp(static_cast<int>(std::abs(phaser_offset)), 1, PHASER_BUFFER_SIZE - 1);
			phaser_buffer[phaser_pos] = sample;
			const int read_pos = (phaser_pos - offset + PHASER_BUFFER_SIZE) % PHASER_BUFFER_SIZE;
			sample += phaser_buffer[read_pos];
			phaser_pos = (phaser_pos + 1) % PHASER_BUFFER_SIZE;
		}

		// --- Apply envelope and volume ---
		sample *= envelope * preset_.master_volume * preset_.gain;

		samples[i] = std::clamp(sample, -1.0f, 1.0f);
	}

	return samples;
}

// ========================================================================
// WAV File Construction
// ========================================================================

std::vector<uint8_t> SoundEditorPanel::BuildWavFile(const std::vector<float>& samples, const int sample_rate) {
	const uint32_t num_samples = static_cast<uint32_t>(samples.size());
	constexpr uint16_t num_channels = 1;
	constexpr uint16_t bits_per_sample = 16;
	constexpr uint16_t block_align = num_channels * (bits_per_sample / 8);
	const uint32_t byte_rate = static_cast<uint32_t>(sample_rate) * block_align;
	const uint32_t data_size = num_samples * block_align;
	const uint32_t file_size = 36 + data_size;

	std::vector<uint8_t> wav;
	wav.reserve(44 + data_size);

	// Helper to write little-endian values
	auto write_u16 = [&](uint16_t v) {
		wav.push_back(static_cast<uint8_t>(v & 0xFF));
		wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
	};
	auto write_u32 = [&](uint32_t v) {
		wav.push_back(static_cast<uint8_t>(v & 0xFF));
		wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
		wav.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
		wav.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
	};
	auto write_tag = [&](const char tag[4]) {
		wav.push_back(static_cast<uint8_t>(tag[0]));
		wav.push_back(static_cast<uint8_t>(tag[1]));
		wav.push_back(static_cast<uint8_t>(tag[2]));
		wav.push_back(static_cast<uint8_t>(tag[3]));
	};

	// RIFF header
	write_tag("RIFF");
	write_u32(file_size);
	write_tag("WAVE");

	// fmt chunk
	write_tag("fmt ");
	write_u32(16); // chunk size
	write_u16(1); // PCM format
	write_u16(num_channels);
	write_u32(static_cast<uint32_t>(sample_rate));
	write_u32(byte_rate);
	write_u16(block_align);
	write_u16(bits_per_sample);

	// data chunk
	write_tag("data");
	write_u32(data_size);

	// Convert float samples [-1, 1] to 16-bit PCM
	for (const float s : samples) {
		const float clamped = std::clamp(s, -1.0f, 1.0f);
		auto pcm = static_cast<int16_t>(clamped * 32767.0f);
		wav.push_back(static_cast<uint8_t>(pcm & 0xFF));
		wav.push_back(static_cast<uint8_t>((pcm >> 8) & 0xFF));
	}

	return wav;
}

// ========================================================================
// Playback
// ========================================================================

void SoundEditorPanel::PlayPreview() {
	StopPreview();

	constexpr int sample_rate = 44100;
	auto samples = SynthesizeFullAudio(sample_rate);
	if (samples.empty()) {
		return;
	}

	// Write to a temporary WAV file for miniaudio playback
	auto wav_data = BuildWavFile(samples, sample_rate);
	// Use a unique filename each time to bypass miniaudio's resource manager cache
	playback_temp_path_ =
			(std::filesystem::temp_directory_path() / ("citrus_sfx_preview_" + std::to_string(preview_counter_++) + ".wav"))
					.string();

	if (!engine::assets::AssetManager::SaveBinaryFile(std::filesystem::path(playback_temp_path_), wav_data)) {
		return;
	}

	auto& audio = engine::audio::AudioSystem::Get();
	if (!audio.IsInitialized()) {
		audio.Initialize();
	}

	playback_clip_id_ = audio.LoadClip(playback_temp_path_);
	if (playback_clip_id_ == 0) {
		return;
	}

	playback_handle_ = audio.PlaySoundClip(playback_clip_id_, 1.0f, false);
	if (playback_handle_ != 0) {
		is_playing_ = true;
	}
}

void SoundEditorPanel::StopPreview() {
	auto& audio = engine::audio::AudioSystem::Get();
	if (playback_handle_ != 0) {
		audio.StopSound(playback_handle_);
		playback_handle_ = 0;
	}
	if (playback_clip_id_ != 0) {
		audio.UnloadClip(playback_clip_id_);
		playback_clip_id_ = 0;
	}
	is_playing_ = false;

	// Clean up temp file
	if (!playback_temp_path_.empty()) {
		std::filesystem::remove(std::filesystem::path(playback_temp_path_));
		playback_temp_path_.clear();
	}
}

// ========================================================================
// WAV Export
// ========================================================================

bool SoundEditorPanel::ExportWav(const std::string& path) {
	constexpr int sample_rate = 44100;
	auto samples = SynthesizeFullAudio(sample_rate);
	if (samples.empty()) {
		return false;
	}

	auto wav_data = BuildWavFile(samples, sample_rate);
	if (engine::assets::AssetManager::SaveBinaryFile(std::filesystem::path(path), wav_data)) {
		export_wav_path_ = path;
		// Auto-save the preset to persist the export path association
		if (!current_file_path_.empty()) {
			SavePresetToJson(current_file_path_);
		}
		return true;
	}
	return false;
}

// ========================================================================
// Public API
// ========================================================================

void SoundEditorPanel::NewSound() {
	StopPreview();
	preset_ = SoundPreset();
	preset_name_ = "Untitled";
	current_file_path_ = "";
	export_wav_path_ = "";
	is_playing_ = false;
	SetDirty(false);

	RegeneratePreview();
}

bool SoundEditorPanel::OpenSound(const std::string& path) {
	if (LoadPresetFromJson(path)) {
		current_file_path_ = path;
		preset_name_ = path;
		SetDirty(false);
		RegeneratePreview();
		return true;
	}
	return false;
}

bool SoundEditorPanel::SaveSound(const std::string& path) {
	if (SavePresetToJson(path)) {
		current_file_path_ = path;
		preset_name_ = path;
		SetDirty(false);
		return true;
	}
	return false;
}

// ========================================================================
// File I/O
// ========================================================================

bool SoundEditorPanel::LoadPresetFromJson(const std::string& path) {
	try {
		const auto text = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
		if (!text) {
			return false;
		}

		json j = json::parse(*text);

		// Load preset parameters
		if (j.contains("waveform")) {
			preset_.waveform = static_cast<WaveformType>(j["waveform"].get<int>());
		}
		if (j.contains("base_frequency")) {
			preset_.base_frequency = j["base_frequency"].get<float>();
		}
		if (j.contains("frequency_min")) {
			preset_.frequency_min = j["frequency_min"].get<float>();
		}
		if (j.contains("frequency_max")) {
			preset_.frequency_max = j["frequency_max"].get<float>();
		}
		if (j.contains("frequency_slide")) {
			preset_.frequency_slide = j["frequency_slide"].get<float>();
		}

		if (j.contains("attack_time")) {
			preset_.attack_time = j["attack_time"].get<float>();
		}
		if (j.contains("sustain_time")) {
			preset_.sustain_time = j["sustain_time"].get<float>();
		}
		if (j.contains("sustain_level")) {
			preset_.sustain_level = j["sustain_level"].get<float>();
		}
		if (j.contains("decay_time")) {
			preset_.decay_time = j["decay_time"].get<float>();
		}

		if (j.contains("vibrato_depth")) {
			preset_.vibrato_depth = j["vibrato_depth"].get<float>();
		}
		if (j.contains("vibrato_speed")) {
			preset_.vibrato_speed = j["vibrato_speed"].get<float>();
		}

		if (j.contains("phaser_offset")) {
			preset_.phaser_offset = j["phaser_offset"].get<float>();
		}
		if (j.contains("phaser_sweep")) {
			preset_.phaser_sweep = j["phaser_sweep"].get<float>();
		}

		if (j.contains("lowpass_cutoff")) {
			preset_.lowpass_cutoff = j["lowpass_cutoff"].get<float>();
		}
		if (j.contains("lowpass_sweep")) {
			preset_.lowpass_sweep = j["lowpass_sweep"].get<float>();
		}
		if (j.contains("highpass_cutoff")) {
			preset_.highpass_cutoff = j["highpass_cutoff"].get<float>();
		}

		if (j.contains("master_volume")) {
			preset_.master_volume = j["master_volume"].get<float>();
		}
		if (j.contains("gain")) {
			preset_.gain = j["gain"].get<float>();
		}
		if (j.contains("export_wav_path")) {
			export_wav_path_ = j["export_wav_path"].get<std::string>();
		}

		return true;
	}
	catch (const std::exception& e) {
		return false;
	}
}

bool SoundEditorPanel::SavePresetToJson(const std::string& path) {
	try {
		json j;
		j["asset_type"] = "sound";

		// Save preset parameters
		j["waveform"] = static_cast<int>(preset_.waveform);
		j["base_frequency"] = preset_.base_frequency;
		j["frequency_min"] = preset_.frequency_min;
		j["frequency_max"] = preset_.frequency_max;
		j["frequency_slide"] = preset_.frequency_slide;

		j["attack_time"] = preset_.attack_time;
		j["sustain_time"] = preset_.sustain_time;
		j["sustain_level"] = preset_.sustain_level;
		j["decay_time"] = preset_.decay_time;

		j["vibrato_depth"] = preset_.vibrato_depth;
		j["vibrato_speed"] = preset_.vibrato_speed;

		j["phaser_offset"] = preset_.phaser_offset;
		j["phaser_sweep"] = preset_.phaser_sweep;

		j["lowpass_cutoff"] = preset_.lowpass_cutoff;
		j["lowpass_sweep"] = preset_.lowpass_sweep;
		j["highpass_cutoff"] = preset_.highpass_cutoff;

		j["master_volume"] = preset_.master_volume;
		j["gain"] = preset_.gain;

		if (!export_wav_path_.empty()) {
			j["export_wav_path"] = export_wav_path_;
		}

		return engine::assets::AssetManager::SaveTextFile(std::filesystem::path(path), j.dump(2));
	}
	catch (const std::exception& e) {
		return false;
	}
}

} // namespace editor
