module;

// Standard library includes for scene management
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

export module engine.scene;

export import engine.scene.assets;

import glm;
import engine.ecs;
import engine.platform;
import engine.rendering;

export namespace engine::scene {
// =============================================================================
// Core Scene Types
// =============================================================================

using Vec3 = glm::vec3;
using SceneId = uint32_t;

inline constexpr SceneId INVALID_SCENE = 0;

// =============================================================================
// Flecs-Integrated Scene Management
// =============================================================================

class Scene {
public:
	Scene(const std::string& name, ecs::ECSWorld& ecs_world);

	~Scene();

	// Scene properties
	SceneId GetId() const;

	const std::string& GetName() const;

	void SetName(const std::string& name) const;

	// === ENTITY MANAGEMENT USING FLECS ===

	// Create entity and add to scene
	ecs::Entity CreateEntity(const std::string& name = "") const;

	// Create entity with parent (hierarchy)
	ecs::Entity CreateEntity(const std::string& name, const ecs::Entity& parent) const;

	// Destroy entity from scene
	void DestroyEntity(const ecs::Entity& entity);

	// Get all entities in this scene
	std::vector<ecs::Entity> GetAllEntities() const;

	// Find entity by name within this scene
	ecs::Entity FindEntityByName(const std::string& name) const;

	// === HIERARCHY MANAGEMENT ===

	// Get the scene root entity
	ecs::Entity GetSceneRoot() const;

	// Create hierarchical structures
	void SetParent(const ecs::Entity& child, const ecs::Entity& parent) const;

	void RemoveParent(const ecs::Entity& child) const;

	ecs::Entity GetParent(const ecs::Entity& entity) const;

	std::vector<ecs::Entity> GetChildren(const ecs::Entity& parent) const;

	std::vector<ecs::Entity> GetDescendants(const ecs::Entity& root) const;

	// === SPATIAL QUERIES ===

	// Find entities at a point within this scene
	std::vector<ecs::Entity> QueryPoint(const Vec3& point, uint32_t layer_mask = 0xFFFFFFFF) const;

	// Find entities in sphere within this scene
	std::vector<ecs::Entity> QuerySphere(const Vec3& center, float radius, uint32_t layer_mask = 0xFFFFFFFF) const;

	// === SCENE STATE ===

	// Scene bounds
	void SetWorldBounds(const Vec3& min, const Vec3& max) const;

	std::pair<Vec3, Vec3> GetWorldBounds() const;

	// Scene state
	void SetActive(bool active) const;

	bool IsActive() const;

	void SetLoaded(bool loaded) const;

	bool IsLoaded() const;

	// Asset management
	void SetFilePath(const platform::fs::Path& path) const;

	platform::fs::Path GetFilePath() const;

	bool LoadAssets() const;

	void UnloadAssets() const;

	SceneAssets& GetAssets();

	const SceneAssets& GetAssets() const;

	// Update scene (called each frame)
	void Update(float delta_time);

	// === LIFECYCLE CALLBACKS ===

	// Lifecycle callback types
	using InitializeCallback = std::function<void()>;
	using ShutdownCallback = std::function<void()>;
	using UpdateCallback = std::function<void(float)>;
	using RenderCallback = std::function<void()>;

	// Set lifecycle callbacks
	void SetInitializeCallback(InitializeCallback callback);
	void SetShutdownCallback(ShutdownCallback callback);
	void SetUpdateCallback(UpdateCallback callback);
	void SetRenderCallback(RenderCallback callback);

	// Trigger lifecycle events (called by SceneManager)
	void Initialize();
	void Shutdown();
	void Render();

private:
	struct Impl;
	std::unique_ptr<Impl> pimpl_;
};

// =============================================================================
// Scene Manager
// =============================================================================

class SceneManager {
public:
	explicit SceneManager(ecs::ECSWorld& ecs_world);

	~SceneManager();

	// Scene creation and management
	SceneId CreateScene(const std::string& name) const;

	void DestroyScene(SceneId scene_id) const;

	// Scene access
	Scene& GetScene(SceneId scene_id);

	const Scene& GetScene(SceneId scene_id) const;

	Scene* TryGetScene(SceneId scene_id);

	const Scene* TryGetScene(SceneId scene_id) const;

	SceneId FindSceneByName(const std::string& name) const;

	std::vector<SceneId> GetAllScenes() const;

	std::vector<SceneId> GetActiveScenes() const;

	// Scene activation
	void SetActiveScene(SceneId scene_id) const;

	SceneId GetActiveScene() const;

	void ActivateScene(SceneId scene_id) const;

	void DeactivateScene(SceneId scene_id);

	// Multi-scene support
	void ActivateAdditionalScene(SceneId scene_id);

	void DeactivateAdditionalScene(SceneId scene_id);

	// Scene transitions
	void TransitionToScene(SceneId new_scene, float transition_time = 0.0f) const;

	bool IsTransitioning() const;

	float GetTransitionProgress() const;

	// Used for file-based scenes
	bool SaveScene(SceneId scene_id, const platform::fs::Path& file_path);
	SceneId LoadSceneFromFile(const platform::fs::Path& file_path);

	// Used for in-memory scenes
	bool LoadScene(SceneId scene_id) const;
	void UnloadScene(SceneId scene_id) const;

	// Global spatial queries (across all active scenes)
	std::vector<ecs::Entity> QueryPoint(const Vec3& point, uint32_t layer_mask = 0xFFFFFFFF) const;

	std::vector<ecs::Entity> QuerySphere(const Vec3& center, float radius, uint32_t layer_mask = 0xFFFFFFFF) const;

	// Update all active scenes
	void Update(float delta_time);

	// Render all active scenes
	void Render();

	// Statistics
	size_t GetSceneCount() const;

	size_t GetActiveSceneCount() const;

	size_t GetTotalEntityCount() const;

	// Clear all scenes
	void Clear() const;

private:
	struct Impl;
	std::unique_ptr<Impl> pimpl_;
};

// =============================================================================
// Utility Functions
// =============================================================================

// Global scene manager access (initialized with ECS world)
SceneManager& GetSceneManager();

// Initialize/shutdown scene management
void InitializeSceneSystem(ecs::ECSWorld& ecs_world);

void ShutdownSceneSystem();
} // namespace engine::scene
