#include "texture_editor_panel.h"
#include "asset_editor_registry.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>
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
	NewTexture();

	open_dialog_.SetCallback([this](const std::string& path) { OpenTexture(path); });
	save_dialog_.SetCallback([this](const std::string& path) { SaveTexture(path); });
	export_dialog_.SetCallback([this](const std::string& path) { ExportPng(path); });
}

TextureEditorPanel::~TextureEditorPanel() {
	if (preview_texture_id_ != 0) {
		glDeleteTextures(1, &preview_texture_id_);
	}
}

std::string_view TextureEditorPanel::GetPanelName() const { return "Texture Editor"; }

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

	// Image asset picker
	ImGui::Text("Image Asset:");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::InputText(
				"##ImagePath", image_path_buf_, sizeof(image_path_buf_), ImGuiInputTextFlags_EnterReturnsTrue)) {
		// Update any Texture Sample nodes with this path
		for (const auto& node : texture_graph_->GetNodes()) {
			if (node.type_name == "Texture Sample") {
				if (auto* mutable_node = texture_graph_->GetNode(node.id)) {
					for (auto& input : mutable_node->inputs) {
						if (input.name == "Path") {
							input.default_value = std::string(image_path_buf_);
						}
					}
				}
			}
		}
		UpdatePreview();
	}
	ImGui::TextDisabled("Enter path and press Enter");

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
	const ImVec2 node_size(NODE_WIDTH * canvas_zoom_, 0);

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

	// Draw input pins
	const ImVec2 mouse_pos = ImGui::GetMousePos();
	for (size_t i = 0; i < node.inputs.size(); ++i) {
		const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), false);
		const bool pin_hovered = HitTestPin(mouse_pos, pin_pos);
		const ImU32 pin_color = pin_hovered ? IM_COL32(150, 255, 150, 255) : IM_COL32(100, 200, 100, 255);
		const float radius = pin_hovered ? PIN_RADIUS * canvas_zoom_ * 1.3f : PIN_RADIUS * canvas_zoom_;
		draw_list->AddCircleFilled(pin_pos, radius, pin_color);

		// Pin label
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

		// Pin label (right-aligned)
		const char* label = node.outputs[i].name.c_str();
		const ImVec2 label_size = ImGui::CalcTextSize(label);
		const auto label_pos = ImVec2(pin_pos.x - label_size.x - 10.0f, pin_pos.y - 7.0f);
		draw_list->AddText(label_pos, IM_COL32(200, 200, 200, 255), label);
	}

	// Handle node interaction
	ImGui::SetCursorScreenPos(node_rect_min);
	ImGui::InvisibleButton(
			("node_" + std::to_string(node.id)).c_str(),
			ImVec2(node_rect_max.x - node_rect_min.x, node_rect_max.y - node_rect_min.y));

	if (ImGui::IsItemHovered()) {
		hovered_node_id_ = node.id;
	}

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		if (!is_creating_link_) {
			is_dragging_node_ = true;
			selected_node_id_ = node.id;
			selected_link_id_ = -1;
		}
	}

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !is_creating_link_) {
		selected_node_id_ = node.id;
		selected_link_id_ = -1;
	}

	// Node dragging
	if (is_dragging_node_ && selected_node_id_ == node.id && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		const ImVec2 delta = ImGui::GetIO().MouseDelta;
		if (auto* mutable_node = texture_graph_->GetNode(node.id)) {
			mutable_node->position.x += delta.x / canvas_zoom_;
			mutable_node->position.y += delta.y / canvas_zoom_;
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

	UpdatePreview();
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

// ============================================================================
// Texture Generation — Full Node Graph Evaluation
// ============================================================================

// Simple hash-based Perlin noise implementation
namespace {

float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

float Grad(int hash, float x, float y) {
	const int h = hash & 3;
	const float u = h < 2 ? x : y;
	const float v = h < 2 ? y : x;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Permutation table
const int kPerm[512] = {
		151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
		69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
		94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
		171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
		60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
		1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
		164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
		255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
		119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
		19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
		238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
		181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
		222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,
		// repeat
		151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,
		69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,
		94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
		171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
		60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161,
		1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
		164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126,
		255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
		119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
		19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193,
		238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
		181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
		222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180};

float PerlinNoise2D(float x, float y) {
	const int xi = static_cast<int>(std::floor(x)) & 255;
	const int yi = static_cast<int>(std::floor(y)) & 255;
	const float xf = x - std::floor(x);
	const float yf = y - std::floor(y);

	const float u = Fade(xf);
	const float v = Fade(yf);

	const int aa = kPerm[kPerm[xi] + yi];
	const int ab = kPerm[kPerm[xi] + yi + 1];
	const int ba = kPerm[kPerm[xi + 1] + yi];
	const int bb = kPerm[kPerm[xi + 1] + yi + 1];

	const float x1 = std::lerp(Grad(aa, xf, yf), Grad(ba, xf - 1.0f, yf), u);
	const float x2 = std::lerp(Grad(ab, xf, yf - 1.0f), Grad(bb, xf - 1.0f, yf - 1.0f), u);
	return (std::lerp(x1, x2, v) + 1.0f) * 0.5f; // Map to [0, 1]
}

float FractionalBrownianMotion(float x, float y, int octaves) {
	float value = 0.0f;
	float amplitude = 0.5f;
	float frequency = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		value += amplitude * PerlinNoise2D(x * frequency, y * frequency);
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}
	return value;
}

// Simple hash for Voronoi cell points
glm::vec2 VoronoiRandomPoint(int ix, int iy, float randomness) {
	const int n = ix * 374761393 + iy * 668265263;
	const int hash = (n ^ (n >> 13)) * 1274126177;
	const float fx = static_cast<float>((hash & 0xFFFF)) / 65535.0f;
	const float fy = static_cast<float>(((hash >> 16) & 0xFFFF)) / 65535.0f;
	return glm::vec2(
			static_cast<float>(ix) + 0.5f + (fx - 0.5f) * randomness,
			static_cast<float>(iy) + 0.5f + (fy - 0.5f) * randomness);
}

float VoronoiNoise(float x, float y, float randomness) {
	const int cell_x = static_cast<int>(std::floor(x));
	const int cell_y = static_cast<int>(std::floor(y));

	float min_dist = 1e10f;
	for (int dy = -1; dy <= 1; ++dy) {
		for (int dx = -1; dx <= 1; ++dx) {
			const glm::vec2 pt = VoronoiRandomPoint(cell_x + dx, cell_y + dy, randomness);
			const float dist = glm::length(glm::vec2(x, y) - pt);
			min_dist = std::min(min_dist, dist);
		}
	}
	return std::clamp(min_dist, 0.0f, 1.0f);
}

// HSV <-> RGB conversion helpers
glm::vec3 RgbToHsv(glm::vec3 rgb) {
	const float cmax = std::max({rgb.r, rgb.g, rgb.b});
	const float cmin = std::min({rgb.r, rgb.g, rgb.b});
	const float delta = cmax - cmin;
	float h = 0.0f;
	if (delta > 0.0001f) {
		if (cmax == rgb.r) {
			h = std::fmod((rgb.g - rgb.b) / delta, 6.0f);
		}
		else if (cmax == rgb.g) {
			h = (rgb.b - rgb.r) / delta + 2.0f;
		}
		else {
			h = (rgb.r - rgb.g) / delta + 4.0f;
		}
		h /= 6.0f;
		if (h < 0.0f) {
			h += 1.0f;
		}
	}
	const float s = cmax > 0.0001f ? delta / cmax : 0.0f;
	return {h, s, cmax};
}

glm::vec3 HsvToRgb(glm::vec3 hsv) {
	const float h = hsv.x * 6.0f;
	const float s = hsv.y;
	const float v = hsv.z;
	const float c = v * s;
	const float x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
	const float m = v - c;
	glm::vec3 rgb;
	if (h < 1.0f) {
		rgb = {c, x, 0.0f};
	}
	else if (h < 2.0f) {
		rgb = {x, c, 0.0f};
	}
	else if (h < 3.0f) {
		rgb = {0.0f, c, x};
	}
	else if (h < 4.0f) {
		rgb = {0.0f, x, c};
	}
	else if (h < 5.0f) {
		rgb = {x, 0.0f, c};
	}
	else {
		rgb = {c, 0.0f, x};
	}
	return rgb + glm::vec3(m);
}

float OverlayBlendChannel(float a, float b) { return a < 0.5f ? 2.0f * a * b : 1.0f - 2.0f * (1.0f - a) * (1.0f - b); }

} // anonymous namespace

// Helper: extract a float from a PinValue
static float PinValueToFloat(const engine::graph::PinValue& val) {
	if (auto* f = std::get_if<float>(&val)) {
		return *f;
	}
	if (auto* i = std::get_if<int>(&val)) {
		return static_cast<float>(*i);
	}
	if (auto* v4 = std::get_if<glm::vec4>(&val)) {
		return v4->r;
	}
	return 0.0f;
}

static glm::vec4 PinValueToColor(const engine::graph::PinValue& val) {
	if (auto* v4 = std::get_if<glm::vec4>(&val)) {
		return *v4;
	}
	if (auto* f = std::get_if<float>(&val)) {
		return glm::vec4(*f, *f, *f, 1.0f);
	}
	return glm::vec4(1.0f);
}

static glm::vec2 PinValueToVec2(const engine::graph::PinValue& val) {
	if (auto* v2 = std::get_if<glm::vec2>(&val)) {
		return *v2;
	}
	if (auto* f = std::get_if<float>(&val)) {
		return glm::vec2(*f);
	}
	return glm::vec2(0.0f);
}

// Find which node/pin feeds into a given input pin
const engine::graph::Link* FindInputLink(
		const engine::graph::NodeGraph& graph, int node_id, int pin_index) {
	for (const auto& link : graph.GetLinks()) {
		if (link.to_node_id == node_id && link.to_pin_index == pin_index) {
			return &link;
		}
	}
	return nullptr;
}

float TextureEditorPanel::GetInputFloat(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		const glm::vec4 result = EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
		return result.r;
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToFloat(node.inputs[pin_index].default_value);
	}
	return 0.0f;
}

glm::vec4 TextureEditorPanel::GetInputColor(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		return EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToColor(node.inputs[pin_index].default_value);
	}
	return glm::vec4(1.0f);
}

