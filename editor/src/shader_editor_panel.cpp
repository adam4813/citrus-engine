#include "shader_editor_panel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>

using json = nlohmann::json;

namespace editor {

ShaderEditorPanel::ShaderEditorPanel() : shader_graph_(std::make_unique<engine::graph::NodeGraph>()) { NewShader(); }

ShaderEditorPanel::~ShaderEditorPanel() = default;

void ShaderEditorPanel::Render() {
	if (!is_visible_) {
		return;
	}

	ImGui::Begin("Shader Editor", &is_visible_, ImGuiWindowFlags_MenuBar);

	RenderToolbar();

	// Main content area
	ImGui::BeginChild("Content", ImVec2(0, -200), true, ImGuiWindowFlags_HorizontalScrollbar);

	if (mode_ == EditorMode::Code) {
		RenderCodeEditor();
	}
	else {
		RenderNodeGraph();
	}

	ImGui::EndChild();

	// Bottom panels (split between uniform inspector and errors)
	ImGui::BeginChild("BottomPanels", ImVec2(0, 0), false);

	// Uniform inspector on left
	ImGui::BeginChild("UniformInspector", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);
	RenderUniformInspector();
	ImGui::EndChild();

	ImGui::SameLine();

	// Error panel on right
	ImGui::BeginChild("ErrorPanel", ImVec2(0, 0), true);
	RenderErrorPanel();
	ImGui::EndChild();

	ImGui::EndChild();

	ImGui::End();
}

void ShaderEditorPanel::RenderToolbar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New")) {
				NewShader();
			}
			if (ImGui::MenuItem("Open...")) {
				// For now, just load from a default path
				OpenShader("shader_test.shader.json");
			}
			if (ImGui::MenuItem("Save")) {
				if (current_file_path_.empty()) {
					SaveShader("shader_test.shader.json");
				}
				else {
					SaveShader(current_file_path_);
				}
			}
			if (ImGui::MenuItem("Save As...")) {
				SaveShader("shader_test.shader.json");
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Mode")) {
			if (ImGui::MenuItem("Code Editor", nullptr, mode_ == EditorMode::Code)) {
				mode_ = EditorMode::Code;
			}
			if (ImGui::MenuItem("Node Graph", nullptr, mode_ == EditorMode::NodeGraph)) {
				mode_ = EditorMode::NodeGraph;
			}
			ImGui::EndMenu();
		}

		// Compile button
		ImGui::Spacing();
		if (ImGui::Button("Compile")) {
			CompileShader();
		}

		ImGui::EndMenuBar();
	}
}

