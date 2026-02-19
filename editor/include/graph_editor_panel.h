#pragma once

#include <imgui.h>
#include <memory>
#include <string>

#include "editor_panel.h"
#include "file_dialog.h"

import engine;
import glm;

namespace editor {

/**
 * @brief Basic graph editor panel
 * 
 * Provides a foundation for node graph editing using ImGui.
 * This is a minimal implementation - consumer editors (shader, texture, etc.)
 * will build on top of this for their specific needs.
 */
class GraphEditorPanel : public EditorPanel {
public:
	GraphEditorPanel();
	~GraphEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;

	/**
	 * @brief Render the graph editor panel
	 */
	void Render();

	/**
	 * @brief Get the current graph
	 */
	engine::graph::NodeGraph& GetGraph() { return *graph_; }

	/**
	 * @brief Get the current graph (const)
	 */
	[[nodiscard]] const engine::graph::NodeGraph& GetGraph() const { return *graph_; }

	/**
	 * @brief Get the per-editor node type registry
	 */
	engine::graph::NodeTypeRegistry& GetRegistry() { return registry_; }

	/**
	 * @brief Create a new empty graph
	 */
	void NewGraph();

	/**
	 * @brief Save the current graph to a file
	 */
	bool SaveGraph(const std::string& path);

	/**
	 * @brief Load a graph from a file
	 */
	bool LoadGraph(const std::string& path);

private:
	void RenderToolbar();
	void RenderCanvas();
	void RenderNode(const engine::graph::Node& node);
	void RenderLink(const engine::graph::Link& link);
	void RenderContextMenu();
	void RenderAddNodeMenu();

	// Get screen position for a node
	ImVec2 GetNodeScreenPos(const engine::graph::Node& node) const;

	// Get screen position for a pin
	ImVec2 GetPinScreenPos(const engine::graph::Node& node, int pin_index, bool is_output) const;

	// Hit-test a point against a pin circle, returns true if within radius
	bool HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const;

	// Find which pin (if any) is under the mouse. Returns true if found.
	bool FindPinUnderMouse(const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index, bool& out_is_output) const;

	// Find which link (if any) is near the mouse for selection/deletion
	int FindLinkUnderMouse(const ImVec2& mouse_pos) const;

	std::unique_ptr<engine::graph::NodeGraph> graph_;

	// Per-editor node type registry (isolated from other editors)
	engine::graph::NodeTypeRegistry registry_;

	// Editor state
	ImVec2 canvas_offset_{0.0f, 0.0f};
	float canvas_zoom_ = 1.0f;
	int selected_node_id_ = -1;
	int hovered_node_id_ = -1;
	int selected_link_id_ = -1;
	bool is_panning_ = false;
	bool is_dragging_node_ = false;
	ImVec2 pan_start_{0.0f, 0.0f};
	ImVec2 canvas_p0_{0.0f, 0.0f}; // Cached per-frame canvas origin

	// Context menu state
	enum class ContextTarget { None, Canvas, Node, Link };
	ContextTarget context_target_ = ContextTarget::None;
	int context_node_id_ = -1;
	int context_link_id_ = -1;
	ImVec2 context_menu_pos_{0.0f, 0.0f}; // Where the right-click happened

	// Connection state
	bool is_creating_link_ = false;
	int link_start_node_id_ = -1;
	int link_start_pin_index_ = -1;
	bool link_start_is_output_ = false;

	// Canvas constants
	static constexpr float GRID_SIZE = 64.0f;
	static constexpr float NODE_WIDTH = 200.0f;
	static constexpr float PIN_RADIUS = 6.0f;

	// File state
	std::string current_file_path_;
	FileDialogPopup open_dialog_{"Open Graph", FileDialogMode::Open, {".json"}};
	FileDialogPopup save_dialog_{"Save Graph As", FileDialogMode::Save, {".json"}};
};

} // namespace editor
