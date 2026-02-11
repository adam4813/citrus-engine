#include "graph_editor_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace editor {

GraphEditorPanel::GraphEditorPanel() : graph_(std::make_unique<engine::graph::NodeGraph>()) {}

GraphEditorPanel::~GraphEditorPanel() = default;

std::string_view GraphEditorPanel::GetPanelName() const { return "Graph Editor"; }

void GraphEditorPanel::Render() {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Graph Editor", &VisibleRef(), ImGuiWindowFlags_MenuBar);

	RenderToolbar();

	// Main canvas area
	ImGui::BeginChild("Canvas", ImVec2(0, 0), true,
					  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

	RenderCanvas();

	ImGui::EndChild();

	ImGui::End();
}

void GraphEditorPanel::RenderToolbar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewGraph();
			}
			if (ImGui::MenuItem("Save")) {
				// For now, just save to a default path
				SaveGraph("graph.json");
			}
			if (ImGui::MenuItem("Load")) {
				LoadGraph("graph.json");
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Add Node")) {
				// Show add node menu at center
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void GraphEditorPanel::RenderCanvas() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	canvas_p0_ = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	const ImVec2 canvas_p1 = ImVec2(canvas_p0_.x + canvas_sz.x, canvas_p0_.y + canvas_sz.y);

	// Draw grid background
	draw_list->AddRectFilled(canvas_p0_, canvas_p1, IM_COL32(30, 30, 30, 255));

	// Draw grid lines
	const float grid_step = GRID_SIZE * canvas_zoom_;
	for (float x = fmodf(canvas_offset_.x, grid_step); x < canvas_sz.x; x += grid_step) {
		draw_list->AddLine(ImVec2(canvas_p0_.x + x, canvas_p0_.y), ImVec2(canvas_p0_.x + x, canvas_p1.y),
						   IM_COL32(50, 50, 50, 255));
	}
	for (float y = fmodf(canvas_offset_.y, grid_step); y < canvas_sz.y; y += grid_step) {
		draw_list->AddLine(ImVec2(canvas_p0_.x, canvas_p0_.y + y), ImVec2(canvas_p1.x, canvas_p0_.y + y),
						   IM_COL32(50, 50, 50, 255));
	}

	// Clip rendering to canvas
	draw_list->PushClipRect(canvas_p0_, canvas_p1, true);

	// Handle pin click for link creation (checked before nodes so it works on first click)
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
	for (const auto& link : graph_->GetLinks()) {
		RenderLink(link);
	}

	// Render all nodes
	hovered_node_id_ = -1;
	for (const auto& node : graph_->GetNodes()) {
		RenderNode(node);
	}

	// Render in-progress link
	if (is_creating_link_) {
		const auto* start_node = graph_->GetNode(link_start_node_id_);
		if (start_node) {
			const ImVec2 start_pos =
					GetPinScreenPos(*start_node, link_start_pin_index_, link_start_is_output_);
			const ImVec2 end_pos = ImGui::GetMousePos();

			const ImVec2 p1 = link_start_is_output_ ? start_pos : end_pos;
			const ImVec2 p2 = link_start_is_output_ ? end_pos : start_pos;
			const float distance = std::abs(p2.x - p1.x);
			const float offset = std::max(distance * 0.5f, 50.0f);
			const ImVec2 cp1 = ImVec2(p1.x + offset, p1.y);
			const ImVec2 cp2 = ImVec2(p2.x - offset, p2.y);

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
						graph_->AddLink(link_start_node_id_, link_start_pin_index_, end_node_id,
										end_pin_index);
					}
					else {
						graph_->AddLink(end_node_id, end_pin_index, link_start_node_id_,
										link_start_pin_index_);
					}
				}
			}
			is_creating_link_ = false;
		}
	}

	// Handle link selection (click near a link, but not on a pin or node)
	if (!is_creating_link_ && !is_dragging_node_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
		&& ImGui::IsWindowHovered() && hovered_node_id_ < 0) {
		const ImVec2 mouse_pos = ImGui::GetMousePos();
		int dummy_node = -1, dummy_pin = -1;
		bool dummy_output = false;
		// Only select link if we didn't click a pin
		if (!FindPinUnderMouse(mouse_pos, dummy_node, dummy_pin, dummy_output)) {
			const int link_id = FindLinkUnderMouse(mouse_pos);
			if (link_id >= 0) {
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
			graph_->RemoveLink(selected_link_id_);
			selected_link_id_ = -1;
		}
		else if (selected_node_id_ >= 0) {
			graph_->RemoveNode(selected_node_id_);
			selected_node_id_ = -1;
		}
	}

	// Handle panning
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive()) {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
			is_panning_ = true;
			pan_start_ = ImGui::GetMousePos();
		}
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
		const float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f) {
			const float zoom_delta = wheel * 0.1f;
			canvas_zoom_ = std::max(0.1f, std::min(2.0f, canvas_zoom_ + zoom_delta));
		}
	}

	// Right-click context menu — determine what's under the cursor
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		const ImVec2 mouse_pos = ImGui::GetMousePos();
		context_menu_pos_ = mouse_pos;

		if (hovered_node_id_ >= 0) {
			context_target_ = ContextTarget::Node;
			context_node_id_ = hovered_node_id_;
		}
		else {
			const int link_under_mouse = FindLinkUnderMouse(mouse_pos);
			if (link_under_mouse >= 0) {
				context_target_ = ContextTarget::Link;
				context_link_id_ = link_under_mouse;
			}
			else {
				context_target_ = ContextTarget::Canvas;
			}
		}
		ImGui::OpenPopup("CanvasContextMenu");
	}

	RenderContextMenu();

	draw_list->PopClipRect();
}

