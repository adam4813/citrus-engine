#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

import engine;

namespace editor {

/**
 * @brief Behavior Tree Editor Panel
 *
 * Provides a visual editor for creating and editing behavior trees.
 * Features:
 * - Tree visualization (top-down hierarchy)
 * - Node selection and property editing
 * - Add/remove nodes via context menu
 * - Save/load behavior trees as .bt.json files
 */
class BehaviorTreeEditorPanel {
public:
	BehaviorTreeEditorPanel();
	~BehaviorTreeEditorPanel();

	/**
	 * @brief Render the behavior tree editor panel
	 */
	void Render();

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
	 * @brief Create a new empty behavior tree
	 */
	void NewTree();

	/**
	 * @brief Save the current behavior tree to a file
	 */
	bool SaveTree(const std::string& path);

	/**
	 * @brief Load a behavior tree from a file
	 */
	bool LoadTree(const std::string& path);

private:
	void RenderToolbar();
	void RenderTreeView();
	void RenderNodeTree(engine::ai::BTNode* node, int depth = 0);
	void RenderPropertiesPanel();
	void RenderContextMenu();

	// Helper to create nodes
	std::unique_ptr<engine::ai::BTNode> CreateNode(const std::string& node_type, const std::string& name);

	bool is_visible_ = false;
	std::unique_ptr<engine::ai::BTNode> root_node_;
	engine::ai::BTNode* selected_node_ = nullptr;
	std::string current_file_path_;

	// UI state
	ImVec2 scroll_pos_{0.0F, 0.0F};
	bool show_new_dialog_ = false;
	bool show_open_dialog_ = false;
	bool show_save_dialog_ = false;
};

} // namespace editor
