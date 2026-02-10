#pragma once

#include "../command.h"
#include "../editor_utils.h"

#include <flecs.h>
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
		SerializeEntity();
	}

	void Execute() override {
		if (entity_.is_valid()) {
			// Capture state before destroying so Redo after Undo works
			entity_name_ = entity_.name().c_str();
			parent_ = scene_->GetParent(entity_);
			SerializeEntity();
			scene_->DestroyEntity(entity_);
		}
	}

	void Undo() override {
		if (entity_json_.empty()) {
			return;
		}

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

	std::string GetDescription() const override { return "Delete Entity: " + entity_name_; }

private:
	void SerializeEntity() {
		if (!entity_.is_valid()) {
			return;
		}
		entity_json_ = entity_.to_json();
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