void ShaderEditorPanel::RenderCodeEditor() {
	// Tabs for vertex and fragment shaders
	if (ImGui::BeginTabBar("ShaderTabs")) {
		if (ImGui::BeginTabItem("Vertex Shader")) {
			active_tab_ = ShaderTab::Vertex;

			// Multi-line text editor
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font as monospace

			// Line numbers area
			ImGui::BeginChild("LineNumbers", ImVec2(40, 0), false, ImGuiWindowFlags_NoScrollbar);

			// Count lines in vertex shader
			int line_count = 1;
			for (char c : vertex_source_) {
				if (c == '\n') {
					line_count++;
				}
			}

			// Draw line numbers
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			for (int i = 1; i <= line_count; ++i) {
				ImGui::Text("%4d", i);
			}
			ImGui::PopStyleColor();

			ImGui::EndChild();

			ImGui::SameLine();

			// Text editor
			ImGui::BeginChild("VertexEditor", ImVec2(0, 0), false);

			// Allocate buffer if needed (with extra space)
			static char vertex_buffer[16384];
			if (vertex_source_.size() < sizeof(vertex_buffer) - 1) {
				std::strncpy(vertex_buffer, vertex_source_.c_str(), sizeof(vertex_buffer) - 1);
				vertex_buffer[sizeof(vertex_buffer) - 1] = '\0';
			}

			if (ImGui::InputTextMultiline(
						"##VertexSource",
						vertex_buffer,
						sizeof(vertex_buffer),
						ImVec2(-1, -1),
						ImGuiInputTextFlags_AllowTabInput)) {
				vertex_source_ = vertex_buffer;
			}

			ImGui::EndChild();
			ImGui::PopFont();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Fragment Shader")) {
			active_tab_ = ShaderTab::Fragment;

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

			// Line numbers area
			ImGui::BeginChild("LineNumbers", ImVec2(40, 0), false, ImGuiWindowFlags_NoScrollbar);

			int line_count = 1;
			for (char c : fragment_source_) {
				if (c == '\n') {
					line_count++;
				}
			}

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			for (int i = 1; i <= line_count; ++i) {
				ImGui::Text("%4d", i);
			}
			ImGui::PopStyleColor();

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("FragmentEditor", ImVec2(0, 0), false);

			static char fragment_buffer[16384];
			if (fragment_source_.size() < sizeof(fragment_buffer) - 1) {
				std::strncpy(fragment_buffer, fragment_source_.c_str(), sizeof(fragment_buffer) - 1);
				fragment_buffer[sizeof(fragment_buffer) - 1] = '\0';
			}

			if (ImGui::InputTextMultiline(
						"##FragmentSource",
						fragment_buffer,
						sizeof(fragment_buffer),
						ImVec2(-1, -1),
						ImGuiInputTextFlags_AllowTabInput)) {
				fragment_source_ = fragment_buffer;
			}

			ImGui::EndChild();
			ImGui::PopFont();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void ShaderEditorPanel::RenderNodeGraph() {
	ImGui::Text("Node Graph Mode - Coming Soon");
	ImGui::Text("(Graph canvas will render here)");

	// TODO: Implement full graph rendering similar to GraphEditorPanel
	// For now, just show a placeholder
	RenderGraphCanvas();
}

void ShaderEditorPanel::RenderGraphCanvas() {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	canvas_p0_ = ImGui::GetCursorScreenPos();
	const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
	const ImVec2 canvas_p1 = ImVec2(canvas_p0_.x + canvas_sz.x, canvas_p0_.y + canvas_sz.y);

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

	// Render nodes and links
	for (const auto& link : shader_graph_->GetLinks()) {
		RenderGraphLink(link);
	}

	for (const auto& node : shader_graph_->GetNodes()) {
		RenderGraphNode(node);
	}

	draw_list->PopClipRect();

	// Handle panning
	ImGui::SetCursorScreenPos(canvas_p0_);
	ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		ImVec2 delta = ImGui::GetIO().MouseDelta;
		canvas_offset_.x += delta.x;
		canvas_offset_.y += delta.y;
	}
}

void ShaderEditorPanel::RenderGraphNode(const engine::graph::Node& node) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 node_pos = GetNodeScreenPos(node);
	const float node_height = 60.0f + (std::max(node.inputs.size(), node.outputs.size()) * 20.0f);

	// Node background
	const ImU32 node_bg_col = (selected_node_id_ == node.id) ? IM_COL32(80, 80, 120, 255) : IM_COL32(60, 60, 60, 255);

	draw_list->AddRectFilled(node_pos, ImVec2(node_pos.x + NODE_WIDTH, node_pos.y + node_height), node_bg_col, 4.0f);

	draw_list->AddRect(
			node_pos,
			ImVec2(node_pos.x + NODE_WIDTH, node_pos.y + node_height),
			IM_COL32(100, 100, 100, 255),
			4.0f,
			0,
			1.5f);

	// Node title
	draw_list->AddText(ImVec2(node_pos.x + 5, node_pos.y + 5), IM_COL32(255, 255, 255, 255), node.type_name.c_str());
}

void ShaderEditorPanel::RenderGraphLink(const engine::graph::Link& link) {
	// Find source and target nodes
	const engine::graph::Node* src_node = shader_graph_->GetNode(link.from_node_id);
	const engine::graph::Node* dst_node = shader_graph_->GetNode(link.to_node_id);

	if (!src_node || !dst_node) {
		return;
	}

	const ImVec2 p1 = GetPinScreenPos(*src_node, link.from_pin_index, true);
	const ImVec2 p2 = GetPinScreenPos(*dst_node, link.to_pin_index, false);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw bezier curve
	const float curve_strength = 50.0f;
	draw_list->AddBezierCubic(
			p1,
			ImVec2(p1.x + curve_strength, p1.y),
			ImVec2(p2.x - curve_strength, p2.y),
			p2,
			IM_COL32(200, 200, 200, 255),
			2.0f);
}

void ShaderEditorPanel::RenderGraphContextMenu() {
	// Context menu for adding nodes
	if (ImGui::BeginPopupContextWindow("GraphContext")) {
		ImGui::Text("Add Node");
		ImGui::Separator();

		// Get registered shader node types
		using namespace engine::graph;
		auto& registry = NodeTypeRegistry::GetGlobal();

		for (const auto& category : registry.GetCategories()) {
			if (ImGui::BeginMenu(category.c_str())) {
				for (const auto* node_type : registry.GetByCategory(category)) {
					if (ImGui::MenuItem(node_type->name.c_str())) {
						// Create node at context menu position
						// TODO: Implement node creation
					}
				}
				ImGui::EndMenu();
			}
		}

		ImGui::EndPopup();
	}
}

ImVec2 ShaderEditorPanel::GetNodeScreenPos(const engine::graph::Node& node) const {
	return ImVec2(
			canvas_p0_.x + canvas_offset_.x + node.position.x * canvas_zoom_,
			canvas_p0_.y + canvas_offset_.y + node.position.y * canvas_zoom_);
}

ImVec2 ShaderEditorPanel::GetPinScreenPos(const engine::graph::Node& node, int pin_index, bool is_output) const {
	const ImVec2 node_pos = GetNodeScreenPos(node);
	const float pin_y_offset = 40.0f + pin_index * 20.0f;

	if (is_output) {
		return ImVec2(node_pos.x + NODE_WIDTH, node_pos.y + pin_y_offset);
	}
	else {
		return ImVec2(node_pos.x, node_pos.y + pin_y_offset);
	}
}

bool ShaderEditorPanel::HitTestPin(const ImVec2& point, const ImVec2& pin_pos) const {
	const float dx = point.x - pin_pos.x;
	const float dy = point.y - pin_pos.y;
	return (dx * dx + dy * dy) <= (PIN_RADIUS * PIN_RADIUS);
}

bool ShaderEditorPanel::FindPinUnderMouse(
		const ImVec2& mouse_pos, int& out_node_id, int& out_pin_index, bool& out_is_output) const {
	for (const auto& node : shader_graph_->GetNodes()) {
		// Check output pins
		for (size_t i = 0; i < node.outputs.size(); ++i) {
			const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), true);
			if (HitTestPin(mouse_pos, pin_pos)) {
				out_node_id = node.id;
				out_pin_index = static_cast<int>(i);
				out_is_output = true;
				return true;
			}
		}

		// Check input pins
		for (size_t i = 0; i < node.inputs.size(); ++i) {
			const ImVec2 pin_pos = GetPinScreenPos(node, static_cast<int>(i), false);
			if (HitTestPin(mouse_pos, pin_pos)) {
				out_node_id = node.id;
				out_pin_index = static_cast<int>(i);
				out_is_output = false;
				return true;
			}
		}
	}

	return false;
}

