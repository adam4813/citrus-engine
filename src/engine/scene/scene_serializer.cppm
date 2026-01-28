module;

#include <cstdint>
#include <string>

export module engine.scene.serializer;

import engine.ecs;
import engine.platform;
import engine.scene;

export namespace engine::scene {

using SceneId = uint32_t;

/// Scene serialization format version
inline constexpr int SCENE_FORMAT_VERSION = 1;

/// Scene serializer - handles saving and loading scenes to/from JSON files
///
/// File format:
/// {
///     "version": 1,
///     "name": "Scene Name",
///     "metadata": { ... },
///     "active_camera": "path/to/camera/entity",  // optional
///     "flecs_data": "<flecs world JSON>",
///     "assets": ["asset1.png", "asset2.obj"]
/// }
class SceneSerializer {
public:
	/// Save a scene to a file
	/// @param scene The scene to save
	/// @param world The ECS world containing entity data
	/// @param path The file path to save to
	/// @return true if save succeeded
	static bool Save(const Scene& scene, ecs::ECSWorld& world, const platform::fs::Path& path);

	/// Load a scene from a file
	/// @param path The file path to load from
	/// @param manager The scene manager to create the scene in
	/// @param world The ECS world to populate with entities
	/// @return The loaded scene ID, or INVALID_SCENE on failure
	static SceneId Load(const platform::fs::Path& path, SceneManager& manager, ecs::ECSWorld& world);

private:
	/// Serialize scene entities to flecs JSON format
	static std::string SerializeEntities(const Scene& scene, ecs::ECSWorld& world);

	/// Deserialize entities from flecs JSON format
	static bool DeserializeEntities(const std::string& flecs_json, ecs::ECSWorld& world);

	/// Get the path of the active camera entity (empty string if none)
	static std::string GetActiveCameraPath(const ecs::ECSWorld& world);

	/// Set the active camera from a saved entity path
	static void SetActiveCameraFromPath(const std::string& path, ecs::ECSWorld& world);
};

} // namespace engine::scene
