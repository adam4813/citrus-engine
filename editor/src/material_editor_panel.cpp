#include "material_editor_panel.h"

#include "asset_editor_registry.h"

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
		auto text = engine::assets::AssetManager::LoadTextFile(std::filesystem::path(path));
		if (!text) {
			std::cerr << "MaterialEditor: Failed to read " << path << std::endl;
			return;
		}
		json j = json::parse(*text);

		// Use the asset registry's FromJson factory
		const auto* type_info = engine::scene::AssetRegistry::Instance().GetTypeInfo(engine::scene::AssetType::MATERIAL);
		if (!type_info || !type_info->from_json_factory) {
			std::cerr << "MaterialEditor: Material type not registered" << std::endl;
			return;
		}

		auto asset = type_info->from_json_factory(j);
		material_ = std::shared_ptr<engine::scene::MaterialAssetInfo>(
				static_cast<engine::scene::MaterialAssetInfo*>(asset.release()));

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

	// Render save dialog if active
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

	// Keyboard shortcut
	if (material_ && IsDirty() && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_S)) {
		SaveMaterial();
	}
}

void MaterialEditorPanel::RenderMaterialProperties() {
	ImGui::Text("Material: %s", material_->name.c_str());
	ImGui::Text("File: %s", current_file_path_.c_str());
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

	// Shader
	{
		char buffer[256];
		std::strncpy(buffer, material_->shader_name.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText("Shader", buffer, sizeof(buffer))) {
			material_->shader_name = buffer;
			modified = true;
		}
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

	auto RenderTextureSlot = [&](const char* label, std::string& texture_name) {
		char buf[256];
		std::strncpy(buf, texture_name.c_str(), sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
		ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 30.0f);
		if (ImGui::InputText(label, buf, sizeof(buf))) {
			texture_name = buf;
			modified = true;
		}
		ImGui::SameLine();
		std::string btn_id = std::string("...##") + label;
		if (ImGui::Button(btn_id.c_str())) {
			// Scan for texture files
			auto* target = &texture_name;
			FileDialogPopup dialog("Browse Texture", FileDialogMode::Open, {".png", ".jpg", ".jpeg", ".tga", ".bmp"});
			dialog.SetCallback([target, &modified](const std::string& path) {
				*target = path;
				modified = true;
			});
			dialog.Open();
		}
	};

	RenderTextureSlot("Albedo", material_->albedo_texture);
	RenderTextureSlot("Normal", material_->normal_texture);
	RenderTextureSlot("Metallic", material_->metallic_texture);
	RenderTextureSlot("Roughness", material_->roughness_texture);
	RenderTextureSlot("AO", material_->ao_texture);
	RenderTextureSlot("Emissive", material_->emissive_texture);
	RenderTextureSlot("Height", material_->height_texture);

	if (modified) {
		SetDirty(true);
	}
}

} // namespace editor
