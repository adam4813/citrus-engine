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
 * - Save/Load: JSON-based shader asset format
 */
class ShaderEditorPanel {
public:
	ShaderEditorPanel();
	~ShaderEditorPanel();

	/**
	 * @brief Render the shader editor panel
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
	 * @brief Create a new empty shader
	 */
	void NewShader();

	/**
	 * @brief Open a shader from file
	 */
	bool OpenShader(const std::string& path);

	/**
	 * @brief Save the current shader to file
	 */
	bool SaveShader(const std::string& path);

	/**
	 * @brief Compile the current shader (validates syntax)
	 */
	bool CompileShader();

private:
	// ========================================================================
	// Rendering Methods
	// ========================================================================
	void RenderToolbar();
	void RenderCodeEditor();
	void RenderNodeGraph();
	void RenderUniformInspector();
	void RenderErrorPanel();

	// ========================================================================
	// Shader Management
	// ========================================================================
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

	// Shader data
	std::string shader_name_ = "Untitled";
	std::string vertex_source_;
	std::string fragment_source_;
	std::string current_file_path_;

	// Code editor state
	enum class ShaderTab { Vertex, Fragment };
	ShaderTab active_tab_ = ShaderTab::Vertex;

	// Uniform detection
	struct UniformInfo {
		std::string name;
		std::string type; // "float", "vec2", "vec3", "vec4", "int", "sampler2D", etc.
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