int ShaderEditorPanel::FindLinkUnderMouse(const ImVec2& mouse_pos) const {
	// Simplified link hit testing - just check if mouse is near the midpoint
	for (const auto& link : shader_graph_->GetLinks()) {
		const engine::graph::Node* src_node = shader_graph_->GetNode(link.from_node_id);
		const engine::graph::Node* dst_node = shader_graph_->GetNode(link.to_node_id);

		if (!src_node || !dst_node) {
			continue;
		}

		const ImVec2 p1 = GetPinScreenPos(*src_node, link.from_pin_index, true);
		const ImVec2 p2 = GetPinScreenPos(*dst_node, link.to_pin_index, false);
		const ImVec2 midpoint = ImVec2((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f);

		const float dx = mouse_pos.x - midpoint.x;
		const float dy = mouse_pos.y - midpoint.y;
		const float dist_sq = dx * dx + dy * dy;

		if (dist_sq < 100.0f) { // 10 pixel threshold
			return link.id;
		}
	}

	return -1;
}

void ShaderEditorPanel::RenderUniformInspector() {
	ImGui::Text("Uniforms");
	ImGui::Separator();

	if (uniforms_.empty()) {
		ImGui::TextDisabled("No uniforms detected. Compile shader to detect uniforms.");
	}
	else {
		for (auto& uniform : uniforms_) {
			ImGui::Text("%s %s", uniform.type.c_str(), uniform.name.c_str());

			// Editable default value
			char buffer[256];
			std::strncpy(buffer, uniform.default_value.c_str(), sizeof(buffer) - 1);
			buffer[sizeof(buffer) - 1] = '\0';

			ImGui::PushID(uniform.name.c_str());
			if (ImGui::InputText("##default", buffer, sizeof(buffer))) {
				uniform.default_value = buffer;
			}
			ImGui::PopID();
		}
	}
}

void ShaderEditorPanel::RenderErrorPanel() {
	ImGui::Text("Compilation Errors");
	ImGui::Separator();

	if (has_errors_) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::TextWrapped("%s", error_message_.c_str());
		ImGui::PopStyleColor();
	}
	else {
		ImGui::TextDisabled("No errors");
	}
}

