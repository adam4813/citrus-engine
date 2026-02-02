#include "hierarchy_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <string>

namespace editor {

void HierarchyPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void HierarchyPanel::Render(const engine::scene::SceneId scene_id, const engine::ecs::Entity selected_entity) {
	if (!is_visible_) {
		return;
	}

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoDockingOverMe
										 | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Hierarchy", &is_visible_);

	// Clickable "Scene" header to deselect entity and show scene properties
	if (const bool scene_selected = !selected_entity.is_valid(); ImGui::Selectable("Scene", scene_selected)) {
		if (callbacks_.on_entity_selected) {
			callbacks_.on_entity_selected(flecs::entity()); // Pass invalid entity to deselect
		}
	}
	ImGui::Separator();

	auto& scene_manager = engine::scene::GetSceneManager();

	if (auto* scene = scene_manager.TryGetScene(scene_id)) {
		// Get only root-level entities (direct children of scene root)

		if (const auto root_entities = GetChildren(scene->GetSceneRoot()); root_entities.empty()) {
			ImGui::TextDisabled("No entities in scene");
			ImGui::TextDisabled("Use Scene > Add Entity to create one");
		}
		else {
			for (const auto& entity : root_entities) {
				RenderEntityNode(entity, scene, selected_entity);
			}
		}
	}
	else {
		ImGui::TextDisabled("No active scene");
	}

	ImGui::End();
}

std::vector<engine::ecs::Entity> HierarchyPanel::GetChildren(const engine::ecs::Entity entity) {
	std::vector<engine::ecs::Entity> children;
	entity.children([&children](const engine::ecs::Entity child) { children.push_back(child); });
	return children;
}

void HierarchyPanel::RenderEntityNode(
		const engine::ecs::Entity entity, engine::scene::Scene* scene, const engine::ecs::Entity selected_entity) {
	std::string name = entity.name().c_str();
	if (name.empty()) {
		name = "Entity_" + std::to_string(entity.id());
	}

	// Ensure node state exists
	auto& node_state = node_states_[entity.id()];
	node_state.entity = entity;

	// Get children to determine if this is a leaf node
	const auto children = GetChildren(entity);
	const bool has_children = !children.empty();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
	if (!has_children) {
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (entity == selected_entity) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (node_state.is_expanded && has_children) {
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	// Visibility indicator
	ImGui::PushStyleColor(ImGuiCol_Text, node_state.is_visible ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

	const bool node_open =
			ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity.id())), flags, "%s", name.c_str());

	// Track expanded state
	if (has_children) {
		node_state.is_expanded = node_open;
	}

	ImGui::PopStyleColor();

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		if (callbacks_.on_entity_selected) {
			callbacks_.on_entity_selected(entity);
		}
		ImGui::ClearActiveID();
	}

	// Right-click context menu
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Add Child Entity")) {
			if (callbacks_.on_add_child_entity) {
				callbacks_.on_add_child_entity(entity);
			}
		}

		// Add Component submenu - grouped by category
		if (ImGui::BeginMenu("Add Component")) {
			const auto& registry = engine::ecs::ComponentRegistry::Instance();
			for (const auto& category : registry.GetCategories()) {
				if (ImGui::BeginMenu(category.c_str())) {
					for (const auto* comp : registry.GetComponentsByCategory(category)) {
						// Check if entity already has this component
						const bool has_component = entity.has(comp->id);
						if (has_component) {
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
			ImGui::EndMenu();
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Rename")) {
			if (callbacks_.on_show_rename_dialog) {
				callbacks_.on_show_rename_dialog(entity);
			}
		}
		if (ImGui::MenuItem("Delete")) {
			scene->DestroyEntity(entity);
			if (callbacks_.on_entity_deleted) {
				callbacks_.on_entity_deleted(entity);
			}
			if (callbacks_.on_scene_modified) {
				callbacks_.on_scene_modified();
			}
		}
		ImGui::Separator();
		if (ImGui::MenuItem(node_state.is_visible ? "Hide" : "Show")) {
			node_state.is_visible = !node_state.is_visible;
		}
		if (ImGui::MenuItem(node_state.is_locked ? "Unlock" : "Lock")) {
			node_state.is_locked = !node_state.is_locked;
		}
		ImGui::EndPopup();
	}

	// Render children recursively if node is open
	if (node_open && has_children) {
		for (const auto& child : children) {
			RenderEntityNode(child, scene, selected_entity);
		}
		ImGui::TreePop();
	}
}

void HierarchyPanel::ClearNodeState() { node_states_.clear(); }

HierarchyNodeState* HierarchyPanel::GetNodeState(const uint64_t entity_id) {
	auto it = node_states_.find(entity_id);
	return it != node_states_.end() ? &it->second : nullptr;
}

} // namespace editor
