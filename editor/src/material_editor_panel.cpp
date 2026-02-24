#include "material_editor_panel.h"

#include "asset_editor_registry.h"
#include "field_widgets.h"

#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

import engine;

using json = nlohmann::json;

namespace editor {

void MaterialEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("material", [this](const std::string& path) { OpenMaterial(path); });
}

void MaterialEditorPanel::OpenMaterial(const std::string& path) {
	try {
		if (material_ = std::dynamic_pointer_cast<engine::assets::MaterialAssetInfo>(
					engine::assets::AssetCache::Instance().LoadFromFile(path));
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

	if (RenderFieldDialogs()) {
		SetDirty(true);
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

void MaterialEditorPanel::RenderMaterialProperties() {
	ImGui::Text("Material: %s", material_->name.c_str());
	ImGui::TextDisabled("%s", current_file_path_.c_str());
	ImGui::Separator();

	const auto* type_info = engine::assets::AssetTypeRegistry::Instance().GetTypeInfo("material");
	if (!type_info) {
		return;
	}

	// Section headers inserted before the first field of each group
	static const std::unordered_map<std::string, const char*> kSectionHeaders = {
			{"base_color", "Colors"},
			{"albedo_map", "Texture Maps"},
			{"metallic_factor", "PBR Properties"},
	};

	bool modified = false;

	// Render name field manually (always first)
	{
		char buffer[256];
		std::strncpy(buffer, material_->name.c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
			material_->name = buffer;
			modified = true;
		}
	}

	for (const auto& field : type_info->fields) {
		if (field.name == "name") {
			continue;
		}
		if (const auto it = kSectionHeaders.find(field.name); it != kSectionHeaders.end()) {
			ImGui::Separator();
			ImGui::Text("%s", it->second);
		}
		void* field_ptr = static_cast<char*>(static_cast<void*>(material_.get())) + field.offset;
		if (RenderFieldWidget(field, field_ptr)) {
			modified = true;
		}
	}

	if (modified) {
		SetDirty(true);
	}
}

} // namespace editor
