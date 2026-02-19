#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "editor_panel.h"
#include "file_dialog.h"

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

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
class TextureEditorPanel : public EditorPanel {
public:
	TextureEditorPanel();
	~TextureEditorPanel() override;

	[[nodiscard]] std::string_view GetPanelName() const override;
	void RegisterAssetHandlers(AssetEditorRegistry& registry) override;
	void OnInitialized() override;

	/**
	 * @brief Render the texture editor panel
	 */
	void Render();

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
	// Texture Generation — buffer-per-node pipeline
	// ========================================================================
	void UpdatePreview();
	void GenerateTextureData();
	void UploadPreviewTexture();
	void ExportPng(const std::string& path);

	struct NodeBuffer {
		std::vector<glm::vec4> pixels;
		int width = 0;
		int height = 0;
		GLuint thumbnail_tex = 0;
	};

	std::vector<int> TopologicalSort() const;
	void EvaluateGraphToBuffers();
	void EvaluateNodeToBuffer(int node_id);
	glm::vec4 EvaluateNodePixel(const engine::graph::Node& node, glm::vec2 uv) const;
	glm::vec4 SampleBuffer(int node_id, int output_pin, glm::vec2 uv) const;
	float SampleInputFloat(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const;
	glm::vec4 SampleInputColor(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const;
	glm::vec2 SampleInputVec2(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const;
	void UploadNodeThumbnails();
	void CleanupNodeBuffers();

	// ========================================================================
	// State
	// ========================================================================

	// Texture data
	std::string texture_name_ = "Untitled";
	std::string current_file_path_;

	// Preview settings
	int preview_resolution_ = 256;
	glm::vec4 preview_color_{1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<unsigned char> preview_pixels_;
	GLuint preview_texture_id_ = 0;
	bool preview_dirty_ = true;

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
	static constexpr float NODE_WIDTH = 200.0f;
	static constexpr float PIN_RADIUS = 6.0f;
	static constexpr float THUMBNAIL_SIZE = 64.0f;

	// File dialogs
	FileDialogPopup open_dialog_{"Open Texture", FileDialogMode::Open, {".json"}};
	FileDialogPopup save_dialog_{"Save Texture As", FileDialogMode::Save, {".json"}};
	FileDialogPopup export_dialog_{"Export PNG", FileDialogMode::Save, {".png"}};
	FileDialogPopup node_path_dialog_{"Select Image", FileDialogMode::Open, {".png", ".jpg", ".jpeg", ".bmp", ".hdr"}};

	// Per-editor node type registry (isolated from other editors)
	engine::graph::NodeTypeRegistry registry_;

	// Node path dialog state
	int node_path_dialog_node_id_ = -1;
	int node_path_dialog_pin_index_ = -1;

	// Sampler cache for Input Image nodes (keyed by file path)
	struct SamplerEntry { std::vector<uint8_t> pixels; int width = 0; int height = 0; };
	mutable std::unordered_map<std::string, SamplerEntry> sampler_cache_;

	// Per-node evaluation buffers (keyed by node_id)
	mutable std::unordered_map<int, NodeBuffer> node_buffers_;
};

/**
 * @brief Register texture-specific node types
 */
void RegisterTextureGraphNodes(engine::graph::NodeTypeRegistry& registry);

} // namespace editor
