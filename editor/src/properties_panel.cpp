#include "properties_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace editor {

void PropertiesPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void PropertiesPanel::Render(const engine::ecs::Entity selected_entity) {
	if (!is_visible_)
		return;

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton
										 | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Properties", &is_visible_);

	if (selected_entity.is_valid()) {
		std::string name = selected_entity.name().c_str();
		if (name.empty()) {
			name = "Entity_" + std::to_string(selected_entity.id());
		}

		ImGui::Text("Entity: %s", name.c_str());
		ImGui::Separator();

		RenderComponentSections(selected_entity);
		RenderAddComponentButton(selected_entity);
	}
	else {
		ImGui::TextDisabled("No entity selected");
		ImGui::TextDisabled("Select an entity in the Hierarchy panel");
	}

	ImGui::End();
}

void PropertiesPanel::RenderComponentSections(const engine::ecs::Entity entity) const {
	for (const auto& registry = engine::ecs::ComponentRegistry::Instance();
		 const auto& comp : registry.GetComponents()) {
		if (!entity.has(comp.id)) {
			continue;
		}

		if (ImGui::CollapsingHeader(comp.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (comp.fields.empty()) {
				ImGui::TextDisabled("(No editable fields)");
			}
			else {
				RenderComponentFields(entity, comp);
			}
		}
	}
}

void PropertiesPanel::RenderComponentFields(
		const engine::ecs::Entity entity, const engine::ecs::ComponentInfo& comp) const {
	// Get raw pointer to component data
	void* comp_ptr = entity.try_get_mut(comp.id);
	if (!comp_ptr) {
		ImGui::TextDisabled("(Cannot access component data)");
		return;
	}

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
			strncpy_s(buffer, str->c_str(), sizeof(buffer) - 1);
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
		}
	}

	if (modified && callbacks_.on_scene_modified) {
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

} // namespace editor
