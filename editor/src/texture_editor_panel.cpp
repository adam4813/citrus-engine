#include "texture_editor_panel.h"
#include "asset_editor_registry.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <variant>

using json = nlohmann::json;

namespace editor {

// ============================================================================
// Node Type Registration
// ============================================================================

void RegisterTextureGraphNodes() {
	using namespace engine::graph;
	auto& registry = NodeTypeRegistry::GetGlobal();

	// Generator nodes
	{
		engine::graph::NodeTypeDefinition perlin;
		perlin.name = "Perlin Noise";
		perlin.category = "Generators";
		perlin.default_outputs = {Pin(0, "Value", PinType::Float, PinDirection::Output, 0.0f)};
		perlin.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
				Pin(0, "Scale", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Octaves", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(perlin);

		engine::graph::NodeTypeDefinition checkerboard;
		checkerboard.name = "Checkerboard";
		checkerboard.category = "Generators";
		checkerboard.default_outputs = {Pin(0, "Pattern", PinType::Float, PinDirection::Output, 0.0f)};
		checkerboard.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
				Pin(0, "Scale", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(checkerboard);

		engine::graph::NodeTypeDefinition gradient;
		gradient.name = "Gradient";
		gradient.category = "Generators";
		gradient.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		gradient.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
				Pin(0, "ColorA", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "ColorB", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(gradient);

		engine::graph::NodeTypeDefinition solid_color;
		solid_color.name = "Solid Color";
		solid_color.category = "Generators";
		solid_color.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		solid_color.default_inputs = {Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(solid_color);

		engine::graph::NodeTypeDefinition voronoi;
		voronoi.name = "Voronoi";
		voronoi.category = "Generators";
		voronoi.default_outputs = {Pin(0, "Value", PinType::Float, PinDirection::Output, 0.0f)};
		voronoi.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
				Pin(0, "Scale", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Randomness", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(voronoi);
	}

	// Math nodes
	{
		engine::graph::NodeTypeDefinition add;
		add.name = "Add";
		add.category = "Math";
		add.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		add.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(add);

		engine::graph::NodeTypeDefinition multiply;
		multiply.name = "Multiply";
		multiply.category = "Math";
		multiply.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		multiply.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(multiply);

		engine::graph::NodeTypeDefinition lerp;
		lerp.name = "Lerp";
		lerp.category = "Math";
		lerp.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		lerp.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "T", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(lerp);

		engine::graph::NodeTypeDefinition clamp;
		clamp.name = "Clamp";
		clamp.category = "Math";
		clamp.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		clamp.default_inputs = {
				Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Min", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Max", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(clamp);

		engine::graph::NodeTypeDefinition remap;
		remap.name = "Remap";
		remap.category = "Math";
		remap.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		remap.default_inputs = {
				Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "InMin", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "InMax", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "OutMin", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "OutMax", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(remap);

		engine::graph::NodeTypeDefinition power;
		power.name = "Power";
		power.category = "Math";
		power.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		power.default_inputs = {
				Pin(0, "Base", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Exponent", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(power);
	}

	// Filter nodes
	{
		engine::graph::NodeTypeDefinition blur;
		blur.name = "Blur";
		blur.category = "Filters";
		blur.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		blur.default_inputs = {
				Pin(0, "Input", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "Radius", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(blur);

		engine::graph::NodeTypeDefinition levels;
		levels.name = "Levels";
		levels.category = "Filters";
		levels.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		levels.default_inputs = {
				Pin(0, "Input", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "Min", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Max", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Gamma", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(levels);

		engine::graph::NodeTypeDefinition invert;
		invert.name = "Invert";
		invert.category = "Filters";
		invert.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		invert.default_inputs = {Pin(0, "Input", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(invert);
	}

	// Color nodes
	{
		engine::graph::NodeTypeDefinition hsv_adjust;
		hsv_adjust.name = "HSV Adjust";
		hsv_adjust.category = "Color";
		hsv_adjust.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		hsv_adjust.default_inputs = {
				Pin(0, "Input", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "H", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "S", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "V", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(hsv_adjust);

		engine::graph::NodeTypeDefinition channel_split;
		channel_split.name = "Channel Split";
		channel_split.category = "Color";
		channel_split.default_outputs = {
				Pin(0, "R", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "G", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "A", PinType::Float, PinDirection::Output, 0.0f)};
		channel_split.default_inputs = {Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(channel_split);

		engine::graph::NodeTypeDefinition channel_merge;
		channel_merge.name = "Channel Merge";
		channel_merge.category = "Color";
		channel_merge.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		channel_merge.default_inputs = {
				Pin(0, "R", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "G", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f)};
		registry.Register(channel_merge);

		engine::graph::NodeTypeDefinition colorize;
		colorize.name = "Colorize";
		colorize.category = "Color";
		colorize.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		colorize.default_inputs = {
				Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(colorize);
	}

	// Blend nodes
	{
		engine::graph::NodeTypeDefinition blend_multiply;
		blend_multiply.name = "Blend Multiply";
		blend_multiply.category = "Blend";
		blend_multiply.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		blend_multiply.default_inputs = {
				Pin(0, "A", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "B", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(blend_multiply);

		engine::graph::NodeTypeDefinition blend_overlay;
		blend_overlay.name = "Blend Overlay";
		blend_overlay.category = "Blend";
		blend_overlay.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		blend_overlay.default_inputs = {
				Pin(0, "A", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "B", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(blend_overlay);

		engine::graph::NodeTypeDefinition blend_screen;
		blend_screen.name = "Blend Screen";
		blend_screen.category = "Blend";
		blend_screen.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		blend_screen.default_inputs = {
				Pin(0, "A", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "B", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(blend_screen);

		engine::graph::NodeTypeDefinition blend_add;
		blend_add.name = "Blend Add";
		blend_add.category = "Blend";
		blend_add.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		blend_add.default_inputs = {
				Pin(0, "A", PinType::Color, PinDirection::Input, glm::vec4(1.0f)),
				Pin(0, "B", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(blend_add);
	}

	// Texture input node
	{
		engine::graph::NodeTypeDefinition texture_sample;
		texture_sample.name = "Texture Sample";
		texture_sample.category = "Generators";
		texture_sample.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		texture_sample.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
				Pin(0, "Path", PinType::String, PinDirection::Input, std::string("")),
		};
		registry.Register(texture_sample);
	}

	// Output node
	{
		engine::graph::NodeTypeDefinition output;
		output.name = "Texture Output";
		output.category = "Output";
		output.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		output.default_inputs = {Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(output);
	}
}

// ============================================================================
// TextureEditorPanel Implementation
// ============================================================================

TextureEditorPanel::TextureEditorPanel() : texture_graph_(std::make_unique<engine::graph::NodeGraph>()) {
	// NOTE: NewTexture() is NOT called here — node types (registered in EditorScene::Initialize
	// via RegisterTextureGraphNodes) aren't available yet, and OpenGL isn't initialized.
	// OnInitialized() is called after everything is ready.

	open_dialog_.SetCallback([this](const std::string& path) { OpenTexture(path); });
	save_dialog_.SetCallback([this](const std::string& path) { SaveTexture(path); });
	export_dialog_.SetCallback([this](const std::string& path) { ExportPng(path); });
	node_path_dialog_.SetCallback([this](const std::string& path) {
		if (!texture_graph_ || node_path_dialog_node_id_ < 0) return;
		auto* node = texture_graph_->GetNode(node_path_dialog_node_id_);
		if (!node || node_path_dialog_pin_index_ >= static_cast<int>(node->inputs.size())) return;
		node->inputs[node_path_dialog_pin_index_].default_value = path;
		SetDirty(true);
		UpdatePreview();
	});
}

TextureEditorPanel::~TextureEditorPanel() {
	if (preview_texture_id_ != 0) {
		glDeleteTextures(1, &preview_texture_id_);
	}
}

std::string_view TextureEditorPanel::GetPanelName() const { return "Texture Editor"; }

void TextureEditorPanel::OnInitialized() {
	// Node types are registered and GL context is ready — safe to build the default graph.
	NewTexture();
}

void TextureEditorPanel::RegisterAssetHandlers(AssetEditorRegistry& registry) {
	registry.Register("procedural_texture", [this](const std::string& path) {
		OpenTexture(path);
		SetVisible(true);
	});
}

void TextureEditorPanel::Render() {
	if (!BeginPanel(ImGuiWindowFlags_MenuBar)) {
		return;
	}

	RenderToolbar();

	// Split layout: graph on left, preview on right
	ImGui::BeginChild(
			"GraphArea",
			ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, 0),
			true,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	RenderGraphCanvas();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("PreviewArea", ImVec2(0, 0), true);
	RenderPreviewPanel();
	ImGui::EndChild();

	EndPanel();
}

void TextureEditorPanel::RenderToolbar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewTexture();
			}
			if (ImGui::MenuItem("Open...")) {
				open_dialog_.Open();
			}
			if (ImGui::MenuItem("Save", nullptr, false, !current_file_path_.empty())) {
				SaveTexture(current_file_path_);
			}
			if (ImGui::MenuItem("Save As...")) {
				save_dialog_.Open("texture.proctex.json");
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export PNG...")) {
				export_dialog_.Open("texture.png");
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Zoom In")) {
				canvas_zoom_ = std::min(2.0f, canvas_zoom_ + 0.1f);
			}
			if (ImGui::MenuItem("Zoom Out")) {
				canvas_zoom_ = std::max(0.1f, canvas_zoom_ - 0.1f);
			}
			if (ImGui::MenuItem("Reset View")) {
				canvas_offset_ = ImVec2(0.0f, 0.0f);
				canvas_zoom_ = 1.0f;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	open_dialog_.Render();
	save_dialog_.Render();
	export_dialog_.Render();
	node_path_dialog_.Render();
}

void TextureEditorPanel::RenderGraphCanvas() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	canvas_p0_ = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	const auto canvas_p1 = ImVec2(canvas_p0_.x + canvas_sz.x, canvas_p0_.y + canvas_sz.y);

	// Draw grid background
	draw_list->AddRectFilled(canvas_p0_, canvas_p1, IM_COL32(30, 30, 30, 255));

	// Draw grid lines
	const float grid_step = GRID_SIZE * canvas_zoom_;
	for (float x = fmodf(canvas_offset_.x, grid_step); x < canvas_sz.x; x += grid_step) {
		draw_list->AddLine(
				ImVec2(canvas_p0_.x + x, canvas_p0_.y),
				ImVec2(canvas_p0_.x + x, canvas_p1.y),
				IM_COL32(50, 50, 50, 255));
	}
	for (float y = fmodf(canvas_offset_.y, grid_step); y < canvas_sz.y; y += grid_step) {
		draw_list->AddLine(
				ImVec2(canvas_p0_.x, canvas_p0_.y + y),
				ImVec2(canvas_p1.x, canvas_p0_.y + y),
				IM_COL32(50, 50, 50, 255));
	}

	// Clip rendering to canvas
	draw_list->PushClipRect(canvas_p0_, canvas_p1, true);

	// Handle pin click for link creation
	if (!is_creating_link_ && !is_dragging_node_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
		&& ImGui::IsWindowHovered()) {
		const ImVec2 mouse_pos = ImGui::GetMousePos();
		int pin_node_id = -1, pin_index = -1;
		bool pin_is_output = false;
		if (FindPinUnderMouse(mouse_pos, pin_node_id, pin_index, pin_is_output)) {
			is_creating_link_ = true;
			link_start_node_id_ = pin_node_id;
			link_start_pin_index_ = pin_index;
			link_start_is_output_ = pin_is_output;
		}
	}

	// Render all links first (so they appear behind nodes)
	for (const auto& link : texture_graph_->GetLinks()) {
		RenderGraphLink(link);
	}

	// Render all nodes
	hovered_node_id_ = -1;
	for (const auto& node : texture_graph_->GetNodes()) {
		RenderGraphNode(node);
	}

	// Render in-progress link
	if (is_creating_link_) {
		if (const auto* start_node = texture_graph_->GetNode(link_start_node_id_)) {
			const ImVec2 start_pos = GetPinScreenPos(*start_node, link_start_pin_index_, link_start_is_output_);
			const ImVec2 end_pos = ImGui::GetMousePos();

			const ImVec2 p1 = link_start_is_output_ ? start_pos : end_pos;
			const ImVec2 p2 = link_start_is_output_ ? end_pos : start_pos;
			const float distance = std::abs(p2.x - p1.x);
			const float offset = std::max(distance * 0.5f, 50.0f);
			const auto cp1 = ImVec2(p1.x + offset, p1.y);
			const auto cp2 = ImVec2(p2.x - offset, p2.y);

			draw_list->AddBezierCubic(p1, cp1, cp2, p2, IM_COL32(200, 200, 100, 128), 2.0f * canvas_zoom_);
		}

		// Complete or cancel link
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			const ImVec2 mouse_pos = ImGui::GetMousePos();
			int end_node_id = -1, end_pin_index = -1;
			bool end_is_output = false;
			if (FindPinUnderMouse(mouse_pos, end_node_id, end_pin_index, end_is_output)) {
				// Must connect output→input (different directions)
				if (end_is_output != link_start_is_output_ && end_node_id != link_start_node_id_) {
					if (link_start_is_output_) {
						texture_graph_->AddLink(link_start_node_id_, link_start_pin_index_, end_node_id, end_pin_index);
						SetDirty(true);
					}
					else {
						texture_graph_->AddLink(end_node_id, end_pin_index, link_start_node_id_, link_start_pin_index_);
						SetDirty(true);
					}
					UpdatePreview();
				}
			}
			is_creating_link_ = false;
		}
	}

	// Handle link selection
	if (!is_creating_link_ && !is_dragging_node_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
		&& ImGui::IsWindowHovered() && hovered_node_id_ < 0) {
		const ImVec2 mouse_pos = ImGui::GetMousePos();
		int dummy_node = -1, dummy_pin = -1;
		bool dummy_output = false;
		if (!FindPinUnderMouse(mouse_pos, dummy_node, dummy_pin, dummy_output)) {
			if (const int link_id = FindLinkUnderMouse(mouse_pos); link_id >= 0) {
				selected_link_id_ = link_id;
				selected_node_id_ = -1;
			}
			else {
				selected_link_id_ = -1;
				selected_node_id_ = -1;
			}
		}
	}

	// Delete key removes selected node or link
	if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		if (selected_link_id_ >= 0) {
			texture_graph_->RemoveLink(selected_link_id_);
			selected_link_id_ = -1;
			SetDirty(true);
			UpdatePreview();
		}
		else if (selected_node_id_ >= 0) {
			texture_graph_->RemoveNode(selected_node_id_);
			selected_node_id_ = -1;
			SetDirty(true);
			UpdatePreview();
		}
	}

	// Handle panning
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
		is_panning_ = true;
		pan_start_ = ImGui::GetMousePos();
	}

	if (is_panning_) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
			const ImVec2 mouse_pos = ImGui::GetMousePos();
			canvas_offset_.x += mouse_pos.x - pan_start_.x;
			canvas_offset_.y += mouse_pos.y - pan_start_.y;
			pan_start_ = mouse_pos;
		}
		else {
			is_panning_ = false;
		}
	}

	// Handle zoom
	if (ImGui::IsWindowHovered()) {
		if (const float wheel = ImGui::GetIO().MouseWheel; wheel != 0.0f) {
			const float zoom_delta = wheel * 0.1f;
			canvas_zoom_ = std::max(0.1f, std::min(2.0f, canvas_zoom_ + zoom_delta));
		}
	}

	// Right-click context menu
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		const ImVec2 mouse_pos = ImGui::GetMousePos();
		context_menu_pos_ = mouse_pos;

		if (hovered_node_id_ >= 0) {
			context_target_ = ContextTarget::Node;
			context_node_id_ = hovered_node_id_;
		}
		else {
			if (const int link_under_mouse = FindLinkUnderMouse(mouse_pos); link_under_mouse >= 0) {
				context_target_ = ContextTarget::Link;
				context_link_id_ = link_under_mouse;
			}
			else {
				context_target_ = ContextTarget::Canvas;
			}
		}
		ImGui::OpenPopup("TextureCanvasContextMenu");
	}

	RenderGraphContextMenu();

	draw_list->PopClipRect();
}

void TextureEditorPanel::RenderPreviewPanel() {
	ImGui::Text("Preview");
	ImGui::Separator();

	// Resolution selector
	ImGui::Text("Resolution:");
	const char* resolutions[] = {"64", "128", "256", "512", "1024"};
	int current_res_idx = 2; // Default to 256
	if (preview_resolution_ == 64) {
		current_res_idx = 0;
	}
	else if (preview_resolution_ == 128) {
		current_res_idx = 1;
	}
	else if (preview_resolution_ == 256) {
		current_res_idx = 2;
	}
	else if (preview_resolution_ == 512) {
		current_res_idx = 3;
	}
	else if (preview_resolution_ == 1024) {
		current_res_idx = 4;
	}

	if (ImGui::Combo("##Resolution", &current_res_idx, resolutions, IM_ARRAYSIZE(resolutions))) {
		switch (current_res_idx) {
		case 0: preview_resolution_ = 64; break;
		case 1: preview_resolution_ = 128; break;
		case 2: preview_resolution_ = 256; break;
		case 3: preview_resolution_ = 512; break;
		case 4: preview_resolution_ = 1024; break;
		}
		UpdatePreview();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Draw preview with actual generated texture
	constexpr ImVec2 preview_size(256, 256);
	const ImVec2 preview_pos = ImGui::GetCursorScreenPos();
	const auto preview_max = ImVec2(preview_pos.x + preview_size.x, preview_pos.y + preview_size.y);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw checkerboard background for alpha
	constexpr int checker_size = 16;
	for (int y = 0; y < static_cast<int>(preview_size.y); y += checker_size) {
		for (int x = 0; x < static_cast<int>(preview_size.x); x += checker_size) {
			const bool is_dark = ((x / checker_size) + (y / checker_size)) % 2 == 0;
			const ImU32 color = is_dark ? IM_COL32(100, 100, 100, 255) : IM_COL32(150, 150, 150, 255);
			draw_list->AddRectFilled(
					ImVec2(preview_pos.x + x, preview_pos.y + y),
					ImVec2(preview_pos.x + std::min(x + checker_size, static_cast<int>(preview_size.x)),
						   preview_pos.y + std::min(y + checker_size, static_cast<int>(preview_size.y))),
					color);
		}
	}

	// Draw generated texture if available
	if (preview_texture_id_ != 0) {
		ImGui::SetCursorScreenPos(preview_pos);
		ImGui::Image((ImTextureID)(uintptr_t)preview_texture_id_, preview_size);
	}
	else {
		// Fallback: flat color
		const ImU32 preview_color = IM_COL32(
				static_cast<int>(preview_color_.r * 255),
				static_cast<int>(preview_color_.g * 255),
				static_cast<int>(preview_color_.b * 255),
				static_cast<int>(preview_color_.a * 255));
		draw_list->AddRectFilled(preview_pos, preview_max, preview_color);
		ImGui::Dummy(preview_size);
	}

	// Draw border
	draw_list->AddRect(preview_pos, preview_max, IM_COL32(200, 200, 200, 255), 0.0f, 0, 2.0f);

	// Export button
	ImGui::Spacing();
	if (ImGui::Button("Export PNG...", ImVec2(-1, 0))) {
		export_dialog_.Open("texture.png");
	}

	// Display color values
	ImGui::Spacing();
	ImGui::Text("Output Color:");
	ImGui::Text("R: %.3f", preview_color_.r);
	ImGui::Text("G: %.3f", preview_color_.g);
	ImGui::Text("B: %.3f", preview_color_.b);
	ImGui::Text("A: %.3f", preview_color_.a);
}

void TextureEditorPanel::RenderGraphNode(const engine::graph::Node& node) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const ImVec2 node_pos = GetNodeScreenPos(node);

	// Calculate node height
	const int pin_count = std::max(static_cast<int>(node.inputs.size()), static_cast<int>(node.outputs.size()));
	const float node_height = 40.0f + pin_count * 20.0f;

	const ImVec2 node_rect_min = node_pos;
	const auto node_rect_max = ImVec2(node_pos.x + NODE_WIDTH * canvas_zoom_, node_pos.y + node_height * canvas_zoom_);

	// Draw node background
	const bool is_selected = (node.id == selected_node_id_);
	const ImU32 node_bg_color = is_selected ? IM_COL32(80, 80, 120, 255) : IM_COL32(60, 60, 60, 255);
	draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
	draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f, 0, 2.0f);

	// Draw node title
	const auto title_pos = ImVec2(node_pos.x + 5.0f, node_pos.y + 5.0f);
	draw_list->AddText(title_pos, IM_COL32(255, 255, 255, 255), node.type_name.c_str());

	// InvisibleButton submitted first — SetNextItemAllowOverlap lets widgets submitted
	// after it claim hover/active state over the button
	ImGui::SetNextItemAllowOverlap();
	ImGui::SetCursorScreenPos(node_rect_min);
	ImGui::InvisibleButton(
			("node_" + std::to_string(node.id)).c_str(),
			ImVec2(node_rect_max.x - node_rect_min.x, node_rect_max.y - node_rect_min.y));
	const bool node_active = ImGui::IsItemActive();
	const bool node_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

	// Inline editors for unconnected input pins (submitted after InvisibleButton to win input)
	auto* mutable_node = texture_graph_->GetNode(node.id);
	if (mutable_node && canvas_zoom_ >= 0.7f) {
		for (size_t i = 0; i < mutable_node->inputs.size(); ++i) {
			bool has_link = false;
			for (const auto& link : texture_graph_->GetLinks()) {
				if (link.to_node_id == node.id && link.to_pin_index == static_cast<int>(i)) {
					has_link = true;
					break;
				}
			}
			if (has_link) continue;

			const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), false);
			ImGui::SetCursorScreenPos(ImVec2(node_pos.x + 100.0f * canvas_zoom_, pin_pos.y - 9.0f));

			auto& pin = mutable_node->inputs[i];
			bool changed = false;
			const std::string pin_id = "##pv_" + std::to_string(node.id) + "_" + std::to_string(i);

			using engine::graph::PinType;
			switch (pin.type) {
			case PinType::Float: {
				const auto* p = std::get_if<float>(&pin.default_value);
				float val = p ? *p : 0.0f;
				ImGui::PushItemWidth(90.0f * canvas_zoom_);
				if (ImGui::DragFloat(pin_id.c_str(), &val, 0.01f, 0.0f, 0.0f, "%.2f")) {
					pin.default_value = val;
					changed = true;
				}
				ImGui::PopItemWidth();
				break;
			}
			case PinType::Int: {
				const auto* p = std::get_if<int>(&pin.default_value);
				int val = p ? *p : 0;
				ImGui::PushItemWidth(90.0f * canvas_zoom_);
				if (ImGui::DragInt(pin_id.c_str(), &val)) {
					pin.default_value = val;
					changed = true;
				}
				ImGui::PopItemWidth();
				break;
			}
			case PinType::Bool: {
				const auto* p = std::get_if<bool>(&pin.default_value);
				bool val = p ? *p : false;
				if (ImGui::Checkbox(pin_id.c_str(), &val)) {
					pin.default_value = val;
					changed = true;
				}
				break;
			}
			case PinType::Color:
			case PinType::Vec4: {
				const auto* p = std::get_if<glm::vec4>(&pin.default_value);
				glm::vec4 col = p ? *p : glm::vec4(1.0f);
				float arr[4] = {col.x, col.y, col.z, col.w};
				if (ImGui::ColorEdit4(pin_id.c_str(), arr, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
					pin.default_value = glm::vec4(arr[0], arr[1], arr[2], arr[3]);
					changed = true;
				}
				break;
			}
			case PinType::Vec2: {
				const auto* p = std::get_if<glm::vec2>(&pin.default_value);
				glm::vec2 val = p ? *p : glm::vec2(0.0f);
				float arr[2] = {val.x, val.y};
				ImGui::PushItemWidth(90.0f * canvas_zoom_);
				if (ImGui::DragFloat2(pin_id.c_str(), arr, 0.01f)) {
					pin.default_value = glm::vec2(arr[0], arr[1]);
					changed = true;
				}
				ImGui::PopItemWidth();
				break;
			}
			case PinType::Vec3: {
				const auto* p = std::get_if<glm::vec3>(&pin.default_value);
				glm::vec3 val = p ? *p : glm::vec3(0.0f);
				float arr[3] = {val.x, val.y, val.z};
				ImGui::PushItemWidth(90.0f * canvas_zoom_);
				if (ImGui::DragFloat3(pin_id.c_str(), arr, 0.01f)) {
					pin.default_value = glm::vec3(arr[0], arr[1], arr[2]);
					changed = true;
				}
				ImGui::PopItemWidth();
				break;
			}
			case PinType::String: {
				const auto* p = std::get_if<std::string>(&pin.default_value);
				char buf[256];
				strncpy_s(buf, p ? p->c_str() : "", sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = '\0';
				ImGui::PushItemWidth(64.0f * canvas_zoom_);
				if (ImGui::InputText(pin_id.c_str(), buf, sizeof(buf))) {
					pin.default_value = std::string(buf);
					changed = true;
				}
				ImGui::PopItemWidth();
				ImGui::SameLine(0, 2);
				if (ImGui::Button(("...##br_" + std::to_string(node.id) + "_" + std::to_string(i)).c_str(),
						ImVec2(22.0f * canvas_zoom_, 0))) {
					node_path_dialog_node_id_ = node.id;
					node_path_dialog_pin_index_ = static_cast<int>(i);
					node_path_dialog_.Open();
				}
				break;
			}
			default: break;
			}

			if (changed) {
				SetDirty(true);
				UpdatePreview();
			}
		}
	}

	// Draw input pin circles and labels on top of inline editors
	const ImVec2 mouse_pos = ImGui::GetMousePos();
	for (size_t i = 0; i < node.inputs.size(); ++i) {
		const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), false);
		const bool pin_hovered = HitTestPin(mouse_pos, pin_pos);
		const ImU32 pin_color = pin_hovered ? IM_COL32(150, 255, 150, 255) : IM_COL32(100, 200, 100, 255);
		const float radius = pin_hovered ? PIN_RADIUS * canvas_zoom_ * 1.3f : PIN_RADIUS * canvas_zoom_;
		draw_list->AddCircleFilled(pin_pos, radius, pin_color);
		const auto label_pos = ImVec2(pin_pos.x + 10.0f, pin_pos.y - 7.0f);
		draw_list->AddText(label_pos, IM_COL32(200, 200, 200, 255), node.inputs[i].name.c_str());
	}

	// Draw output pins
	for (size_t i = 0; i < node.outputs.size(); ++i) {
		const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), true);
		const bool pin_hovered = HitTestPin(mouse_pos, pin_pos);
		const ImU32 pin_color = pin_hovered ? IM_COL32(255, 150, 150, 255) : IM_COL32(200, 100, 100, 255);
		const float radius = pin_hovered ? PIN_RADIUS * canvas_zoom_ * 1.3f : PIN_RADIUS * canvas_zoom_;
		draw_list->AddCircleFilled(pin_pos, radius, pin_color);
		const char* label = node.outputs[i].name.c_str();
		const ImVec2 label_size = ImGui::CalcTextSize(label);
		const auto label_pos = ImVec2(pin_pos.x - label_size.x - 10.0f, pin_pos.y - 7.0f);
		draw_list->AddText(label_pos, IM_COL32(200, 200, 200, 255), label);
	}

	// Hover detection via rect (reliable when node contains multiple widgets)
	if (ImGui::IsMouseHoveringRect(node_rect_min, node_rect_max)) {
		hovered_node_id_ = node.id;
	}

	if (node_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		if (!is_creating_link_) {
			is_dragging_node_ = true;
			selected_node_id_ = node.id;
			selected_link_id_ = -1;
		}
	}

	if (node_clicked && !is_creating_link_) {
		selected_node_id_ = node.id;
		selected_link_id_ = -1;
	}

	// Node dragging
	if (is_dragging_node_ && selected_node_id_ == node.id && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		if (auto* mn = texture_graph_->GetNode(node.id)) {
			mn->position.x += delta.x / canvas_zoom_;
			mn->position.y += delta.y / canvas_zoom_;
			SetDirty(true);
		}
	}

	if (is_dragging_node_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		is_dragging_node_ = false;
	}
}

void TextureEditorPanel::RenderGraphLink(const engine::graph::Link& link) {
	const engine::graph::Node* from_node = texture_graph_->GetNode(link.from_node_id);
	const engine::graph::Node* to_node = texture_graph_->GetNode(link.to_node_id);

	if (!from_node || !to_node) {
		return;
	}

	if (link.from_pin_index >= static_cast<int>(from_node->outputs.size())
		|| link.to_pin_index >= static_cast<int>(to_node->inputs.size())) {
		return;
	}

	const ImVec2 p1 = GetPinScreenPos(*from_node, link.from_pin_index, true);
	const ImVec2 p2 = GetPinScreenPos(*to_node, link.to_pin_index, false);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw bezier curve
	const float distance = std::abs(p2.x - p1.x);
	const float offset = distance * 0.5f;
	const auto cp1 = ImVec2(p1.x + offset, p1.y);
	const auto cp2 = ImVec2(p2.x - offset, p2.y);

	const ImU32 link_color =
			(link.id == selected_link_id_) ? IM_COL32(255, 255, 100, 255) : IM_COL32(200, 200, 100, 255);
	const float link_thickness = (link.id == selected_link_id_) ? 3.0f * canvas_zoom_ : 2.0f * canvas_zoom_;
	draw_list->AddBezierCubic(p1, cp1, cp2, p2, link_color, link_thickness);
}

void TextureEditorPanel::RenderGraphContextMenu() {
	if (ImGui::BeginPopup("TextureCanvasContextMenu")) {
		switch (context_target_) {
		case ContextTarget::Canvas: RenderAddNodeMenu(); break;

		case ContextTarget::Node:
			if (ImGui::MenuItem("Delete Node")) {
				texture_graph_->RemoveNode(context_node_id_);
				SetDirty(true);
				if (selected_node_id_ == context_node_id_) {
					selected_node_id_ = -1;
				}
				UpdatePreview();
			}
			break;

		case ContextTarget::Link:
			if (ImGui::MenuItem("Delete Connection")) {
				texture_graph_->RemoveLink(context_link_id_);
				SetDirty(true);
				if (selected_link_id_ == context_link_id_) {
					selected_link_id_ = -1;
				}
				UpdatePreview();
			}
			break;

		default: break;
		}

		ImGui::EndPopup();
	}
}

void TextureEditorPanel::RenderAddNodeMenu() {
	auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
	auto categories = registry.GetCategories();

	if (categories.empty()) {
		ImGui::TextDisabled("No node types registered");
		return;
	}

	for (const auto& category : categories) {
		if (ImGui::BeginMenu(category.c_str())) {
			for (auto types = registry.GetByCategory(category); const auto* type : types) {
				if (ImGui::MenuItem(type->name.c_str())) {
					const float x = (context_menu_pos_.x - canvas_p0_.x - canvas_offset_.x) / canvas_zoom_;
					const float y = (context_menu_pos_.y - canvas_p0_.y - canvas_offset_.y) / canvas_zoom_;

					const int node_id = texture_graph_->AddNode(type->name, glm::vec2(x, y));
					SetDirty(true);

					if (auto* node = texture_graph_->GetNode(node_id)) {
						node->inputs = type->default_inputs;
						node->outputs = type->default_outputs;
					}

					UpdatePreview();
				}
			}

			ImGui::EndMenu();
		}
	}
}

// ============================================================================
// Graph Helper Methods
// ============================================================================

ImVec2 TextureEditorPanel::GetNodeScreenPos(const engine::graph::Node& node) const {
	return ImVec2(
			canvas_p0_.x + canvas_offset_.x + node.position.x * canvas_zoom_,
			canvas_p0_.y + canvas_offset_.y + node.position.y * canvas_zoom_);
}

ImVec2 TextureEditorPanel::GetPinScreenPos(const engine::graph::Node& node, int pin_index, bool is_output) const {
	const ImVec2 node_pos = GetNodeScreenPos(node);
	const float y_offset = 30.0f + pin_index * 20.0f;

	return ImVec2(
			is_output ? node_pos.x + NODE_WIDTH * canvas_zoom_ : node_pos.x, node_pos.y + y_offset * canvas_zoom_);
}

bool TextureEditorPanel::HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const {
	const float radius = PIN_RADIUS * canvas_zoom_ * 2.0f;
	const float dx = point.x - pin_pos.x;
	const float dy = point.y - pin_pos.y;
	return (dx * dx + dy * dy) <= (radius * radius);
}

bool TextureEditorPanel::FindPinUnderMouse(
		const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index, bool& out_is_output) const {
	for (const auto& node : texture_graph_->GetNodes()) {
		for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
			if (HitTestPin(mouse_pos, GetPinScreenPos(node, i, true))) {
				out_node_id = node.id;
				out_pin_index = i;
				out_is_output = true;
				return true;
			}
		}
		for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
			if (HitTestPin(mouse_pos, GetPinScreenPos(node, i, false))) {
				out_node_id = node.id;
				out_pin_index = i;
				out_is_output = false;
				return true;
			}
		}
	}
	return false;
}

int TextureEditorPanel::FindLinkUnderMouse(const ImVec2& mouse_pos) const {
	for (const auto& link : texture_graph_->GetLinks()) {
		const auto* from_node = texture_graph_->GetNode(link.from_node_id);
		const auto* to_node = texture_graph_->GetNode(link.to_node_id);
		if (!from_node || !to_node) {
			continue;
		}
		if (link.from_pin_index >= static_cast<int>(from_node->outputs.size())
			|| link.to_pin_index >= static_cast<int>(to_node->inputs.size())) {
			continue;
		}

		const ImVec2 p1 = GetPinScreenPos(*from_node, link.from_pin_index, true);
		const ImVec2 p2 = GetPinScreenPos(*to_node, link.to_pin_index, false);

		const float distance = std::abs(p2.x - p1.x);
		const float offset = distance * 0.5f;
		const auto cp1 = ImVec2(p1.x + offset, p1.y);
		const auto cp2 = ImVec2(p2.x - offset, p2.y);

		constexpr int samples = 20;
		for (int s = 0; s <= samples; ++s) {
			const float t = static_cast<float>(s) / static_cast<float>(samples);
			const float u = 1.0f - t;
			const float x = u * u * u * p1.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x + t * t * t * p2.x;
			const float y = u * u * u * p1.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y + t * t * t * p2.y;
			const float dx = mouse_pos.x - x;
			const float dy = mouse_pos.y - y;
			if (constexpr float threshold = 8.0f; dx * dx + dy * dy <= threshold * threshold) {
				return link.id;
			}
		}
	}
	return -1;
}

// ============================================================================
// File Operations
// ============================================================================

void TextureEditorPanel::NewTexture() {
	texture_graph_ = std::make_unique<engine::graph::NodeGraph>();
	texture_name_ = "Untitled";
	current_file_path_ = "";
	selected_node_id_ = -1;
	selected_link_id_ = -1;
	hovered_node_id_ = -1;
	is_dragging_node_ = false;
	is_creating_link_ = false;
	canvas_offset_ = ImVec2(0.0f, 0.0f);
	canvas_zoom_ = 1.0f;

	// Create default graph: Solid Color -> Output
	const int color_node_id = texture_graph_->AddNode("Solid Color", glm::vec2(100.0f, 100.0f));
	const int output_node_id = texture_graph_->AddNode("Texture Output", glm::vec2(400.0f, 100.0f));

	if (auto* color_node = texture_graph_->GetNode(color_node_id)) {
		auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
		if (const auto* type = registry.Get("Solid Color")) {
			color_node->inputs = type->default_inputs;
			color_node->outputs = type->default_outputs;
		}
	}

	if (auto* output_node = texture_graph_->GetNode(output_node_id)) {
		const auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
		if (const auto* type = registry.Get("Texture Output")) {
			output_node->inputs = type->default_inputs;
			output_node->outputs = type->default_outputs;
		}
	}

	// Connect Solid Color output to Output input
	texture_graph_->AddLink(color_node_id, 0, output_node_id, 0);

	//UpdatePreview();  -- called by OnInitialized() after GL is ready; OpenTexture() calls it directly
	SetDirty(false);
}

bool TextureEditorPanel::OpenTexture(const std::string& path) {
	if (engine::graph::NodeGraph new_graph; engine::graph::GraphSerializer::Load(path, new_graph)) {
		*texture_graph_ = std::move(new_graph);
		current_file_path_ = path;
		selected_node_id_ = -1;
		hovered_node_id_ = -1;
		UpdatePreview();
		SetDirty(false);
		return true;
	}
	return false;
}

bool TextureEditorPanel::SaveTexture(const std::string& path) {
	// Serialize graph, then override asset_type for texture files
	auto json_str = engine::graph::GraphSerializer::Serialize(*texture_graph_);
	auto j = nlohmann::json::parse(json_str);
	j["asset_type"] = "procedural_texture";

	if (engine::assets::AssetManager::SaveTextFile(std::filesystem::path(path), j.dump(2))) {
		current_file_path_ = path;
		SetDirty(false);
		return true;
	}
	return false;
}

void TextureEditorPanel::UpdatePreview() {
	preview_dirty_ = true;
	GenerateTextureData();
	UploadPreviewTexture();
	preview_dirty_ = false;
}

} // namespace editor
