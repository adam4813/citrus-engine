#include "hierarchy_panel.h"

#include "commands/entity_commands.h"
#include "commands/transform_command.h"

#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <string>

namespace editor {

std::string_view HierarchyPanel::GetPanelName() const { return "Hierarchy"; }

void HierarchyPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void HierarchyPanel::Render(const engine::scene::SceneId scene_id, const engine::ecs::Entity selected_entity) {
	if (!IsVisible()) {
		return;
	}

	ImGuiWindowClass win_class;
	win_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoDockingOverMe
										 | ImGuiDockNodeFlags_NoDockingOverOther
										 | ImGuiDockNodeFlags_NoDockingOverEmpty;
	ImGui::SetNextWindowClass(&win_class);

	ImGui::Begin("Hierarchy", &VisibleRef());

	// Search bar
	ImGui::SetNextItemWidth(-50.0f); // Leave space for clear button
	if (ImGui::InputText("##search", search_buffer_, sizeof(search_buffer_))) {
		search_query_ = search_buffer_;
	}
	ImGui::SameLine();
	if (ImGui::Button("X##clear_search")) {
		search_query_.clear();
		search_buffer_[0] = '\0';
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Clear search");
	}

	ImGui::Separator();

	// Clickable "Scene" header to deselect entity and show scene properties
	if (const bool scene_selected = !selected_entity.is_valid(); ImGui::Selectable("Scene", scene_selected)) {
		if (callbacks_.on_entity_selected) {
			callbacks_.on_entity_selected(flecs::entity()); // Pass invalid entity to deselect
		}
	}

	// Right-click context menu on Scene
	if (ImGui::BeginPopupContextItem("SceneContextMenu")) {
		if (ImGui::MenuItem("New Group")) {
			auto& scene_manager = engine::scene::GetSceneManager();
			if (auto* scene = scene_manager.TryGetScene(scene_id); scene && callbacks_.on_execute_command) {
				auto command = std::make_unique<CreateEntityCommand>(scene, engine::ecs::Entity(), "New Group");
				const auto* cmd_ptr = command.get();
				callbacks_.on_execute_command(std::move(command));
				cmd_ptr->GetCreatedEntity().add<engine::components::Group>();
			}
		}
		ImGui::EndPopup();
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
	// Check if entity matches filter
	const bool is_filtering = !search_query_.empty();
	const bool matches = MatchesFilter(entity);
	const bool has_matching_descendants = EntityOrDescendantsMatchFilter(entity);

	// Skip this entity if filtering and neither it nor its descendants match
	if (is_filtering && !matches && !has_matching_descendants) {
		return;
	}

	std::string name = entity.name().c_str();
	if (name.empty()) {
		name = "Entity_" + std::to_string(entity.id());
	}

	// Check if this is a prefab instance (has IsA relationship to a prefab)
	const bool is_prefab_instance =
			entity.has<engine::components::PrefabInstance>() || entity.target(flecs::IsA).is_valid();
	if (is_prefab_instance) {
		name = "[P] " + name; // Prefix with prefab icon
	}
	// Check if this is a group entity
	else if (const bool is_group = entity.has<engine::components::Group>(); is_group) {
		name = "[G] " + name; // Prefix with folder icon
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

	// Dim entities that don't match the filter
	const bool should_dim = is_filtering && !matches;
	if (should_dim) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	}
	else if (is_prefab_instance && node_state.is_visible) {
		// Prefab instances get a distinct blue tint
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
	}
	else {
		// Visibility indicator
		ImGui::PushStyleColor(
				ImGuiCol_Text, node_state.is_visible ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	}

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
			if (callbacks_.on_execute_command) {
				auto command = std::make_unique<CreateEntityCommand>(scene, entity);
				const auto* cmd_ptr = command.get();
				callbacks_.on_execute_command(std::move(command));
				if (callbacks_.on_entity_selected) {
					callbacks_.on_entity_selected(cmd_ptr->GetCreatedEntity());
				}
			}
		}

		if (ImGui::MenuItem("New Group") && callbacks_.on_execute_command) {
			auto command = std::make_unique<CreateEntityCommand>(scene, entity, "New Group");
			const auto* cmd_ptr = command.get();
			callbacks_.on_execute_command(std::move(command));
			cmd_ptr->GetCreatedEntity().add<engine::components::Group>();
		}

		// Unparent: move entity to scene root (only if parented to a non-root entity)
		const auto parent = entity.parent();
		const auto scene_root = scene->GetSceneRoot();
		if (parent.is_valid() && parent != scene_root && ImGui::MenuItem("Unparent") && callbacks_.on_execute_command) {
			const auto parent_parent = parent.parent();
			callbacks_.on_execute_command(
					std::make_unique<ReparentEntityCommand>(
							scene, entity, parent_parent.is_valid() ? parent_parent : scene_root));
		}

		// Add Empty Parent: wrap entity in a new parent entity
		if (ImGui::MenuItem("Add Empty Parent")) {
			if (callbacks_.on_execute_command) {
				callbacks_.on_execute_command(
						std::make_unique<WrapEntityCommand>(scene, entity, parent.is_valid() ? parent : scene_root));
			}
		}

		ImGui::Separator();

		// Copy/Paste/Duplicate operations
		if (ImGui::MenuItem("Copy", "Ctrl+C")) {
			if (callbacks_.on_entity_selected) {
				callbacks_.on_entity_selected(entity);
			}
			if (callbacks_.on_copy_entity) {
				callbacks_.on_copy_entity();
			}
		}

		if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
			if (callbacks_.on_entity_selected) {
				callbacks_.on_entity_selected(entity);
			}
			if (callbacks_.on_duplicate_entity) {
				callbacks_.on_duplicate_entity();
			}
		}

		if (ImGui::MenuItem("Paste", "Ctrl+V")) {
			if (callbacks_.on_entity_selected) {
				callbacks_.on_entity_selected(entity);
			}
			if (callbacks_.on_paste_entity) {
				callbacks_.on_paste_entity();
			}
		}

		ImGui::Separator();

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
						else if (ImGui::MenuItem(comp->name.c_str()) && callbacks_.on_execute_command) {
							callbacks_.on_execute_command(
									std::make_unique<AddComponentCommand>(entity, comp->id, comp->name));
						}
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		// Prefab operations
		if (ImGui::MenuItem("Save as Prefab...")) {
			if (world_) {
				// Save to assets directory with entity name
				std::string name_str = entity.name().c_str();
				if (name_str.empty()) {
					name_str = "Entity_" + std::to_string(entity.id());
				}
				if (const std::string prefab_path = "assets/" + name_str + ".prefab.json";
					engine::scene::PrefabUtility::SaveAsPrefab(entity, *world_, prefab_path).is_valid()) {
					std::cout << "HierarchyPanel: Saved prefab to " << prefab_path << std::endl;
				}
			}
		}

		// Show prefab-specific options if this is a flecs prefab instance (has IsA relationship)
		if (const auto prefab_target = entity.target(flecs::IsA); prefab_target.is_valid()) {
			if (ImGui::MenuItem("Apply to Prefab")) {
				if (world_) {
					engine::scene::PrefabUtility::ApplyToSource(entity, *world_);
				}
			}
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Rename")) {
			if (callbacks_.on_show_rename_dialog) {
				callbacks_.on_show_rename_dialog(entity);
			}
		}
		if (ImGui::MenuItem("Delete")) {
			if (callbacks_.on_execute_command && world_) {
				// Notify before executing so selection can be cleared
				if (callbacks_.on_entity_deleted) {
					callbacks_.on_entity_deleted(entity);
				}
				auto command = std::make_unique<DeleteEntityCommand>(scene, entity, *world_);
				callbacks_.on_execute_command(std::move(command));
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
	const auto it = node_states_.find(entity_id);
	return it != node_states_.end() ? &it->second : nullptr;
}

bool HierarchyPanel::MatchesFilter(const engine::ecs::Entity entity) const {
	if (search_query_.empty()) {
		return true;
	}

	// Get entity name
	std::string name = entity.name().c_str();
	if (name.empty()) {
		name = "Entity_" + std::to_string(entity.id());
	}

	// Convert to lowercase for case-insensitive search
	std::string name_lower = name;
	std::string query_lower = search_query_;
	std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
	std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

	// Check if name contains search query
	return name_lower.find(query_lower) != std::string::npos;
}

bool HierarchyPanel::EntityOrDescendantsMatchFilter(const engine::ecs::Entity entity) const {
	// Check if entity itself matches
	if (MatchesFilter(entity)) {
		return true;
	}

	// Check if any descendant matches
	const auto children = GetChildren(entity);
	for (const auto& child : children) {
		if (EntityOrDescendantsMatchFilter(child)) {
			return true;
		}
	}

	return false;
}

} // namespace editor
