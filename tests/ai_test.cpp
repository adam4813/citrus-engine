#include <gtest/gtest.h>

#include <memory>
#include <string>

import engine.ai;

using namespace engine::ai;

// === Helper: Simple action node that always returns a fixed status ===

class FixedStatusNode : public BTNode {
public:
	explicit FixedStatusNode(NodeStatus status, std::string name = "FixedStatus")
		: BTNode(std::move(name)), status_(status) {}

	NodeStatus Tick(Blackboard& /*blackboard*/) override {
		++tick_count_;
		return status_;
	}

	[[nodiscard]] std::string GetTypeName() const override { return "FixedStatus"; }

	void SetStatus(NodeStatus s) { status_ = s; }
	[[nodiscard]] int GetTickCount() const { return tick_count_; }

private:
	NodeStatus status_;
	int tick_count_ = 0;
};

// === Helper: Node that returns Running N times, then a final status ===

class CountdownNode : public BTNode {
public:
	CountdownNode(int running_ticks, NodeStatus final_status, std::string name = "Countdown")
		: BTNode(std::move(name)), remaining_(running_ticks), final_status_(final_status) {}

	NodeStatus Tick(Blackboard& /*blackboard*/) override {
		if (remaining_ > 0) {
			--remaining_;
			return NodeStatus::Running;
		}
		return final_status_;
	}

	[[nodiscard]] std::string GetTypeName() const override { return "Countdown"; }

private:
	int remaining_;
	NodeStatus final_status_;
};

// =============================================================================
// Blackboard Tests
// =============================================================================

TEST(BlackboardTest, initially_empty) {
	Blackboard bb;
	EXPECT_TRUE(bb.IsEmpty());
	EXPECT_EQ(bb.Size(), 0u);
}

TEST(BlackboardTest, set_and_get_int) {
	Blackboard bb;
	bb.Set<int>("health", 100);
	auto val = bb.Get<int>("health");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 100);
}

TEST(BlackboardTest, set_and_get_float) {
	Blackboard bb;
	bb.Set<float>("speed", 5.5f);
	auto val = bb.Get<float>("speed");
	ASSERT_TRUE(val.has_value());
	EXPECT_FLOAT_EQ(*val, 5.5f);
}

TEST(BlackboardTest, set_and_get_string) {
	Blackboard bb;
	bb.Set<std::string>("name", "enemy");
	auto val = bb.Get<std::string>("name");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, "enemy");
}

TEST(BlackboardTest, set_and_get_bool) {
	Blackboard bb;
	bb.Set<bool>("is_alive", true);
	auto val = bb.Get<bool>("is_alive");
	ASSERT_TRUE(val.has_value());
	EXPECT_TRUE(*val);
}

TEST(BlackboardTest, get_missing_key_returns_nullopt) {
	Blackboard bb;
	auto val = bb.Get<int>("nonexistent");
	EXPECT_FALSE(val.has_value());
}

TEST(BlackboardTest, get_wrong_type_returns_nullopt) {
	Blackboard bb;
	bb.Set<int>("health", 100);
	auto val = bb.Get<float>("health");
	EXPECT_FALSE(val.has_value());
}

TEST(BlackboardTest, has_returns_true_for_existing_key) {
	Blackboard bb;
	bb.Set<int>("x", 1);
	EXPECT_TRUE(bb.Has("x"));
	EXPECT_FALSE(bb.Has("y"));
}

TEST(BlackboardTest, remove_key) {
	Blackboard bb;
	bb.Set<int>("x", 1);
	EXPECT_TRUE(bb.Remove("x"));
	EXPECT_FALSE(bb.Has("x"));
	EXPECT_FALSE(bb.Remove("x")); // Already removed
}

TEST(BlackboardTest, clear_removes_all) {
	Blackboard bb;
	bb.Set<int>("a", 1);
	bb.Set<int>("b", 2);
	EXPECT_EQ(bb.Size(), 2u);

	bb.Clear();
	EXPECT_TRUE(bb.IsEmpty());
	EXPECT_EQ(bb.Size(), 0u);
}

TEST(BlackboardTest, overwrite_existing_key) {
	Blackboard bb;
	bb.Set<int>("val", 10);
	bb.Set<int>("val", 20);
	auto val = bb.Get<int>("val");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 20);
}

