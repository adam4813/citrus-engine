#include <gtest/gtest.h>

#include <any>
#include <memory>

import engine.graph;
import glm;

class GraphFrameworkTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Clear the registry before each test
		auto& registry = engine::graph::NodeTypeRegistry::GetGlobal();
		registry.Clear();
	}

	void TearDown() override {}
};

// === Type Compatibility Tests ===

TEST_F(GraphFrameworkTest, type_compatibility_exact_match) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Float));
	EXPECT_TRUE(AreTypesCompatible(PinType::Int, PinType::Int));
	EXPECT_TRUE(AreTypesCompatible(PinType::Bool, PinType::Bool));
	EXPECT_TRUE(AreTypesCompatible(PinType::Vec3, PinType::Vec3));
}

TEST_F(GraphFrameworkTest, type_compatibility_any_accepts_all) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Any, PinType::Float));
	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Any));
	EXPECT_TRUE(AreTypesCompatible(PinType::Any, PinType::Vec3));
	EXPECT_TRUE(AreTypesCompatible(PinType::Any, PinType::Any));
}

TEST_F(GraphFrameworkTest, type_compatibility_float_broadcasts_to_vectors) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Vec2));
	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Vec3));
	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Vec4));
	EXPECT_TRUE(AreTypesCompatible(PinType::Float, PinType::Color));
}

TEST_F(GraphFrameworkTest, type_compatibility_color_and_vec4_interchangeable) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Color, PinType::Vec4));
	EXPECT_TRUE(AreTypesCompatible(PinType::Vec4, PinType::Color));
}

TEST_F(GraphFrameworkTest, type_compatibility_int_promotes_to_float) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Int, PinType::Float));
	EXPECT_FALSE(AreTypesCompatible(PinType::Float, PinType::Int));
}

TEST_F(GraphFrameworkTest, type_compatibility_bool_converts_to_int) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Bool, PinType::Int));
	EXPECT_FALSE(AreTypesCompatible(PinType::Int, PinType::Bool));
}

TEST_F(GraphFrameworkTest, type_compatibility_flow_only_connects_to_flow) {
	using engine::graph::AreTypesCompatible;
	using engine::graph::PinType;

	EXPECT_TRUE(AreTypesCompatible(PinType::Flow, PinType::Flow));
	EXPECT_FALSE(AreTypesCompatible(PinType::Flow, PinType::Float));
	EXPECT_FALSE(AreTypesCompatible(PinType::Float, PinType::Flow));
	// Even Any can't connect to Flow
	EXPECT_FALSE(AreTypesCompatible(PinType::Any, PinType::Flow));
	EXPECT_FALSE(AreTypesCompatible(PinType::Flow, PinType::Any));
}

// === NodeGraph Tests ===

TEST_F(GraphFrameworkTest, node_graph_add_and_remove_nodes) {
	engine::graph::NodeGraph graph;

	EXPECT_EQ(graph.GetNodes().size(), 0);

	int node1_id = graph.AddNode("TestNode", glm::vec2(0, 0));
	EXPECT_GT(node1_id, 0);
	EXPECT_EQ(graph.GetNodes().size(), 1);

	int node2_id = graph.AddNode("TestNode2", glm::vec2(100, 100));
	EXPECT_GT(node2_id, 0);
	EXPECT_NE(node1_id, node2_id);
	EXPECT_EQ(graph.GetNodes().size(), 2);

	graph.RemoveNode(node1_id);
	EXPECT_EQ(graph.GetNodes().size(), 1);

	graph.RemoveNode(node2_id);
	EXPECT_EQ(graph.GetNodes().size(), 0);
}

TEST_F(GraphFrameworkTest, node_graph_get_node_by_id) {
	engine::graph::NodeGraph graph;

	int node_id = graph.AddNode("TestNode", glm::vec2(50, 50));

	auto* node = graph.GetNode(node_id);
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(node->id, node_id);
	EXPECT_EQ(node->type_name, "TestNode");
	EXPECT_EQ(node->position.x, 50.0f);
	EXPECT_EQ(node->position.y, 50.0f);

	auto* invalid_node = graph.GetNode(999);
	EXPECT_EQ(invalid_node, nullptr);
}