void GraphEditorPanel::RenderNode(const engine::graph::Node& node) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const ImVec2 node_pos = GetNodeScreenPos(node);
	const ImVec2 node_size(NODE_WIDTH * canvas_zoom_, 0); // Height will be calculated

	// Calculate node height based on pin count
	const int pin_count = std::max(static_cast<int>(node.inputs.size()),
								   static_cast<int>(node.outputs.size()));
	const float node_height = 40.0f + pin_count * 20.0f;

	const ImVec2 node_rect_min = node_pos;
	const ImVec2 node_rect_max = ImVec2(node_pos.x + NODE_WIDTH * canvas_zoom_,
										 node_pos.y + node_height * canvas_zoom_);

	// Draw node background
	const bool is_selected = (node.id == selected_node_id_);
	const ImU32 node_bg_color = is_selected ? IM_COL32(80, 80, 120, 255) : IM_COL32(60, 60, 60, 255);
	draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
	draw_list->AddRect(node_rect_min, node_rect_max, IM_COL32(100, 100, 100, 255), 4.0f, 0, 2.0f);

	// Draw node title
	const ImVec2 title_pos = ImVec2(node_pos.x + 5.0f, node_pos.y + 5.0f);
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
		const ImVec2 label_pos = ImVec2(pin_pos.x + 10.0f, pin_pos.y - 7.0f);
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
		const ImVec2 label_pos = ImVec2(pin_pos.x - label_size.x - 10.0f, pin_pos.y - 7.0f);
		draw_list->AddText(label_pos, IM_COL32(200, 200, 200, 255), label);
	}

	// Handle node interaction
	ImGui::SetCursorScreenPos(node_rect_min);
	ImGui::InvisibleButton(("node_" + std::to_string(node.id)).c_str(),
						   ImVec2(node_rect_max.x - node_rect_min.x, node_rect_max.y - node_rect_min.y));

	if (ImGui::IsItemHovered()) {
		hovered_node_id_ = node.id;
	}

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		// Only start dragging if we're NOT on a pin
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
		if (auto* mutable_node = graph_->GetNode(node.id)) {
			mutable_node->position.x += delta.x / canvas_zoom_;
			mutable_node->position.y += delta.y / canvas_zoom_;
		}
	}

	if (is_dragging_node_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		is_dragging_node_ = false;
	}
}

void GraphEditorPanel::RenderLink(const engine::graph::Link& link) {
	const engine::graph::Node* from_node = graph_->GetNode(link.from_node_id);
	const engine::graph::Node* to_node = graph_->GetNode(link.to_node_id);

	if (!from_node || !to_node) {
		return;
	}

	if (link.from_pin_index >= static_cast<int>(from_node->outputs.size()) ||
		link.to_pin_index >= static_cast<int>(to_node->inputs.size())) {
		return;
	}

	const ImVec2 p1 = GetPinScreenPos(*from_node, link.from_pin_index, true);
	const ImVec2 p2 = GetPinScreenPos(*to_node, link.to_pin_index, false);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw bezier curve
	const float distance = std::abs(p2.x - p1.x);
	const float offset = distance * 0.5f;
	const ImVec2 cp1 = ImVec2(p1.x + offset, p1.y);
	const ImVec2 cp2 = ImVec2(p2.x - offset, p2.y);

	const ImU32 link_color = (link.id == selected_link_id_) ? IM_COL32(255, 255, 100, 255)
															: IM_COL32(200, 200, 100, 255);
	const float link_thickness = (link.id == selected_link_id_) ? 3.0f * canvas_zoom_ : 2.0f * canvas_zoom_;
	draw_list->AddBezierCubic(p1, cp1, cp2, p2, link_color, link_thickness);
}

void GraphEditorPanel::RenderContextMenu() {
	if (ImGui::BeginPopup("CanvasContextMenu")) {
		switch (context_target_) {
		case ContextTarget::Canvas:
			RenderAddNodeMenu();
			break;

		case ContextTarget::Node:
			if (ImGui::MenuItem("Delete Node")) {
				graph_->RemoveNode(context_node_id_);
				if (selected_node_id_ == context_node_id_) {
					selected_node_id_ = -1;
				}
			}
			break;

		case ContextTarget::Link:
			if (ImGui::MenuItem("Delete Connection")) {
				graph_->RemoveLink(context_link_id_);
				if (selected_link_id_ == context_link_id_) {
					selected_link_id_ = -1;
				}
			}
			break;

		default:
			break;
		}

		ImGui::EndPopup();
	}
}