glm::vec2 TextureEditorPanel::GetInputVec2(const engine::graph::Node& node, int pin_index, glm::vec2 uv) const {
	if (const auto* link = FindInputLink(*texture_graph_, node.id, pin_index)) {
		const glm::vec4 result = EvaluateNodeOutput(link->from_node_id, link->from_pin_index, uv);
		return glm::vec2(result.x, result.y);
	}
	if (pin_index < static_cast<int>(node.inputs.size())) {
		return PinValueToVec2(node.inputs[pin_index].default_value);
	}
	return uv; // Default UV passthrough
}

glm::vec4 TextureEditorPanel::EvaluateNodeOutput(int node_id, int pin_index, glm::vec2 uv) const {
	const auto* node = texture_graph_->GetNode(node_id);
	if (!node) {
		return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta = error
	}

	const auto& type = node->type_name;

	// --- Generators ---
	if (type == "Perlin Noise") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		// Default UV if nothing connected and default is zero
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 4.0f;
		}
		float octaves_f = GetInputFloat(*node, 2, uv);
		int octaves = octaves_f > 0.0f ? static_cast<int>(octaves_f) : 4;
		octaves = std::clamp(octaves, 1, 8);
		const float val = FractionalBrownianMotion(sample_uv.x * scale, sample_uv.y * scale, octaves);
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Checkerboard") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 8.0f;
		}
		const int cx = static_cast<int>(std::floor(sample_uv.x * scale));
		const int cy = static_cast<int>(std::floor(sample_uv.y * scale));
		const float val = ((cx + cy) % 2 == 0) ? 1.0f : 0.0f;
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Gradient") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		const glm::vec4 color_a = GetInputColor(*node, 1, uv);
		const glm::vec4 color_b = GetInputColor(*node, 2, uv);
		const float t = std::clamp(sample_uv.x, 0.0f, 1.0f);
		return glm::mix(color_a, color_b, t);
	}

	if (type == "Solid Color") {
		return GetInputColor(*node, 0, uv);
	}

	if (type == "Voronoi") {
		glm::vec2 sample_uv = GetInputVec2(*node, 0, uv);
		if (sample_uv == glm::vec2(0.0f) && !FindInputLink(*texture_graph_, node->id, 0)) {
			sample_uv = uv;
		}
		float scale = GetInputFloat(*node, 1, uv);
		if (scale <= 0.0f) {
			scale = 4.0f;
		}
		float randomness = GetInputFloat(*node, 2, uv);
		if (randomness <= 0.0f) {
			randomness = 1.0f;
		}
		const float val = VoronoiNoise(sample_uv.x * scale, sample_uv.y * scale, randomness);
		return glm::vec4(val, val, val, 1.0f);
	}

	if (type == "Texture Sample") {
		// Not implemented — would need actual image loading per pixel
		return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	}

	// --- Math ---
	if (type == "Add") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float r = a + b;
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Multiply") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float r = a * b;
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Lerp") {
		const float a = GetInputFloat(*node, 0, uv);
		const float b = GetInputFloat(*node, 1, uv);
		const float t = std::clamp(GetInputFloat(*node, 2, uv), 0.0f, 1.0f);
		const float r = std::lerp(a, b, t);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Clamp") {
		const float val = GetInputFloat(*node, 0, uv);
		const float lo = GetInputFloat(*node, 1, uv);
		float hi = GetInputFloat(*node, 2, uv);
		if (hi <= lo) {
			hi = 1.0f;
		}
		const float r = std::clamp(val, lo, hi);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Remap") {
		const float val = GetInputFloat(*node, 0, uv);
		const float in_min = GetInputFloat(*node, 1, uv);
		float in_max = GetInputFloat(*node, 2, uv);
		const float out_min = GetInputFloat(*node, 3, uv);
		const float out_max = GetInputFloat(*node, 4, uv);
		if (std::abs(in_max - in_min) < 0.0001f) {
			in_max = in_min + 1.0f;
		}
		const float t = (val - in_min) / (in_max - in_min);
		const float r = out_min + t * (out_max - out_min);
		return glm::vec4(r, r, r, 1.0f);
	}

	if (type == "Power") {
		const float base = std::max(0.0f, GetInputFloat(*node, 0, uv));
		float exponent = GetInputFloat(*node, 1, uv);
		if (exponent == 0.0f) {
			exponent = 1.0f;
		}
		const float r = std::pow(base, exponent);
		return glm::vec4(r, r, r, 1.0f);
	}

	// --- Filters ---
	if (type == "Invert") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		return glm::vec4(1.0f - input.r, 1.0f - input.g, 1.0f - input.b, input.a);
	}

	if (type == "Levels") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		float lo = GetInputFloat(*node, 1, uv);
		float hi = GetInputFloat(*node, 2, uv);
		float gamma = GetInputFloat(*node, 3, uv);
		if (hi <= lo) {
			lo = 0.0f;
			hi = 1.0f;
		}
		if (gamma <= 0.0f) {
			gamma = 1.0f;
		}
		auto apply = [&](float v) {
			v = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
			return std::pow(v, 1.0f / gamma);
		};
		return glm::vec4(apply(input.r), apply(input.g), apply(input.b), input.a);
	}

	if (type == "Blur") {
		// Approximate blur by sampling neighbors
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		float radius = GetInputFloat(*node, 1, uv);
		if (radius <= 0.0f) {
			return input;
		}
		const float step = radius / static_cast<float>(preview_resolution_);
		glm::vec4 accum(0.0f);
		int count = 0;
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const glm::vec2 offset_uv = uv + glm::vec2(static_cast<float>(dx), static_cast<float>(dy)) * step;
				accum += GetInputColor(*node, 0, offset_uv);
				++count;
			}
		}
		return accum / static_cast<float>(count);
	}

	// --- Color ---
	if (type == "HSV Adjust") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		const float h_offset = GetInputFloat(*node, 1, uv);
		const float s_offset = GetInputFloat(*node, 2, uv);
		const float v_offset = GetInputFloat(*node, 3, uv);
		glm::vec3 hsv = RgbToHsv(glm::vec3(input));
		hsv.x = std::fmod(hsv.x + h_offset, 1.0f);
		if (hsv.x < 0.0f) {
			hsv.x += 1.0f;
		}
		hsv.y = std::clamp(hsv.y + s_offset, 0.0f, 1.0f);
		hsv.z = std::clamp(hsv.z + v_offset, 0.0f, 1.0f);
		const glm::vec3 rgb = HsvToRgb(hsv);
		return glm::vec4(rgb, input.a);
	}

	if (type == "Channel Split") {
		const glm::vec4 input = GetInputColor(*node, 0, uv);
		// Return the requested channel based on pin_index
		if (pin_index == 0) {
			return glm::vec4(input.r, input.r, input.r, 1.0f);
		}
		if (pin_index == 1) {
			return glm::vec4(input.g, input.g, input.g, 1.0f);
		}
		if (pin_index == 2) {
			return glm::vec4(input.b, input.b, input.b, 1.0f);
		}
		return glm::vec4(input.a, input.a, input.a, 1.0f);
	}

	if (type == "Channel Merge") {
		const float r = GetInputFloat(*node, 0, uv);
		const float g = GetInputFloat(*node, 1, uv);
		const float b = GetInputFloat(*node, 2, uv);
		const float a = GetInputFloat(*node, 3, uv);
		return glm::vec4(r, g, b, a > 0.0f ? a : 1.0f);
	}

	if (type == "Colorize") {
		const float val = std::clamp(GetInputFloat(*node, 0, uv), 0.0f, 1.0f);
		const glm::vec4 color = GetInputColor(*node, 1, uv);
		return color * val;
	}

	// --- Blend ---
	if (type == "Blend Multiply") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
	}

	if (type == "Blend Screen") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(
				1.0f - (1.0f - a.r) * (1.0f - b.r),
				1.0f - (1.0f - a.g) * (1.0f - b.g),
				1.0f - (1.0f - a.b) * (1.0f - b.b),
				a.a);
	}

	if (type == "Blend Overlay") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::vec4(
				OverlayBlendChannel(a.r, b.r),
				OverlayBlendChannel(a.g, b.g),
				OverlayBlendChannel(a.b, b.b),
				a.a);
	}

	if (type == "Blend Add") {
		const glm::vec4 a = GetInputColor(*node, 0, uv);
		const glm::vec4 b = GetInputColor(*node, 1, uv);
		return glm::clamp(a + b, glm::vec4(0.0f), glm::vec4(1.0f));
	}

	// --- Output ---
	if (type == "Texture Output") {
		return GetInputColor(*node, 0, uv);
	}

	// Unknown node type
	return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
}

