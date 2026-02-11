module;

#include <flecs.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

module engine.scene.serializer;

import engine.assets;
import engine.ecs;
import engine.platform;
import engine.scene;
import glm;

using json = nlohmann::json;

namespace engine::scene {

bool SceneSerializer::Save(const Scene& scene, ecs::ECSWorld& world, const platform::fs::Path& path) {
	try {
		// Create the scene JSON document
		json doc;
		doc["version"] = SCENE_FORMAT_VERSION;
		doc["name"] = scene.GetName();

		// Metadata
		json metadata;
		metadata["engine_version"] = "0.0.9";
		doc["metadata"] = metadata;

		// Scene settings
		json settings;
		auto bg_color = scene.GetBackgroundColor();
		settings["background_color"] = {bg_color.r, bg_color.g, bg_color.b, bg_color.a};
		auto ambient = scene.GetAmbientLight();
		settings["ambient_light"] = {ambient.r, ambient.g, ambient.b, ambient.a};
		auto gravity = scene.GetGravity();
		settings["gravity"] = {gravity.x, gravity.y};
		settings["author"] = scene.GetAuthor();
		settings["description"] = scene.GetDescription();
		doc["settings"] = settings;

		// Serialize assets (before entities - order matters for loading)
		json assets_array = json::array();
		for (const auto& asset_ptr : scene.GetAssets().GetAll()) {
			if (asset_ptr) {
				json asset_json;
				asset_ptr->ToJson(asset_json);
				assets_array.push_back(asset_json);
			}
		}
		doc["assets"] = assets_array;

		// Serialize entities using flecs
		const std::string flecs_json = SerializeEntities(scene, world);
		doc["flecs_data"] = flecs_json;

		// Save active camera entity path (if any) - after entities
		if (const std::string active_camera_path = GetActiveCameraPath(world); !active_camera_path.empty()) {
			doc["active_camera"] = active_camera_path;
		}

		// Write to file
		const std::string json_str = doc.dump(2); // Pretty print with 2-space indent
		if (!assets::AssetManager::SaveTextFile(path, json_str)) {
			std::cerr << "SceneSerializer: Failed to open file for writing: " << path << std::endl;
			return false;
		}

		std::cout << "SceneSerializer: Saved scene '" << scene.GetName() << "' to " << path << std::endl;
		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "SceneSerializer: Error saving scene: " << e.what() << std::endl;
		return false;
	}
}

SceneId SceneSerializer::Load(const platform::fs::Path& path, SceneManager& manager, ecs::ECSWorld& world) {
	try {
		// Read file
		auto text = assets::AssetManager::LoadTextFile(path);
		if (!text) {
			std::cerr << "SceneSerializer: Failed to open file for reading: " << path << std::endl;
			return INVALID_SCENE;
		}

		json doc = json::parse(*text);

		// Validate version
		if (const int version = doc.value("version", 0); version != SCENE_FORMAT_VERSION) {
			std::cerr << "SceneSerializer: Unsupported scene format version: " << version << std::endl;
			return INVALID_SCENE;
		}

		// Get scene name
		const std::string name = doc.value("name", "Untitled");

		// Create the scene
		const SceneId scene_id = manager.CreateScene(name);
		if (scene_id == INVALID_SCENE) {
			std::cerr << "SceneSerializer: Failed to create scene" << std::endl;
			return INVALID_SCENE;
		}

		auto& scene = manager.GetScene(scene_id);
		scene.SetFilePath(path);

		// Load scene settings (if present)
		if (doc.contains("settings")) {
			const auto& settings = doc["settings"];
			if (settings.contains("background_color") && settings["background_color"].is_array()) {
				auto bg = settings["background_color"];
				scene.SetBackgroundColor(glm::vec4(bg[0], bg[1], bg[2], bg[3]));
			}
			if (settings.contains("ambient_light") && settings["ambient_light"].is_array()) {
				auto amb = settings["ambient_light"];
				scene.SetAmbientLight(glm::vec4(amb[0], amb[1], amb[2], amb[3]));
			}
			if (settings.contains("gravity") && settings["gravity"].is_array()) {
				auto grav = settings["gravity"];
				scene.SetGravity(glm::vec2(grav[0], grav[1]));
			}
			if (settings.contains("author")) {
				scene.SetAuthor(settings["author"].get<std::string>());
			}
			if (settings.contains("description")) {
				scene.SetDescription(settings["description"].get<std::string>());
			}
		}

		// Add assets BEFORE entities (order matters - entities may reference assets)
		if (doc.contains("assets") && doc["assets"].is_array()) {
			for (const auto& asset_json : doc["assets"]) {
				if (auto asset = AssetInfo::FromJson(asset_json)) {
					scene.GetAssets().Add(std::move(asset));
				}
			}
		}

		if (!scene.LoadAssets()) {
			std::cerr << "SceneSerializer: Warning - some assets failed to load" << std::endl;
		}

		// Deserialize entities
		if (doc.contains("flecs_data")) {
			if (const std::string flecs_json = doc["flecs_data"].get<std::string>();
				!DeserializeEntities(flecs_json, world)) {
				std::cerr << "SceneSerializer: Warning - some entities may not have loaded correctly" << std::endl;
			}
		}

		// Restore active camera (must be done after entities are deserialized)
		if (doc.contains("active_camera")) {
			const std::string active_camera_path = doc["active_camera"].get<std::string>();
			SetActiveCameraFromPath(active_camera_path, world);
		}

		std::cout << "SceneSerializer: Loaded scene '" << name << "' from " << path << std::endl;
		scene.SetLoaded(true);
		return scene_id;
	}
	catch (const std::exception& e) {
		std::cerr << "SceneSerializer: Error loading scene: " << e.what() << std::endl;
		return INVALID_SCENE;
	}
}

std::string SceneSerializer::SerializeEntities(const Scene& scene, ecs::ECSWorld& world) {
	// Get the scene root and serialize all entities under it
	const ecs::Entity scene_root = scene.GetSceneRoot();
	if (!scene_root.is_valid()) {
		return "{}";
	}

	// Serialize the scene root entity and all its descendants
	// Note: flecs::entity::to_json() serializes a single entity
	// We'll collect all entities and serialize them as an array
	json entities_array = json::array();

	std::function<void(ecs::Entity)> serialize_entity = [&](const ecs::Entity entity) {
		if (!entity.is_valid()) {
			return;
		}

		// Serialize this entity
		const flecs::string entity_json = entity.to_json();
		if (entity_json.c_str() != nullptr && entity_json.length() > 0) {
			try {
				json entity_data;
				entity_data["path"] = entity.path().c_str();
				entity_data["data"] = entity_json.c_str();
				entities_array.push_back(entity_data);
			}
			catch (...) {
				// Skip entities that fail to serialize
			}
		}

		// Recursively serialize children
		entity.children([&](const ecs::Entity child) { serialize_entity(child); });
	};

	// Start from scene root
	serialize_entity(scene_root);

	return entities_array.dump();
}

bool SceneSerializer::DeserializeEntities(const std::string& flecs_json, ecs::ECSWorld& world) {
	if (flecs_json.empty() || flecs_json == "{}") {
		return true; // Empty scene is valid
	}

	try {
		json entities_array = json::parse(flecs_json);
		if (!entities_array.is_array()) {
			std::cerr << "SceneSerializer: Expected array of entities" << std::endl;
			return false;
		}

		const flecs::world& flecs_world = world.GetWorld();

		for (const auto& entity_entry : entities_array) {
			const std::string entity_path = entity_entry.value("path", "");
			if (entity_path.empty()) {
				continue;
			}

			// Create or find entity by path
			ecs::Entity entity = flecs_world.entity(entity_path.c_str());
			if (!entity.is_valid()) {
				entity = flecs_world.entity(entity_path.c_str());
			}

			// Deserialize entity data
			if (entity_entry.contains("data")) {
				const std::string data_json = entity_entry["data"];
				entity.from_json(data_json.c_str());
			}
		}

		return true;
	}
	catch (const std::exception& e) {
		std::cerr << "SceneSerializer: Error deserializing entities: " << e.what() << std::endl;
		return false;
	}
}

std::string SceneSerializer::GetActiveCameraPath(const ecs::ECSWorld& world) {
	if (const ecs::Entity active_camera = world.GetActiveCamera(); active_camera.is_valid()) {
		return active_camera.path().c_str();
	}
	return "";
}

void SceneSerializer::SetActiveCameraFromPath(const std::string& path, ecs::ECSWorld& world) {
	if (path.empty()) {
		return;
	}

	const flecs::world& flecs_world = world.GetWorld();
	if (const ecs::Entity entity = flecs_world.lookup(path.c_str()); entity.is_valid()) {
		world.SetActiveCamera(entity);
		std::cout << "SceneSerializer: Set active camera to '" << path << "'" << std::endl;
	}
	else {
		std::cerr << "SceneSerializer: Could not find camera entity at path: " << path << std::endl;
	}
}

std::string SceneSerializer::SnapshotEntities(const Scene& scene, ecs::ECSWorld& world) {
	return SerializeEntities(scene, world);
}

bool SceneSerializer::RestoreEntities(const std::string& snapshot, const Scene& scene, ecs::ECSWorld& world) {
	// The scene root itself should still exist (only children were destroyed).
	// Deserialize the snapshot to recreate all child entities under the scene root.
	return DeserializeEntities(snapshot, world);
}

} // namespace engine::scene