TEST_F(GraphFrameworkTest, node_graph_add_link_validates_pins) {
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	engine::graph::NodeGraph graph;

	// Create two nodes
	int node1_id = graph.AddNode("Node1", glm::vec2(0, 0));
	int node2_id = graph.AddNode("Node2", glm::vec2(100, 0));

	// Add pins
	auto* node1 = graph.GetNode(node1_id);
	auto* node2 = graph.GetNode(node2_id);

	node1->outputs.push_back(Pin(1, "Out", PinType::Float, PinDirection::Output, 0.0f));
	node2->inputs.push_back(Pin(2, "In", PinType::Float, PinDirection::Input, 0.0f));

	// Valid connection
	int link_id = graph.AddLink(node1_id, 0, node2_id, 0);
	EXPECT_GT(link_id, 0);
	EXPECT_EQ(graph.GetLinks().size(), 1);

	// Invalid connection (out of bounds)
	int invalid_link = graph.AddLink(node1_id, 99, node2_id, 0);
	EXPECT_EQ(invalid_link, -1);
	EXPECT_EQ(graph.GetLinks().size(), 1); // No new link added
}

TEST_F(GraphFrameworkTest, node_graph_cannot_connect_node_to_itself) {
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	engine::graph::NodeGraph graph;

	int node_id = graph.AddNode("Node", glm::vec2(0, 0));
	auto* node = graph.GetNode(node_id);

	node->outputs.push_back(Pin(1, "Out", PinType::Float, PinDirection::Output, 0.0f));
	node->inputs.push_back(Pin(2, "In", PinType::Float, PinDirection::Input, 0.0f));

	// Try to connect node to itself - should fail
	int link_id = graph.AddLink(node_id, 0, node_id, 0);
	EXPECT_EQ(link_id, -1);
	EXPECT_EQ(graph.GetLinks().size(), 0);
}

TEST_F(GraphFrameworkTest, node_graph_removing_node_removes_connected_links) {
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	engine::graph::NodeGraph graph;

	int node1_id = graph.AddNode("Node1", glm::vec2(0, 0));
	int node2_id = graph.AddNode("Node2", glm::vec2(100, 0));
	int node3_id = graph.AddNode("Node3", glm::vec2(200, 0));

	auto* node1 = graph.GetNode(node1_id);
	auto* node2 = graph.GetNode(node2_id);
	auto* node3 = graph.GetNode(node3_id);

	node1->outputs.push_back(Pin(1, "Out", PinType::Float, PinDirection::Output));
	node2->inputs.push_back(Pin(2, "In", PinType::Float, PinDirection::Input));
	node2->outputs.push_back(Pin(3, "Out", PinType::Float, PinDirection::Output));
	node3->inputs.push_back(Pin(4, "In", PinType::Float, PinDirection::Input));

	graph.AddLink(node1_id, 0, node2_id, 0);
	graph.AddLink(node2_id, 0, node3_id, 0);

	EXPECT_EQ(graph.GetLinks().size(), 2);

	// Remove middle node - should remove both links
	graph.RemoveNode(node2_id);

	EXPECT_EQ(graph.GetLinks().size(), 0);
	EXPECT_EQ(graph.GetNodes().size(), 2);
}

TEST_F(GraphFrameworkTest, node_graph_clear_removes_everything) {
	engine::graph::NodeGraph graph;

	graph.AddNode("Node1", glm::vec2(0, 0));
	graph.AddNode("Node2", glm::vec2(100, 0));

	EXPECT_EQ(graph.GetNodes().size(), 2);

	graph.Clear();

	EXPECT_EQ(graph.GetNodes().size(), 0);
	EXPECT_EQ(graph.GetLinks().size(), 0);
}

// === Node Type Registry Tests ===

TEST_F(GraphFrameworkTest, node_type_registry_register_and_get) {
	using engine::graph::NodeTypeDefinition;
	using engine::graph::NodeTypeRegistry;

	auto& registry = NodeTypeRegistry::GetGlobal();

	NodeTypeDefinition def("Add", "Math", "Adds two numbers");
	registry.Register(def);

	const auto* retrieved = registry.Get("Add");
	ASSERT_NE(retrieved, nullptr);
	EXPECT_EQ(retrieved->name, "Add");
	EXPECT_EQ(retrieved->category, "Math");
	EXPECT_EQ(retrieved->description, "Adds two numbers");
}

