#pragma once

#include "../command.h"

#include <vector>

namespace editor {

/**
 * @brief A command that groups multiple sub-commands as a single undo/redo step
 *
 * Useful for operations that logically consist of multiple atomic changes
 * but should be treated as one action from the user's perspective.
 *
 * Example: Moving an entity to a new parent involves:
 * - Removing from old parent
 * - Adding to new parent
 * - Updating transform
 *
 * All three operations should be undone/redone together.
 */
class CompoundCommand : public ICommand {
public:
	/**
	 * @brief Create a compound command with a description
	 *
	 * @param description Human-readable description of the compound operation
	 */
	explicit CompoundCommand(std::string description) : description_(std::move(description)) {}

	/**
	 * @brief Add a sub-command to this compound command
	 *
	 * Commands are executed in the order they are added.
	 * They are undone in reverse order.
	 *
	 * @param command The sub-command to add (ownership is transferred)
	 */
	void AddCommand(std::unique_ptr<ICommand> command) { commands_.push_back(std::move(command)); }

	void Execute() override {
		// Execute all sub-commands in order
		for (auto& command : commands_) {
			command->Execute();
		}
	}

	void Undo() override {
		// Undo all sub-commands in reverse order
		for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
			(*it)->Undo();
		}
	}

	void Redo() override {
		// Redo all sub-commands in order
		for (auto& command : commands_) {
			command->Redo();
		}
	}

	std::string GetDescription() const override { return description_; }

private:
	std::string description_;
	std::vector<std::unique_ptr<ICommand>> commands_;
};

} // namespace editor
