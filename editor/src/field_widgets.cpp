#include "field_widgets.h"

#include <cstdint>
#include <cstring>
#include <imgui.h>
#include <optional>
#include <string>
#include <vector>

#include "file_dialog.h"

import engine;

namespace editor {

namespace {

/// Static file dialog for field browse buttons (only one active at a time)
std::optional<FileDialogPopup> s_field_file_dialog;
bool s_field_file_dialog_modified = false;

void OpenFieldBrowseDialog(const char* title, const std::vector<std::string>& extensions, std::string* target) {
	s_field_file_dialog.emplace(title, FileDialogMode::Open, extensions);
	s_field_file_dialog->SetCallback([target](const std::string& path) {
		*target = path;
		s_field_file_dialog_modified = true;
	});
	s_field_file_dialog->Open();
}

} // namespace

bool RenderFieldWidget(const engine::ecs::FieldInfo& field, void* data, engine::scene::Scene* scene) {
	const char* label = field.display_name.empty() ? field.name.c_str() : field.display_name.c_str();

	switch (field.type) {
	case engine::ecs::FieldType::Bool: return ImGui::Checkbox(label, static_cast<bool*>(data));

	case engine::ecs::FieldType::Int: return ImGui::InputInt(label, static_cast<int*>(data));

	case engine::ecs::FieldType::Float: return ImGui::InputFloat(label, static_cast<float*>(data));

	case engine::ecs::FieldType::Slider:
		return ImGui::SliderFloat(label, static_cast<float*>(data), field.slider_min, field.slider_max);

	case engine::ecs::FieldType::String:
	{
		auto* str = static_cast<std::string*>(data);
		char buffer[256];
		std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText(label, buffer, sizeof(buffer))) {
			*str = buffer;
			return true;
		}
		return false;
	}

	case engine::ecs::FieldType::FilePath:
	{
		auto* str = static_cast<std::string*>(data);
		char buffer[256];
		std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		bool modified = false;
		ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 30.0f);
		if (ImGui::InputText(label, buffer, sizeof(buffer))) {
			*str = buffer;
			modified = true;
		}
		ImGui::SameLine();
		std::string browse_id = std::string("...##filepath_browse_") + field.name;
		if (ImGui::Button(browse_id.c_str())) {
			OpenFieldBrowseDialog("Browse File", field.file_extensions, str);
		}
		return modified;
	}

	case engine::ecs::FieldType::Vec2: return ImGui::InputFloat2(label, static_cast<float*>(data));

	case engine::ecs::FieldType::Vec3: return ImGui::InputFloat3(label, static_cast<float*>(data));

	case engine::ecs::FieldType::Vec4: return ImGui::InputFloat4(label, static_cast<float*>(data));

	case engine::ecs::FieldType::Color: return ImGui::ColorEdit4(label, static_cast<float*>(data));

	case engine::ecs::FieldType::ReadOnly: ImGui::Text("%s: (read-only)", label); return false;

