#pragma once

#include "editor_callbacks.h"

#include <flecs.h>
#include <unordered_map>
#include <vector>

import engine;

namespace editor {

/**
 * @brief Per-node state for hierarchy tree items
 */
struct HierarchyNodeState {
	engine::ecs::Entity entity;
	bool is_expanded = false; // Tree node open/closed
	bool is_visible = true;   // Visibility in viewport
	bool is_locked = false;   // Prevent edits
};

/**
 * @brief Scene hierarchy tree panel
 *
 * Displays entities in a tree view with selection, context menus,
 * and per-node state (expanded, visible, locked).
 */
class HierarchyPanel {
public:
	HierarchyPanel() = default;
	~HierarchyPanel() = default;

	/**
	 * @brief Set callbacks for panel events
	 */
	void SetCallbacks(const EditorCallbacks& callbacks);

	/**
	 * @brief Render the hierarchy panel
	 * @param scene_id The scene to display entities from
	 * @param selected_entity Currently selected entity (for highlighting)
	 */
	void Render(engine::scene::SceneId scene_id, engine::ecs::Entity selected_entity);

	/**
	 * @brief Check if panel is visible
	 */
	[[nodiscard]] bool IsVisible() const { return is_visible_; }

	/**
	 * @brief Set panel visibility
	 */
	void SetVisible(bool visible) { is_visible_ = visible; }

	/**
	 * @brief Get mutable reference to visibility (for ImGui::MenuItem binding)
	 */
	bool& VisibleRef() { return is_visible_; }

	/**
	 * @brief Clear all node state (call when scene changes)
	 */
	void ClearNodeState();

	/**
	 * @brief Get node state for an entity
	 */
	[[nodiscard]] HierarchyNodeState* GetNodeState(uint64_t entity_id);

private:
	/**
	 * @brief Render a single entity node and its children recursively
	 */
	void RenderEntityNode(engine::ecs::Entity entity, engine::scene::Scene* scene, engine::ecs::Entity selected_entity);

	/**
	 * @brief Get direct children of an entity
	 */
	static std::vector<engine::ecs::Entity> GetChildren(engine::ecs::Entity entity);

	EditorCallbacks callbacks_;
	bool is_visible_ = true;
	std::unordered_map<uint64_t, HierarchyNodeState> node_states_;
};

} // namespace editor
