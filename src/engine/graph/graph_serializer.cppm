module;

#include <cstdint>
#include <string>

export module engine.graph:graph_serializer;

import :node_graph;
import engine.platform;

export namespace engine::graph {

/// Graph serialization format version
constexpr int GRAPH_FORMAT_VERSION = 1;

/// JSON serializer for node graphs
class GraphSerializer {
public:
	/// Save a graph to a JSON file
	/// @param graph The graph to save
	/// @param path File path to save to
	/// @return true if save succeeded
	static bool Save(const NodeGraph& graph, const platform::fs::Path& path);

	/// Load a graph from a JSON file
	/// @param path File path to load from
	/// @param graph Graph to load into (will be cleared first)
	/// @return true if load succeeded
	static bool Load(const platform::fs::Path& path, NodeGraph& graph);

	/// Serialize graph to JSON string
	/// @param graph The graph to serialize
	/// @return JSON string representation
	static std::string Serialize(const NodeGraph& graph);

	/// Deserialize graph from JSON string
	/// @param json JSON string to deserialize
	/// @param graph Graph to load into (will be cleared first)
	/// @return true if deserialization succeeded
	static bool Deserialize(const std::string& json, NodeGraph& graph);
};

} // namespace engine::graph
