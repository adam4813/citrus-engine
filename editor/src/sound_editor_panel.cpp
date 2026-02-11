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
}

SoundEditorPanel::~SoundEditorPanel() = default;

std::string_view SoundEditorPanel::GetPanelName() const { return "Sound Editor"; }

void SoundEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("sound", [this](const std::string& path) {
		OpenSound(path);
		SetVisible(true);
	});
}

void SoundEditorPanel::Render() {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Sound Editor", &VisibleRef(), ImGuiWindowFlags_MenuBar);

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

	ImGui::End();
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
				ImGui::OpenPopup("ExportNotImplemented");
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	open_dialog_.Render();
	save_dialog_.Render();

	// Export not implemented popup
	if (ImGui::BeginPopupModal("ExportNotImplemented", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("WAV export is not yet implemented.");
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void SoundEditorPanel::RenderWaveformSelector() {
	ImGui::Text("Waveform Type");

	const char* waveform_names[] = {"Sine", "Square", "Saw", "Triangle", "Noise"};
	int current_waveform = static_cast<int>(preset_.waveform);

	if (ImGui::Combo("##Waveform", &current_waveform, waveform_names, IM_ARRAYSIZE(waveform_names))) {
		preset_.waveform = static_cast<WaveformType>(current_waveform);
	}
}

void SoundEditorPanel::RenderFrequencyControls() {
	ImGui::Text("Frequency");

	ImGui::SliderFloat("Base Frequency (Hz)", &preset_.base_frequency, 20.0f, 2000.0f, "%.1f");
	ImGui::SliderFloat("Min Frequency (Hz)", &preset_.frequency_min, 20.0f, 2000.0f, "%.1f");
	ImGui::SliderFloat("Max Frequency (Hz)", &preset_.frequency_max, 20.0f, 2000.0f, "%.1f");
	ImGui::SliderFloat("Frequency Slide", &preset_.frequency_slide, -1.0f, 1.0f, "%.3f");
}

void SoundEditorPanel::RenderEnvelopeControls() {
	ImGui::Text("Envelope");

	ImGui::SliderFloat("Attack Time (s)", &preset_.attack_time, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Sustain Time (s)", &preset_.sustain_time, 0.0f, 2.0f, "%.3f");
	ImGui::SliderFloat("Sustain Level", &preset_.sustain_level, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Decay Time (s)", &preset_.decay_time, 0.0f, 2.0f, "%.3f");
}

void SoundEditorPanel::RenderEffectControls() {
	ImGui::Text("Effects");

	if (ImGui::TreeNode("Vibrato")) {
		ImGui::SliderFloat("Depth##Vibrato", &preset_.vibrato_depth, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Speed (Hz)##Vibrato", &preset_.vibrato_speed, 0.0f, 20.0f, "%.1f");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Phaser")) {
		ImGui::SliderFloat("Offset##Phaser", &preset_.phaser_offset, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Sweep##Phaser", &preset_.phaser_sweep, -1.0f, 1.0f, "%.3f");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Filter")) {
		ImGui::SliderFloat("Low-pass Cutoff", &preset_.lowpass_cutoff, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Low-pass Sweep", &preset_.lowpass_sweep, -1.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("High-pass Cutoff", &preset_.highpass_cutoff, 0.0f, 1.0f, "%.3f");
		ImGui::TreePop();
	}
}

void SoundEditorPanel::RenderVolumeControls() {
	ImGui::Text("Volume");

	ImGui::SliderFloat("Master Volume", &preset_.master_volume, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Gain", &preset_.gain, 0.0f, 2.0f, "%.3f");
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

	const float button_width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

	if (!is_playing_) {
		if (ImGui::Button("Play", ImVec2(button_width, 0))) {
			is_playing_ = true;
			// TODO: Trigger audio playback (stretch goal)
		}
	}
	else {
		if (ImGui::Button("Stop", ImVec2(button_width, 0))) {
			is_playing_ = false;
			// TODO: Stop audio playback
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Generate", ImVec2(button_width, 0))) {
		// Regenerate waveform preview
		GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
	}
}

void SoundEditorPanel::RenderWaveformVisualizer() {
	ImGui::Text("Waveform Preview");

	// Generate waveform if not yet generated
	if (waveform_samples_.empty() || waveform_samples_.size() != WAVEFORM_SAMPLE_COUNT) {
		GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
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

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
}

// ========================================================================
// Waveform Generation
// ========================================================================

void SoundEditorPanel::GenerateWaveform(std::vector<float>& samples, const int sample_count) {
	samples.resize(sample_count);

	for (int i = 0; i < sample_count; ++i) {
		// Simple phase calculation (0 to 2*PI over the sample count)
		const float phase = (static_cast<float>(i) / static_cast<float>(sample_count)) * 2.0f * 3.14159265359f;
		samples[i] = GenerateOscillatorSample(phase);
	}
}

float SoundEditorPanel::GenerateOscillatorSample(const float phase) const {
	const float pi = 3.14159265359f;

	switch (preset_.waveform) {
	case WaveformType::Sine: return std::sin(phase);

	case WaveformType::Square: return (std::sin(phase) >= 0.0f) ? 1.0f : -1.0f;

	case WaveformType::Saw:
		// Sawtooth: linear rise from -1 to 1
		return (phase / pi) - 1.0f;

	case WaveformType::Triangle:
	{
		// Triangle wave
		const float t = phase / (2.0f * pi);
		return (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
	}

	case WaveformType::Noise:
	{
		// Simple noise (pseudo-random based on phase)
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		return dist(gen);
	}

	default: return 0.0f;
	}
}

// ========================================================================
// Public API
// ========================================================================

void SoundEditorPanel::NewSound() {
	preset_ = SoundPreset();
	preset_name_ = "Untitled";
	current_file_path_ = "";
	is_playing_ = false;

	GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
}

bool SoundEditorPanel::OpenSound(const std::string& path) {
	if (LoadPresetFromJson(path)) {
		current_file_path_ = path;
		preset_name_ = path;
		GenerateWaveform(waveform_samples_, WAVEFORM_SAMPLE_COUNT);
		return true;
	}
	return false;
}

bool SoundEditorPanel::SaveSound(const std::string& path) {
	if (SavePresetToJson(path)) {
		current_file_path_ = path;
		preset_name_ = path;
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

		return engine::assets::AssetManager::SaveTextFile(std::filesystem::path(path), j.dump(2));
	}
	catch (const std::exception& e) {
		return false;
	}
}

} // namespace editor