void ShaderEditorPanel::NewShader() {
	shader_name_ = "Untitled";
	vertex_source_ = GetDefaultVertexShader();
	fragment_source_ = GetDefaultFragmentShader();
	current_file_path_ = "";
	uniforms_.clear();
	has_errors_ = false;
	error_message_ = "";

	// Clear graph
	shader_graph_ = std::make_unique<engine::graph::NodeGraph>();
}

bool ShaderEditorPanel::OpenShader(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		error_message_ = "Failed to open file: " + path;
		has_errors_ = true;
		return false;
	}

	try {
		json j;
		file >> j;

		shader_name_ = j.value("name", "Untitled");
		vertex_source_ = j.value("vertex_source", GetDefaultVertexShader());
		fragment_source_ = j.value("fragment_source", GetDefaultFragmentShader());
		current_file_path_ = path;

		// Load uniforms
		if (j.contains("uniforms")) {
			uniforms_.clear();
			for (const auto& u : j["uniforms"]) {
				UniformInfo uniform;
				uniform.name = u.value("name", "");
				uniform.type = u.value("type", "float");
				uniform.default_value = u.value("default_value", "0.0");
				uniforms_.push_back(uniform);
			}
		}

		// Load graph data if present
		if (j.contains("graph_data")) {
			// TODO: Deserialize graph
		}

		has_errors_ = false;
		error_message_ = "";
		return true;
	}
	catch (const std::exception& e) {
		error_message_ = std::string("Failed to parse JSON: ") + e.what();
		has_errors_ = true;
		return false;
	}
}

bool ShaderEditorPanel::SaveShader(const std::string& path) {
	json j;
	j["name"] = shader_name_;
	j["vertex_source"] = vertex_source_;
	j["fragment_source"] = fragment_source_;

	// Save uniforms
	json uniforms_json = json::array();
	for (const auto& uniform : uniforms_) {
		json u;
		u["name"] = uniform.name;
		u["type"] = uniform.type;
		u["default_value"] = uniform.default_value;
		uniforms_json.push_back(u);
	}
	j["uniforms"] = uniforms_json;

	// Save graph data
	// TODO: Serialize graph
	j["graph_data"] = json::object();

	std::ofstream file(path);
	if (!file.is_open()) {
		error_message_ = "Failed to save file: " + path;
		has_errors_ = true;
		return false;
	}

	file << j.dump(2);
	current_file_path_ = path;
	return true;
}

