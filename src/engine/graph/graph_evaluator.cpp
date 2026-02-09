module;

#include <any>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

module engine.graph;

import :graph_evaluator;
import :node_graph;
import :types;

namespace engine::graph {

std::vector<int> GraphEvaluator::TopologicalSort(const NodeGraph& graph) const {
	std::vector<int> sorted;
	std::map<int, VisitState> visit_state;

	// Initialize all nodes as unvisited
	for (const auto& node : graph.GetNodes()) {
		visit_state[node.id] = VisitState::Unvisited;
	}

	// Visit each unvisited node
	for (const auto& node : graph.GetNodes()) {
		if (visit_state[node.id] == VisitState::Unvisited) {
			if (!TopologicalSortDFS(node.id, graph, visit_state, sorted)) {
				// Cycle detected
				return {};
			}
		}
	}

	return sorted;
}

bool GraphEvaluator::HasCycles(const NodeGraph& graph) const {
	return TopologicalSort(graph).empty() && !graph.GetNodes().empty();
}

std::map<int, std::any> GraphEvaluator::Evaluate(const NodeGraph& graph,
												 const std::map<std::string, INodeEvaluator*>& evaluators) {
	// Get evaluation order
	std::vector<int> sorted = TopologicalSort(graph);
	if (sorted.empty() && !graph.GetNodes().empty()) {
		// Graph has cycles - can't evaluate
		return {};
	}

	// Map of node ID -> output pin index -> value
	std::map<int, std::map<int, std::any>> evaluated_outputs;

	// Evaluate each node in order
	for (int node_id : sorted) {
		const Node* node = graph.GetNode(node_id);
		if (!node) {
			continue;
		}

		// Find the evaluator for this node type
		auto eval_it = evaluators.find(node->type_name);
		if (eval_it == evaluators.end()) {
			// No evaluator for this node type - skip it
			continue;
		}

		INodeEvaluator* evaluator = eval_it->second;
		if (!evaluator) {
			continue;
		}

		// Get input values for this node
		std::map<int, std::any> inputs = GetNodeInputs(*node, graph, evaluated_outputs);

		// Evaluate the node
		std::map<int, std::any> outputs = evaluator->Evaluate(*node, inputs);

		// Store the outputs
		evaluated_outputs[node_id] = std::move(outputs);
	}

	// Flatten the results into a single map (node_id -> first output value)
	std::map<int, std::any> results;
	for (const auto& [node_id, outputs] : evaluated_outputs) {
		if (!outputs.empty()) {
			results[node_id] = outputs.begin()->second;
		}
	}

	return results;
}

bool GraphEvaluator::TopologicalSortDFS(int node_id, const NodeGraph& graph,
										std::map<int, VisitState>& visit_state,
										std::vector<int>& sorted) const {
	// Mark as visiting
	visit_state[node_id] = VisitState::Visiting;

	// Visit all dependencies (nodes connected to our inputs)
	const Node* node = graph.GetNode(node_id);
	if (!node) {
		return true;
	}

	// Find all links that connect to this node's inputs
	for (const auto& link : graph.GetLinks()) {
		if (link.to_node_id == node_id) {
			int dependency_id = link.from_node_id;

			if (visit_state[dependency_id] == VisitState::Visiting) {
				// Cycle detected
				return false;
			}

			if (visit_state[dependency_id] == VisitState::Unvisited) {
				if (!TopologicalSortDFS(dependency_id, graph, visit_state, sorted)) {
					return false;
				}
			}
		}
	}

	// Mark as visited and add to sorted list
	visit_state[node_id] = VisitState::Visited;
	sorted.push_back(node_id);

	return true;
}

std::map<int, std::any> GraphEvaluator::GetNodeInputs(
	const Node& node, const NodeGraph& graph,
	const std::map<int, std::map<int, std::any>>& evaluated_outputs) const {

	std::map<int, std::any> inputs;

	// For each input pin, find the connected link and get the value
	for (size_t i = 0; i < node.inputs.size(); ++i) {
		const Pin& input_pin = node.inputs[i];

		// Find link connected to this input
		bool found_connection = false;
		for (const auto& link : graph.GetLinks()) {
			if (link.to_node_id == node.id && link.to_pin_index == static_cast<int>(i)) {
				// Found a connection
				auto output_it = evaluated_outputs.find(link.from_node_id);
				if (output_it != evaluated_outputs.end()) {
					auto pin_it = output_it->second.find(link.from_pin_index);
					if (pin_it != output_it->second.end()) {
						inputs[static_cast<int>(i)] = pin_it->second;
						found_connection = true;
						break;
					}
				}
			}
		}

		// If no connection, use the default value
		if (!found_connection) {
			inputs[static_cast<int>(i)] = input_pin.default_value;
		}
	}

	return inputs;
}

} // namespace engine::graph
