#include "material_editor_panel.h"

#include "asset_editor_registry.h"
#include "file_utils.h"

#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>

import engine;

using json = nlohmann::json;

namespace editor {

void MaterialEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("material", [this](const std::string& path) { OpenMaterial(path); });
}

void MaterialEditorPanel::OpenMaterial(const std::string& path) {
	try {
		const auto text = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
		if (!text) {
			std::cerr << "MaterialEditor: Failed to read " << path << std::endl;
			return;
		}
		json j = json::parse(*text);

		if (material_ = std::dynamic_pointer_cast<engine::assets::MaterialAssetInfo>(
					engine::assets::AssetRegistry::Instance().FromJson(j));
			!material_) {
			std::cerr << "MaterialEditor: JSON does not represent a MaterialAssetInfo: " << path << std::endl;
			return;
		}

		current_file_path_ = path;
		SetDirty(false);
		SetVisible(true);

		std::cout << "MaterialEditor: Opened " << path << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "MaterialEditor: Error opening " << path << ": " << e.what() << std::endl;
	}
}

bool MaterialEditorPanel::SaveMaterial() {
	if (!material_ || current_file_path_.empty()) {
		return false;
	}

	try {
		json j;
		material_->ToJson(j);
		const std::string json_str = j.dump(2);

		if (engine::assets::AssetManager::SaveTextFile(std::filesystem::path(current_file_path_), json_str)) {
			SetDirty(false);
			std::cout << "MaterialEditor: Saved " << current_file_path_ << std::endl;
			return true;
		}
		std::cerr << "MaterialEditor: Failed to save " << current_file_path_ << std::endl;
		return false;
	}
	catch (const std::exception& e) {
		std::cerr << "MaterialEditor: Error saving: " << e.what() << std::endl;
		return false;
	}
}

void MaterialEditorPanel::Render() {
	if (!BeginPanel(ImGuiWindowFlags_MenuBar)) {
		return;
	}

	RenderToolbar();

	if (material_) {
		RenderMaterialProperties();
	}
	else {
		ImGui::TextDisabled("No material loaded. Open a .material.json file from the asset browser.");
	}

	if (save_dialog_) {
		save_dialog_->Render();
	}

	EndPanel();
}

void MaterialEditorPanel::RenderToolbar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Save", "Ctrl+S", false, material_ != nullptr && IsDirty())) {
				SaveMaterial();
			}
			if (ImGui::MenuItem("Save As...", nullptr, false, material_ != nullptr)) {
				save_dialog_.emplace("Save Material", FileDialogMode::Save, std::vector<std::string>{".json"});
				save_dialog_->SetCallback([this](const std::string& path) {
					current_file_path_ = path;
					SaveMaterial();
				});
				save_dialog_->Open();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	if (material_ && IsDirty() && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_S)) {
		SaveMaterial();
	}
}

namespace {

/// Render an AssetRef combo for a string field. Returns true if value changed.
bool RenderAssetRefCombo(
		const char* label,
		std::string& value,
		const std::string& asset_type,
		const std::vector<std::string>& file_extensions = {}) {
	// Build combined list: scene assets + file system matches
	std::vector<std::string> names;
	names.emplace_back(""); // (None) option

	// Scan file system for matching files
	if (!file_extensions.empty()) {
		for (auto& file_path : ScanAssetFiles(file_extensions)) {
			if (std::ranges::find(names, file_path) == names.end()) {
				names.push_back(std::move(file_path));
			}
		}
	}

	// Ensure current value is in the list
	if (!value.empty() && std::ranges::find(names, value) == names.end()) {
		names.push_back(value);
	}

	int current_index = 0;
	for (size_t i = 0; i < names.size(); ++i) {
		if (names[i] == value) {
			current_index = static_cast<int>(i);
			break;
		}
	}

	bool modified = false;
	const char* preview = value.empty() ? "(None)" : value.c_str();
	if (ImGui::BeginCombo(label, preview)) {
		for (size_t i = 0; i < names.size(); ++i) {
			const bool is_selected = (current_index == static_cast<int>(i));
			const char* item_label = names[i].empty() ? "(None)" : names[i].c_str();
			if (ImGui::Selectable(item_label, is_selected)) {
				value = names[i];
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

} // namespace

void MaterialEditorPanel::RenderMaterialProperties() {
	ImGui::Text("Material: %s", material_->name.c_str());
	ImGui::TextDisabled("%s", current_file_path_.c_str());
	ImGui::Separator();

	bool modified = false;

	// Name
	{
		char buffer[256];
		std::strncpy(buffer, material_->name.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
			material_->name = buffer;
			modified = true;
		}
	}

	// Shader (AssetRef combo — shaders are scene assets, not file assets)
	if (RenderAssetRefCombo("Shader", material_->shader_name, "shader")) {
		modified = true;
	}

	ImGui::Separator();
	ImGui::Text("Colors");

	if (ImGui::ColorEdit4("Base Color", &material_->base_color.x)) {
		modified = true;
	}
	if (ImGui::ColorEdit4("Emissive Color", &material_->emissive_color.x)) {
		modified = true;
	}

	ImGui::Separator();
	ImGui::Text("PBR Properties");

	if (ImGui::SliderFloat("Metallic", &material_->metallic_factor, 0.0f, 1.0f)) {
		modified = true;
	}
	if (ImGui::SliderFloat("Roughness", &material_->roughness_factor, 0.0f, 1.0f)) {
		modified = true;
	}
	if (ImGui::SliderFloat("AO Strength", &material_->ao_strength, 0.0f, 1.0f)) {
		modified = true;
	}
	if (ImGui::SliderFloat("Emissive Intensity", &material_->emissive_intensity, 0.0f, 10.0f)) {
		modified = true;
	}
	if (ImGui::SliderFloat("Normal Strength", &material_->normal_strength, 0.0f, 2.0f)) {
		modified = true;
	}
	if (ImGui::SliderFloat("Alpha Cutoff", &material_->alpha_cutoff, 0.0f, 1.0f)) {
		modified = true;
	}

	ImGui::Separator();
	ImGui::Text("Texture Maps");

	static const std::vector<std::string> tex_exts = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};

	if (RenderAssetRefCombo("Albedo Map", material_->albedo_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("Normal Map", material_->normal_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("Metallic Map", material_->metallic_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("Roughness Map", material_->roughness_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("AO Map", material_->ao_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("Emissive Map", material_->emissive_map, "texture", tex_exts)) {
		modified = true;
	}
	if (RenderAssetRefCombo("Height Map", material_->height_map, "texture", tex_exts)) {
		modified = true;
	}

	if (modified) {
		SetDirty(true);
	}
}

} // namespace editor