bool ShaderEditorPanel::CompileShader() {
	has_errors_ = false;
	error_message_ = "";

	// Basic syntax validation
	// Check for matching braces and semicolons
	auto validate_source = [this](const std::string& source, const std::string& shader_type) -> bool {
		int brace_count = 0;
		int line_num = 1;

		for (size_t i = 0; i < source.size(); ++i) {
			char c = source[i];

			if (c == '\n') {
				line_num++;
			}
			else if (c == '{') {
				brace_count++;
			}
			else if (c == '}') {
				brace_count--;
				if (brace_count < 0) {
					error_message_ =
							shader_type + " line " + std::to_string(line_num) + ": Unmatched closing brace '}'";
					return false;
				}
			}
		}

		if (brace_count != 0) {
			error_message_ = shader_type + ": Unmatched braces (missing " + std::to_string(std::abs(brace_count))
							 + " closing brace(s))";
			return false;
		}

		return true;
	};

	if (!validate_source(vertex_source_, "Vertex shader")) {
		has_errors_ = true;
		return false;
	}

	if (!validate_source(fragment_source_, "Fragment shader")) {
		has_errors_ = true;
		return false;
	}

	// Extract uniforms from shader source
	ExtractUniforms();

	return true;
}

void ShaderEditorPanel::ExtractUniforms() {
	uniforms_.clear();

	// Regular expression to match uniform declarations
	// Pattern: uniform <type> <name>;
	std::regex uniform_regex(R"(uniform\s+(\w+)\s+(\w+)\s*;)");

	auto extract_from_source = [&](const std::string& source) {
		std::sregex_iterator iter(source.begin(), source.end(), uniform_regex);
		std::sregex_iterator end;

		for (; iter != end; ++iter) {
			std::smatch match = *iter;
			UniformInfo uniform;
			uniform.type = match[1].str();
			uniform.name = match[2].str();

			// Set default value based on type
			if (uniform.type == "float") {
				uniform.default_value = "0.0";
			}
			else if (uniform.type == "int") {
				uniform.default_value = "0";
			}
			else if (uniform.type == "vec2") {
				uniform.default_value = "0.0, 0.0";
			}
			else if (uniform.type == "vec3") {
				uniform.default_value = "0.0, 0.0, 0.0";
			}
			else if (uniform.type == "vec4") {
				uniform.default_value = "0.0, 0.0, 0.0, 1.0";
			}
			else if (uniform.type == "mat4") {
				uniform.default_value = "identity";
			}
			else if (uniform.type == "sampler2D") {
				uniform.default_value = "texture0";
			}
			else {
				uniform.default_value = "0";
			}

			uniforms_.push_back(uniform);
		}
	};

	extract_from_source(vertex_source_);
	extract_from_source(fragment_source_);
}

void ShaderEditorPanel::GenerateShaderFromGraph() {
	// TODO: Implement shader code generation from node graph
	// This would traverse the graph and generate GLSL code
}

std::string ShaderEditorPanel::GetDefaultVertexShader() const {
	return R"(#version 300 es
precision mediump float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

out vec2 v_TexCoord;
out vec4 v_Color;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}
)";
}

std::string ShaderEditorPanel::GetDefaultFragmentShader() const {
	return R"(#version 300 es
precision mediump float;

in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_Texture;
uniform vec4 u_TintColor;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);
    FragColor = texColor * v_Color * u_TintColor;
}
)";
}

// ============================================================================
// Register shader-specific node types
// ============================================================================

