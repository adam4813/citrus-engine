#include "properties_panel.h"

#include <cstring>
#include <flecs.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <string>
#include <vector>

namespace editor {

std::string_view PropertiesPanel::GetPanelName() const { return "Properties"; }

void PropertiesPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void PropertiesPanel::Render(
		const engine::ecs::Entity selected_entity,
		engine::ecs::ECSWorld& world,
		engine::scene::Scene* scene,
		const AssetSelection& selected_asset,
		const engine::ecs::Entity scene_active_camera) {
	if (!IsVisible())
		return;

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
										 | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Properties", &VisibleRef());

	if (selected_entity.is_valid()) {
		std::string name = selected_entity.name().c_str();
		if (name.empty()) {
			name = "Entity_" + std::to_string(selected_entity.id());
		}

		// Detect if this is a prefab template entity
		const bool is_prefab_template = selected_entity.has(flecs::Prefab);
		if (is_prefab_template) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
			ImGui::Text("Prefab Template");
			ImGui::PopStyleColor();
			ImGui::Text("Name: %s", name.c_str());
			ImGui::Separator();
		}
		else {
			ImGui::Text("Entity: %s", name.c_str());
			ImGui::Separator();
		}

		RenderComponentSections(selected_entity, scene);
		RenderAddComponentButton(selected_entity);
	}
	else if (selected_asset.IsValid() && scene) {
		RenderAssetProperties(scene, selected_asset);
	}
	else {
		RenderSceneProperties(world, scene_active_camera);
	}

	ImGui::End();
}

