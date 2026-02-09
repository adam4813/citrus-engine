#include "command.h"

#include <iostream>

namespace editor {

CommandHistory::CommandHistory() = default;

CommandHistory::~CommandHistory() = default;

void CommandHistory::Execute(std::unique_ptr<ICommand> command) {
	if (!command) {
		return;
	}

	// Execute the command
	command->Execute();

	// Add to undo stack
	undo_stack_.push_back(std::move(command));

	// Clear redo stack when a new command is executed
	redo_stack_.clear();

	// Increment current position
	++current_position_;

	// Enforce max depth by removing oldest commands
	while (undo_stack_.size() > max_depth_) {
		undo_stack_.pop_front();
		// Adjust save position if we removed commands before it
		if (save_position_ > 0) {
			--save_position_;
		}
		--current_position_;
	}
}

bool CommandHistory::Undo() {
	if (undo_stack_.empty()) {
		return false;
	}

	// Pop from undo stack
	auto command = std::move(undo_stack_.back());
	undo_stack_.pop_back();

	// Undo the command
	command->Undo();

	// Push to redo stack
	redo_stack_.push_back(std::move(command));

	// Decrement current position
	--current_position_;

	return true;
}

bool CommandHistory::Redo() {
	if (redo_stack_.empty()) {
		return false;
	}

	// Pop from redo stack
	auto command = std::move(redo_stack_.back());
	redo_stack_.pop_back();

	// Redo the command
	command->Redo();

	// Push back to undo stack
	undo_stack_.push_back(std::move(command));

	// Increment current position
	++current_position_;

	return true;
}

void CommandHistory::Clear() {
	undo_stack_.clear();
	redo_stack_.clear();
	current_position_ = 0;
	save_position_ = 0;
}

bool CommandHistory::IsDirty() const { return current_position_ != save_position_; }

void CommandHistory::SetSavePosition() { save_position_ = current_position_; }

} // namespace editor