TEST_F(GraphFrameworkTest, node_type_registry_get_categories) {
	using engine::graph::NodeTypeDefinition;
	using engine::graph::NodeTypeRegistry;

	auto& registry = NodeTypeRegistry::GetGlobal();

	registry.Register(NodeTypeDefinition("Add", "Math"));
	registry.Register(NodeTypeDefinition("Multiply", "Math"));
	registry.Register(NodeTypeDefinition("Noise", "Texture"));

	auto categories = registry.GetCategories();
	EXPECT_EQ(categories.size(), 2);
	// Categories are sorted
	EXPECT_EQ(categories[0], "Math");
	EXPECT_EQ(categories[1], "Texture");
}

TEST_F(GraphFrameworkTest, node_type_registry_get_by_category) {
	using engine::graph::NodeTypeDefinition;
	using engine::graph::NodeTypeRegistry;

	auto& registry = NodeTypeRegistry::GetGlobal();

	registry.Register(NodeTypeDefinition("Add", "Math"));
	registry.Register(NodeTypeDefinition("Multiply", "Math"));
	registry.Register(NodeTypeDefinition("Noise", "Texture"));

	auto math_types = registry.GetByCategory("Math");
	EXPECT_EQ(math_types.size(), 2);

	auto texture_types = registry.GetByCategory("Texture");
	EXPECT_EQ(texture_types.size(), 1);

	auto empty = registry.GetByCategory("NonExistent");
	EXPECT_EQ(empty.size(), 0);
}

// === Graph Evaluator Tests ===

TEST_F(GraphFrameworkTest, graph_evaluator_detects_cycles) {
	using engine::graph::GraphEvaluator;
	using engine::graph::NodeGraph;
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	NodeGraph graph;
	GraphEvaluator evaluator;

	// Create a cycle: A -> B -> C -> A
	int node_a = graph.AddNode("A", glm::vec2(0, 0));
	int node_b = graph.AddNode("B", glm::vec2(100, 0));
	int node_c = graph.AddNode("C", glm::vec2(200, 0));

	auto* a = graph.GetNode(node_a);
	auto* b = graph.GetNode(node_b);
	auto* c = graph.GetNode(node_c);

	a->outputs.push_back(Pin(1, "Out", PinType::Float, PinDirection::Output));
	a->inputs.push_back(Pin(2, "In", PinType::Float, PinDirection::Input));

	b->outputs.push_back(Pin(3, "Out", PinType::Float, PinDirection::Output));
	b->inputs.push_back(Pin(4, "In", PinType::Float, PinDirection::Input));

	c->outputs.push_back(Pin(5, "Out", PinType::Float, PinDirection::Output));
	c->inputs.push_back(Pin(6, "In", PinType::Float, PinDirection::Input));

	graph.AddLink(node_a, 0, node_b, 0); // A -> B
	graph.AddLink(node_b, 0, node_c, 0); // B -> C
	graph.AddLink(node_c, 0, node_a, 0); // C -> A (creates cycle)

	EXPECT_TRUE(evaluator.HasCycles(graph));
}

TEST_F(GraphFrameworkTest, graph_evaluator_topological_sort_simple_chain) {
	using engine::graph::GraphEvaluator;
	using engine::graph::NodeGraph;
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	NodeGraph graph;
	GraphEvaluator evaluator;

	// Create a simple chain: A -> B -> C
	int node_a = graph.AddNode("A", glm::vec2(0, 0));
	int node_b = graph.AddNode("B", glm::vec2(100, 0));
	int node_c = graph.AddNode("C", glm::vec2(200, 0));

	auto* a = graph.GetNode(node_a);
	auto* b = graph.GetNode(node_b);
	auto* c = graph.GetNode(node_c);

	a->outputs.push_back(Pin(1, "Out", PinType::Float, PinDirection::Output));
	b->inputs.push_back(Pin(2, "In", PinType::Float, PinDirection::Input));
	b->outputs.push_back(Pin(3, "Out", PinType::Float, PinDirection::Output));
	c->inputs.push_back(Pin(4, "In", PinType::Float, PinDirection::Input));

	graph.AddLink(node_a, 0, node_b, 0); // A -> B
	graph.AddLink(node_b, 0, node_c, 0); // B -> C

	auto sorted = evaluator.TopologicalSort(graph);

	ASSERT_EQ(sorted.size(), 3);
	// A should come before B, B should come before C
	auto a_pos = std::find(sorted.begin(), sorted.end(), node_a);
	auto b_pos = std::find(sorted.begin(), sorted.end(), node_b);
	auto c_pos = std::find(sorted.begin(), sorted.end(), node_c);

	EXPECT_LT(a_pos, b_pos);
	EXPECT_LT(b_pos, c_pos);
}

