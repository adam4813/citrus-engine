module;

#include <memory>
#include <string>
#include <vector>

export module engine.ai.behavior_tree;

import engine.ai.blackboard;

export namespace engine::ai {

// === BEHAVIOR TREE NODE EXECUTION STATUS ===

/**
 * @brief Result status of a behavior tree node execution
 */
enum class NodeStatus {
	Running,  // Node is still executing
	Success,  // Node completed successfully
	Failure   // Node failed
};

// === BASE BEHAVIOR TREE NODE ===

/**
 * @brief Base class for all behavior tree nodes
 *
 * All nodes must implement the Tick() method which returns a NodeStatus.
 * Composite and decorator nodes can have children.
 */
class BTNode {
public:
	explicit BTNode(std::string name) : name_(std::move(name)) {}
	virtual ~BTNode() = default;

	// Disable copying
	BTNode(const BTNode&) = delete;
	BTNode& operator=(const BTNode&) = delete;

	// Allow moving
	BTNode(BTNode&&) noexcept = default;
	BTNode& operator=(BTNode&&) noexcept = default;

	/**
	 * @brief Execute the node logic
	 * @param blackboard Shared data storage
	 * @return Status of node execution
	 */
	virtual NodeStatus Tick(Blackboard& blackboard) = 0;

	/**
	 * @brief Get the node name
	 */
	[[nodiscard]] const std::string& GetName() const { return name_; }

	/**
	 * @brief Set the node name
	 */
	void SetName(std::string name) { name_ = std::move(name); }

	/**
	 * @brief Get the children of this node (for composites)
	 */
	[[nodiscard]] const std::vector<std::unique_ptr<BTNode>>& GetChildren() const { return children_; }

	/**
	 * @brief Add a child node (for composites)
	 */
	void AddChild(std::unique_ptr<BTNode> child) { children_.push_back(std::move(child)); }

	/**
	 * @brief Get node type name for serialization/debugging
	 */
	[[nodiscard]] virtual std::string GetTypeName() const = 0;

protected:
	std::string name_;
	std::vector<std::unique_ptr<BTNode>> children_;
};

// === COMPOSITE NODES ===

/**
 * @brief Selector (OR) node - tries children left-to-right, returns Success on first success
 *
 * Returns:
 * - Success: If any child returns Success
 * - Failure: If all children return Failure
 * - Running: If current child returns Running
 */
class SelectorNode : public BTNode {
public:
	explicit SelectorNode(std::string name = "Selector") : BTNode(std::move(name)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Selector"; }

private:
	size_t current_child_ = 0;
};

/**
 * @brief Sequence (AND) node - runs children left-to-right, returns Failure on first failure
 *
 * Returns:
 * - Success: If all children return Success
 * - Failure: If any child returns Failure
 * - Running: If current child returns Running
 */
class SequenceNode : public BTNode {
public:
	explicit SequenceNode(std::string name = "Sequence") : BTNode(std::move(name)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Sequence"; }

private:
	size_t current_child_ = 0;
};

/**
 * @brief Parallel node - runs all children, configurable success/fail policy
 *
 * Policies:
 * - RequireAll: All children must succeed for success
 * - RequireOne: At least one child must succeed for success
 */
class ParallelNode : public BTNode {
public:
	enum class Policy { RequireAll, RequireOne };

	explicit ParallelNode(std::string name = "Parallel", Policy policy = Policy::RequireAll) :
			BTNode(std::move(name)), policy_(policy) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Parallel"; }

	void SetPolicy(Policy policy) { policy_ = policy; }
	[[nodiscard]] Policy GetPolicy() const { return policy_; }

private:
	Policy policy_;
};

// === DECORATOR NODES ===

/**
 * @brief Inverter node - inverts child result (Success <-> Failure)
 *
 * Returns:
 * - Success: If child returns Failure
 * - Failure: If child returns Success
 * - Running: If child returns Running
 */
class InverterNode : public BTNode {
public:
	explicit InverterNode(std::string name = "Inverter") : BTNode(std::move(name)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Inverter"; }
};

/**
 * @brief Repeater node - repeats child N times or until failure
 *
 * Returns:
 * - Success: If child succeeds N times
 * - Failure: If child fails before N repetitions
 * - Running: If still executing
 */
class RepeaterNode : public BTNode {
public:
	explicit RepeaterNode(std::string name = "Repeater", int count = 1) :
			BTNode(std::move(name)), repeat_count_(count) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Repeater"; }

	void SetRepeatCount(int count) { repeat_count_ = count; }
	[[nodiscard]] int GetRepeatCount() const { return repeat_count_; }

private:
	int repeat_count_;
	int current_iteration_ = 0;
};

/**
 * @brief Succeeder node - always returns Success
 */
class SucceederNode : public BTNode {
public:
	explicit SucceederNode(std::string name = "Succeeder") : BTNode(std::move(name)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Succeeder"; }
};

// === LEAF/ACTION NODES ===

/**
 * @brief Wait node - waits for a duration, returns Running until done
 */
class WaitNode : public BTNode {
public:
	explicit WaitNode(std::string name = "Wait", float duration = 1.0F) :
			BTNode(std::move(name)), duration_(duration) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Wait"; }

	void SetDuration(float duration) { duration_ = duration; }
	[[nodiscard]] float GetDuration() const { return duration_; }

private:
	float duration_;
	float elapsed_time_ = 0.0F;
};

/**
 * @brief Log node - prints a message, returns Success
 */
class LogNode : public BTNode {
public:
	explicit LogNode(std::string name = "Log", std::string message = "") :
			BTNode(std::move(name)), message_(std::move(message)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Log"; }

	void SetMessage(std::string message) { message_ = std::move(message); }
	[[nodiscard]] const std::string& GetMessage() const { return message_; }

private:
	std::string message_;
};

/**
 * @brief Condition node - checks a blackboard key, returns Success/Failure
 */
class ConditionNode : public BTNode {
public:
	explicit ConditionNode(std::string name = "Condition", std::string key = "") :
			BTNode(std::move(name)), blackboard_key_(std::move(key)) {}

	NodeStatus Tick(Blackboard& blackboard) override;

	[[nodiscard]] std::string GetTypeName() const override { return "Condition"; }

	void SetKey(std::string key) { blackboard_key_ = std::move(key); }
	[[nodiscard]] const std::string& GetKey() const { return blackboard_key_; }

private:
	std::string blackboard_key_;
};

} // namespace engine::ai
