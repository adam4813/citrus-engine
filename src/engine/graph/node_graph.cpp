module;

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

module engine.graph;

import :node_graph;
import :types;
import glm;

namespace engine::graph {

int NodeGraph::AddNode(const std::string& type_name, glm::vec2 position) {
	int node_id = next_id_++;

	Node node(node_id, type_name, position);
	nodes_.push_back(std::move(node));

	return node_id;
}

void NodeGraph::RemoveNode(int node_id) {
	// Remove all links connected to this node
	links_.erase(std::remove_if(links_.begin(), links_.end(),
								[node_id](const Link& link) {
									return link.from_node_id == node_id || link.to_node_id == node_id;
								}),
				 links_.end());

	// Remove the node
	auto it = FindNode(node_id);
	if (it != nodes_.end()) {
		nodes_.erase(it);
	}
}

Node* NodeGraph::GetNode(int id) {
	auto it = FindNode(id);
	return it != nodes_.end() ? &(*it) : nullptr;
}

const Node* NodeGraph::GetNode(int id) const {
	auto it = FindNode(id);
	return it != nodes_.end() ? &(*it) : nullptr;
}

int NodeGraph::AddLink(int from_node, int from_pin, int to_node, int to_pin) {
	// Validate connection
	if (!CanConnect(from_node, from_pin, to_node, to_pin)) {
		return -1;
	}

	// Remove any existing link to the input pin (inputs can only have one connection)
	const Node* to_node_ptr = GetNode(to_node);
	if (to_node_ptr && to_pin < static_cast<int>(to_node_ptr->inputs.size())) {
		links_.erase(std::remove_if(links_.begin(), links_.end(),
									[to_node, to_pin](const Link& link) {
										return link.to_node_id == to_node && link.to_pin_index == to_pin;
									}),
					 links_.end());
	}

	int link_id = next_id_++;
	links_.emplace_back(link_id, from_node, from_pin, to_node, to_pin);

	return link_id;
}

void NodeGraph::RemoveLink(int link_id) {
	auto it = FindLink(link_id);
	if (it != links_.end()) {
		links_.erase(it);
	}
}

Link* NodeGraph::GetLink(int id) {
	auto it = FindLink(id);
	return it != links_.end() ? &(*it) : nullptr;
}

const Link* NodeGraph::GetLink(int id) const {
	auto it = FindLink(id);
	return it != links_.end() ? &(*it) : nullptr;
}

bool NodeGraph::CanConnect(int from_node, int from_pin, int to_node, int to_pin) const {
	// Can't connect a node to itself
	if (from_node == to_node) {
		return false;
	}

	// Find the nodes
	const Node* from_node_ptr = GetNode(from_node);
	const Node* to_node_ptr = GetNode(to_node);

	if (!from_node_ptr || !to_node_ptr) {
		return false;
	}

	// Validate pin indices
	if (from_pin < 0 || from_pin >= static_cast<int>(from_node_ptr->outputs.size())) {
		return false;
	}

	if (to_pin < 0 || to_pin >= static_cast<int>(to_node_ptr->inputs.size())) {
		return false;
	}

	// Get the pins
	const Pin& from = from_node_ptr->outputs[from_pin];
	const Pin& to = to_node_ptr->inputs[to_pin];

	// Check direction (output -> input)
	if (from.direction != PinDirection::Output || to.direction != PinDirection::Input) {
		return false;
	}

	// Check type compatibility
	return AreTypesCompatible(from.type, to.type);
}

void NodeGraph::Clear() {
	nodes_.clear();
	links_.clear();
	next_id_ = 1;
}

auto NodeGraph::FindNode(int id) -> decltype(nodes_.begin()) {
	return std::find_if(nodes_.begin(), nodes_.end(), [id](const Node& node) { return node.id == id; });
}

auto NodeGraph::FindNode(int id) const -> decltype(nodes_.begin()) {
	return std::find_if(nodes_.begin(), nodes_.end(), [id](const Node& node) { return node.id == id; });
}

auto NodeGraph::FindLink(int id) -> decltype(links_.begin()) {
	return std::find_if(links_.begin(), links_.end(), [id](const Link& link) { return link.id == id; });
}

auto NodeGraph::FindLink(int id) const -> decltype(links_.begin()) {
	return std::find_if(links_.begin(), links_.end(), [id](const Link& link) { return link.id == id; });
}

} // namespace engine::graph
