module;

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

module engine.graph;

import :node_type_registry;
import :types;

namespace engine::graph {

void NodeTypeRegistry::Register(const NodeTypeDefinition& def) { types_.push_back(def); }

const NodeTypeDefinition* NodeTypeRegistry::Get(const std::string& name) const {
	// Try exact match first
	auto it = std::find_if(types_.begin(), types_.end(),
						   [&name](const NodeTypeDefinition& def) { return def.name == name; });

	if (it != types_.end()) {
		return &(*it);
	}

	// Try full name match (category/name)
	it = std::find_if(types_.begin(), types_.end(), [&name](const NodeTypeDefinition& def) {
		return BuildFullName(def.category, def.name) == name;
	});

	if (it != types_.end()) {
		return &(*it);
	}

	return nullptr;
}

std::vector<std::string> NodeTypeRegistry::GetCategories() const {
	std::vector<std::string> categories;

	for (const auto& type : types_) {
		// Add category if not already in the list
		if (std::find(categories.begin(), categories.end(), type.category) == categories.end()) {
			categories.push_back(type.category);
		}
	}

	// Sort for consistent ordering
	std::sort(categories.begin(), categories.end());

	return categories;
}

std::vector<const NodeTypeDefinition*> NodeTypeRegistry::GetByCategory(const std::string& category) const {
	std::vector<const NodeTypeDefinition*> results;

	for (const auto& type : types_) {
		if (type.category == category) {
			results.push_back(&type);
		}
	}

	return results;
}

std::vector<const NodeTypeDefinition*> NodeTypeRegistry::GetAll() const {
	std::vector<const NodeTypeDefinition*> results;
	results.reserve(types_.size());

	for (const auto& type : types_) {
		results.push_back(&type);
	}

	return results;
}

void NodeTypeRegistry::Clear() { types_.clear(); }

NodeTypeRegistry& NodeTypeRegistry::GetGlobal() {
	static NodeTypeRegistry registry;
	return registry;
}

std::string NodeTypeRegistry::BuildFullName(const std::string& category, const std::string& name) {
	if (category.empty()) {
		return name;
	}
	return category + "/" + name;
}

} // namespace engine::graph