	case engine::ecs::FieldType::Enum:
	{
		auto* value = static_cast<int*>(data);
		if (const auto& labels = field.enum_labels;
			!labels.empty() && *value >= 0 && *value < static_cast<int>(labels.size())) {
			if (ImGui::BeginCombo(label, labels[*value].c_str())) {
				for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
					const bool is_selected = (*value == i);
					if (ImGui::Selectable(labels[i].c_str(), is_selected)) {
						*value = i;
						ImGui::EndCombo();
						return true;
					}
					if (!field.enum_tooltips.empty() && i < static_cast<int>(field.enum_tooltips.size())) {
						ImGui::SetItemTooltip("%s", field.enum_tooltips[i].c_str());
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		else {
			ImGui::Text("%s: %d", label, *value);
		}
		return false;
	}

	case engine::ecs::FieldType::Selection:
	{
		auto* str = static_cast<std::string*>(data);
		int current_index = 0;
		for (size_t i = 0; i < field.options.size(); ++i) {
			if (field.options[i] == *str) {
				current_index = static_cast<int>(i);
				break;
			}
		}
		if (ImGui::BeginCombo(label, str->c_str())) {
			for (size_t i = 0; i < field.options.size(); ++i) {
				const bool is_selected = (current_index == static_cast<int>(i));
				if (ImGui::Selectable(field.options[i].c_str(), is_selected)) {
					*str = field.options[i];
					ImGui::EndCombo();
					return true;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return false;
	}

	case engine::ecs::FieldType::ListInt:
	{
		auto* vec = static_cast<std::vector<int>*>(data);
		bool modified = false;
		std::string header = std::string(label) + " (" + std::to_string(vec->size()) + ")";
		if (ImGui::TreeNode(header.c_str())) {
			int remove_idx = -1;
			int swap_idx = -1;
			for (size_t i = 0; i < vec->size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 70.0f);
				if (ImGui::InputInt("##v", &(*vec)[i])) {
					modified = true;
				}
				ImGui::SameLine();
				if (i > 0) {
					if (ImGui::SmallButton("^")) {
						swap_idx = static_cast<int>(i) - 1;
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("^").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (i + 1 < vec->size()) {
					if (ImGui::SmallButton("v")) {
						swap_idx = static_cast<int>(i);
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("v").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X")) {
					remove_idx = static_cast<int>(i);
				}
				ImGui::PopID();
			}
			if (remove_idx >= 0) {
				vec->erase(vec->begin() + remove_idx);
				modified = true;
			}
			if (swap_idx >= 0) {
				std::swap((*vec)[swap_idx], (*vec)[swap_idx + 1]);
				modified = true;
			}
			if (ImGui::Button("+ Add")) {
				vec->emplace_back(0);
				modified = true;
			}
			ImGui::TreePop();
		}
		return modified;
	}

	case engine::ecs::FieldType::ListFloat:
	{
		auto* vec = static_cast<std::vector<float>*>(data);
		bool modified = false;
		std::string header = std::string(label) + " (" + std::to_string(vec->size()) + ")";
		if (ImGui::TreeNode(header.c_str())) {
			int remove_idx = -1;
			int swap_idx = -1;
			for (size_t i = 0; i < vec->size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 70.0f);
				if (ImGui::InputFloat("##v", &(*vec)[i])) {
					modified = true;
				}
				ImGui::SameLine();
				if (i > 0) {
					if (ImGui::SmallButton("^")) {
						swap_idx = static_cast<int>(i) - 1;
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("^").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (i + 1 < vec->size()) {
					if (ImGui::SmallButton("v")) {
						swap_idx = static_cast<int>(i);
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("v").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X")) {
					remove_idx = static_cast<int>(i);
				}
				ImGui::PopID();
			}
			if (remove_idx >= 0) {
				vec->erase(vec->begin() + remove_idx);
				modified = true;
			}
			if (swap_idx >= 0) {
				std::swap((*vec)[swap_idx], (*vec)[swap_idx + 1]);
				modified = true;
			}
			if (ImGui::Button("+ Add")) {
				vec->emplace_back(0.0f);
				modified = true;
			}
			ImGui::TreePop();
		}
		return modified;
	}

	case engine::ecs::FieldType::ListString:
	{
		auto* vec = static_cast<std::vector<std::string>*>(data);
		bool modified = false;
		std::string header = std::string(label) + " (" + std::to_string(vec->size()) + ")";
		if (ImGui::TreeNode(header.c_str())) {
			const bool is_asset_ref = !field.asset_type.empty();
			const bool is_file_path = !is_asset_ref && !field.file_extensions.empty();

			std::vector<std::string> asset_names;
			if (is_asset_ref) {
				asset_names.push_back("");
				for (const auto& asset : engine::assets::AssetCache::Instance().GetAll()) {
					if (const auto* type_info = engine::assets::AssetRegistry::Instance().GetTypeInfo(asset->type);
						type_info && type_info->type_name == field.asset_type) {
						asset_names.push_back(asset->name);
					}
				}
			}

			int remove_idx = -1;
			int swap_idx = -1;
			for (size_t i = 0; i < vec->size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				auto& elem = (*vec)[i];

				if (is_asset_ref) {
					const char* preview = elem.empty() ? "(None)" : elem.c_str();
					ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 100.0f);
					if (ImGui::BeginCombo("##v", preview)) {
						for (const auto& name : asset_names) {
							const bool selected = (name == elem);
							const char* item_label = name.empty() ? "(None)" : name.c_str();
							if (ImGui::Selectable(item_label, selected)) {
								elem = name;
								modified = true;
							}
							if (selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}
				else if (is_file_path) {
					char buffer[256];
					std::strncpy(buffer, elem.c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 130.0f);
					if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
						elem = buffer;
						modified = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("...")) {
						OpenFieldBrowseDialog("Browse File", field.file_extensions, &elem);
					}
				}
				else {
					char buffer[256];
					std::strncpy(buffer, elem.c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 70.0f);
					if (ImGui::InputText("##v", buffer, sizeof(buffer))) {
						elem = buffer;
						modified = true;
					}
				}

				ImGui::SameLine();
				if (i > 0) {
					if (ImGui::SmallButton("^")) {
						swap_idx = static_cast<int>(i) - 1;
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("^").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (i + 1 < vec->size()) {
					if (ImGui::SmallButton("v")) {
						swap_idx = static_cast<int>(i);
					}
				}
				else {
					ImGui::Dummy(ImVec2(ImGui::CalcTextSize("v").x + ImGui::GetStyle().FramePadding.x * 2, 0));
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("X")) {
					remove_idx = static_cast<int>(i);
				}
				ImGui::PopID();
			}
			if (remove_idx >= 0) {
				vec->erase(vec->begin() + remove_idx);
				modified = true;
			}
			if (swap_idx >= 0) {
				std::swap((*vec)[swap_idx], (*vec)[swap_idx + 1]);
				modified = true;
			}
			if (ImGui::Button("+ Add")) {
				vec->emplace_back();
				modified = true;
			}
			ImGui::TreePop();
		}
		return modified;
	}

	case engine::ecs::FieldType::AssetRef:
	{
		// Cache-only combo: shows assets registered in AssetCache matching the field's asset_type
		auto* str = static_cast<std::string*>(data);

		std::vector<std::string> asset_names;
		asset_names.push_back("");
		for (const auto& asset : engine::assets::AssetCache::Instance().GetAll()) {
			if (const auto* type_info = engine::assets::AssetRegistry::Instance().GetTypeInfo(asset->type);
				type_info && type_info->type_name == field.asset_type) {
				asset_names.push_back(asset->name);
			}
		}

		int current_index = 0;
		for (size_t i = 0; i < asset_names.size(); ++i) {
			if (asset_names[i] == *str) {
				current_index = static_cast<int>(i);
				break;
			}
		}

		bool modified = false;
		const char* preview = str->empty() ? "(None)" : str->c_str();
		if (ImGui::BeginCombo(label, preview)) {
			for (size_t i = 0; i < asset_names.size(); ++i) {
				const bool is_selected = (current_index == static_cast<int>(i));
				const char* item_label = asset_names[i].empty() ? "(None)" : asset_names[i].c_str();
				if (ImGui::Selectable(item_label, is_selected)) {
					*str = asset_names[i];
					modified = true;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		// Audio preview button for sound asset references
		if (field.asset_type == "sound" && !str->empty() && scene) {
			static bool s_audio_playing = false;
			static uint32_t s_clip_id = 0;
			static uint32_t s_play_handle = 0;
			static std::string s_playing_asset;

			if (s_audio_playing && s_play_handle != 0) {
				auto& audio = engine::audio::AudioSystem::Get();
				if (!audio.IsSoundPlaying(s_play_handle)) {
					audio.UnloadClip(s_clip_id);
					s_audio_playing = false;
					s_clip_id = 0;
					s_play_handle = 0;
					s_playing_asset.clear();
				}
			}

			const bool is_this_playing = s_audio_playing && s_playing_asset == *str;
			ImGui::SameLine();
			const char* btn_label = is_this_playing ? "Stop##snd_preview" : "Play##snd_preview";
			if (ImGui::SmallButton(btn_label)) {
				auto& audio = engine::audio::AudioSystem::Get();
				if (is_this_playing) {
					audio.StopSound(s_play_handle);
					audio.UnloadClip(s_clip_id);
					s_audio_playing = false;
					s_clip_id = 0;
					s_play_handle = 0;
					s_playing_asset.clear();
				}
				else {
					if (s_audio_playing) {
						audio.StopSound(s_play_handle);
						audio.UnloadClip(s_clip_id);
						s_clip_id = 0;
						s_play_handle = 0;
						s_playing_asset.clear();
						s_audio_playing = false;
					}
					if (auto sound_asset =
								engine::assets::AssetCache::Instance().FindTyped<engine::assets::SoundAssetInfo>(*str);
						sound_asset && !sound_asset->file_path.empty()) {
						if (!audio.IsInitialized()) {
							audio.Initialize();
						}
						s_clip_id = audio.LoadClip(sound_asset->file_path);
						if (s_clip_id != 0) {
							s_play_handle = audio.PlaySoundClip(s_clip_id, sound_asset->volume, false);
							s_audio_playing = s_play_handle != 0;
							s_playing_asset = s_audio_playing ? *str : "";
						}
					}
				}
			}
		}

		return modified;
	}

	case engine::ecs::FieldType::UintAssetRef:
	{
		// Cache-only combo: shows assets of matching type by name, stores GUID in uint32_t
		auto* guid = static_cast<uint32_t*>(data);

		std::vector<std::pair<uint32_t, std::string>> assets;
		assets.emplace_back(0u, "(None)");
		for (const auto& asset : engine::assets::AssetCache::Instance().GetAll()) {
			if (const auto* type_info = engine::assets::AssetRegistry::Instance().GetTypeInfo(asset->type);
				type_info && type_info->type_name == field.asset_type) {
				assets.emplace_back(asset->guid, asset->name);
			}
		}

		int current_index = 0;
		for (size_t i = 0; i < assets.size(); ++i) {
			if (assets[i].first == *guid) {
				current_index = static_cast<int>(i);
				break;
			}
		}

		bool modified = false;
		const char* preview = assets[current_index].second.c_str();
		if (ImGui::BeginCombo(label, preview)) {
			for (size_t i = 0; i < assets.size(); ++i) {
				const bool is_selected = (current_index == static_cast<int>(i));
				if (ImGui::Selectable(assets[i].second.c_str(), is_selected)) {
					*guid = assets[i].first;
					modified = true;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		return modified;
	}
	}
	return false;
}

bool RenderFieldDialogs() {
	if (s_field_file_dialog) {
		s_field_file_dialog->Render();
	}
	if (s_field_file_dialog_modified) {
		s_field_file_dialog_modified = false;
		return true;
	}
	return false;
}

} // namespace editor