// =============================================================================
// Leaf Node Tests
// =============================================================================

TEST(ConditionNodeTest, returns_success_when_blackboard_key_is_true) {
	Blackboard bb;
	bb.Set<bool>("enemy_visible", true);
	ConditionNode node("CheckEnemy", "enemy_visible");
	EXPECT_EQ(node.Tick(bb), NodeStatus::Success);
}

TEST(ConditionNodeTest, returns_failure_when_blackboard_key_is_false) {
	Blackboard bb;
	bb.Set<bool>("enemy_visible", false);
	ConditionNode node("CheckEnemy", "enemy_visible");
	EXPECT_EQ(node.Tick(bb), NodeStatus::Failure);
}

TEST(ConditionNodeTest, returns_failure_when_key_missing) {
	Blackboard bb;
	ConditionNode node("CheckEnemy", "enemy_visible");
	EXPECT_EQ(node.Tick(bb), NodeStatus::Failure);
}

TEST(LogNodeTest, returns_success) {
	Blackboard bb;
	LogNode node("TestLog", "hello");
	EXPECT_EQ(node.Tick(bb), NodeStatus::Success);
}

TEST(WaitNodeTest, returns_running_then_success) {
	Blackboard bb;
	bb.Set<float>("delta_time", 0.5f);

	WaitNode node("Wait1s", 1.0f);

	// First tick: 0.5s elapsed, not done yet
	EXPECT_EQ(node.Tick(bb), NodeStatus::Running);

	// Second tick: 1.0s elapsed, done
	EXPECT_EQ(node.Tick(bb), NodeStatus::Success);
}

TEST(WaitNodeTest, uses_default_delta_when_not_in_blackboard) {
	Blackboard bb;
	// No delta_time set — defaults to ~0.016
	WaitNode node("WaitShort", 0.02f);

	// First tick: ~0.016s elapsed
	EXPECT_EQ(node.Tick(bb), NodeStatus::Running);

	// Second tick: ~0.032s elapsed, exceeds 0.02
	EXPECT_EQ(node.Tick(bb), NodeStatus::Success);
}

// =============================================================================
// Sequence Node Tests
// =============================================================================

TEST(SequenceNodeTest, all_children_succeed_returns_success) {
	Blackboard bb;
	auto seq = std::make_unique<SequenceNode>("Seq");
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "A"));
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "B"));
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "C"));

	EXPECT_EQ(seq->Tick(bb), NodeStatus::Success);
}

TEST(SequenceNodeTest, first_child_fails_returns_failure) {
	Blackboard bb;
	auto seq = std::make_unique<SequenceNode>("Seq");
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "Fail"));
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "Skip"));

	EXPECT_EQ(seq->Tick(bb), NodeStatus::Failure);
}

TEST(SequenceNodeTest, child_running_returns_running) {
	Blackboard bb;
	auto seq = std::make_unique<SequenceNode>("Seq");
	seq->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "Done"));
	seq->AddChild(std::make_unique<CountdownNode>(2, NodeStatus::Success, "Wait"));

	// First tick: second child returns Running
	EXPECT_EQ(seq->Tick(bb), NodeStatus::Running);
}

TEST(SequenceNodeTest, resumes_at_running_child) {
	Blackboard bb;
	auto seq = std::make_unique<SequenceNode>("Seq");

	auto* counter = new FixedStatusNode(NodeStatus::Success, "Counter");
	seq->AddChild(std::unique_ptr<BTNode>(counter));
	seq->AddChild(std::make_unique<CountdownNode>(1, NodeStatus::Success, "Wait"));

	// First tick: counter runs (tick 1), countdown returns Running
	EXPECT_EQ(seq->Tick(bb), NodeStatus::Running);

	// Second tick: resumes at countdown (now succeeds)
	// Counter should NOT be ticked again (sequence resumes at running child)
	EXPECT_EQ(seq->Tick(bb), NodeStatus::Success);
}

TEST(SequenceNodeTest, empty_sequence_returns_success) {
	Blackboard bb;
	SequenceNode seq("EmptySeq");
	EXPECT_EQ(seq.Tick(bb), NodeStatus::Success);
}

