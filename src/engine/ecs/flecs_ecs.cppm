module;

#include <cstdint>
#include <flecs.h>
#include <functional>
#include <string>
#include <vector>

export module engine.ecs;

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
		const glm::vec3 center = (bounding_min + bounding_max) * 0.5f;
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
	if (!transform.dirty)
		return;
	// Create transformation matrix: T * R * S
	glm::mat4 translation = glm::translate(glm::mat4(1.0F), transform.position);
	glm::mat4 rotation_x = glm::rotate(glm::mat4(1.0F), transform.rotation.x, glm::vec3(1, 0, 0));
	glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0F), transform.rotation.y, glm::vec3(0, 1, 0));
	glm::mat4 rotation_z = glm::rotate(glm::mat4(1.0F), transform.rotation.z, glm::vec3(0, 0, 1));
	glm::mat4 scale_mat = glm::scale(glm::mat4(1.0F), transform.scale);
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
	void SetParent(const flecs::entity child, const flecs::entity parent);

	// Remove parent relationship
	void RemoveParent(const flecs::entity child);

	// Get parent entity (returns invalid entity if no parent)
	flecs::entity GetParent(const flecs::entity entity);

	// Get all children of an entity
	std::vector<flecs::entity> GetChildren(const flecs::entity parent);

	// Get all descendants (recursive)
	std::vector<flecs::entity> GetDescendants(const flecs::entity root);

	// Find entity by name in hierarchy
	flecs::entity FindEntityByName(const char* name, const flecs::entity root = {});

	// Check if entity is descendant of another
	bool IsDescendantOf(const flecs::entity entity, const flecs::entity ancestor);

	// === CAMERA MANAGEMENT ===

	void SetActiveCamera(const flecs::entity camera);

	[[nodiscard]] flecs::entity GetActiveCamera() const;

	// === SPATIAL QUERIES ===

	// Find entities at a point
	[[nodiscard]] std::vector<flecs::entity>
	QueryPoint(const glm::vec3& point, const uint32_t layer_mask = 0xFFFFFFFF) const;

	// Find entities in sphere
	[[nodiscard]] std::vector<flecs::entity>
	QuerySphere(const glm::vec3& center, const float radius, const uint32_t layer_mask = 0xFFFFFFFF) const;

	// Progress the world (run systems)
	void Progress(const float delta_time) const;

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
};

// === COMPONENT REGISTRY ===

/**
     * @brief Information about a registered component
     */
struct ComponentInfo {
	std::string name;
	std::string category; // User-defined category string
	flecs::entity_t id = 0;
};

// Forward declaration
class ComponentRegistry;

/**
     * @brief Builder for registering components with fluent API
     *
     * Usage:
     *   ComponentRegistry::Instance().Register<Transform>("Transform", world)
     *       .Category("Core")
     *       .Build();
     */
template <typename T> class ComponentRegistration {
public:
	ComponentRegistration(ComponentRegistry& registry, const std::string& name, const flecs::world& world);

	ComponentRegistration& Category(const std::string& cat) {
		info_.category = cat;
		return *this;
	}

	void Build();

private:
	ComponentRegistry& registry_;
	ComponentInfo info_;
};

/**
     * @brief Singleton registry of all available components
     *
     * Components register themselves using the builder pattern in ECSWorld constructor.
     * Registry only stores metadata - use flecs APIs directly for adding/checking components.
     */
class ComponentRegistry {
public:
	/**
         * @brief Get the singleton instance
         */
	static ComponentRegistry& Instance();

	/**
         * @brief Start registering a component with builder pattern
         */
	template <typename T> ComponentRegistration<T> Register(const std::string& name, flecs::world& world) {
		return ComponentRegistration<T>(*this, name, world);
	}

	/**
         * @brief Get list of all registered components
         */
	[[nodiscard]] const std::vector<ComponentInfo>& GetComponents() const { return components_; }

	/**
         * @brief Get unique category names (sorted)
         */
	[[nodiscard]] std::vector<std::string> GetCategories() const;

	/**
         * @brief Get components filtered by category
         */
	[[nodiscard]] std::vector<const ComponentInfo*> GetComponentsByCategory(const std::string& category) const;

	/**
         * @brief Find component by name
         * @return nullptr if not found
         */
	[[nodiscard]] const ComponentInfo* FindComponent(const std::string& name) const;

	// Called by ComponentRegistration::Build()
	void AddComponent(ComponentInfo info) { components_.push_back(std::move(info)); }

private:
	ComponentRegistry() = default;
	~ComponentRegistry() = default;
	ComponentRegistry(const ComponentRegistry&) = delete;
	ComponentRegistry& operator=(const ComponentRegistry&) = delete;

	std::vector<ComponentInfo> components_;
};

// Template implementation
template <typename T>
ComponentRegistration<T>::ComponentRegistration(
		ComponentRegistry& registry, const std::string& name, const flecs::world& world) : registry_(registry) {
	info_.name = name;
	info_.category = "Other"; // Default category
	info_.id = world.component<T>().id();
}

template <typename T> void ComponentRegistration<T>::Build() { registry_.AddComponent(std::move(info_)); }
} // namespace engine::ecs