void TextureEditorPanel::GenerateTextureData() {
	const int res = preview_resolution_;
	preview_pixels_.resize(res * res * 4);

	// Find the output node
	int output_node_id = -1;
	for (const auto& node : texture_graph_->GetNodes()) {
		if (node.type_name == "Texture Output") {
			output_node_id = node.id;
			break;
		}
	}

	for (int y = 0; y < res; ++y) {
		for (int x = 0; x < res; ++x) {
			const glm::vec2 uv(
					static_cast<float>(x) / static_cast<float>(res - 1),
					static_cast<float>(y) / static_cast<float>(res - 1));

			glm::vec4 color;
			if (output_node_id >= 0) {
				color = EvaluateNodeOutput(output_node_id, 0, uv);
			}
			else {
				color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
			}

			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			const int idx = (y * res + x) * 4;
			preview_pixels_[idx + 0] = static_cast<unsigned char>(color.r * 255.0f);
			preview_pixels_[idx + 1] = static_cast<unsigned char>(color.g * 255.0f);
			preview_pixels_[idx + 2] = static_cast<unsigned char>(color.b * 255.0f);
			preview_pixels_[idx + 3] = static_cast<unsigned char>(color.a * 255.0f);
		}
	}

	// Set preview_color_ to the center pixel for the status display
	const glm::vec2 center_uv(0.5f, 0.5f);
	if (output_node_id >= 0) {
		preview_color_ = EvaluateNodeOutput(output_node_id, 0, center_uv);
	}
	else {
		preview_color_ = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
	}
}

void TextureEditorPanel::UploadPreviewTexture() {
	if (preview_pixels_.empty()) {
		return;
	}

	const int res = preview_resolution_;

	if (preview_texture_id_ == 0) {
		glGenTextures(1, &preview_texture_id_);
	}

	glBindTexture(GL_TEXTURE_2D, preview_texture_id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, preview_pixels_.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureEditorPanel::UpdatePreview() {
	preview_dirty_ = true;
	GenerateTextureData();
	UploadPreviewTexture();
	preview_dirty_ = false;
}

void TextureEditorPanel::ExportPng(const std::string& path) {
	// Ensure data is up-to-date
	GenerateTextureData();

	const int res = preview_resolution_;
	stbi_write_png(path.c_str(), res, res, 4, preview_pixels_.data(), res * 4);
}

} // namespace editor