void PropertiesPanel::RenderComponentSections(const engine::ecs::Entity entity, engine::scene::Scene* scene) const {
	for (const auto& registry = engine::ecs::ComponentRegistry::Instance();
		 const auto& comp : registry.GetComponents()) {
		if (!entity.has(comp.id)) {
			continue;
		}

		if (ImGui::CollapsingHeader(comp.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			// Special handling for Tags component
			if (comp.name == "Tags") {
				auto* tags_comp = entity.try_get_mut<engine::components::Tags>();
				if (tags_comp) {
					ImGui::PushID("Tags");
					bool modified = false;

					// Display existing tags with remove buttons
					for (size_t i = 0; i < tags_comp->tags.size(); ++i) {
						ImGui::PushID(static_cast<int>(i));
						ImGui::Text("%s", tags_comp->tags[i].c_str());
						ImGui::SameLine();
						if (ImGui::SmallButton("X")) {
							tags_comp->tags.erase(tags_comp->tags.begin() + i);
							modified = true;
							ImGui::PopID();
							break; // Exit loop since we modified the vector
						}
						ImGui::PopID();
					}

					// Add new tag input
					static char tag_buffer[128] = {};
					ImGui::InputText("##new_tag", tag_buffer, sizeof(tag_buffer));
					ImGui::SameLine();
					if (ImGui::Button("Add Tag") && tag_buffer[0] != '\0') {
						tags_comp->AddTag(tag_buffer);
						tag_buffer[0] = '\0'; // Clear input
						modified = true;
					}

					if (modified && callbacks_.on_scene_modified) {
						entity.modified<engine::components::Tags>();
						callbacks_.on_scene_modified();
					}

					ImGui::PopID();
				}
			}
			else if (comp.fields.empty()) {
				ImGui::TextDisabled("(No editable fields)");
			}
			else {
				RenderComponentFields(entity, comp, scene);
			}
		}
	}
}

void PropertiesPanel::RenderComponentFields(
		const engine::ecs::Entity entity, const engine::ecs::ComponentInfo& comp, engine::scene::Scene* scene) const {
	// Get raw pointer to component data
	void* comp_ptr = entity.try_get_mut(comp.id);
	if (!comp_ptr) {
		ImGui::TextDisabled("(Cannot access component data)");
		return;
	}

	// Push component name as ID to avoid conflicts with same-named fields in different components
	ImGui::PushID(comp.name.c_str());

	bool modified = false;

	for (const auto& field : comp.fields) {
		void* field_ptr = static_cast<char*>(comp_ptr) + field.offset;

		switch (field.type) {
		case engine::ecs::FieldType::Bool:
		{
			if (ImGui::Checkbox(field.name.c_str(), static_cast<bool*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Int:
		{
			if (ImGui::InputInt(field.name.c_str(), static_cast<int*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Float:
		{
			if (ImGui::InputFloat(field.name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::String:
		{
			auto* str = static_cast<std::string*>(field_ptr);
			char buffer[256];
			std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
			buffer[sizeof(buffer) - 1] = '\0';
			if (ImGui::InputText(field.name.c_str(), buffer, sizeof(buffer))) {
				*str = buffer;
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Vec2:
		{
			if (ImGui::InputFloat2(field.name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Vec3:
		{
			if (ImGui::InputFloat3(field.name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Vec4:
		{
			if (ImGui::InputFloat4(field.name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::Color:
		{
			if (ImGui::ColorEdit4(field.name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::ecs::FieldType::ReadOnly:
		{
			ImGui::Text("%s: (read-only)", field.name.c_str());
			break;
		}
		case engine::ecs::FieldType::ListInt:
		case engine::ecs::FieldType::ListFloat:
		case engine::ecs::FieldType::ListString:
		{
			ImGui::TextDisabled("%s: (list editing not yet implemented)", field.name.c_str());
			break;
		}
		case engine::ecs::FieldType::AssetRef:
		{
			auto* str = static_cast<std::string*>(field_ptr);

			// Get list of assets of this type from the scene
			std::vector<std::string> asset_names;
			asset_names.push_back(""); // Allow empty/none selection
			if (scene) {
				for (const auto& asset : scene->GetAssets().GetAll()) {
					// Get type_key for this asset's type
					if (const auto* type_info = engine::scene::AssetRegistry::Instance().GetTypeInfo(asset->type);
						type_info && type_info->type_name == field.asset_type) {
						asset_names.push_back(asset->name);
					}
				}
			}

			// Find current selection index
			int current_index = 0;
			for (size_t i = 0; i < asset_names.size(); ++i) {
				if (asset_names[i] == *str) {
					current_index = static_cast<int>(i);
					break;
				}
			}

			// Render combo
			const char* preview = str->empty() ? "(None)" : str->c_str();
			if (ImGui::BeginCombo(field.name.c_str(), preview)) {
				for (size_t i = 0; i < asset_names.size(); ++i) {
					const bool is_selected = (current_index == static_cast<int>(i));
					const char* label = asset_names[i].empty() ? "(None)" : asset_names[i].c_str();
					if (ImGui::Selectable(label, is_selected)) {
						*str = asset_names[i];
						modified = true;
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			break;
		}
		}
	}

	ImGui::PopID(); // Pop component ID

	if (modified && callbacks_.on_scene_modified) {
		entity.modified(comp.id);
		callbacks_.on_scene_modified();
	}
}

void PropertiesPanel::RenderAddComponentButton(const engine::ecs::Entity entity) const {
	ImGui::Spacing();
	if (ImGui::Button("Add Component")) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup")) {
		ImGui::TextDisabled("Available Components:");
		ImGui::Separator();

		for (const auto& registry = engine::ecs::ComponentRegistry::Instance();
			 const auto& category : registry.GetCategories()) {
			if (ImGui::BeginMenu(category.c_str())) {
				for (const auto* comp : registry.GetComponentsByCategory(category)) {
					if (entity.has(comp->id)) {
						ImGui::TextDisabled("%s (already added)", comp->name.c_str());
					}
					else if (ImGui::MenuItem(comp->name.c_str())) {
						if (callbacks_.on_add_component) {
							callbacks_.on_add_component(entity, comp->name);
						}
					}
				}
				ImGui::EndMenu();
			}
		}

		ImGui::EndPopup();
	}
}

void PropertiesPanel::RenderSceneProperties(
		engine::ecs::ECSWorld& world, const engine::ecs::Entity scene_active_camera) const {
	ImGui::Text("Scene Properties");
	ImGui::Separator();

	// Get the current scene
	auto& scene_manager = engine::scene::GetSceneManager();
	auto* scene = scene_manager.TryGetScene(scene_manager.GetActiveScene());
	if (!scene) {
		ImGui::TextDisabled("No active scene");
		return;
	}

	// Scene Name
	if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
		char name_buffer[256];
		std::string scene_name = scene->GetName();
		std::strncpy(name_buffer, scene_name.c_str(), sizeof(name_buffer) - 1);
		name_buffer[sizeof(name_buffer) - 1] = '\0';
		if (ImGui::InputText("Scene Name", name_buffer, sizeof(name_buffer))) {
			scene->SetName(name_buffer);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}

		// Author
		char author_buffer[256];
		std::string author = scene->GetAuthor();
		std::strncpy(author_buffer, author.c_str(), sizeof(author_buffer) - 1);
		author_buffer[sizeof(author_buffer) - 1] = '\0';
		if (ImGui::InputText("Author", author_buffer, sizeof(author_buffer))) {
			scene->SetAuthor(author_buffer);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}

		// Description
		char desc_buffer[512];
		std::string description = scene->GetDescription();
		std::strncpy(desc_buffer, description.c_str(), sizeof(desc_buffer) - 1);
		desc_buffer[sizeof(desc_buffer) - 1] = '\0';
		if (ImGui::InputTextMultiline("Description", desc_buffer, sizeof(desc_buffer))) {
			scene->SetDescription(desc_buffer);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
	}

	// Rendering Settings
	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Background color
		glm::vec4 bg_color = scene->GetBackgroundColor();
		if (ImGui::ColorEdit4("Background Color", &bg_color[0])) {
			scene->SetBackgroundColor(bg_color);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}

		// Ambient light
		glm::vec4 ambient = scene->GetAmbientLight();
		if (ImGui::ColorEdit4("Ambient Light", &ambient[0])) {
			scene->SetAmbientLight(ambient);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
	}

	// Physics Settings (placeholder until F18)
	if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::vec2 gravity = scene->GetGravity();
		if (ImGui::InputFloat2("Gravity", &gravity[0])) {
			scene->SetGravity(gravity);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
		ImGui::TextDisabled("(Physics system coming in F18)");
	}

	// Collect all entities with Camera component (excluding EditorCamera)
	std::vector<flecs::entity> camera_entities;
	const flecs::world& flecs_world = world.GetWorld();

	flecs_world.query<engine::components::Camera>().each(
			[&](const flecs::entity entity, const engine::components::Camera&) {
				// HACK: Filter out EditorCamera - it's not part of the scene
				if (const char* name = entity.name().c_str(); name && std::string(name) == "EditorCamera") {
					return;
				}
				camera_entities.push_back(entity);
			});

	// Active Camera selection (uses scene_active_camera, not ECS active camera which is the editor camera)
	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Build preview string from scene_active_camera (not ECS active camera)
		std::string preview = "(None)";
		int current_index = -1;
		if (scene_active_camera.is_valid()) {
			preview = scene_active_camera.name().c_str();
			if (preview.empty()) {
				preview = "Entity_" + std::to_string(scene_active_camera.id());
			}
			// Find index
			for (size_t i = 0; i < camera_entities.size(); ++i) {
				if (camera_entities[i] == scene_active_camera) {
					current_index = static_cast<int>(i);
					break;
				}
			}
		}

		if (ImGui::BeginCombo("Active Camera", preview.c_str())) {
			// Option to clear active camera
			if (ImGui::Selectable("(None)", current_index == -1)) {
				if (callbacks_.on_scene_camera_changed) {
					callbacks_.on_scene_camera_changed(flecs::entity()); // Pass invalid entity to clear
				}
				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
			}

			for (size_t i = 0; i < camera_entities.size(); ++i) {
				const auto& cam_entity = camera_entities[i];
				std::string cam_name = cam_entity.name().c_str();
				if (cam_name.empty()) {
					cam_name = "Entity_" + std::to_string(cam_entity.id());
				}

				const bool is_selected = (static_cast<int>(i) == current_index);
				if (ImGui::Selectable(cam_name.c_str(), is_selected)) {
					if (callbacks_.on_scene_camera_changed) {
						callbacks_.on_scene_camera_changed(cam_entity);
					}
					if (callbacks_.on_scene_modified) {
						callbacks_.on_scene_modified();
					}
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (camera_entities.empty()) {
			ImGui::TextDisabled("No cameras in scene");
			ImGui::TextDisabled("Add a Camera component to an entity");
		}
	}
}

void PropertiesPanel::RenderAssetProperties(engine::scene::Scene* scene, const AssetSelection& selected_asset) const {
	if (!scene || !selected_asset.IsValid()) {
		return;
	}

	// Find the asset
	const auto asset = scene->GetAssets().Find(selected_asset.name, selected_asset.type);
	if (!asset) {
		ImGui::TextDisabled("Asset not found");
		return;
	}

	// Get type info for field metadata
	const auto* type_info = engine::scene::AssetRegistry::Instance().GetTypeInfo(selected_asset.type);
	if (!type_info) {
		ImGui::TextDisabled("Unknown asset type");
		return;
	}

	ImGui::Text("%s: %s", type_info->display_name.c_str(), asset->name.c_str());
	ImGui::Separator();

	if (type_info->fields.empty()) {
		ImGui::TextDisabled("(No editable fields)");
		return;
	}

	bool modified = false;

	// Render each field based on its type and offset
	for (const auto& field : type_info->fields) {
		void* field_ptr = reinterpret_cast<char*>(asset.get()) + field.offset;

		switch (field.type) {
		case engine::scene::AssetFieldType::String:
		case engine::scene::AssetFieldType::FilePath:
		{
			auto* str = static_cast<std::string*>(field_ptr);
			char buffer[256];
			std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
			buffer[sizeof(buffer) - 1] = '\0';
			if (ImGui::InputText(field.display_name.c_str(), buffer, sizeof(buffer))) {
				*str = buffer;
				modified = true;
			}
			// For file paths, add a hint
			if (field.type == engine::scene::AssetFieldType::FilePath) {
				ImGui::SameLine();
				ImGui::TextDisabled("(?)");
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Relative path to file");
				}
			}
			break;
		}
		case engine::scene::AssetFieldType::Int:
		{
			if (ImGui::InputInt(field.display_name.c_str(), static_cast<int*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::scene::AssetFieldType::Float:
		{
			if (ImGui::InputFloat(field.display_name.c_str(), static_cast<float*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::scene::AssetFieldType::Bool:
		{
			if (ImGui::Checkbox(field.display_name.c_str(), static_cast<bool*>(field_ptr))) {
				modified = true;
			}
			break;
		}
		case engine::scene::AssetFieldType::Selection:
		{
			auto* str = static_cast<std::string*>(field_ptr);
			// Find current selection index
			int current_index = 0;
			for (size_t i = 0; i < field.options.size(); ++i) {
				if (field.options[i] == *str) {
					current_index = static_cast<int>(i);
					break;
				}
			}
			// Build combo items
			if (ImGui::BeginCombo(field.display_name.c_str(), str->c_str())) {
				for (size_t i = 0; i < field.options.size(); ++i) {
					const bool is_selected = (current_index == static_cast<int>(i));
					if (ImGui::Selectable(field.options[i].c_str(), is_selected)) {
						*str = field.options[i];
						modified = true;
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			break;
		}
		case engine::scene::AssetFieldType::ReadOnly:
		{
			ImGui::Text("%s: (read-only)", field.display_name.c_str());
			break;
		}
		}
	}

	if (modified && callbacks_.on_scene_modified) {
		callbacks_.on_scene_modified();
	}
}

} // namespace editor
