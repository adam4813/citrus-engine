#include "properties_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace editor {

void PropertiesPanel::SetCallbacks(const EditorCallbacks& callbacks) {
	callbacks_ = callbacks;
}

void PropertiesPanel::Render(engine::ecs::Entity selected_entity) {
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

		RenderTransformSection(selected_entity);
		RenderAddComponentButton(selected_entity);
	}
	else {
		ImGui::TextDisabled("No entity selected");
		ImGui::TextDisabled("Select an entity in the Hierarchy panel");
	}

	ImGui::End();
}

void PropertiesPanel::RenderTransformSection(engine::ecs::Entity entity) {
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	if (entity.has<engine::ecs::SceneEntity>()) {
		const auto& [name, visible, static_entity, scene_layer] = entity.get<engine::ecs::SceneEntity>();
		ImGui::Text("Name: %s", name.c_str());
		ImGui::Text("Visible: %s", visible ? "Yes" : "No");
		ImGui::Text("Static: %s", static_entity ? "Yes" : "No");
		ImGui::Text("Scene Layer: %u", scene_layer);
	}

	if (entity.has<engine::components::Transform>()) {
		const auto& initial_transform = entity.get<engine::components::Transform>();
		auto position = initial_transform.position;
		auto rotation = initial_transform.rotation;
		auto scale = initial_transform.scale;

		bool modified = false;
		modified |= ImGui::InputFloat3("Position", &position.x);
		modified |= ImGui::InputFloat3("Rotation", &rotation.x);
		modified |= ImGui::InputFloat3("Scale", &scale.x);

		if (modified) {
			auto& transform = entity.get_mut<engine::components::Transform>();
			transform.position = position;
			transform.rotation = rotation;
			transform.scale = scale;

			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
	}

	ImGui::TextDisabled("(Component editing coming soon)");
}

void PropertiesPanel::RenderAddComponentButton(engine::ecs::Entity entity) {
	ImGui::Spacing();
	if (ImGui::Button("Add Component")) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup")) {
		ImGui::TextDisabled("Available Components:");
		ImGui::Separator();
		if (ImGui::MenuItem("Sprite Renderer")) {
			// TODO: Add sprite renderer component
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
		if (ImGui::MenuItem("Camera")) {
			// TODO: Add camera component
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
		ImGui::EndPopup();
	}
}

} // namespace editor
