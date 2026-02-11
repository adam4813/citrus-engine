#pragma once

#include <flecs.h>
#include <nlohmann/json.hpp>
#include <string>

import engine;

namespace editor {

/// Generate a unique entity name. If the name already exists, increments a
/// trailing _N suffix (e.g. "Foo" → "Foo_1" → "Foo_2", "Bar_3" → "Bar_4").
inline std::string MakeUniqueEntityName(const std::string& base_name, engine::scene::Scene* scene) {
	const auto scene_root = scene->GetSceneRoot();
	if (scene_root.lookup(base_name.c_str()) == flecs::entity::null()) {
		return base_name;
	}

	// Split off existing _N suffix to get the stem
	std::string stem = base_name;
	int count = 1;
	if (const auto pos = base_name.rfind('_'); pos != std::string::npos) {
		const std::string suffix = base_name.substr(pos + 1);
		bool all_digits = !suffix.empty();
		for (const char c : suffix) {
			if (c < '0' || c > '9') {
				all_digits = false;
				break;
			}
		}
		if (all_digits) {
			stem = base_name.substr(0, pos);
			count = std::stoi(suffix) + 1;
		}
	}

	std::string candidate;
	do {
		candidate = stem + "_" + std::to_string(count++);
	} while (scene_root.lookup(candidate.c_str()) != flecs::entity::null());
	return candidate;
}

/// Strip runtime relationships (ChildOf, IsA) from a flecs JSON string so
/// from_json() won't re-parent or re-link the entity to its original hierarchy.
inline std::string StripEntityRelationships(const std::string& entity_json) {
	auto data = nlohmann::json::parse(entity_json);
	if (data.contains("pairs")) {
		auto& pairs = data["pairs"];
		pairs.erase("flecs.core.ChildOf");
		pairs.erase("flecs.core.IsA");
		if (pairs.empty()) {
			data.erase("pairs");
		}
	}
	return data.dump();
}

} // namespace editor