TEST_F(GraphFrameworkTest, graph_evaluator_empty_graph_has_no_cycles) {
	using engine::graph::GraphEvaluator;
	using engine::graph::NodeGraph;

	NodeGraph graph;
	GraphEvaluator evaluator;

	EXPECT_FALSE(evaluator.HasCycles(graph));
}

// === Serialization Tests ===

TEST_F(GraphFrameworkTest, graph_serialization_round_trip) {
	using engine::graph::GraphSerializer;
	using engine::graph::NodeGraph;
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	NodeGraph original_graph;

	// Create some nodes
	int node1 = original_graph.AddNode("Math/Add", glm::vec2(100, 100));
	int node2 = original_graph.AddNode("Math/Multiply", glm::vec2(300, 100));

	// Add pins
	auto* n1 = original_graph.GetNode(node1);
	auto* n2 = original_graph.GetNode(node2);

	n1->inputs.push_back(Pin(1, "A", PinType::Float, PinDirection::Input, 1.0f));
	n1->inputs.push_back(Pin(2, "B", PinType::Float, PinDirection::Input, 2.0f));
	n1->outputs.push_back(Pin(3, "Result", PinType::Float, PinDirection::Output));

	n2->inputs.push_back(Pin(4, "A", PinType::Float, PinDirection::Input, 3.0f));
	n2->outputs.push_back(Pin(5, "Result", PinType::Float, PinDirection::Output));

	// Add a link
	original_graph.AddLink(node1, 0, node2, 0);

	// Serialize to JSON
	std::string json = GraphSerializer::Serialize(original_graph);
	EXPECT_FALSE(json.empty());

	// Deserialize to new graph
	NodeGraph loaded_graph;
	bool success = GraphSerializer::Deserialize(json, loaded_graph);
	ASSERT_TRUE(success);

	// Verify nodes
	EXPECT_EQ(loaded_graph.GetNodes().size(), 2);

	// Verify links
	EXPECT_EQ(loaded_graph.GetLinks().size(), 1);

	// Verify node data is preserved
	const auto* loaded_n1 = loaded_graph.GetNode(node1);
	ASSERT_NE(loaded_n1, nullptr);
	EXPECT_EQ(loaded_n1->type_name, "Math/Add");
	EXPECT_EQ(loaded_n1->position.x, 100.0f);
	EXPECT_EQ(loaded_n1->position.y, 100.0f);
	EXPECT_EQ(loaded_n1->inputs.size(), 2);
	EXPECT_EQ(loaded_n1->outputs.size(), 1);
}

TEST_F(GraphFrameworkTest, graph_serialization_preserves_pin_types) {
	using engine::graph::GraphSerializer;
	using engine::graph::NodeGraph;
	using engine::graph::Pin;
	using engine::graph::PinDirection;
	using engine::graph::PinType;

	NodeGraph graph;
	int node_id = graph.AddNode("TestNode", glm::vec2(0, 0));
	auto* node = graph.GetNode(node_id);

	// Add pins of different types
	node->inputs.push_back(Pin(1, "Bool", PinType::Bool, PinDirection::Input, true));
	node->inputs.push_back(Pin(2, "Int", PinType::Int, PinDirection::Input, 42));
	node->inputs.push_back(Pin(3, "Float", PinType::Float, PinDirection::Input, 3.14f));
	node->inputs.push_back(Pin(4, "Vec3", PinType::Vec3, PinDirection::Input, glm::vec3(1.0f, 2.0f, 3.0f)));
	node->inputs.push_back(Pin(5, "String", PinType::String, PinDirection::Input, std::string("test")));

	// Serialize and deserialize
	std::string json = GraphSerializer::Serialize(graph);
	NodeGraph loaded_graph;
	ASSERT_TRUE(GraphSerializer::Deserialize(json, loaded_graph));

	// Verify pin types are preserved
	const auto* loaded_node = loaded_graph.GetNode(node_id);
	ASSERT_NE(loaded_node, nullptr);
	ASSERT_EQ(loaded_node->inputs.size(), 5);

	EXPECT_EQ(loaded_node->inputs[0].type, PinType::Bool);
	EXPECT_EQ(loaded_node->inputs[1].type, PinType::Int);
	EXPECT_EQ(loaded_node->inputs[2].type, PinType::Float);
	EXPECT_EQ(loaded_node->inputs[3].type, PinType::Vec3);
	EXPECT_EQ(loaded_node->inputs[4].type, PinType::String);
}
