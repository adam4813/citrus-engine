module;

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

export module engine.graph:graph_evaluator;

import :types;
import :node_graph;

export namespace engine::graph {

/// Interface for node evaluators
/// Each node type provides an evaluator that knows how to compute its outputs
class INodeEvaluator {
public:
	virtual ~INodeEvaluator() = default;

	/// Evaluate a node given its input values
	/// @param node The node to evaluate
	/// @param inputs Map of input pin indices to their values
	/// @return Map of output pin indices to their computed values
	virtual std::map<int, std::any> Evaluate(const Node& node, const std::map<int, std::any>& inputs) = 0;
};

/// Graph evaluator - executes a node graph
/// Performs topological sort and evaluates nodes in dependency order
class GraphEvaluator {
public:
	GraphEvaluator() = default;
	~GraphEvaluator() = default;

	/// Perform topological sort on the graph
	/// @param graph The graph to sort
	/// @return List of node IDs in evaluation order, empty if graph has cycles
	std::vector<int> TopologicalSort(const NodeGraph& graph) const;

	/// Check if the graph contains cycles
	/// @param graph The graph to check
	/// @return true if the graph has cycles (invalid for evaluation)
	bool HasCycles(const NodeGraph& graph) const;

	/// Evaluate the entire graph
	/// @param graph The graph to evaluate
	/// @param evaluators Map of node type names to their evaluator instances
	/// @return Map of node IDs to their output values
	std::map<int, std::any> Evaluate(const NodeGraph& graph,
									 const std::map<std::string, INodeEvaluator*>& evaluators);

private:
	/// Helper for topological sort - depth-first search
	enum class VisitState { Unvisited, Visiting, Visited };

	bool TopologicalSortDFS(int node_id, const NodeGraph& graph,
							std::map<int, VisitState>& visit_state, std::vector<int>& sorted) const;

	/// Get input values for a node from evaluated results
	std::map<int, std::any> GetNodeInputs(const Node& node, const NodeGraph& graph,
										  const std::map<int, std::map<int, std::any>>& evaluated_outputs) const;
};

} // namespace engine::graph
