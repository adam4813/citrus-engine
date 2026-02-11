#pragma once

#include "editor_callbacks.h"
#include "editor_panel.h"

#include <flecs.h>
#include <unordered_map>
#include <vector>

import engine;

namespace editor {

class ICommand;

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
class HierarchyPanel : public EditorPanel {
public:
	HierarchyPanel() = default;
	~HierarchyPanel() override = default;

	[[nodiscard]] std::string_view GetPanelName() const override;

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
	 * @brief Set the ECS world reference (needed for delete command)
	 */
	void SetWorld(engine::ecs::ECSWorld* world) { world_ = world; }

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

	/**
	 * @brief Check if an entity matches the current search/filter
	 */
	bool MatchesFilter(const engine::ecs::Entity entity) const;

	/**
	 * @brief Check if an entity or any of its descendants match the filter
	 */
	bool EntityOrDescendantsMatchFilter(const engine::ecs::Entity entity) const;

	EditorCallbacks callbacks_;
	std::unordered_map<uint64_t, HierarchyNodeState> node_states_;
	engine::ecs::ECSWorld* world_ = nullptr;

	// Search and filter state
	std::string search_query_;
	char search_buffer_[256] = {};
	std::string tag_filter_;
	bool filter_by_tag_ = false;
};

} // namespace editor
