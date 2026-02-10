module;

#include <algorithm>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

module engine.graph;

import :graph_serializer;
import :node_graph;
import :types;
import engine.assets;
import engine.platform;
import glm;

using json = nlohmann::json;

namespace engine::graph {

// Helper to convert PinValue to JSON
static void to_json(json& j, const PinValue& value) {
	std::visit(
		[&j](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, int>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, float>) {
				j = arg;
			}
			else if constexpr (std::is_same_v<T, glm::vec2>) {
				j = {arg.x, arg.y};
			}
			else if constexpr (std::is_same_v<T, glm::vec3>) {
				j = {arg.x, arg.y, arg.z};
			}
			else if constexpr (std::is_same_v<T, glm::vec4>) {
				j = {arg.x, arg.y, arg.z, arg.w};
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				j = arg;
			}
		},
		value);
}

// Helper to convert JSON to PinValue based on type
static PinValue from_json_value(const json& j, PinType type) {
	switch (type) {
		case PinType::Bool:
			return j.get<bool>();
		case PinType::Int:
		case PinType::Texture:
			return j.get<int>();
		case PinType::Float:
			return j.get<float>();
		case PinType::Vec2:
			if (j.is_array() && j.size() >= 2) {
				return glm::vec2(j[0].get<float>(), j[1].get<float>());
			}
			return glm::vec2(0.0f);
		case PinType::Vec3:
			if (j.is_array() && j.size() >= 3) {
				return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
			}
			return glm::vec3(0.0f);
		case PinType::Vec4:
		case PinType::Color:
			if (j.is_array() && j.size() >= 4) {
				return glm::vec4(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(),
								 j[3].get<float>());
			}
			return glm::vec4(0.0f);
		case PinType::String:
			return j.get<std::string>();
		default:
			return 0.0f;
	}
}

std::string GraphSerializer::Serialize(const NodeGraph& graph) {
	json j;

	// Version
	j["version"] = GRAPH_FORMAT_VERSION;

	// Serialize nodes
	json nodes_json = json::array();
	for (const auto& node : graph.GetNodes()) {
		json node_json;
		node_json["id"] = node.id;
		node_json["type"] = node.type_name;
		node_json["position"] = {node.position.x, node.position.y};

		// Inputs
		json inputs_json = json::array();
		for (const auto& pin : node.inputs) {
			json pin_json;
			pin_json["id"] = pin.id;
			pin_json["name"] = pin.name;
			pin_json["type"] = static_cast<int>(pin.type);
			to_json(pin_json["default_value"], pin.default_value);
			inputs_json.push_back(pin_json);
		}
		node_json["inputs"] = inputs_json;

		// Outputs
		json outputs_json = json::array();
		for (const auto& pin : node.outputs) {
			json pin_json;
			pin_json["id"] = pin.id;
			pin_json["name"] = pin.name;
			pin_json["type"] = static_cast<int>(pin.type);
			to_json(pin_json["default_value"], pin.default_value);
			outputs_json.push_back(pin_json);
		}
		node_json["outputs"] = outputs_json;

		// Properties
		json props_json;
		for (const auto& [key, value] : node.properties) {
			to_json(props_json[key], value);
		}
		node_json["properties"] = props_json;

		nodes_json.push_back(node_json);
	}
	j["nodes"] = nodes_json;

	// Serialize links
	json links_json = json::array();
	for (const auto& link : graph.GetLinks()) {
		json link_json;
		link_json["id"] = link.id;
		link_json["from_node"] = link.from_node_id;
		link_json["from_pin"] = link.from_pin_index;
		link_json["to_node"] = link.to_node_id;
		link_json["to_pin"] = link.to_pin_index;
		links_json.push_back(link_json);
	}
	j["links"] = links_json;

	return j.dump(2); // Pretty print with 2-space indent
}

bool GraphSerializer::Deserialize(const std::string& json_str, NodeGraph& graph) {
	try {
		json j = json::parse(json_str);

		// Check version
		if (!j.contains("version") || j["version"].get<int>() != GRAPH_FORMAT_VERSION) {
			return false;
		}

		// Clear the graph
		graph.Clear();

		// Track the max ID we've seen
		int max_id = 0;

		// Deserialize nodes
		if (j.contains("nodes") && j["nodes"].is_array()) {
			for (const auto& node_json : j["nodes"]) {
				Node node;
				node.id = node_json["id"].get<int>();
				max_id = std::max(max_id, node.id);
				
				node.type_name = node_json["type"].get<std::string>();

				if (node_json.contains("position") && node_json["position"].is_array()) {
					const auto& pos = node_json["position"];
					if (pos.size() >= 2) {
						node.position = glm::vec2(pos[0].get<float>(), pos[1].get<float>());
					}
				}

				// Deserialize inputs
				if (node_json.contains("inputs") && node_json["inputs"].is_array()) {
					for (const auto& pin_json : node_json["inputs"]) {
						Pin pin;
						pin.id = pin_json["id"].get<int>();
						max_id = std::max(max_id, pin.id);
						
						pin.name = pin_json["name"].get<std::string>();
						pin.type = static_cast<PinType>(pin_json["type"].get<int>());
						pin.direction = PinDirection::Input;

						if (pin_json.contains("default_value")) {
							pin.default_value = from_json_value(pin_json["default_value"], pin.type);
						}

						node.inputs.push_back(pin);
					}
				}

				// Deserialize outputs
				if (node_json.contains("outputs") && node_json["outputs"].is_array()) {
					for (const auto& pin_json : node_json["outputs"]) {
						Pin pin;
						pin.id = pin_json["id"].get<int>();
						max_id = std::max(max_id, pin.id);
						
						pin.name = pin_json["name"].get<std::string>();
						pin.type = static_cast<PinType>(pin_json["type"].get<int>());
						pin.direction = PinDirection::Output;

						if (pin_json.contains("default_value")) {
							pin.default_value = from_json_value(pin_json["default_value"], pin.type);
						}

						node.outputs.push_back(pin);
					}
				}

				// Deserialize properties
				if (node_json.contains("properties") && node_json["properties"].is_object()) {
					// Properties require type information to deserialize properly
					// For now, skip them - node types will need to handle their own properties
				}

				// Add the node directly (friend access)
				graph.nodes_.push_back(node);
			}
		}

		// Deserialize links
		if (j.contains("links") && j["links"].is_array()) {
			for (const auto& link_json : j["links"]) {
				Link link;
				link.id = link_json["id"].get<int>();
				max_id = std::max(max_id, link.id);
				
				link.from_node_id = link_json["from_node"].get<int>();
				link.from_pin_index = link_json["from_pin"].get<int>();
				link.to_node_id = link_json["to_node"].get<int>();
				link.to_pin_index = link_json["to_pin"].get<int>();

				// Add the link directly (friend access)
				graph.links_.push_back(link);
			}
		}

		// Set the next ID to be one higher than the max we've seen
		graph.SetNextId(max_id + 1);

		return true;
	}
	catch (...) {
		return false;
	}
}

bool GraphSerializer::Save(const NodeGraph& graph, const platform::fs::Path& path) {
	std::string json_str = Serialize(graph);
	return assets::AssetManager::SaveTextFile(path, json_str);
}

bool GraphSerializer::Load(const platform::fs::Path& path, NodeGraph& graph) {
	auto text = assets::AssetManager::LoadTextFile(path);
	if (!text) {
		return false;
	}
	return Deserialize(*text, graph);
}

} // namespace engine::graph
