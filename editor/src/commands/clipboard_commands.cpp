#include "commands/clipboard_commands.h"
#include "editor_utils.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

namespace editor {

// =============================================================================
// PasteEntityCommand Implementation
// =============================================================================

void PasteEntityCommand::Execute() {
	if (clipboard_json_.empty()) {
		std::cerr << "PasteEntityCommand: Clipboard is empty" << std::endl;
		return;
	}

	try {
		// Parse the clipboard JSON
		json clipboard_data = json::parse(clipboard_json_);

		if (!clipboard_data.is_object() || !clipboard_data.contains("entities")) {
			std::cerr << "PasteEntityCommand: Invalid clipboard format" << std::endl;
			return;
		}

		const auto& entities_array = clipboard_data["entities"];
		if (!entities_array.is_array() || entities_array.empty()) {
			std::cerr << "PasteEntityCommand: No entities in clipboard" << std::endl;
			return;
		}

		const flecs::world& flecs_world = world_.GetWorld();

		// Store original entity paths to new entity mapping
		std::unordered_map<std::string, engine::ecs::Entity> path_mapping;

		// First pass: Create all entities with new IDs
		for (const auto& entity_entry : entities_array) {
			const std::string original_path = entity_entry.value("path", "");
			if (original_path.empty()) {
				continue;
			}

			// Extract entity name from path (last component)
			std::string entity_name = original_path;
			if (const size_t last_slash = original_path.find_last_of('/'); last_slash != std::string::npos) {
				entity_name = original_path.substr(last_slash + 1);
			}
			entity_name = MakeUniqueEntityName(entity_name, scene_);

			// Create new entity (without setting path yet - we'll build hierarchy later)
			engine::ecs::Entity new_entity;
			if (parent_.is_valid()) {
				new_entity = scene_->CreateEntity(entity_name, parent_);
			}
			else {
				new_entity = scene_->CreateEntity(entity_name);
			}

			if (!new_entity.is_valid()) {
				std::cerr << "PasteEntityCommand: Failed to create entity" << std::endl;
				continue;
			}

			// Store the first created entity as the pasted entity
			if (!pasted_entity_.is_valid()) {
				pasted_entity_ = new_entity;
			}

			// Map original path to new entity
			path_mapping[original_path] = new_entity;

			// Deserialize component data, stripping parent relationships
			if (entity_entry.contains("data")) {
				const std::string data_json = entity_entry["data"];
				new_entity.from_json(StripEntityRelationships(data_json).c_str());
			}

			// Offset position if requested and entity has Transform component
			if (offset_position_ && new_entity.has<engine::components::Transform>()) {
				OffsetEntityPosition(new_entity);
			}
		}

		// Second pass: Restore hierarchy relationships
		// Note: Flecs from_json already handles ChildOf relationships, but we need to
		// ensure the parent is set correctly for entities pasted under a specific parent
		if (parent_.is_valid() && pasted_entity_.is_valid()) {
			// The top-level pasted entity should be child of the specified parent
			scene_->SetParent(pasted_entity_, parent_);
		}

		// Serialize the pasted entities for undo
		SerializePastedEntity();

		std::cout << "PasteEntityCommand: Pasted entity tree" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "PasteEntityCommand: Error pasting: " << e.what() << std::endl;
	}
}

void PasteEntityCommand::Undo() {
	// Destroy the pasted entity and all its descendants
	if (pasted_entity_.is_valid()) {
		scene_->DestroyEntity(pasted_entity_);
		pasted_entity_ = engine::ecs::Entity();
	}
}

void PasteEntityCommand::SerializePastedEntity() {
	if (pasted_entity_.is_valid()) {
		pasted_entity_json_ = pasted_entity_.to_json();
	}
}

void PasteEntityCommand::OffsetEntityPosition(const engine::ecs::Entity entity) const {
	if (entity.has<engine::components::Transform>()) {
		auto& transform = entity.get_mut<engine::components::Transform>();
		// Offset by a small amount to avoid exact overlap
		constexpr float PASTE_OFFSET = 0.5f;
		transform.position.x += PASTE_OFFSET;
		transform.position.y += PASTE_OFFSET;
	}
}

// =============================================================================
// DuplicateEntityCommand Implementation
// =============================================================================

void DuplicateEntityCommand::Execute() {
	if (!entity_.is_valid()) {
		std::cerr << "DuplicateEntityCommand: Invalid entity" << std::endl;
		return;
	}

	// Serialize the entity first time if not done
	if (entity_json_.empty()) {
		SerializeEntity();
	}

	try {
		// Create clipboard JSON format
		json clipboard_data;
		clipboard_data["entities"] = json::array();

		// Serialize entity and all children
		std::function<void(engine::ecs::Entity, json&)> serialize_tree = [&](engine::ecs::Entity ent, json& arr) {
			if (!ent.is_valid()) {
				return;
			}

			// Serialize this entity
			json entity_entry;
			entity_entry["path"] = ent.path().c_str();
			entity_entry["data"] = ent.to_json().c_str();
			arr.push_back(entity_entry);

			// Serialize children
			ent.children([&](engine::ecs::Entity child) { serialize_tree(child, arr); });
		};

		serialize_tree(entity_, clipboard_data["entities"]);

		const std::string clipboard_json = clipboard_data.dump();

		// Get parent of the entity being duplicated
		const engine::ecs::Entity parent = scene_->GetParent(entity_);

		// Parse and create duplicate
		const json clipboard = json::parse(clipboard_json);
		const auto& entities_array = clipboard["entities"];

		const flecs::world& flecs_world = world_.GetWorld();
		std::unordered_map<std::string, engine::ecs::Entity> path_mapping;

		// Create all entities
		for (const auto& entity_entry : entities_array) {
			const std::string original_path = entity_entry.value("path", "");
			if (original_path.empty()) {
				continue;
			}

			// Extract entity name from path
			std::string entity_name = original_path;
			if (const size_t last_slash = original_path.find_last_of('/'); last_slash != std::string::npos) {
				entity_name = original_path.substr(last_slash + 1);
			}
			entity_name = MakeUniqueEntityName(entity_name, scene_);

			// Create new entity
			engine::ecs::Entity new_entity;
			if (parent.is_valid()) {
				new_entity = scene_->CreateEntity(entity_name, parent);
			}
			else {
				new_entity = scene_->CreateEntity(entity_name);
			}

			if (!new_entity.is_valid()) {
				continue;
			}

			// Store the first created entity as the duplicated entity
			if (!duplicated_entity_.is_valid()) {
				duplicated_entity_ = new_entity;
			}

			path_mapping[original_path] = new_entity;

			// Deserialize component data, stripping parent relationships
			if (entity_entry.contains("data")) {
				const std::string data_json = entity_entry["data"];
				new_entity.from_json(StripEntityRelationships(data_json).c_str());
			}

			// Offset position
			if (new_entity.has<engine::components::Transform>()) {
				OffsetEntityPosition(new_entity);
			}
		}

		// Serialize the duplicated entity for undo
		SerializeDuplicatedEntity();

		std::cout << "DuplicateEntityCommand: Duplicated entity" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "DuplicateEntityCommand: Error duplicating: " << e.what() << std::endl;
	}
}

void DuplicateEntityCommand::Undo() {
	// Destroy the duplicated entity and all its descendants
	if (duplicated_entity_.is_valid()) {
		scene_->DestroyEntity(duplicated_entity_);
		duplicated_entity_ = engine::ecs::Entity();
	}
}

void DuplicateEntityCommand::SerializeEntity() {
	if (entity_.is_valid()) {
		entity_json_ = entity_.to_json();
	}
}

void DuplicateEntityCommand::SerializeDuplicatedEntity() {
	if (duplicated_entity_.is_valid()) {
		duplicated_entity_json_ = duplicated_entity_.to_json();
	}
}

void DuplicateEntityCommand::OffsetEntityPosition(const engine::ecs::Entity entity) const {
	if (entity.has<engine::components::Transform>()) {
		auto& transform = entity.get_mut<engine::components::Transform>();
		// Offset by a small amount to avoid exact overlap
		constexpr float DUPLICATE_OFFSET = 0.5f;
		transform.position.x += DUPLICATE_OFFSET;
		transform.position.y += DUPLICATE_OFFSET;
	}
}

// =============================================================================
// CutEntityCommand Implementation
// =============================================================================

void CutEntityCommand::Execute() {
	if (entity_.is_valid()) {
		scene_->DestroyEntity(entity_);
	}
}

void CutEntityCommand::Undo() {
	// Restore the entity from JSON
	if (!entity_json_.empty()) {
		DeserializeEntity();
	}
}

void CutEntityCommand::SerializeEntity() {
	if (entity_.is_valid()) {
		entity_json_ = entity_.to_json();
	}
}

void CutEntityCommand::DeserializeEntity() {
	// Create a fresh entity — the old ID is stale after destruction
	if (parent_.is_valid()) {
		entity_ = scene_->CreateEntity(entity_name_, parent_);
	}
	else {
		entity_ = scene_->CreateEntity(entity_name_);
	}

	if (!entity_.is_valid()) {
		return;
	}

	// Restore components, stripping hierarchy pairs so from_json
	// doesn't try to re-parent to the (possibly stale) old parent
	entity_.from_json(StripEntityRelationships(entity_json_).c_str());
}

} // namespace editor
