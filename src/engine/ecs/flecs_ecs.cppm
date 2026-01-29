module;

#include <cstdint>
#include <flecs.h>
#include <string>
#include <vector>

export module engine.ecs;

export import engine.ecs.component_registry;
import engine.components;
import engine.rendering;
import glm;

export namespace engine::ecs {
// Re-export flecs types for convenience
using World = flecs::world;
using Entity = flecs::entity;
using Query = flecs::query_base;

// === SCENE HIERARCHY COMPONENTS ===

// Scene metadata component
struct SceneEntity {
	std::string name;
	bool visible = true;
	bool static_entity = false; // Static entities don't move (optimization hint)
	uint32_t scene_layer = 0; // For organizing entities within scenes
};

// Spatial component for entities that need spatial queries (replaces BoundingBoxComponent)
struct Spatial {
	glm::vec3 bounding_min{-0.5F, -0.5F, -0.5F};
	glm::vec3 bounding_max{0.5F, 0.5F, 0.5F};
	bool bounds_dirty = true;
	uint32_t spatial_layer = 0; // For filtering spatial queries

	void UpdateBounds(const glm::vec3& center, const glm::vec3& extents) {
		bounding_min = center - extents;
		bounding_max = center + extents;
		bounds_dirty = true;
	}

	[[nodiscard]] bool ContainsPoint(const glm::vec3& point) const {
		return point.x >= bounding_min.x && point.x <= bounding_max.x && point.y >= bounding_min.y
			   && point.y <= bounding_max.y && point.z >= bounding_min.z && point.z <= bounding_max.z;
	}

	[[nodiscard]] float DistanceToPoint(const glm::vec3& point) const {
		const glm::vec3 center = (bounding_min + bounding_max) * 0.5F;
		return glm::distance(center, point);
	}

	[[nodiscard]] bool IntersectsWith(const Spatial& other) const {
		return bounding_min.x <= other.bounding_max.x && bounding_max.x >= other.bounding_min.x
			   && bounding_min.y <= other.bounding_max.y && bounding_max.y >= other.bounding_min.y
			   && bounding_min.z <= other.bounding_max.z && bounding_max.z >= other.bounding_min.z;
	}
};

// === FLECS RELATIONSHIP TAGS ===

// Built-in flecs relationships we'll use
// ChildOf - for parent-child hierarchy (built into flecs)
// DependsOn - for dependency relationships (built into flecs)

// Custom relationship tags
struct SceneRoot {}; // Tag for scene root entities

// === COMPONENT HELPER FUNCTIONS ===
namespace component_helpers {
// Update transform matrix
inline void UpdateTransformMatrix(components::Transform& transform) {
	if (!transform.dirty) {
		return;
	}
	// Create transformation matrix: T * R * S
	const glm::mat4 translation = glm::translate(glm::mat4(1.0F), transform.position);
	const glm::mat4 rotation_x = glm::rotate(glm::mat4(1.0F), transform.rotation.x, glm::vec3(1, 0, 0));
	const glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0F), transform.rotation.y, glm::vec3(0, 1, 0));
	const glm::mat4 rotation_z = glm::rotate(glm::mat4(1.0F), transform.rotation.z, glm::vec3(0, 0, 1));
	const glm::mat4 scale_mat = glm::scale(glm::mat4(1.0F), transform.scale);
	transform.world_matrix = translation * rotation_z * rotation_y * rotation_x * scale_mat;
	transform.dirty = false;
}

// Update camera view matrix
inline void UpdateCameraViewMatrix(components::Camera& camera, const glm::vec3& position) {
	camera.view_matrix = glm::lookAt(position, camera.target, camera.up);
}

// Update camera projection matrix
inline void UpdateCameraProjectionMatrix(components::Camera& camera) {
	camera.projection_matrix =
			glm::perspective(glm::radians(camera.fov), camera.aspect_ratio, camera.near_plane, camera.far_plane);
}

// Get view-projection matrix
inline glm::mat4 GetViewProjectionMatrix(const components::Camera& camera) {
	return camera.projection_matrix * camera.view_matrix;
}
} // namespace component_helpers

// Flecs world wrapper with engine-specific functionality
class ECSWorld {
private:
	flecs::world world_;
	flecs::entity active_camera_;

public:
	ECSWorld();

	// Get the underlying flecs world
	flecs::world& GetWorld();

	[[nodiscard]] const flecs::world& GetWorld() const;

	// === ENTITY CREATION ===

	// Create an entity
	[[nodiscard]] flecs::entity CreateEntity() const;

	// Create an entity with a name
	flecs::entity CreateEntity(const char* name) const;

	// Create a scene root entity
	flecs::entity CreateSceneRoot(const char* name) const;

	// === HIERARCHY MANAGEMENT ===

	// Set parent-child relationship using flecs built-in ChildOf
	void SetParent(flecs::entity child, flecs::entity parent);

	// Remove parent relationship
	void RemoveParent(flecs::entity child);

	// Get parent entity (returns invalid entity if no parent)
	flecs::entity GetParent(flecs::entity entity);

	// Get all children of an entity
	std::vector<flecs::entity> GetChildren(flecs::entity parent);

	// Get all descendants (recursive)
	std::vector<flecs::entity> GetDescendants(flecs::entity root);

	// Find entity by name in hierarchy
	flecs::entity FindEntityByName(const char* name, flecs::entity root = {});

	// Check if entity is descendant of another
	bool IsDescendantOf(flecs::entity entity, flecs::entity ancestor);

	// === CAMERA MANAGEMENT ===

	void SetActiveCamera(flecs::entity camera);

	[[nodiscard]] flecs::entity GetActiveCamera() const;

	// === SPATIAL QUERIES ===

	// Find entities at a point
	[[nodiscard]] std::vector<flecs::entity> QueryPoint(const glm::vec3& point, uint32_t layer_mask = 0xFFFFFFFF) const;

	// Find entities in sphere
	[[nodiscard]] std::vector<flecs::entity>
	QuerySphere(const glm::vec3& center, float radius, uint32_t layer_mask = 0xFFFFFFFF) const;

	// Progress the world (run systems)
	void Progress(float delta_time) const;

	// Query entities with specific components
	template <typename... Components> auto Query() const { return world_.query<Components...>(); }

	// Submit render commands for all renderable entities
	void SubmitRenderCommands(const rendering::Renderer& renderer);

private:
	void SetupMovementSystem() const;

	void SetupRotationSystem() const;

	void SetupCameraSystem() const;

	void SetupHierarchySystem() const;

	void SetupSpatialSystem() const;

	void SetupTransformSystem() const;

	void RegisterTransformSystem() const;

	void SetupShaderRefIntegration();

	void SetupMeshRefIntegration();
};
} // namespace engine::ecs
