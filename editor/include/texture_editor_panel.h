#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

import engine;
import glm;

namespace editor {

/**
 * @brief Procedural texture editor panel
 * 
 * Features:
 * - Node graph for texture generation
 * - Preview panel with resolution settings
 * - Generator nodes: Perlin, Checkerboard, Gradient, Solid Color, Voronoi
 * - Math nodes: Add, Multiply, Lerp, Clamp, Remap, Power
 * - Filter nodes: Blur, Levels, Invert
 * - Color nodes: HSV Adjust, Channel Split/Merge, Colorize
 * - Blend nodes: Multiply, Overlay, Screen, Add
 * - Save/Load: .proctex.json format
 */
class TextureEditorPanel {
public:
	TextureEditorPanel();
	~TextureEditorPanel();

	/**
	 * @brief Render the texture editor panel
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
	 * @brief Create a new empty texture graph
	 */
	void NewTexture();

	/**
	 * @brief Open a texture from file
	 */
	bool OpenTexture(const std::string& path);

	/**
	 * @brief Save the current texture to file
	 */
	bool SaveTexture(const std::string& path);

private:
	// ========================================================================
	// Rendering Methods
	// ========================================================================
	void RenderToolbar();
	void RenderGraphCanvas();
	void RenderPreviewPanel();
	void RenderGraphNode(const engine::graph::Node& node);
	void RenderGraphLink(const engine::graph::Link& link);
	void RenderGraphContextMenu();
	void RenderAddNodeMenu();

	// ========================================================================
	// Graph Helpers
	// ========================================================================
	ImVec2 GetNodeScreenPos(const engine::graph::Node& node) const;
	ImVec2 GetPinScreenPos(const engine::graph::Node& node, int pin_index, bool is_output) const;
	bool HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const;
	bool FindPinUnderMouse(const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index, bool& out_is_output) const;
	int FindLinkUnderMouse(const ImVec2& mouse_pos) const;

	// ========================================================================
	// Texture Generation
	// ========================================================================
	void UpdatePreview();
	glm::vec4 EvaluateOutputColor() const;

	// ========================================================================
	// State
	// ========================================================================
	bool is_visible_ = true;

	// Texture data
	std::string texture_name_ = "Untitled";
	std::string current_file_path_;

	// Preview settings
	int preview_resolution_ = 256;
	glm::vec4 preview_color_{1.0f, 1.0f, 1.0f, 1.0f};

	// Graph state
	std::unique_ptr<engine::graph::NodeGraph> texture_graph_;

	// Canvas state for node graph rendering
	ImVec2 canvas_offset_{0.0f, 0.0f};
	float canvas_zoom_ = 1.0f;
	int selected_node_id_ = -1;
	int hovered_node_id_ = -1;
	int selected_link_id_ = -1;
	bool is_panning_ = false;
	bool is_dragging_node_ = false;
	ImVec2 pan_start_{0.0f, 0.0f};
	ImVec2 canvas_p0_{0.0f, 0.0f};

	// Context menu state
	enum class ContextTarget { None, Canvas, Node, Link };
	ContextTarget context_target_ = ContextTarget::None;
	int context_node_id_ = -1;
	int context_link_id_ = -1;
	ImVec2 context_menu_pos_{0.0f, 0.0f};

	// Connection state
	bool is_creating_link_ = false;
	int link_start_node_id_ = -1;
	int link_start_pin_index_ = -1;
	bool link_start_is_output_ = false;

	// Canvas constants
	static constexpr float GRID_SIZE = 64.0f;
	static constexpr float NODE_WIDTH = 150.0f;
	static constexpr float PIN_RADIUS = 6.0f;
};

/**
 * @brief Register texture-specific node types
 */
void RegisterTextureGraphNodes();

} // namespace editor
