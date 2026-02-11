module;

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

export module engine.graph:node_graph;

import :types;
import glm;

export namespace engine::graph {

// Forward declaration for friend
class GraphSerializer;

/// Main node graph container
/// Manages nodes, links, and provides operations for graph manipulation
class NodeGraph {
public:
	NodeGraph() = default;
	~NodeGraph() = default;

	// Node operations

	/// Add a new node to the graph
	/// @param type_name The registered type name for this node
	/// @param position Canvas position for the node
	/// @return The ID of the newly created node
	int AddNode(const std::string& type_name, glm::vec2 position);

	/// Remove a node from the graph (also removes connected links)
	/// @param node_id The ID of the node to remove
	void RemoveNode(int node_id);

	/// Get a node by ID (mutable)
	/// @param id The node ID
	/// @return Pointer to the node, or nullptr if not found
	Node* GetNode(int id);

	/// Get a node by ID (const)
	/// @param id The node ID
	/// @return Pointer to the node, or nullptr if not found
	const Node* GetNode(int id) const;

	/// Get all nodes in the graph
	const std::vector<Node>& GetNodes() const { return nodes_; }

	// Link operations

	/// Add a link between two pins
	/// @param from_node The source node ID
	/// @param from_pin The source pin index
	/// @param to_node The destination node ID
	/// @param to_pin The destination pin index
	/// @return The ID of the newly created link, or -1 if connection is invalid
	int AddLink(int from_node, int from_pin, int to_node, int to_pin);

	/// Remove a link from the graph
	/// @param link_id The ID of the link to remove
	void RemoveLink(int link_id);

	/// Get all links in the graph
	const std::vector<Link>& GetLinks() const { return links_; }

	/// Get a link by ID
	/// @param id The link ID
	/// @return Pointer to the link, or nullptr if not found
	Link* GetLink(int id);

	/// Get a link by ID (const)
	const Link* GetLink(int id) const;

	// Connection validation

	/// Check if two pins can be connected
	/// @param from_node Source node ID
	/// @param from_pin Source pin index
	/// @param to_node Destination node ID
	/// @param to_pin Destination pin index
	/// @return true if connection is valid
	bool CanConnect(int from_node, int from_pin, int to_node, int to_pin) const;

	// Utility

	/// Clear all nodes and links
	void Clear();

	/// Get the next available ID (for external use if needed)
	int GetNextId() const { return next_id_; }

	/// Set the next ID (for deserialization)
	void SetNextId(int id) { next_id_ = id; }

private:
	friend class GraphSerializer;
	std::vector<Node> nodes_;
	std::vector<Link> links_;
	int next_id_ = 1;

	/// Find node iterator by ID
	auto FindNode(int id) -> decltype(nodes_.begin());
	auto FindNode(int id) const -> decltype(nodes_.begin());

	/// Find link iterator by ID
	auto FindLink(int id) -> decltype(links_.begin());
	auto FindLink(int id) const -> decltype(links_.begin());
};

} // namespace engine::graph