void GraphEditorPanel::RenderAddNodeMenu() {
	// Get all categories from the registry
	auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
	auto categories = registry.GetCategories();

	if (categories.empty()) {
		ImGui::TextDisabled("No node types registered");
		return;
	}

	for (const auto& category : categories) {
		if (ImGui::BeginMenu(category.c_str())) {
			auto types = registry.GetByCategory(category);

			for (const auto* type : types) {
				if (ImGui::MenuItem(type->name.c_str())) {
					// Add node at the right-click position (canvas space)
					const float x = (context_menu_pos_.x - canvas_p0_.x - canvas_offset_.x) / canvas_zoom_;
					const float y = (context_menu_pos_.y - canvas_p0_.y - canvas_offset_.y) / canvas_zoom_;

					const int node_id = graph_->AddNode(type->name, glm::vec2(x, y));

					// Initialize the node with default pins from the type definition
					if (auto* node = graph_->GetNode(node_id)) {
						node->inputs = type->default_inputs;
						node->outputs = type->default_outputs;
					}
				}
			}

			ImGui::EndMenu();
		}
	}
}

ImVec2 GraphEditorPanel::GetNodeScreenPos(const engine::graph::Node& node) const {
	return ImVec2(canvas_p0_.x + canvas_offset_.x + node.position.x * canvas_zoom_,
				  canvas_p0_.y + canvas_offset_.y + node.position.y * canvas_zoom_);
}

ImVec2 GraphEditorPanel::GetPinScreenPos(const engine::graph::Node& node, int pin_index,
										  bool is_output) const {
	const ImVec2 node_pos = GetNodeScreenPos(node);
	const float y_offset = 30.0f + pin_index * 20.0f;

	if (is_output) {
		return ImVec2(node_pos.x + NODE_WIDTH * canvas_zoom_, node_pos.y + y_offset * canvas_zoom_);
	}
	else {
		return ImVec2(node_pos.x, node_pos.y + y_offset * canvas_zoom_);
	}
}

bool GraphEditorPanel::HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const {
	const float radius = PIN_RADIUS * canvas_zoom_ * 2.0f; // Generous hit area
	const float dx = point.x - pin_pos.x;
	const float dy = point.y - pin_pos.y;
	return (dx * dx + dy * dy) <= (radius * radius);
}

bool GraphEditorPanel::FindPinUnderMouse(const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index,
										  bool& out_is_output) const {
	for (const auto& node : graph_->GetNodes()) {
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

int GraphEditorPanel::FindLinkUnderMouse(const ImVec2& mouse_pos) const {
	constexpr float threshold = 8.0f;
	for (const auto& link : graph_->GetLinks()) {
		const auto* from_node = graph_->GetNode(link.from_node_id);
		const auto* to_node = graph_->GetNode(link.to_node_id);
		if (!from_node || !to_node) {
			continue;
		}
		if (link.from_pin_index >= static_cast<int>(from_node->outputs.size())
			|| link.to_pin_index >= static_cast<int>(to_node->inputs.size())) {
			continue;
		}

		const ImVec2 p1 = GetPinScreenPos(*from_node, link.from_pin_index, true);
		const ImVec2 p2 = GetPinScreenPos(*to_node, link.to_pin_index, false);

		// Sample points along the bezier and check distance
		const float distance = std::abs(p2.x - p1.x);
		const float offset = distance * 0.5f;
		const ImVec2 cp1 = ImVec2(p1.x + offset, p1.y);
		const ImVec2 cp2 = ImVec2(p2.x - offset, p2.y);

		constexpr int samples = 20;
		for (int s = 0; s <= samples; ++s) {
			const float t = static_cast<float>(s) / static_cast<float>(samples);
			const float u = 1.0f - t;
			const float x = u * u * u * p1.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x
							+ t * t * t * p2.x;
			const float y = u * u * u * p1.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y
							+ t * t * t * p2.y;
			const float dx = mouse_pos.x - x;
			const float dy = mouse_pos.y - y;
			if (dx * dx + dy * dy <= threshold * threshold) {
				return link.id;
			}
		}
	}
	return -1;
}

void GraphEditorPanel::NewGraph() {
	graph_ = std::make_unique<engine::graph::NodeGraph>();
	selected_node_id_ = -1;
	selected_link_id_ = -1;
	hovered_node_id_ = -1;
	is_dragging_node_ = false;
	is_creating_link_ = false;
	canvas_offset_ = ImVec2(0.0f, 0.0f);
	canvas_zoom_ = 1.0f;
}

bool GraphEditorPanel::SaveGraph(const std::string& path) {
	return engine::graph::GraphSerializer::Save(*graph_, path);
}

bool GraphEditorPanel::LoadGraph(const std::string& path) {
	engine::graph::NodeGraph new_graph;
	if (engine::graph::GraphSerializer::Load(path, new_graph)) {
		*graph_ = std::move(new_graph);
		selected_node_id_ = -1;
		hovered_node_id_ = -1;
		return true;
	}
	return false;
}

} // namespace editor
