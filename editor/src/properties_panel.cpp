#include "properties_panel.h"

#include <cstring>
#include <flecs.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "commands/transform_command.h"
#include "file_dialog.h"

namespace editor {

namespace {

/// Static file dialog for field browse buttons (only one can be active at a time)
std::optional<FileDialogPopup> s_field_file_dialog;
bool s_field_file_dialog_modified = false;

/// Open a file browse dialog targeting a string field
void OpenFieldBrowseDialog(
		const char* title,
		const std::vector<std::string>& extensions,
		std::string* target) {
	s_field_file_dialog.emplace(title, FileDialogMode::Open, extensions);
	s_field_file_dialog->SetCallback([target](const std::string& path) {
		*target = path;
		s_field_file_dialog_modified = true;
	});
	s_field_file_dialog->Open();
}

/// Get the display label for a field (prefer display_name, fall back to name)
const char* GetFieldLabel(const engine::ecs::FieldInfo& field) {
	return field.display_name.empty() ? field.name.c_str() : field.display_name.c_str();
}

/// Check if a field should be visible given the current component/asset data.
/// Returns false if a visibility predicate is set and the controlling field's value doesn't match.
bool IsFieldVisible(
		const engine::ecs::FieldInfo& field,
		const std::vector<engine::ecs::FieldInfo>& all_fields,
		const void* base_ptr) {
	if (field.visible_when_field.empty()) {
		return true;
	}
	for (const auto& other : all_fields) {
		if (other.name == field.visible_when_field) {
			const int value = *reinterpret_cast<const int*>(static_cast<const char*>(base_ptr) + other.offset);
			for (const int v : field.visible_when_values) {
				if (value == v) {
					return true;
				}
			}
			return false;
		}
	}
	return true; // Controlling field not found — show by default
}

/// Render a single field based on its FieldType. Returns true if modified.
bool RenderSingleField(const engine::ecs::FieldInfo& field, void* field_ptr, engine::scene::Scene* scene) {
	const char* label = GetFieldLabel(field);

	switch (field.type) {
	case engine::ecs::FieldType::Bool:
		return ImGui::Checkbox(label, static_cast<bool*>(field_ptr));

	case engine::ecs::FieldType::Int:
		return ImGui::InputInt(label, static_cast<int*>(field_ptr));

	case engine::ecs::FieldType::Float:
		return ImGui::InputFloat(label, static_cast<float*>(field_ptr));

	case engine::ecs::FieldType::String:
	{
		auto* str = static_cast<std::string*>(field_ptr);
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
		auto* str = static_cast<std::string*>(field_ptr);
		char buffer[256];
		std::strncpy(buffer, str->c_str(), sizeof(buffer) - 1);
		buffer[sizeof(buffer) - 1] = '\0';
		bool modified = false;

		// Use reduced width to make room for browse button
		ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - 30.0f);
		if (ImGui::InputText(label, buffer, sizeof(buffer))) {
			*str = buffer;
			modified = true;
		}
		ImGui::SameLine();
		std::string browse_id = std::string("...##filepath_browse_") + field.name;
		if (ImGui::Button(browse_id.c_str())) {
			auto exts = field.file_extensions; // use field-specified extensions if any
			OpenFieldBrowseDialog("Browse File", exts, str);
		}
		return modified;
	}
	case engine::ecs::FieldType::Vec2:
		return ImGui::InputFloat2(label, static_cast<float*>(field_ptr));

	case engine::ecs::FieldType::Vec3:
		return ImGui::InputFloat3(label, static_cast<float*>(field_ptr));

	case engine::ecs::FieldType::Vec4:
		return ImGui::InputFloat4(label, static_cast<float*>(field_ptr));

	case engine::ecs::FieldType::Color:
		return ImGui::ColorEdit4(label, static_cast<float*>(field_ptr));

	case engine::ecs::FieldType::ReadOnly:
		ImGui::Text("%s: (read-only)", label);
		return false;

	case engine::ecs::FieldType::Enum:
	{
		auto* value = static_cast<int*>(field_ptr);
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
		auto* str = static_cast<std::string*>(field_ptr);
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
	case engine::ecs::FieldType::ListFloat:
	case engine::ecs::FieldType::ListString:
		ImGui::TextDisabled("%s: (list editing not yet implemented)", label);
		return false;

	case engine::ecs::FieldType::AssetRef:
	{
		auto* str = static_cast<std::string*>(field_ptr);

		// Get list of assets of this type from the scene
		std::vector<std::string> asset_names;
		asset_names.push_back(""); // Allow empty/none selection
		if (scene) {
			for (const auto& asset : scene->GetAssets().GetAll()) {
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

		// Browse button for file-based asset selection
		ImGui::SameLine();
		{
			std::string browse_id = std::string("...##assetref_browse_") + field.name;
			if (ImGui::Button(browse_id.c_str())) {
				auto exts = field.file_extensions.empty()
					? std::vector<std::string>{".json"}
					: field.file_extensions;
				OpenFieldBrowseDialog("Browse Asset", exts, str);
			}
		}

		// Audio preview button for sound asset references
		if (field.asset_type == "sound" && !str->empty() && scene) {
			static bool s_audio_playing = false;
			static uint32_t s_clip_id = 0;
			static uint32_t s_play_handle = 0;
			static std::string s_playing_asset;

			// Check if current playback finished
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
					// Stop current playback
					audio.StopSound(s_play_handle);
					audio.UnloadClip(s_clip_id);
					s_audio_playing = false;
					s_clip_id = 0;
					s_play_handle = 0;
					s_playing_asset.clear();
				} else {
					// Stop any existing preview first
					if (s_audio_playing) {
						audio.StopSound(s_play_handle);
						audio.UnloadClip(s_clip_id);
						s_clip_id = 0;
						s_play_handle = 0;
						s_playing_asset.clear();
						s_audio_playing = false;
					}
					// Load and play the referenced sound asset
					auto sound_asset = scene->GetAssets().FindTyped<engine::scene::SoundAssetInfo>(*str);
					if (sound_asset && !sound_asset->file_path.empty()) {
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
	}
	return false;
}

/// Shape type labels for compound child shape combo
static constexpr const char* kChildShapeTypeLabels[] = {"Box", "Sphere", "Capsule", "Cylinder"};
static constexpr int kChildShapeTypeCount = 4;

/// Render the compound shape children editor. Returns true if any child was modified.
bool RenderCompoundChildren(engine::physics::CollisionShape& shape) {
	bool modified = false;
	auto& children = shape.compound_children;

	ImGui::Separator();
	ImGui::Text("Compound Children (%d)", static_cast<int>(children.size()));

	int remove_index = -1;

	for (size_t i = 0; i < children.size(); ++i) {
		auto& child = children[i];
		ImGui::PushID(static_cast<int>(i));

		// Collapsible header per child with remove button
		int type_idx = static_cast<int>(child.type);
		const char* type_label = (type_idx >= 0 && type_idx < kChildShapeTypeCount) ? kChildShapeTypeLabels[type_idx] : "Unknown";
		std::string header = "Child " + std::to_string(i) + " (" + type_label + ")";
		bool child_open = ImGui::CollapsingHeader(
				header.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
		ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 20.0f);
		if (ImGui::SmallButton("X")) {
			remove_index = static_cast<int>(i);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Remove child shape");
		}

		if (child_open) {
			// Shape type combo (only basic shape types for children)
			int type_val = static_cast<int>(child.type);
			if (ImGui::Combo("Shape Type", &type_val, kChildShapeTypeLabels, kChildShapeTypeCount)) {
				child.type = static_cast<engine::physics::ShapeType>(type_val);
				modified = true;
			}

			// Shape-specific parameters
			switch (child.type) {
			case engine::physics::ShapeType::Box:
				if (ImGui::InputFloat3("Half Extents", &child.box_half_extents[0])) {
					modified = true;
				}
				break;
			case engine::physics::ShapeType::Sphere:
				if (ImGui::InputFloat("Radius", &child.sphere_radius)) {
					modified = true;
				}
				break;
			case engine::physics::ShapeType::Capsule:
				if (ImGui::InputFloat("Radius##cap", &child.capsule_radius)) {
					modified = true;
				}
				if (ImGui::InputFloat("Height##cap", &child.capsule_height)) {
					modified = true;
				}
				break;
			case engine::physics::ShapeType::Cylinder:
				if (ImGui::InputFloat("Radius##cyl", &child.cylinder_radius)) {
					modified = true;
				}
				if (ImGui::InputFloat("Height##cyl", &child.cylinder_height)) {
					modified = true;
				}
				break;
			default: break;
			}

			// Position and rotation (shared by all child types)
			if (ImGui::InputFloat3("Position", &child.position[0])) {
				modified = true;
			}
			// Display rotation as euler angles for usability
			glm::vec3 euler = glm::degrees(glm::eulerAngles(child.rotation));
			if (ImGui::InputFloat3("Rotation", &euler[0])) {
				child.rotation = glm::quat(glm::radians(euler));
				modified = true;
			}
		}

		ImGui::PopID();
	}

	// Remove pending child
	if (remove_index >= 0) {
		children.erase(children.begin() + remove_index);
		modified = true;
	}

	// Add child button
	if (ImGui::Button("+ Add Child Shape")) {
		children.emplace_back();
		modified = true;
	}

	return modified;
}

} // anonymous namespace

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

	if (scene != last_scene_) {
		last_scene_ = scene;
		physics_backend_changed_ = false;
	}

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

	// Render any active field file picker dialog
	if (s_field_file_dialog) {
		s_field_file_dialog->Render();
	}
	if (s_field_file_dialog_modified) {
		s_field_file_dialog_modified = false;
		if (callbacks_.on_scene_modified) {
			callbacks_.on_scene_modified();
		}
	}

	ImGui::End();
}

void PropertiesPanel::RenderComponentSections(const engine::ecs::Entity entity, engine::scene::Scene* scene) const {
	flecs::id_t pending_remove_id = 0;
	std::string pending_remove_name;

	for (const auto& registry = engine::ecs::ComponentRegistry::Instance();
		 const auto& comp : registry.GetComponents()) {
		if (!entity.has(comp.id)) {
			continue;
		}

		ImGui::PushID(static_cast<int>(comp.id));

		// Right-aligned remove button on the header line
		bool header_open = true;
		const bool removable = comp.name != "Transform" && comp.name != "WorldTransform";

		if (removable) {
			header_open = ImGui::CollapsingHeader(
					comp.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
			ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 20.0f);
			if (ImGui::SmallButton("X")) {
				pending_remove_id = comp.id;
				pending_remove_name = comp.name;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Remove %s", comp.name.c_str());
			}
		}
		else {
			header_open = ImGui::CollapsingHeader(comp.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
		}

		if (header_open) {
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

		ImGui::PopID();
	}

	// Execute pending remove outside the iteration loop
	if (pending_remove_id != 0 && callbacks_.on_execute_command) {
		callbacks_.on_execute_command(
				std::make_unique<RemoveComponentCommand>(entity, pending_remove_id, pending_remove_name));
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
		if (!IsFieldVisible(field, comp.fields, comp_ptr)) {
			continue;
		}
		void* field_ptr = static_cast<char*>(comp_ptr) + field.offset;
		if (RenderSingleField(field, field_ptr, scene)) {
			modified = true;
		}
	}

	// Custom rendering for CollisionShape compound children
	if (comp.name == "CollisionShape") {
		auto& shape = *static_cast<engine::physics::CollisionShape*>(comp_ptr);
		if (shape.type == engine::physics::ShapeType::Compound) {
			if (RenderCompoundChildren(shape)) {
				modified = true;
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
					if (comp->hidden) {
						continue;
					}
					if (entity.has(comp->id)) {
						ImGui::TextDisabled("%s (already added)", comp->name.c_str());
					}
					else if (ImGui::MenuItem(comp->name.c_str()) && callbacks_.on_execute_command) {
						callbacks_.on_execute_command(
								std::make_unique<AddComponentCommand>(entity, comp->id, comp->name));
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

	// Physics Settings
	if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Physics backend selector
		static const char* backend_labels[] = {"Jolt Physics", "Bullet3", "None"};
		static const char* backend_values[] = {"jolt", "bullet3", "none"};

		std::string current_backend = scene->GetPhysicsBackend();
		int current_idx = 0;
		for (int i = 0; i < 3; ++i) {
			if (current_backend == backend_values[i]) {
				current_idx = i;
				break;
			}
		}

		if (ImGui::Combo("Physics Backend", &current_idx, backend_labels, 3)) {
			scene->SetPhysicsBackend(backend_values[current_idx]);
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
			// Physics backend change requires scene reload to take effect
			physics_backend_changed_ = true;
		}

		if (physics_backend_changed_) {
			ImGui::TextColored(ImVec4(1.0F, 0.8F, 0.2F, 1.0F), "Save and reload scene to apply backend change.");
		}

		if (const flecs::world& fw = world.GetWorld(); fw.has<engine::physics::PhysicsWorldConfig>()) {
			auto& [gravity, fixed_timestep, max_substeps, enable_sleeping, show_debug_physics] =
					fw.get_mut<engine::physics::PhysicsWorldConfig>();
			if (ImGui::InputFloat3("Gravity", &gravity[0])) {
				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
			}
			if (ImGui::InputFloat("Fixed Timestep", &fixed_timestep, 0.001F, 0.01F, "%.4f")) {
				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
			}
			if (ImGui::InputInt("Max Substeps", &max_substeps)) {
				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
			}
			if (ImGui::Checkbox("Enable Sleeping", &enable_sleeping)) {
				if (callbacks_.on_scene_modified) {
					callbacks_.on_scene_modified();
				}
			}
			ImGui::Checkbox("Show Debug Physics", &show_debug_physics);
		}
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

	for (const auto& field : type_info->fields) {
		if (!IsFieldVisible(field, type_info->fields, asset.get())) {
			continue;
		}
		void* field_ptr = reinterpret_cast<char*>(asset.get()) + field.offset;
		if (RenderSingleField(field, field_ptr, scene)) {
			modified = true;
		}
	}

	if (modified && callbacks_.on_scene_modified) {
		callbacks_.on_scene_modified();
	}
}

} // namespace editor