void RegisterShaderGraphNodes() {
	using namespace engine::graph;
	auto& registry = NodeTypeRegistry::GetGlobal();

	// Input nodes
	{
		NodeTypeDefinition def("UV", "Input", "Texture coordinates");
		def.default_outputs = {Pin(0, "UV", PinType::Vec2, PinDirection::Output, glm::vec2(0.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Time", "Input", "Time value");
		def.default_outputs = {Pin(0, "Time", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("VertexColor", "Input", "Vertex color attribute");
		def.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("VertexNormal", "Input", "Vertex normal attribute");
		def.default_outputs = {Pin(0, "Normal", PinType::Vec3, PinDirection::Output, glm::vec3(0.0f, 0.0f, 1.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("VertexPosition", "Input", "Vertex position attribute");
		def.default_outputs = {Pin(0, "Position", PinType::Vec3, PinDirection::Output, glm::vec3(0.0f))};
		registry.Register(def);
	}

	// Math nodes
	{
		NodeTypeDefinition def("Add", "Math", "Add two values");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Multiply", "Math", "Multiply two values");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Subtract", "Math", "Subtract B from A");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Divide", "Math", "Divide A by B");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Power", "Math", "Raise A to the power of B");
		def.default_inputs = {
				Pin(0, "Base", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "Exponent", PinType::Float, PinDirection::Input, 2.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Sqrt", "Math", "Square root");
		def.default_inputs = {Pin(0, "Value", PinType::Float, PinDirection::Input, 1.0f)};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Abs", "Math", "Absolute value");
		def.default_inputs = {Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f)};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Lerp", "Math", "Linear interpolation");
		def.default_inputs = {
				Pin(0, "A", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "T", PinType::Float, PinDirection::Input, 0.5f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Clamp", "Math", "Clamp value between min and max");
		def.default_inputs = {
				Pin(0, "Value", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Min", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Max", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("Step", "Math", "Step function (0 if x < edge, 1 otherwise)");
		def.default_inputs = {
				Pin(0, "Edge", PinType::Float, PinDirection::Input, 0.5f),
				Pin(0, "X", PinType::Float, PinDirection::Input, 0.0f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("SmoothStep", "Math", "Smooth Hermite interpolation");
		def.default_inputs = {
				Pin(0, "Edge0", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "Edge1", PinType::Float, PinDirection::Input, 1.0f),
				Pin(0, "X", PinType::Float, PinDirection::Input, 0.5f),
		};
		def.default_outputs = {Pin(0, "Result", PinType::Float, PinDirection::Output, 0.0f)};
		registry.Register(def);
	}

	// Color nodes
	{
		NodeTypeDefinition def("ColorConstant", "Color", "Constant color value");
		def.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("ChannelSplit", "Color", "Split color into R, G, B, A channels");
		def.default_inputs = {Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(0.0f))};
		def.default_outputs = {
				Pin(0, "R", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "G", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Output, 0.0f),
				Pin(0, "A", PinType::Float, PinDirection::Output, 0.0f),
		};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("ChannelMerge", "Color", "Merge R, G, B, A into color");
		def.default_inputs = {
				Pin(0, "R", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "G", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "B", PinType::Float, PinDirection::Input, 0.0f),
				Pin(0, "A", PinType::Float, PinDirection::Input, 1.0f),
		};
		def.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(0.0f))};
		registry.Register(def);
	}

	// Texture nodes
	{
		NodeTypeDefinition def("TextureSample", "Texture", "Sample texture at UV coordinates");
		def.default_inputs = {
				Pin(0, "UV", PinType::Vec2, PinDirection::Input, glm::vec2(0.0f)),
		};
		def.default_outputs = {Pin(0, "Color", PinType::Color, PinDirection::Output, glm::vec4(1.0f))};
		registry.Register(def);
	}

	// Output nodes
	{
		NodeTypeDefinition def("FragmentOutput", "Output", "Fragment shader output");
		def.default_inputs = {Pin(0, "Color", PinType::Color, PinDirection::Input, glm::vec4(1.0f))};
		registry.Register(def);
	}
	{
		NodeTypeDefinition def("VertexOutput", "Output", "Vertex shader output");
		def.default_inputs = {Pin(0, "Position", PinType::Vec3, PinDirection::Input, glm::vec3(0.0f))};
		registry.Register(def);
	}
}

} // namespace editor