// =============================================================================
// Selector Node Tests
// =============================================================================

TEST(SelectorNodeTest, first_child_succeeds_returns_success) {
	Blackboard bb;
	auto sel = std::make_unique<SelectorNode>("Sel");
	sel->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "Win"));
	sel->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "Skip"));

	EXPECT_EQ(sel->Tick(bb), NodeStatus::Success);
}

TEST(SelectorNodeTest, all_children_fail_returns_failure) {
	Blackboard bb;
	auto sel = std::make_unique<SelectorNode>("Sel");
	sel->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "F1"));
	sel->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "F2"));

	EXPECT_EQ(sel->Tick(bb), NodeStatus::Failure);
}

TEST(SelectorNodeTest, tries_next_child_on_failure) {
	Blackboard bb;
	auto sel = std::make_unique<SelectorNode>("Sel");
	auto* first = new FixedStatusNode(NodeStatus::Failure, "F");
	auto* second = new FixedStatusNode(NodeStatus::Success, "S");
	sel->AddChild(std::unique_ptr<BTNode>(first));
	sel->AddChild(std::unique_ptr<BTNode>(second));

	EXPECT_EQ(sel->Tick(bb), NodeStatus::Success);
	EXPECT_EQ(first->GetTickCount(), 1);
	EXPECT_EQ(second->GetTickCount(), 1);
}

TEST(SelectorNodeTest, child_running_returns_running) {
	Blackboard bb;
	auto sel = std::make_unique<SelectorNode>("Sel");
	sel->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "F"));
	sel->AddChild(std::make_unique<CountdownNode>(1, NodeStatus::Success, "Wait"));

	EXPECT_EQ(sel->Tick(bb), NodeStatus::Running);
}

TEST(SelectorNodeTest, empty_selector_returns_failure) {
	Blackboard bb;
	SelectorNode sel("EmptySel");
	EXPECT_EQ(sel.Tick(bb), NodeStatus::Failure);
}

// =============================================================================
// Decorator Node Tests
// =============================================================================

TEST(InverterNodeTest, inverts_success_to_failure) {
	Blackboard bb;
	auto inv = std::make_unique<InverterNode>("Inv");
	inv->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success));
	EXPECT_EQ(inv->Tick(bb), NodeStatus::Failure);
}

TEST(InverterNodeTest, inverts_failure_to_success) {
	Blackboard bb;
	auto inv = std::make_unique<InverterNode>("Inv");
	inv->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure));
	EXPECT_EQ(inv->Tick(bb), NodeStatus::Success);
}

TEST(InverterNodeTest, preserves_running) {
	Blackboard bb;
	auto inv = std::make_unique<InverterNode>("Inv");
	inv->AddChild(std::make_unique<CountdownNode>(1, NodeStatus::Success));
	EXPECT_EQ(inv->Tick(bb), NodeStatus::Running);
}

TEST(InverterNodeTest, no_child_returns_failure) {
	Blackboard bb;
	InverterNode inv("Inv");
	EXPECT_EQ(inv.Tick(bb), NodeStatus::Failure);
}

TEST(RepeaterNodeTest, repeats_success_n_times) {
	Blackboard bb;
	auto rep = std::make_unique<RepeaterNode>("Rep", 3);
	rep->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success));

	EXPECT_EQ(rep->Tick(bb), NodeStatus::Success);
}

TEST(RepeaterNodeTest, stops_on_child_failure) {
	Blackboard bb;
	auto rep = std::make_unique<RepeaterNode>("Rep", 5);
	rep->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure));

	EXPECT_EQ(rep->Tick(bb), NodeStatus::Failure);
}

TEST(RepeaterNodeTest, no_child_returns_failure) {
	Blackboard bb;
	RepeaterNode rep("Rep", 3);
	EXPECT_EQ(rep.Tick(bb), NodeStatus::Failure);
}

TEST(SucceederNodeTest, always_returns_success) {
	Blackboard bb;
	auto suc = std::make_unique<SucceederNode>("Suc");
	suc->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure));
	EXPECT_EQ(suc->Tick(bb), NodeStatus::Success);
}

