module;

#include <string>

export module engine.scene.prefab;

import engine.platform;
import engine.ecs;
import engine.scene;
import engine.components;

export namespace engine::scene {

/// Prefab utilities using flecs native prefab system.
/// Prefabs are created via world.prefab() and instances via entity.is_a(prefab).
/// Flecs handles component inheritance, sharing, and overriding natively.
class PrefabUtility {
public:
	/// Create a prefab from an entity and convert the entity into an instance.
	/// 1. Creates a new prefab entity by copying the source entity's components
	/// 2. Removes parent relationships from the prefab (standalone template)
	/// 3. Converts the original entity into an is_a() instance of the prefab
	/// 4. Serializes the prefab to disk
	/// @param entity The entity to turn into a prefab (will become an instance)
	/// @param world The ECS world
	/// @param file_path Path to save the .prefab.json file
	/// @return The prefab template entity, or invalid on failure
	static ecs::Entity
	SaveAsPrefab(const ecs::Entity& entity, ecs::ECSWorld& world, const platform::fs::Path& file_path);

	/// Load a prefab from a file into the world as a flecs prefab entity.
	/// If already loaded from this path, returns the cached prefab.
	/// @param prefab_path Path to the .prefab.json file
	/// @param world The ECS world
	/// @return The prefab entity (tagged with flecs::Prefab), or invalid on failure
	static ecs::Entity LoadPrefab(const platform::fs::Path& prefab_path, ecs::ECSWorld& world);

	/// Instantiate a prefab into a scene using flecs is_a() inheritance.
	/// Automatically loads the prefab if not already loaded.
	/// @param prefab_path Path to the .prefab.json file
	/// @param scene The scene to instantiate into
	/// @param world The ECS world
	/// @param parent Optional parent entity
	/// @return The instantiated entity (inherits from the prefab via is_a)
	static ecs::Entity InstantiatePrefab(
			const platform::fs::Path& prefab_path, const Scene* scene, ecs::ECSWorld& world, const ecs::Entity& parent);

	/// Apply changes from a prefab instance back to the prefab source file.
	/// @param instance The entity that is a prefab instance (has is_a relationship)
	/// @param world The ECS world
	/// @return true if apply succeeded
	static bool ApplyToSource(const ecs::Entity& instance, ecs::ECSWorld& world);

	/// Save a prefab template entity to its source file.
	/// Uses the loaded_prefabs cache to resolve the file path.
	/// @param prefab_entity The prefab template entity
	/// @return true if save succeeded
	static bool SavePrefabTemplate(const ecs::Entity& prefab_entity);

	/// Get the file path for a loaded prefab entity (reverse cache lookup).
	/// @param prefab_entity The prefab template entity
	/// @return The file path, or empty string if not found
	static std::string GetPrefabPath(const ecs::Entity& prefab_entity);

private:
	/// Serialize a prefab entity to a JSON file (internal helper).
	static bool WritePrefabFile(const ecs::Entity& prefab_entity, const platform::fs::Path& file_path);
};

} // namespace engine::scene
