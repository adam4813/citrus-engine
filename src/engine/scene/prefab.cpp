module;

#include <flecs.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

module engine.scene.prefab;

import engine.ecs;
import engine.platform;
import engine.scene;
import engine.components;

using json = nlohmann::json;
using namespace engine::components;

namespace {
// Cache of loaded prefab entities keyed by file path
std::unordered_map<std::string, flecs::entity> loaded_prefabs;
} // namespace

namespace engine::scene {

bool PrefabUtility::WritePrefabFile(const ecs::Entity& prefab_entity, const platform::fs::Path& file_path) {
	try {
		json prefab_doc;
		prefab_doc["version"] = 1;
		prefab_doc["name"] = prefab_entity.name().c_str();

		json entities_array = json::array();

		// Serialize the prefab entity's component data
		json entity_entry;
		entity_entry["name"] = prefab_entity.name().c_str();

		// Serialize and strip relationships that are runtime-only
		json data = json::parse(prefab_entity.to_json().c_str());
		if (data.contains("pairs")) {
			auto& pairs = data["pairs"];
			pairs.erase("flecs.core.ChildOf");
			pairs.erase("flecs.core.IsA");
			if (pairs.empty()) {
				data.erase("pairs");
			}
		}
		entity_entry["data"] = data.dump();
		entities_array.push_back(entity_entry);

		prefab_doc["entities"] = entities_array;

		std::ofstream file(file_path);
		if (!file.is_open()) {
			std::cerr << "PrefabUtility: Failed to open file for writing: " << file_path << std::endl;
			return false;
		}

		file << prefab_doc.dump(2);
		file.close();
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "PrefabUtility: Error writing prefab file: " << e.what() << std::endl;
		return false;
	}
}

ecs::Entity
PrefabUtility::SaveAsPrefab(const ecs::Entity& entity, ecs::ECSWorld& world, const platform::fs::Path& file_path) {
	if (!entity.is_valid()) {
		std::cerr << "PrefabUtility: Invalid entity" << std::endl;
		return ecs::Entity();
	}

	try {
		const flecs::world& flecs_world = world.GetWorld();
		const std::string path_str = file_path.string();
		const std::string entity_name = entity.name().c_str();
		const std::string prefab_name = "prefab_" + entity_name;

		// 1. Create a new prefab entity and copy components from the source
		auto prefab_entity = flecs_world.prefab(prefab_name.c_str());

		// Copy component data via JSON serialization (roundtrip)
		json data = json::parse(entity.to_json().c_str());
		// Strip runtime relationships before applying to prefab
		if (data.contains("pairs")) {
			auto& pairs = data["pairs"];
			pairs.erase("flecs.core.ChildOf");
			pairs.erase("flecs.core.IsA");
			if (pairs.empty()) {
				data.erase("pairs");
			}
		}
		prefab_entity.from_json(data.dump().c_str());

		// Store the source file path on the prefab
		prefab_entity.set<PrefabInstance>({.prefab_path = path_str});

		// 2. Convert the original entity into an instance of the prefab
		entity.add(flecs::IsA, prefab_entity);

		// 3. Serialize the prefab to disk
		if (!WritePrefabFile(prefab_entity, file_path)) {
			return ecs::Entity();
		}

		// 4. Cache it
		loaded_prefabs[path_str] = prefab_entity;

		std::cout << "PrefabUtility: Created prefab '" << prefab_name << "' and converted '" << entity_name
				  << "' to instance" << std::endl;
		return prefab_entity;
	}
	catch (const std::exception& e) {
		std::cerr << "PrefabUtility: Error saving as prefab: " << e.what() << std::endl;
		return ecs::Entity();
	}
}

ecs::Entity PrefabUtility::LoadPrefab(const platform::fs::Path& prefab_path, ecs::ECSWorld& world) {
	const std::string path_str = prefab_path.string();

	// Return cached prefab if already loaded
	if (const auto it = loaded_prefabs.find(path_str); it != loaded_prefabs.end()) {
		if (it->second.is_valid() && it->second.is_alive()) {
			return it->second;
		}
		loaded_prefabs.erase(it);
	}

	try {
		std::ifstream file(prefab_path);
		if (!file.is_open()) {
			std::cerr << "PrefabUtility: Failed to open prefab file: " << prefab_path << std::endl;
			return ecs::Entity();
		}

		json prefab_doc;
		file >> prefab_doc;
		file.close();

		if (const int version = prefab_doc.value("version", 0); version != 1) {
			std::cerr << "PrefabUtility: Unsupported prefab format version: " << version << std::endl;
			return ecs::Entity();
		}

		if (!prefab_doc.contains("entities") || !prefab_doc["entities"].is_array() || prefab_doc["entities"].empty()) {
			std::cerr << "PrefabUtility: Invalid prefab format - missing entities" << std::endl;
			return ecs::Entity();
		}

		const flecs::world& flecs_world = world.GetWorld();

		const auto& root_entry = prefab_doc["entities"][0];
		const std::string prefab_name = "prefab_" + prefab_doc.value("name", "unnamed");

		// Create the flecs prefab entity
		auto prefab_entity = flecs_world.prefab(prefab_name.c_str());

		// Store the source path so instances can reference it
		prefab_entity.set<PrefabInstance>({.prefab_path = path_str});

		// Apply component data from JSON
		if (root_entry.contains("data")) {
			const std::string data_json = root_entry["data"];
			prefab_entity.from_json(data_json.c_str());
		}

		loaded_prefabs[path_str] = prefab_entity;

		std::cout << "PrefabUtility: Loaded prefab from " << prefab_path << std::endl;
		return prefab_entity;
	}
	catch (const std::exception& e) {
		std::cerr << "PrefabUtility: Error loading prefab: " << e.what() << std::endl;
		return ecs::Entity();
	}
}

ecs::Entity PrefabUtility::InstantiatePrefab(
		const platform::fs::Path& prefab_path, const Scene* scene, ecs::ECSWorld& world, const ecs::Entity& parent) {
	if (!scene) {
		std::cerr << "PrefabUtility: Invalid scene" << std::endl;
		return ecs::Entity();
	}

	const auto prefab_entity = LoadPrefab(prefab_path, world);
	if (!prefab_entity.is_valid()) {
		return ecs::Entity();
	}

	// Create an instance using flecs is_a() — inherits all prefab components
	const flecs::world& flecs_world = world.GetWorld();
	const auto instance = flecs_world.entity().is_a(prefab_entity);

	const auto scene_root = scene->GetSceneRoot();
	if (parent.is_valid()) {
		instance.child_of(parent);
	}
	else {
		instance.child_of(scene_root);
	}

	if (!instance.has<Transform>()) {
		instance.set<Transform>({});
		instance.set<WorldTransform>({});
	}

	std::cout << "PrefabUtility: Instantiated prefab from " << prefab_path << std::endl;
	return instance;
}

bool PrefabUtility::ApplyToSource(const ecs::Entity& instance, ecs::ECSWorld& world) {
	if (!instance.is_valid()) {
		std::cerr << "PrefabUtility: Invalid instance" << std::endl;
		return false;
	}

	auto prefab = instance.target(flecs::IsA);
	if (!prefab.is_valid() || !prefab.has(flecs::Prefab)) {
		std::cerr << "PrefabUtility: Entity is not a prefab instance" << std::endl;
		return false;
	}

	if (!prefab.has<PrefabInstance>()) {
		std::cerr << "PrefabUtility: Prefab has no source path" << std::endl;
		return false;
	}

	// Update the prefab entity's components from the instance, then re-save
	json data = json::parse(instance.to_json().c_str());
	if (data.contains("pairs")) {
		auto& pairs = data["pairs"];
		pairs.erase("flecs.core.ChildOf");
		pairs.erase("flecs.core.IsA");
		if (pairs.empty()) {
			data.erase("pairs");
		}
	}
	prefab.from_json(data.dump().c_str());

	const auto& [prefab_path] = prefab.get<PrefabInstance>();

	return WritePrefabFile(prefab, prefab_path);
}

} // namespace engine::scene
