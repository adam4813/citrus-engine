#include "editor_scene.h"
#include "commands/clipboard_commands.h"

#include <functional>
#include <imgui.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace editor {

void EditorScene::CopyEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to copy" << std::endl;
		return;
	}

	try {
		// Create clipboard JSON format
		json clipboard_data;
		clipboard_data["entities"] = json::array();

		// Serialize entity and all children
		std::function<void(engine::ecs::Entity, json&)> serialize_tree = [&](const engine::ecs::Entity entity,
																			 json& entities_array) {
			if (!entity.is_valid()) {
				return;
			}

			// Serialize this entity
			json entity_entry;
			entity_entry["path"] = entity.path().c_str();
			entity_entry["data"] = entity.to_json().c_str();
			entities_array.push_back(entity_entry);

			// Serialize children
			entity.children([&](const engine::ecs::Entity child) { serialize_tree(child, entities_array); });
		};

		serialize_tree(selected_entity_, clipboard_data["entities"]);

		// Store in clipboard
		clipboard_json_ = clipboard_data.dump();

		// Also copy to OS clipboard using ImGui
		ImGui::SetClipboardText(clipboard_json_.c_str());

		const std::string entity_name = selected_entity_.name().c_str();
		std::cout << "EditorScene: Copied entity '" << entity_name << "' to clipboard" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "EditorScene: Error copying entity: " << e.what() << std::endl;
	}
}

void EditorScene::CutEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to cut" << std::endl;
		return;
	}

	// Copy the entity first
	CopyEntity();

	// Then delete it using command for undo support
	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		auto command = std::make_unique<CutEntityCommand>(scene, engine_->ecs, selected_entity_);
		command_history_.Execute(std::move(command));

		// Deselect the cut entity
		selected_entity_ = {};
		selection_type_ = SelectionType::None;

		std::cout << "EditorScene: Cut entity" << std::endl;
	}
}

void EditorScene::PasteEntity() {
	if (clipboard_json_.empty()) {
		// Try to get from OS clipboard
		const char* clipboard_text = ImGui::GetClipboardText();
		if (clipboard_text && clipboard_text[0] != '\0') {
			clipboard_json_ = clipboard_text;
		}
		else {
			std::cout << "EditorScene: Clipboard is empty" << std::endl;
			return;
		}
	}

	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		// Paste under the selected entity if it's a valid parent (or use scene root)
		engine::ecs::Entity parent;
		if (selected_entity_.is_valid() && selected_entity_.has<engine::components::Group>()) {
			parent = selected_entity_;
		}

		auto command = std::make_unique<PasteEntityCommand>(scene, engine_->ecs, clipboard_json_, parent, true);
		auto* cmd_ptr = command.get();
		command_history_.Execute(std::move(command));

		// Select the pasted entity
		selected_entity_ = cmd_ptr->GetPastedEntity();

		std::cout << "EditorScene: Pasted entity from clipboard" << std::endl;
	}
}

void EditorScene::DuplicateEntity() {
	if (!selected_entity_.is_valid()) {
		std::cout << "EditorScene: No entity selected to duplicate" << std::endl;
		return;
	}

	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		auto command = std::make_unique<DuplicateEntityCommand>(scene, engine_->ecs, selected_entity_);
		auto* cmd_ptr = command.get();
		command_history_.Execute(std::move(command));

		// Select the duplicated entity
		selected_entity_ = cmd_ptr->GetDuplicatedEntity();

		const std::string entity_name = selected_entity_.name().c_str();
		std::cout << "EditorScene: Duplicated entity '" << entity_name << "'" << std::endl;
	}
}

} // namespace editor