TEST(SucceederNodeTest, no_child_returns_success) {
	Blackboard bb;
	SucceederNode suc("Suc");
	EXPECT_EQ(suc.Tick(bb), NodeStatus::Success);
}

// =============================================================================
// Parallel Node Tests
// =============================================================================

TEST(ParallelNodeTest, require_all_succeeds_when_all_succeed) {
	Blackboard bb;
	auto par = std::make_unique<ParallelNode>("Par", ParallelNode::Policy::RequireAll);
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "A"));
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "B"));

	EXPECT_EQ(par->Tick(bb), NodeStatus::Success);
}

TEST(ParallelNodeTest, require_all_fails_when_one_fails) {
	Blackboard bb;
	auto par = std::make_unique<ParallelNode>("Par", ParallelNode::Policy::RequireAll);
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "A"));
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "B"));

	EXPECT_EQ(par->Tick(bb), NodeStatus::Failure);
}

TEST(ParallelNodeTest, require_one_succeeds_when_one_succeeds) {
	Blackboard bb;
	auto par = std::make_unique<ParallelNode>("Par", ParallelNode::Policy::RequireOne);
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "A"));
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "B"));

	EXPECT_EQ(par->Tick(bb), NodeStatus::Success);
}

TEST(ParallelNodeTest, require_one_fails_when_all_fail) {
	Blackboard bb;
	auto par = std::make_unique<ParallelNode>("Par", ParallelNode::Policy::RequireOne);
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "A"));
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Failure, "B"));

	EXPECT_EQ(par->Tick(bb), NodeStatus::Failure);
}

TEST(ParallelNodeTest, running_child_returns_running) {
	Blackboard bb;
	auto par = std::make_unique<ParallelNode>("Par", ParallelNode::Policy::RequireAll);
	par->AddChild(std::make_unique<FixedStatusNode>(NodeStatus::Success, "A"));
	par->AddChild(std::make_unique<CountdownNode>(1, NodeStatus::Success, "B"));

	EXPECT_EQ(par->Tick(bb), NodeStatus::Running);
}

// =============================================================================
// Tree Composition Tests
// =============================================================================

TEST(BehaviorTreeCompositionTest, selector_with_condition_and_action) {
	Blackboard bb;
	bb.Set<bool>("has_ammo", false);

	// Selector: try shoot (condition + action), fallback to melee
	auto root = std::make_unique<SelectorNode>("Root");

	auto shoot_seq = std::make_unique<SequenceNode>("ShootSequence");
	shoot_seq->AddChild(std::make_unique<ConditionNode>("HasAmmo", "has_ammo"));
	shoot_seq->AddChild(std::make_unique<LogNode>("Shoot", "Firing weapon"));

	auto melee = std::make_unique<LogNode>("Melee", "Melee attack");

	root->AddChild(std::move(shoot_seq));
	root->AddChild(std::move(melee));

	// No ammo: shoot sequence fails (condition fails), selector tries melee (succeeds)
	EXPECT_EQ(root->Tick(bb), NodeStatus::Success);

	// Now give ammo
	bb.Set<bool>("has_ammo", true);
	// Shoot sequence succeeds
	EXPECT_EQ(root->Tick(bb), NodeStatus::Success);
}

TEST(BehaviorTreeCompositionTest, node_name_and_type) {
	SequenceNode seq("MySequence");
	EXPECT_EQ(seq.GetName(), "MySequence");
	EXPECT_EQ(seq.GetTypeName(), "Sequence");

	SelectorNode sel("MySel");
	EXPECT_EQ(sel.GetTypeName(), "Selector");

	InverterNode inv("MyInv");
	EXPECT_EQ(inv.GetTypeName(), "Inverter");

	ConditionNode cond("MyCond", "key");
	EXPECT_EQ(cond.GetTypeName(), "Condition");
	EXPECT_EQ(cond.GetKey(), "key");

	LogNode log("MyLog", "msg");
	EXPECT_EQ(log.GetTypeName(), "Log");
	EXPECT_EQ(log.GetMessage(), "msg");

	WaitNode wait("MyWait", 2.0f);
	EXPECT_EQ(wait.GetTypeName(), "Wait");
	EXPECT_FLOAT_EQ(wait.GetDuration(), 2.0f);
}
