#include "scene_registry.h"
#include <algorithm>

namespace examples {

SceneRegistry& SceneRegistry::Instance() {
	static SceneRegistry instance;
	return instance;
}

void SceneRegistry::RegisterScene(const std::string& name, const std::string& description, SceneFactory factory) {
	// Check if scene already registered
	auto it =
			std::find_if(scenes_.begin(), scenes_.end(), [&name](const SceneInfo& info) { return info.name == name; });

	if (it != scenes_.end()) {
		// Scene already registered - this is a programming error
		// In a production system, might want to log a warning
		return;
	}

	scenes_.push_back({name, description, factory});
}

const std::vector<SceneInfo>& SceneRegistry::GetAllScenes() const { return scenes_; }

const SceneInfo* SceneRegistry::FindScene(const std::string& name) const {
	auto it =
			std::find_if(scenes_.begin(), scenes_.end(), [&name](const SceneInfo& info) { return info.name == name; });

	return it != scenes_.end() ? &(*it) : nullptr;
}

std::unique_ptr<ExampleScene> SceneRegistry::CreateScene(const std::string& name) const {
	const SceneInfo* info = FindScene(name);
	if (!info) {
		return nullptr;
	}

	return info->factory();
}

} // namespace examples
