#pragma once

#include <flecs.h>
#include <string>
#include <vector>

#include "../command.h"

import engine;

namespace editor {

/**
 * @brief Command for pasting an entity from clipboard
 *
 * Deserializes entity JSON and creates new entities with new IDs.
 * Optionally offsets the position to avoid exact overlap.
 */
class PasteEntityCommand : public ICommand {
public:
	/**
	 * @brief Create a paste command
	 *
	 * @param scene The scene to paste into
	 * @param world The ECS world (for deserialization)
	 * @param clipboard_json JSON string of entity to paste
	 * @param parent Optional parent entity to paste under
	 * @param offset_position Whether to offset the position slightly
	 */
	PasteEntityCommand(
			engine::scene::Scene* scene,
			engine::ecs::ECSWorld& world,
			std::string clipboard_json,
			engine::ecs::Entity parent,
			bool offset_position = true) :
			scene_(scene), world_(world), clipboard_json_(std::move(clipboard_json)), parent_(parent),
			offset_position_(offset_position), pasted_entity_() {}

	void Execute() override;

	void Undo() override;

	void Redo() override { Execute(); }

	std::string GetDescription() const override { return "Paste Entity"; }

	engine::ecs::Entity GetPastedEntity() const { return pasted_entity_; }

private:
	void SerializePastedEntity();
	void OffsetEntityPosition(engine::ecs::Entity entity) const;

	engine::scene::Scene* scene_;
	engine::ecs::ECSWorld& world_;
	std::string clipboard_json_;
	engine::ecs::Entity parent_;
	bool offset_position_;
	engine::ecs::Entity pasted_entity_;
	std::string pasted_entity_json_; // For undo
};

/**
 * @brief Command for duplicating an entity
 *
 * Combines copy and paste into a single operation.
 */
class DuplicateEntityCommand : public ICommand {
public:
	/**
	 * @brief Create a duplicate command
	 *
	 * @param scene The scene containing the entity
	 * @param world The ECS world
	 * @param entity The entity to duplicate
	 */
	DuplicateEntityCommand(engine::scene::Scene* scene, engine::ecs::ECSWorld& world, engine::ecs::Entity entity) :
			scene_(scene), world_(world), entity_(entity), duplicated_entity_() {}

	void Execute() override;

	void Undo() override;

	void Redo() override { Execute(); }

	std::string GetDescription() const override {
		const std::string entity_name = entity_.name().c_str();
		return "Duplicate Entity: " + entity_name;
	}

	engine::ecs::Entity GetDuplicatedEntity() const { return duplicated_entity_; }

private:
	void SerializeEntity();
	void SerializeDuplicatedEntity();
	void OffsetEntityPosition(engine::ecs::Entity entity) const;

	engine::scene::Scene* scene_;
	engine::ecs::ECSWorld& world_;
	engine::ecs::Entity entity_;
	engine::ecs::Entity duplicated_entity_;
	std::string entity_json_;
	std::string duplicated_entity_json_; // For undo
};

/**
 * @brief Command for cutting an entity
 *
 * Combines copy and delete operations.
 */
class CutEntityCommand : public ICommand {
public:
	/**
	 * @brief Create a cut command
	 *
	 * @param scene The scene containing the entity
	 * @param world The ECS world
	 * @param entity The entity to cut
	 */
	CutEntityCommand(engine::scene::Scene* scene, engine::ecs::ECSWorld& world, engine::ecs::Entity entity) :
			scene_(scene), world_(world), entity_(entity), entity_name_(entity.name().c_str()),
			parent_(scene->GetParent(entity)) {
		// Serialize the entity for restoration on undo
		SerializeEntity();
	}

	void Execute() override;

	void Undo() override;

	void Redo() override { Execute(); }

	std::string GetDescription() const override { return "Cut Entity: " + entity_name_; }

private:
	void SerializeEntity();
	void DeserializeEntity();

	engine::scene::Scene* scene_;
	engine::ecs::ECSWorld& world_;
	engine::ecs::Entity entity_;
	std::string entity_name_;
	engine::ecs::Entity parent_;
	std::string entity_json_;
};

} // namespace editor
