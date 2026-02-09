#pragma once

#include "../command.h"

#include <any>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

import engine;

namespace editor {

/**
 * @brief Generic command for property changes
 *
 * Stores the old and new values of a property and can restore/apply them.
 * Uses raw byte storage for flexibility with different component types.
 */
class PropertyChangeCommand : public ICommand {
public:
	/**
	 * @brief Create a property change command
	 *
	 * @param entity The entity whose property is being changed
	 * @param component_id The Flecs component ID
	 * @param field_offset Byte offset of the field within the component
	 * @param field_size Size of the field in bytes
	 * @param old_value Pointer to the old value
	 * @param new_value Pointer to the new value
	 * @param description Human-readable description
	 */
	PropertyChangeCommand(
			engine::ecs::Entity entity,
			flecs::id_t component_id,
			size_t field_offset,
			size_t field_size,
			const void* old_value,
			const void* new_value,
			std::string description)
		: entity_(entity), component_id_(component_id), field_offset_(field_offset), field_size_(field_size),
		  description_(std::move(description)) {
		// Store old and new values as raw bytes
		old_value_.resize(field_size);
		new_value_.resize(field_size);
		std::memcpy(old_value_.data(), old_value, field_size);
		std::memcpy(new_value_.data(), new_value, field_size);
	}

	void Execute() override { ApplyValue(new_value_.data()); }

	void Undo() override { ApplyValue(old_value_.data()); }

	std::string GetDescription() const override { return description_; }

private:
	void ApplyValue(const void* value) {
		if (!entity_.is_valid()) {
			return;
		}

		// Get mutable pointer to component data
		void* comp_ptr = entity_.try_get_mut(component_id_);
		if (!comp_ptr) {
			return;
		}

		// Apply value at the field offset
		void* field_ptr = static_cast<char*>(comp_ptr) + field_offset_;
		std::memcpy(field_ptr, value, field_size_);

		// Mark component as modified in Flecs
		entity_.modified(component_id_);
	}

	engine::ecs::Entity entity_;
	flecs::id_t component_id_;
	size_t field_offset_;
	size_t field_size_;
	std::vector<char> old_value_;
	std::vector<char> new_value_;
	std::string description_;
};

} // namespace editor
