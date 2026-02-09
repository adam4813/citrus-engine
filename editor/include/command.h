#pragma once

#include <deque>
#include <memory>
#include <string>

namespace editor {

/**
 * @brief Base interface for all undoable commands
 *
 * Commands represent atomic editor operations that can be undone and redone.
 * Each command must be able to execute itself, undo itself, and provide
 * a description for UI display.
 */
class ICommand {
public:
	virtual ~ICommand() = default;

	/**
	 * @brief Execute the command
	 *
	 * This applies the change to the editor state.
	 */
	virtual void Execute() = 0;

	/**
	 * @brief Undo the command
	 *
	 * This reverses the change made by Execute().
	 */
	virtual void Undo() = 0;

	/**
	 * @brief Redo the command
	 *
	 * This re-applies the change. Default implementation calls Execute().
	 */
	virtual void Redo() { Execute(); }

	/**
	 * @brief Get a human-readable description of this command
	 *
	 * Used for displaying command history in the UI.
	 */
	virtual std::string GetDescription() const = 0;
};

/**
 * @brief Manages command history for undo/redo functionality
 *
 * Maintains two stacks: undo stack and redo stack. When a command is executed,
 * it's pushed to the undo stack and the redo stack is cleared. When undoing,
 * commands move from undo to redo stack, and vice versa for redo.
 *
 * The dirty state tracks whether the current state differs from the last save.
 */
class CommandHistory {
public:
	CommandHistory();
	~CommandHistory();

	/**
	 * @brief Execute a command and add it to the undo stack
	 *
	 * This executes the command immediately and pushes it onto the undo stack.
	 * The redo stack is cleared when a new command is executed.
	 *
	 * @param command The command to execute (ownership is transferred)
	 */
	void Execute(std::unique_ptr<ICommand> command);

	/**
	 * @brief Undo the most recent command
	 *
	 * Pops a command from the undo stack, calls its Undo() method,
	 * and pushes it onto the redo stack.
	 *
	 * @return true if a command was undone, false if undo stack was empty
	 */
	bool Undo();

	/**
	 * @brief Redo the most recently undone command
	 *
	 * Pops a command from the redo stack, calls its Redo() method,
	 * and pushes it back onto the undo stack.
	 *
	 * @return true if a command was redone, false if redo stack was empty
	 */
	bool Redo();

	/**
	 * @brief Check if undo is available
	 */
	bool CanUndo() const { return !undo_stack_.empty(); }

	/**
	 * @brief Check if redo is available
	 */
	bool CanRedo() const { return !redo_stack_.empty(); }

	/**
	 * @brief Clear all command history
	 *
	 * Clears both undo and redo stacks, and resets the save position.
	 */
	void Clear();

	/**
	 * @brief Check if the current state is dirty (unsaved changes)
	 *
	 * Returns true if the current position in the command history is different
	 * from the position when SetSavePosition() was last called.
	 */
	bool IsDirty() const;

	/**
	 * @brief Mark the current position as saved
	 *
	 * Call this when the scene is saved to reset the dirty flag.
	 * The dirty flag will be true again when the command history changes.
	 */
	void SetSavePosition();

	/**
	 * @brief Set the maximum depth of the undo stack
	 *
	 * When the undo stack exceeds this depth, the oldest commands are removed.
	 *
	 * @param depth Maximum number of commands to keep (default: 100)
	 */
	void SetMaxDepth(size_t depth) { max_depth_ = depth; }

	/**
	 * @brief Get the maximum depth of the undo stack
	 */
	size_t GetMaxDepth() const { return max_depth_; }

	/**
	 * @brief Get the current size of the undo stack
	 */
	size_t GetUndoCount() const { return undo_stack_.size(); }

	/**
	 * @brief Get the current size of the redo stack
	 */
	size_t GetRedoCount() const { return redo_stack_.size(); }

private:
	std::deque<std::unique_ptr<ICommand>> undo_stack_;
	std::deque<std::unique_ptr<ICommand>> redo_stack_;
	size_t max_depth_ = 100;
	size_t save_position_ = 0; // Position in command history when last saved
	size_t current_position_ = 0; // Current position in command history
};

} // namespace editor
