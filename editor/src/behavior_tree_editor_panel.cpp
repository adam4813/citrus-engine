#include "behavior_tree_editor_panel.h"

#include <fstream>
#include <imgui.h>
#include <iostream>

import engine;

namespace editor {

BehaviorTreeEditorPanel::BehaviorTreeEditorPanel() {
	// Initialize with an empty tree
	NewTree();
}

BehaviorTreeEditorPanel::~BehaviorTreeEditorPanel() = default;

void BehaviorTreeEditorPanel::Render() {
	if (!is_visible_) {
		return;
	}

	ImGui::Begin("Behavior Tree Editor", &is_visible_);

	RenderToolbar();
	ImGui::Separator();

	// Split view: Tree on left, properties on right
	ImGui::BeginChild("TreeView", ImVec2(ImGui::GetContentRegionAvail().x * 0.6F, 0), true);
	RenderTreeView();
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("PropertiesPanel", ImVec2(0, 0), true);
	RenderPropertiesPanel();
	ImGui::EndChild();

	RenderContextMenu();

	ImGui::End();
}

void BehaviorTreeEditorPanel::RenderToolbar() {
	if (ImGui::Button("New")) {
		NewTree();
	}
	ImGui::SameLine();

	if (ImGui::Button("Open")) {
		show_open_dialog_ = true;
	}
	ImGui::SameLine();

	if (ImGui::Button("Save")) {
		if (!current_file_path_.empty()) {
			SaveTree(current_file_path_);
		}
		else {
			show_save_dialog_ = true;
		}
	}
	ImGui::SameLine();

	if (ImGui::Button("Save As...")) {
		show_save_dialog_ = true;
	}

	ImGui::SameLine();
	ImGui::Text("| File: %s", current_file_path_.empty() ? "(unsaved)" : current_file_path_.c_str());

	// Simple file dialogs (placeholder - would use proper file dialog in production)
	if (show_open_dialog_) {
		ImGui::OpenPopup("Open Behavior Tree");
		show_open_dialog_ = false;
	}

	if (show_save_dialog_) {
		ImGui::OpenPopup("Save Behavior Tree");
		show_save_dialog_ = false;
	}

	// File dialog popups
	if (ImGui::BeginPopupModal("Open Behavior Tree", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		static char path_buffer[256] = "assets/behaviors/tree.bt.json";
		ImGui::InputText("Path", path_buffer, sizeof(path_buffer));

		if (ImGui::Button("Open", ImVec2(120, 0))) {
			if (LoadTree(path_buffer)) {
				current_file_path_ = path_buffer;
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Save Behavior Tree", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		static char path_buffer[256] = "assets/behaviors/tree.bt.json";
		ImGui::InputText("Path", path_buffer, sizeof(path_buffer));

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			if (SaveTree(path_buffer)) {
				current_file_path_ = path_buffer;
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void BehaviorTreeEditorPanel::RenderTreeView() {
	ImGui::Text("Behavior Tree Structure");
	ImGui::Separator();

	if (root_node_) {
		RenderNodeTree(root_node_.get(), 0);
	}
	else {
		ImGui::TextDisabled("(Empty tree - right-click to add root node)");
	}
}

void BehaviorTreeEditorPanel::RenderNodeTree(engine::ai::BTNode* node, int depth) {
	if (!node) {
		return;
	}

	// Indent based on depth
	for (int i = 0; i < depth; ++i) {
		ImGui::Indent(20.0F);
	}

	// Node display
	ImGui::PushID(node);

	bool is_selected = (node == selected_node_);
	ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

	if (is_selected) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	if (node->GetChildren().empty()) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	std::string node_label = "[" + node->GetTypeName() + "] " + node->GetName();
	bool node_open = ImGui::TreeNodeEx(node_label.c_str(), flags);

	// Click to select
	if (ImGui::IsItemClicked()) {
		selected_node_ = node;
	}

	// Context menu for nodes
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Add Child")) {
			// Show submenu to select node type
			ImGui::OpenPopup("SelectNodeType");
		}
		if (ImGui::MenuItem("Delete")) {
			// Note: Deletion would require parent tracking - simplified for now
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// Add child submenu
	if (ImGui::BeginPopup("SelectNodeType")) {
		if (ImGui::BeginMenu("Composite")) {
			if (ImGui::MenuItem("Selector")) {
				node->AddChild(std::make_unique<engine::ai::SelectorNode>("New Selector"));
			}
			if (ImGui::MenuItem("Sequence")) {
				node->AddChild(std::make_unique<engine::ai::SequenceNode>("New Sequence"));
			}
			if (ImGui::MenuItem("Parallel")) {
				node->AddChild(std::make_unique<engine::ai::ParallelNode>("New Parallel"));
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Decorator")) {
			if (ImGui::MenuItem("Inverter")) {
				node->AddChild(std::make_unique<engine::ai::InverterNode>("New Inverter"));
			}
			if (ImGui::MenuItem("Repeater")) {
				node->AddChild(std::make_unique<engine::ai::RepeaterNode>("New Repeater"));
			}
			if (ImGui::MenuItem("Succeeder")) {
				node->AddChild(std::make_unique<engine::ai::SucceederNode>("New Succeeder"));
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Action")) {
			if (ImGui::MenuItem("Wait")) {
				node->AddChild(std::make_unique<engine::ai::WaitNode>("New Wait"));
			}
			if (ImGui::MenuItem("Log")) {
				node->AddChild(std::make_unique<engine::ai::LogNode>("New Log"));
			}
			if (ImGui::MenuItem("Condition")) {
				node->AddChild(std::make_unique<engine::ai::ConditionNode>("New Condition"));
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	// Render children
	if (node_open) {
		for (const auto& child : node->GetChildren()) {
			RenderNodeTree(child.get(), depth + 1);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();

	// Unindent
	for (int i = 0; i < depth; ++i) {
		ImGui::Unindent(20.0F);
	}
}

void BehaviorTreeEditorPanel::RenderPropertiesPanel() {
	ImGui::Text("Node Properties");
	ImGui::Separator();

	if (selected_node_) {
		ImGui::Text("Type: %s", selected_node_->GetTypeName().c_str());

		// Name editing
		static char name_buffer[128];
		strncpy_s(name_buffer, selected_node_->GetName().c_str(), sizeof(name_buffer) - 1);
		name_buffer[sizeof(name_buffer) - 1] = '\0';

		if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
			selected_node_->SetName(name_buffer);
		}

		// Type-specific properties
		ImGui::Separator();
		ImGui::Text("Node-Specific Properties:");

		// WaitNode
		if (auto* wait_node = dynamic_cast<engine::ai::WaitNode*>(selected_node_)) {
			float duration = wait_node->GetDuration();
			if (ImGui::DragFloat("Duration", &duration, 0.1F, 0.0F, 60.0F)) {
				wait_node->SetDuration(duration);
			}
		}
		// LogNode
		else if (auto* log_node = dynamic_cast<engine::ai::LogNode*>(selected_node_)) {
			static char message_buffer[256];
			strncpy_s(message_buffer, log_node->GetMessage().c_str(), sizeof(message_buffer) - 1);
			message_buffer[sizeof(message_buffer) - 1] = '\0';

			if (ImGui::InputTextMultiline("Message", message_buffer, sizeof(message_buffer))) {
				log_node->SetMessage(message_buffer);
			}
		}
		// ConditionNode
		else if (auto* condition_node = dynamic_cast<engine::ai::ConditionNode*>(selected_node_)) {
			static char key_buffer[128];
			strncpy_s(key_buffer, condition_node->GetKey().c_str(), sizeof(key_buffer) - 1);
			key_buffer[sizeof(key_buffer) - 1] = '\0';

			if (ImGui::InputText("Blackboard Key", key_buffer, sizeof(key_buffer))) {
				condition_node->SetKey(key_buffer);
			}
		}
		// RepeaterNode
		else if (auto* repeater_node = dynamic_cast<engine::ai::RepeaterNode*>(selected_node_)) {
			int count = repeater_node->GetRepeatCount();
			if (ImGui::DragInt("Repeat Count", &count, 1.0F, 1, 100)) {
				repeater_node->SetRepeatCount(count);
			}
		}
		// ParallelNode
		else if (auto* parallel_node = dynamic_cast<engine::ai::ParallelNode*>(selected_node_)) {
			int policy_index = (parallel_node->GetPolicy() == engine::ai::ParallelNode::Policy::RequireAll) ? 0 : 1;
			const char* policies[] = {"Require All", "Require One"};
			if (ImGui::Combo("Policy", &policy_index, policies, 2)) {
				parallel_node->SetPolicy(policy_index == 0 ? engine::ai::ParallelNode::Policy::RequireAll
																											 : engine::ai::ParallelNode::Policy::RequireOne);
			}
		}
		else {
			ImGui::TextDisabled("(No editable properties)");
		}
	}
	else {
		ImGui::TextDisabled("(No node selected)");
	}
}

void BehaviorTreeEditorPanel::RenderContextMenu() {
	// Right-click in empty space to add root node
	if (ImGui::BeginPopupContextWindow("TreeContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
		if (!root_node_) {
			if (ImGui::BeginMenu("Add Root Node")) {
				if (ImGui::MenuItem("Selector")) {
					root_node_ = std::make_unique<engine::ai::SelectorNode>("Root Selector");
				}
				if (ImGui::MenuItem("Sequence")) {
					root_node_ = std::make_unique<engine::ai::SequenceNode>("Root Sequence");
				}
				if (ImGui::MenuItem("Parallel")) {
					root_node_ = std::make_unique<engine::ai::ParallelNode>("Root Parallel");
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndPopup();
	}
}

void BehaviorTreeEditorPanel::NewTree() {
	root_node_.reset();
	selected_node_ = nullptr;
	current_file_path_.clear();
}

bool BehaviorTreeEditorPanel::SaveTree(const std::string& path) {
	// Placeholder for JSON serialization
	// In a real implementation, this would serialize the tree to JSON
	std::cout << "Saving behavior tree to: " << path << std::endl;

	// For now, just create a simple placeholder file
	std::ofstream file(path);
	if (file.is_open()) {
		file << "{\n";
		file << "  \"type\": \"behavior_tree\",\n";
		file << "  \"version\": \"1.0\",\n";
		if (root_node_) {
			file << "  \"root\": {\n";
			file << "    \"type\": \"" << root_node_->GetTypeName() << "\",\n";
			file << "    \"name\": \"" << root_node_->GetName() << "\"\n";
			file << "  }\n";
		}
		file << "}\n";
		file.close();
		return true;
	}

	return false;
}

bool BehaviorTreeEditorPanel::LoadTree(const std::string& path) {
	// Placeholder for JSON deserialization
	// In a real implementation, this would load the tree from JSON
	std::cout << "Loading behavior tree from: " << path << std::endl;

	// For now, just create a simple test tree
	NewTree();
	root_node_ = std::make_unique<engine::ai::SelectorNode>("Loaded Root");
	root_node_->AddChild(std::make_unique<engine::ai::LogNode>("Test Log", "Hello from loaded tree!"));
	root_node_->AddChild(std::make_unique<engine::ai::WaitNode>("Test Wait", 2.0F));

	return true;
}

std::unique_ptr<engine::ai::BTNode> BehaviorTreeEditorPanel::CreateNode(const std::string& node_type,
		const std::string& name) {
	if (node_type == "Selector") {
		return std::make_unique<engine::ai::SelectorNode>(name);
	}
	if (node_type == "Sequence") {
		return std::make_unique<engine::ai::SequenceNode>(name);
	}
	if (node_type == "Parallel") {
		return std::make_unique<engine::ai::ParallelNode>(name);
	}
	if (node_type == "Inverter") {
		return std::make_unique<engine::ai::InverterNode>(name);
	}
	if (node_type == "Repeater") {
		return std::make_unique<engine::ai::RepeaterNode>(name);
	}
	if (node_type == "Succeeder") {
		return std::make_unique<engine::ai::SucceederNode>(name);
	}
	if (node_type == "Wait") {
		return std::make_unique<engine::ai::WaitNode>(name);
	}
	if (node_type == "Log") {
		return std::make_unique<engine::ai::LogNode>(name);
	}
	if (node_type == "Condition") {
		return std::make_unique<engine::ai::ConditionNode>(name);
	}

	return nullptr;
}

} // namespace editor
