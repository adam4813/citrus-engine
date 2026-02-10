#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <vector>

import engine;
import glm;

namespace editor {

/**
 * @brief Shader editor panel with code and node graph modes
 * 
 * Features:
 * - Code editor mode: Multi-line text editor with vertex/fragment tabs
 * - Node graph mode: Visual shader graph using GraphEditorPanel
 * - Uniform inspector: Auto-detects and displays shader uniforms
 * - Opens shader assets from scene, edits their vertex/fragment files
 */
class ShaderEditorPanel {
public:
	ShaderEditorPanel();
	~ShaderEditorPanel();

	/**
	 * @brief Render the shader editor panel
	 * @param scene The current scene (for listing shader assets)
	 */
	void Render(engine::scene::Scene* scene);

	[[nodiscard]] bool IsVisible() const { return is_visible_; }
	void SetVisible(bool visible) { is_visible_ = visible; }
	bool& VisibleRef() { return is_visible_; }

	/**
	 * @brief Open a shader asset for editing
	 * @param asset Pointer to the shader asset info
	 */
	void OpenAsset(engine::scene::ShaderAssetInfo* asset);

	/**
	 * @brief Create a new empty shader
	 */
	void NewShader();

	/**
	 * @brief Compile the current shader (validates syntax)
	 */
	bool CompileShader();

private:
	// ========================================================================
	// Rendering Methods
	// ========================================================================
	void RenderToolbar(engine::scene::Scene* scene);
	void RenderCodeEditor();
	void RenderNodeGraph();
	void RenderUniformInspector();
	void RenderErrorPanel();

	// ========================================================================
	// Shader Management
	// ========================================================================
	bool LoadSourceFromFiles();
	bool SaveSourceToFiles();
	void ExtractUniforms();
	void GenerateShaderFromGraph();
	std::string GetDefaultVertexShader() const;
	std::string GetDefaultFragmentShader() const;

	// ========================================================================
	// State
	// ========================================================================
	bool is_visible_ = true;

	// Editor mode
	enum class EditorMode { Code, NodeGraph };
	EditorMode mode_ = EditorMode::Code;

	// Current asset being edited
	engine::scene::ShaderAssetInfo* current_asset_ = nullptr;

	// Shader data
	std::string shader_name_ = "Untitled";
	std::string vertex_source_;
	std::string fragment_source_;

	// Text editor buffers (member, not static, so they update on asset change)
	char vertex_buffer_[16384]{};
	char fragment_buffer_[16384]{};
	int buffer_generation_ = 0; // Incremented on open/new to force ImGui to create fresh widget state

	// Code editor state
	enum class ShaderTab { Vertex, Fragment };
	ShaderTab active_tab_ = ShaderTab::Vertex;

	// Uniform detection
	struct UniformInfo {
		std::string name;
		std::string type;
		std::string default_value;
	};
	std::vector<UniformInfo> uniforms_;

	// Error reporting
	bool has_errors_ = false;
	std::string error_message_;

	// Graph editor state
	std::unique_ptr<engine::graph::NodeGraph> shader_graph_;

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

	// Graph rendering helpers
	void RenderGraphCanvas();
	void RenderGraphNode(const engine::graph::Node& node);
	void RenderGraphLink(const engine::graph::Link& link);
	void RenderGraphContextMenu();
	ImVec2 GetNodeScreenPos(const engine::graph::Node& node) const;
	ImVec2 GetPinScreenPos(const engine::graph::Node& node, int pin_index, bool is_output) const;
	bool HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const;
	bool FindPinUnderMouse(const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index, bool& out_is_output) const;
	int FindLinkUnderMouse(const ImVec2& mouse_pos) const;
};

/**
 * @brief Register shader-specific node types
 */
void RegisterShaderGraphNodes();

} // namespace editor
