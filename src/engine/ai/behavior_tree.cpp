module;

#include <iostream>
#include <memory>
#include <string>
#include <vector>

module engine.ai.behavior_tree;

import engine.ai.blackboard;

namespace engine::ai {

// === COMPOSITE NODES IMPLEMENTATION ===

NodeStatus SelectorNode::Tick(Blackboard& blackboard) {
	// Try each child in order until one succeeds
	for (; current_child_ < children_.size(); ++current_child_) {
		NodeStatus status = children_[current_child_]->Tick(blackboard);

		if (status == NodeStatus::Running) {
			return NodeStatus::Running; // Still processing
		}
		if (status == NodeStatus::Success) {
			current_child_ = 0; // Reset for next run
			return NodeStatus::Success;
		}
		// Continue to next child on Failure
	}

	// All children failed
	current_child_ = 0; // Reset for next run
	return NodeStatus::Failure;
}

NodeStatus SequenceNode::Tick(Blackboard& blackboard) {
	// Run each child in order until one fails
	for (; current_child_ < children_.size(); ++current_child_) {
		NodeStatus status = children_[current_child_]->Tick(blackboard);

		if (status == NodeStatus::Running) {
			return NodeStatus::Running; // Still processing
		}
		if (status == NodeStatus::Failure) {
			current_child_ = 0; // Reset for next run
			return NodeStatus::Failure;
		}
		// Continue to next child on Success
	}

	// All children succeeded
	current_child_ = 0; // Reset for next run
	return NodeStatus::Success;
}

NodeStatus ParallelNode::Tick(Blackboard& blackboard) {
	int success_count = 0;
	int failure_count = 0;
	int running_count = 0;

	// Run all children
	for (auto& child : children_) {
		NodeStatus status = child->Tick(blackboard);

		switch (status) {
		case NodeStatus::Success:
			++success_count;
			break;
		case NodeStatus::Failure:
			++failure_count;
			break;
		case NodeStatus::Running:
			++running_count;
			break;
		}
	}

	// Check if any are still running
	if (running_count > 0) {
		return NodeStatus::Running;
	}

	// Apply policy
	if (policy_ == Policy::RequireAll) {
		// All must succeed
		return (failure_count == 0) ? NodeStatus::Success : NodeStatus::Failure;
	}
	else {
		// At least one must succeed
		return (success_count > 0) ? NodeStatus::Success : NodeStatus::Failure;
	}
}

// === DECORATOR NODES IMPLEMENTATION ===

NodeStatus InverterNode::Tick(Blackboard& blackboard) {
	if (children_.empty()) {
		return NodeStatus::Failure;
	}

	NodeStatus status = children_[0]->Tick(blackboard);

	// Invert Success <-> Failure, keep Running
	if (status == NodeStatus::Success) {
		return NodeStatus::Failure;
	}
	if (status == NodeStatus::Failure) {
		return NodeStatus::Success;
	}
	return NodeStatus::Running;
}

NodeStatus RepeaterNode::Tick(Blackboard& blackboard) {
	if (children_.empty()) {
		return NodeStatus::Failure;
	}

	while (current_iteration_ < repeat_count_) {
		NodeStatus status = children_[0]->Tick(blackboard);

		if (status == NodeStatus::Running) {
			return NodeStatus::Running;
		}
		if (status == NodeStatus::Failure) {
			current_iteration_ = 0; // Reset for next run
			return NodeStatus::Failure;
		}

		// Success - continue to next iteration
		++current_iteration_;
	}

	// All repetitions completed successfully
	current_iteration_ = 0; // Reset for next run
	return NodeStatus::Success;
}

NodeStatus SucceederNode::Tick(Blackboard& blackboard) {
	// Run child if present, but always return Success
	if (!children_.empty()) {
		children_[0]->Tick(blackboard);
	}
	return NodeStatus::Success;
}

// === LEAF/ACTION NODES IMPLEMENTATION ===

NodeStatus WaitNode::Tick(Blackboard& blackboard) {
	// Try to get delta_time from blackboard
	auto delta_time = blackboard.Get<float>("delta_time");
	if (!delta_time) {
		// No delta_time available, assume small value
		delta_time = 0.016F; // ~60 FPS
	}

	elapsed_time_ += *delta_time;

	if (elapsed_time_ >= duration_) {
		elapsed_time_ = 0.0F; // Reset for next run
		return NodeStatus::Success;
	}

	return NodeStatus::Running;
}

NodeStatus LogNode::Tick([[maybe_unused]] Blackboard& blackboard) {
	std::cout << "[BehaviorTree] " << name_ << ": " << message_ << std::endl;
	return NodeStatus::Success;
}

NodeStatus ConditionNode::Tick(Blackboard& blackboard) {
	// Check if the key exists and is true
	auto value = blackboard.Get<bool>(blackboard_key_);
	if (value && *value) {
		return NodeStatus::Success;
	}
	return NodeStatus::Failure;
}

} // namespace engine::ai
