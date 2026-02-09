#pragma once

#include "../command.h"

#include <string>
#include <vector>

import engine;

namespace editor {

/**
 * @brief Command for creating an entity
 *
 * Stores the entity name and parent so it can be recreated on redo
 * and destroyed on undo.
 */
class CreateEntityCommand : public ICommand {
public:
	/**
	 * @brief Create an entity creation command
	 *
	 * @param scene The scene to create the entity in
	 * @param name Name for the new entity
	 * @param parent Parent entity (can be invalid for root entity)
	 */
	CreateEntityCommand(engine::scene::Scene* scene, std::string name, engine::ecs::Entity parent) :
			scene_(scene), name_(std::move(name)), parent_(parent), created_entity_() {}

	void Execute() override {
		if (!scene_) {
			return;
		}

		// Create the entity
		if (parent_.is_valid()) {
			created_entity_ = scene_->CreateEntity(name_, parent_);
		}
		else {
			created_entity_ = scene_->CreateEntity(name_);
		}
	}

	void Undo() override {
		if (created_entity_.is_valid()) {
			scene_->DestroyEntity(created_entity_);
			created_entity_ = engine::ecs::Entity();
		}
	}

	void Redo() override { Execute(); }

	std::string GetDescription() const override { return "Create Entity: " + name_; }

	engine::ecs::Entity GetCreatedEntity() const { return created_entity_; }

private:
	engine::scene::Scene* scene_;
	std::string name_;
	engine::ecs::Entity parent_;
	engine::ecs::Entity created_entity_;
};

/**
 * @brief Command for deleting an entity
 *
 * Stores the entity's JSON representation so it can be restored on undo.
 */
class DeleteEntityCommand : public ICommand {
public:
	/**
	 * @brief Create an entity deletion command
	 *
	 * @param scene The scene containing the entity
	 * @param entity The entity to delete
	 * @param world The ECS world (for serialization)
	 */
	DeleteEntityCommand(engine::scene::Scene* scene, engine::ecs::Entity entity, engine::ecs::ECSWorld& world) :
			scene_(scene), entity_(entity), world_(world), entity_name_(entity.name().c_str()),
			parent_(scene->GetParent(entity)) {
		// Serialize the entity and its descendants to JSON for restoration
		SerializeEntity();
	}

	void Execute() override {
		if (entity_.is_valid()) {
			scene_->DestroyEntity(entity_);
		}
	}

	void Undo() override {
		// Restore the entity from JSON
		if (!entity_json_.empty()) {
			DeserializeEntity();
		}
	}

	std::string GetDescription() const override { return "Delete Entity: " + entity_name_; }

private:
	void SerializeEntity() {
		if (!entity_.is_valid()) {
			return;
		}

		// Use Flecs to_json to serialize the entity
		entity_json_ = entity_.to_json();
	}

	void DeserializeEntity() {
		// Use Flecs from_json to deserialize the entity
		// Note: This restores the entity with the same ID if possible
		entity_.from_json(entity_json_.c_str());

		// Restore parent relationship if there was one
		if (entity_.is_valid() && parent_.is_valid()) {
			scene_->SetParent(entity_, parent_);
		}
	}

	engine::scene::Scene* scene_;
	engine::ecs::Entity entity_;
	engine::ecs::ECSWorld& world_;
	std::string entity_name_;
	engine::ecs::Entity parent_;
	std::string entity_json_;
};

/**
 * @brief Command for reparenting an entity
 *
 * Moves an entity from one parent to another.
 */
class ReparentEntityCommand : public ICommand {
public:
	/**
	 * @brief Create a reparent command
	 *
	 * @param scene The scene containing the entity
	 * @param entity The entity to reparent
	 * @param new_parent The new parent entity
	 */
	ReparentEntityCommand(engine::scene::Scene* scene, engine::ecs::Entity entity, engine::ecs::Entity new_parent) :
			scene_(scene), entity_(entity), old_parent_(scene->GetParent(entity)), new_parent_(new_parent) {}

	void Execute() override {
		if (entity_.is_valid()) {
			if (new_parent_.is_valid()) {
				scene_->SetParent(entity_, new_parent_);
			}
			else {
				scene_->RemoveParent(entity_);
			}
		}
	}

	void Undo() override {
		if (entity_.is_valid()) {
			if (old_parent_.is_valid()) {
				scene_->SetParent(entity_, old_parent_);
			}
			else {
				scene_->RemoveParent(entity_);
			}
		}
	}

	std::string GetDescription() const override {
		const std::string entity_name = entity_.name().c_str();
		const std::string new_parent_name = new_parent_.is_valid() ? new_parent_.name().c_str() : "Scene Root";
		return "Reparent Entity: " + entity_name + " -> " + new_parent_name;
	}

private:
	engine::scene::Scene* scene_;
	engine::ecs::Entity entity_;
	engine::ecs::Entity old_parent_;
	engine::ecs::Entity new_parent_;
};

} // namespace editor
