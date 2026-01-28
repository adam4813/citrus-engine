module;

#include <algorithm>
#include <set>
#include <string>
#include <vector>

module engine.ecs.component_registry;

namespace engine::ecs {

ComponentRegistry& ComponentRegistry::Instance() {
	static ComponentRegistry instance;
	return instance;
}

std::vector<std::string> ComponentRegistry::GetCategories() const {
	std::set<std::string> categories;
	for (const auto& comp : components_) {
		categories.insert(comp.category);
	}
	return {categories.begin(), categories.end()};
}

std::vector<const ComponentInfo*> ComponentRegistry::GetComponentsByCategory(const std::string& category) const {
	std::vector<const ComponentInfo*> result;
	for (const auto& comp : components_) {
		if (comp.category == category) {
			result.push_back(&comp);
		}
	}
	return result;
}

const ComponentInfo* ComponentRegistry::FindComponent(const std::string& name) const {
	for (const auto& comp : components_) {
		if (comp.name == name) {
			return &comp;
		}
	}
	return nullptr;
}

} // namespace engine::ecs
