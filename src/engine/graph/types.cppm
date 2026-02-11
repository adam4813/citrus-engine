module;

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

export module engine.graph:types;

import glm;

export namespace engine::graph {

/// Pin type enumeration - defines what kind of data flows through a pin
enum class PinType {
	Flow,     // Execution flow (for visual scripting)
	Bool,     // Boolean value
	Int,      // Integer value
	Float,    // Floating point value
	Vec2,     // 2D vector
	Vec3,     // 3D vector
	Vec4,     // 4D vector
	Color,    // RGBA color (stored as Vec4)
	Texture,  // Texture reference (stored as Int ID)
	String,   // String value
	Any       // Can accept any type (type-erased)
};

/// Pin direction - input or output
enum class PinDirection { Input, Output };

/// Type-safe value storage for pin default values
using PinValue = std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, std::string>;

/// Pin definition - represents a connection point on a node
struct Pin {
	int id = 0;
	std::string name;
	PinType type = PinType::Float;
	PinDirection direction = PinDirection::Input;
	PinValue default_value = 0.0f;

	Pin() = default;

	Pin(int pin_id, std::string pin_name, PinType pin_type, PinDirection pin_direction,
		PinValue pin_default = 0.0f)
		: id(pin_id), name(std::move(pin_name)), type(pin_type), direction(pin_direction),
		  default_value(std::move(pin_default)) {}
};

/// Node definition - represents a graph node with inputs, outputs, and state
struct Node {
	int id = 0;
	std::string type_name; // Registered type (e.g., "Math/Add", "Texture/Noise")
	glm::vec2 position{0.0f, 0.0f}; // Canvas position for editor
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	std::map<std::string, PinValue> properties; // Node-specific settings

	Node() = default;

	Node(int node_id, std::string node_type, glm::vec2 node_pos = {0.0f, 0.0f})
		: id(node_id), type_name(std::move(node_type)), position(node_pos) {}
};

/// Link definition - represents a connection between two pins
struct Link {
	int id = 0;
	int from_node_id = 0;
	int from_pin_index = 0;
	int to_node_id = 0;
	int to_pin_index = 0;

	Link() = default;

	Link(int link_id, int from_node, int from_pin, int to_node, int to_pin)
		: id(link_id), from_node_id(from_node), from_pin_index(from_pin), to_node_id(to_node),
		  to_pin_index(to_pin) {}
};

/// Check if two pin types are compatible for connection
/// Handles automatic type promotion and special cases
bool AreTypesCompatible(PinType from, PinType to);

} // namespace engine::graph
