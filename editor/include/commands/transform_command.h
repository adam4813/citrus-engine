#pragma once

#include "../command.h"

#include <flecs.h>

import engine;
import glm;

namespace editor {

/**
 * @brief Command for changing entity transform
 *
 * Captures position, rotation, and scale changes for undo/redo.
 * This is a specialized command for Transform component changes,
 * providing better descriptions than the generic PropertyChangeCommand.
 */
class TransformChangeCommand : public ICommand {
public:
	/**
	 * @brief Create a transform change command
	 *
	 * @param entity The entity whose transform is being changed
	 * @param old_transform The transform before the change
	 * @param new_transform The transform after the change
	 * @param description Human-readable description (e.g., "Move Entity")
	 */
	TransformChangeCommand(
			engine::ecs::Entity entity,
			const engine::components::Transform& old_transform,
			const engine::components::Transform& new_transform,
			std::string description)
		: entity_(entity), old_transform_(old_transform), new_transform_(new_transform),
		  description_(std::move(description)) {}

	void Execute() override {
		if (entity_.is_valid()) {
			entity_.set<engine::components::Transform>(new_transform_);
		}
	}

	void Undo() override {
		if (entity_.is_valid()) {
			entity_.set<engine::components::Transform>(old_transform_);
		}
	}

	std::string GetDescription() const override { return description_; }

private:
	engine::ecs::Entity entity_;
	engine::components::Transform old_transform_;
	engine::components::Transform new_transform_;
	std::string description_;
};

/**
 * @brief Command for adding a component to an entity
 *
 * Stores the component type so it can be added/removed for undo/redo.
 */
class AddComponentCommand : public ICommand {
public:
	/**
	 * @brief Create an add component command
	 *
	 * @param entity The entity to add the component to
	 * @param component_id The Flecs component ID
	 * @param component_name Human-readable component name
	 */
	AddComponentCommand(engine::ecs::Entity entity, flecs::id_t component_id, std::string component_name)
		: entity_(entity), component_id_(component_id), component_name_(std::move(component_name)) {}

	void Execute() override {
		if (entity_.is_valid() && !entity_.has(component_id_)) {
			// Add the component with default-constructed value
			entity_.add(component_id_);
		}
	}

	void Undo() override {
		if (entity_.is_valid() && entity_.has(component_id_)) {
			entity_.remove(component_id_);
		}
	}

	std::string GetDescription() const override { return "Add Component: " + component_name_; }

private:
	engine::ecs::Entity entity_;
	flecs::id_t component_id_;
	std::string component_name_;
};

/**
 * @brief Command for removing a component from an entity
 *
 * Stores the component data so it can be restored on undo.
 */
class RemoveComponentCommand : public ICommand {
public:
	/**
	 * @brief Create a remove component command
	 *
	 * @param entity The entity to remove the component from
	 * @param component_id The Flecs component ID
	 * @param component_name Human-readable component name
	 */
	RemoveComponentCommand(engine::ecs::Entity entity, flecs::id_t component_id, std::string component_name)
		: entity_(entity), component_id_(component_id), component_name_(std::move(component_name)) {
		// Store only this component's state for undo
		if (entity.is_valid() && entity.has(component_id)) {
			StoreComponentJson(entity, component_id);
		}
	}

	void Execute() override {
		if (entity_.is_valid() && entity_.has(component_id_)) {
			// Store state before removing (in case Redo is called after Undo)
			StoreComponentJson(entity_, component_id_);
			entity_.remove(component_id_);
		}
	}

	void Undo() override {
		if (entity_.is_valid() && !component_json_.empty()) {
			// Add component back if it doesn't exist
			if (!entity_.has(component_id_)) {
				entity_.add(component_id_);
			}
			// Restore only the removed component's data
			entity_.set_json(component_id_, component_json_.c_str());
		}
	}

	std::string GetDescription() const override { return "Remove Component: " + component_name_; }

private:
	void StoreComponentJson(engine::ecs::Entity entity, flecs::id_t comp_id) {
		flecs::entity_t type_id = ecs_get_typeid(entity.world(), comp_id);
		const void* ptr = ecs_get_id(entity.world(), entity.id(), comp_id);
		if (type_id && ptr) {
			char* json = ecs_ptr_to_json(entity.world(), type_id, ptr);
			if (json) {
				component_json_ = json;
				ecs_os_free(json);
			}
		}
	}

	engine::ecs::Entity entity_;
	flecs::id_t component_id_;
	std::string component_name_;
	std::string component_json_;
};

} // namespace editor
