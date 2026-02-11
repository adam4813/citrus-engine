module;

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

export module engine.graph:node_type_registry;

import :types;

export namespace engine::graph {

// Forward declaration
class INodeEvaluator;

/// Node type definition - describes a node type that can be instantiated
struct NodeTypeDefinition {
	std::string name; // e.g., "Add"
	std::string category; // e.g., "Math"
	std::string description; // Human-readable description
	std::vector<Pin> default_inputs; // Default input pins
	std::vector<Pin> default_outputs; // Default output pins

	/// Factory function to create an evaluator for this node type
	std::function<std::unique_ptr<INodeEvaluator>()> create_evaluator;

	NodeTypeDefinition() = default;

	NodeTypeDefinition(std::string node_name, std::string node_category, std::string node_description = "")
		: name(std::move(node_name)), category(std::move(node_category)),
		  description(std::move(node_description)) {}
};

/// Registry for node types
/// Manages available node types and provides lookup functionality
class NodeTypeRegistry {
public:
	/// Register a new node type
	/// @param def The node type definition
	void Register(const NodeTypeDefinition& def);

	/// Get a node type definition by name
	/// @param name The full type name (category/name or just name)
	/// @return Pointer to the definition, or nullptr if not found
	const NodeTypeDefinition* Get(const std::string& name) const;

	/// Get all registered categories
	/// @return List of unique category names
	std::vector<std::string> GetCategories() const;

	/// Get all node types in a category
	/// @param category The category name
	/// @return List of node type definitions in that category
	std::vector<const NodeTypeDefinition*> GetByCategory(const std::string& category) const;

	/// Get all registered node types
	/// @return List of all node type definitions
	std::vector<const NodeTypeDefinition*> GetAll() const;

	/// Clear all registered types (useful for testing)
	void Clear();

	/// Get the global singleton registry instance
	static NodeTypeRegistry& GetGlobal();

private:
	std::vector<NodeTypeDefinition> types_;

	/// Build full type name (category/name)
	static std::string BuildFullName(const std::string& category, const std::string& name);
};

} // namespace engine::graph
