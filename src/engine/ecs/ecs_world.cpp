module;

#include <flecs.h>
#include <functional>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <string>
#include <vector>

module engine.ecs;
import engine;
import glm;

using namespace engine::components;
using namespace engine::rendering;
using namespace engine::ui;

namespace engine::ecs {

// === ECSWORLD IMPLEMENTATION ===

// Register GLM types with flecs reflection system for JSON serialization
void RegisterGlmTypes(const flecs::world& world) {
	// Register glm::vec2 as a flecs struct type
	world.component<glm::vec2>().member<float>("x").member<float>("y");

	// Register glm::vec3 as a flecs struct type
	world.component<glm::vec3>().member<float>("x").member<float>("y").member<float>("z");

	// Register glm::vec4 as a flecs struct type (also used for Color)
	world.component<glm::vec4>().member<float>("x").member<float>("y").member<float>("z").member<float>("w");

	// Register glm::ivec2 for tilemap tile sizes
	world.component<glm::ivec2>().member<int>("x").member<int>("y");

	// Register glm::mat4 with all 16 members for proper serialization
	// mat4 is column-major: [0] = col0, [1] = col1, [2] = col2, [3] = col3
	world.component<glm::mat4>()
			.member<glm::vec4>("c0")
			.member<glm::vec4>("c1")
			.member<glm::vec4>("c2")
			.member<glm::vec4>("c3");
}

ECSWorld::ECSWorld() {
	// Register GLM types first - required for proper flecs reflection
	RegisterGlmTypes(world_);

	world_.component<std::string>()
			.opaque(flecs::String)
			.serialize([](const flecs::serializer* s, const std::string* data) -> int {
				const char* str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char* value) { *data = value ? value : ""; });

	auto& registry = ComponentRegistry::Instance();

	// Register core components with flecs and registry, including field metadata
	// IMPORTANT: Fields must be registered in the EXACT same order as they appear in the struct!
	// All fields must be registered for correct memory layout, even if not shown in editor.
	registry.Register<Transform>("Transform", world_)
			.Category("Core")
			.Field("position", &Transform::position)
			.Field("rotation", &Transform::rotation)
			.Field("scale", &Transform::scale)
			.Build();

	registry.Register<WorldTransform>("WorldTransform", world_).Category("Core").Build();

	registry.Register<Velocity>("Velocity", world_)
			.Category("Core")
			.Field("linear", &Velocity::linear)
			.Field("angular", &Velocity::angular)
			.Build();

	// Renderable component (basic registration - ShaderRef integration added in SetupShaderRefIntegration)
	registry.Register<Renderable>("Renderable", world_)
			.Category("Rendering")
			.Field("visible", &Renderable::visible)
			.Field("render_layer", &Renderable::render_layer)
			.Field("alpha", &Renderable::alpha)
			.Build();

	registry.Register<Camera>("Camera", world_)
			.Category("Rendering")
			.Field("target", &Camera::target)
			.Field("up", &Camera::up)
			.Field("fov", &Camera::fov)
			.Field("aspect_ratio", &Camera::aspect_ratio)
			.Field("near_plane", &Camera::near_plane)
			.Field("far_plane", &Camera::far_plane)
			.Field("view_matrix", &Camera::view_matrix, FieldType::ReadOnly)
			.Field("projection_matrix", &Camera::projection_matrix, FieldType::ReadOnly)
			.Field("dirty", &Camera::dirty, FieldType::ReadOnly)
			.Build();

	registry.Register<Sprite>("Sprite", world_)
			.Category("Rendering")
			.Field("texture", &Sprite::texture)
			.Field("position", &Sprite::position)
			.Field("size", &Sprite::size)
			.Field("rotation", &Sprite::rotation)
			.Field("color", &Sprite::color, FieldType::Color)
			.Field("texture_offset", &Sprite::texture_offset)
			.Field("texture_scale", &Sprite::texture_scale)
			.Field("layer", &Sprite::layer)
			.Field("pivot", &Sprite::pivot)
			.Field("flip_x", &Sprite::flip_x)
			.Field("flip_y", &Sprite::flip_y)
			.Build();

	// Register Light::Type enum as a flecs component for proper serialization
	// Note: Flecs treats enum classes as integers for serialization
	world_.component<Light::Type>();

	registry.Register<Light>("Light", world_)
			.Category("Rendering")
			.Field("type", &Light::type)
			.Field("color", &Light::color, FieldType::Color)
			.Field("intensity", &Light::intensity)
			.Field("range", &Light::range)
			.Field("attenuation", &Light::attenuation)
			.Field("spot_angle", &Light::spot_angle)
			.Field("spot_falloff", &Light::spot_falloff)
			.Field("direction", &Light::direction)
			.Build();

	registry.Register<Animation>("Animation", world_)
			.Category("Rendering")
			.Field("animation_time", &Animation::animation_time)
			.Field("animation_speed", &Animation::animation_speed)
			.Field("looping", &Animation::looping)
			.Field("playing", &Animation::playing)
			.Build();

	registry.Register<ParticleSystem>("ParticleSystem", world_).Category("Rendering").Build();

	// Register scene components
	registry.Register<SceneEntity>("SceneEntity", world_)
			.Category("Scene")
			.Field("name", &SceneEntity::name)
			.Field("visible", &SceneEntity::visible)
			.Field("static_entity", &SceneEntity::static_entity)
			.Field("scene_layer", &SceneEntity::scene_layer)
			.Build();

	registry.Register<Spatial>("Spatial", world_)
			.Category("Scene")
			.Field("bounding_min", &Spatial::bounding_min)
			.Field("bounding_max", &Spatial::bounding_max)
			.Field("spatial_layer", &Spatial::spatial_layer)
			.Build();

	// Register tag components (no fields)
	registry.Register<Rotating>("Rotating", world_).Category("Tags").Build();

	// Register relationship tags (no fields)
	registry.Register<SceneRoot>("SceneRoot", world_).Category("Tags").Build();
	registry.Register<ActiveCamera>("ActiveCamera", world_).Category("Tags").Build();
	registry.Register<Tilemap>("Tilemap", world_).Category("Rendering").Build();

	// Set up shader reference integration (ShaderRef component, With trait, observers)
	SetupShaderRefIntegration();

	// Set up mesh reference integration (MeshRef component, With trait, observers)
	SetupMeshRefIntegration();

	// Set up built-in systems
	SetupMovementSystem();
	SetupRotationSystem();
	SetupCameraSystem();
	SetupSpatialSystem();
	SetupTransformSystem();
}

// Get the underlying flecs world
flecs::world& ECSWorld::GetWorld() { return world_; }
[[nodiscard]] const flecs::world& ECSWorld::GetWorld() const { return world_; }

// === ENTITY CREATION ===

// Create an entity
[[nodiscard]] flecs::entity ECSWorld::CreateEntity() const {
	const auto entity = world_.entity();
	entity.set<Transform>({});
	entity.set<WorldTransform>({});
	return entity;
}

// Create an entity with a name
flecs::entity ECSWorld::CreateEntity(const char* name) const {
	const auto entity = world_.entity(name);
	entity.set<SceneEntity>({.name = name});
	entity.set<Transform>({});
	entity.set<WorldTransform>({});
	return entity;
}

// Create a scene root entity
flecs::entity ECSWorld::CreateSceneRoot(const char* name) const {
	const auto entity = CreateEntity(name);
	entity.add<SceneRoot>();
	return entity;
}

// === HIERARCHY MANAGEMENT ===

// Set parent-child relationship using flecs built-in ChildOf
void ECSWorld::SetParent(const flecs::entity child, const flecs::entity parent) { child.child_of(parent); }

// Remove parent relationship
void ECSWorld::RemoveParent(const flecs::entity child) { child.remove(flecs::ChildOf, flecs::Wildcard); }

// Get parent entity (returns invalid entity if no parent)
flecs::entity ECSWorld::GetParent(const flecs::entity entity) { return entity.parent(); }

// Get all children of an entity
std::vector<flecs::entity> ECSWorld::GetChildren(const flecs::entity parent) {
	std::vector<flecs::entity> children;
	parent.children([&](const flecs::entity child) { children.push_back(child); });
	return children;
}

// Get all descendants (recursive)
std::vector<flecs::entity> ECSWorld::GetDescendants(const flecs::entity root) {
	std::vector<flecs::entity> descendants;

	std::function<void(flecs::entity)> collect_recursive = [&](const flecs::entity entity) {
		entity.children([&](const flecs::entity child) {
			descendants.push_back(child);
			collect_recursive(child); // Recurse
		});
	};

	collect_recursive(root);
	return descendants;
}

// Find entity by name in hierarchy
flecs::entity ECSWorld::FindEntityByName(const char* name, const flecs::entity root) {
	flecs::entity found;

	const auto query = world_.query_builder<const SceneEntity>().build();

	query.each([&](const flecs::entity entity, const SceneEntity& scene_entity) {
		if (scene_entity.name == name) {
			// If root specified, check if entity is descendant
			if (!root.is_valid() || IsDescendantOf(entity, root)) {
				found = entity;
			}
		}
	});

	return found;
}

// Check if entity is descendant of another
bool ECSWorld::IsDescendantOf(const flecs::entity entity, const flecs::entity ancestor) {
	flecs::entity current = entity.parent();
	while (current.is_valid()) {
		if (current == ancestor) {
			return true;
		}
		current = current.parent();
	}
	return false;
}

// === CAMERA MANAGEMENT ===

void ECSWorld::SetActiveCamera(const flecs::entity camera) {
	// Use deferred operations to avoid modifying tables during iteration
	world_.defer_begin();

	// Remove ActiveCamera tag from all entities
	world_.query_builder<>().with<ActiveCamera>().build().each(
			[](const flecs::entity entity) { entity.remove<ActiveCamera>(); });

	// Add ActiveCamera tag to new camera
	if (camera.is_valid()) {
		camera.add<ActiveCamera>();
		active_camera_ = camera;
	}

	world_.defer_end();
}

[[nodiscard]] flecs::entity ECSWorld::GetActiveCamera() const { return active_camera_; }

// === SPATIAL QUERIES ===

// Find entities at a point
[[nodiscard]] std::vector<flecs::entity> ECSWorld::QueryPoint(const glm::vec3& point, const uint32_t layer_mask) const {
	std::vector<flecs::entity> result;

	const auto query = world_.query<const Transform, const Spatial>();
	query.each([&](const flecs::entity entity, const Transform& transform, const Spatial& spatial) {
		if ((spatial.spatial_layer & layer_mask) == 0) {
			return;
		}

		// Transform bounding box to world space
		const glm::vec3 world_min = transform.position + spatial.bounding_min;
		const glm::vec3 world_max = transform.position + spatial.bounding_max;

		if (point.x >= world_min.x && point.x <= world_max.x && point.y >= world_min.y && point.y <= world_max.y
			&& point.z >= world_min.z && point.z <= world_max.z) {
			result.push_back(entity);
		}
	});

	return result;
}

// Find entities in sphere
[[nodiscard]] std::vector<flecs::entity>
ECSWorld::QuerySphere(const glm::vec3& center, const float radius, const uint32_t layer_mask) const {
	std::vector<flecs::entity> result;
	const float radius_sq = radius * radius;

	const auto query = world_.query<const Transform, const Spatial>();
	query.each([&](const flecs::entity entity, const Transform& transform, const Spatial& spatial) {
		if ((spatial.spatial_layer & layer_mask) == 0) {
			return;
		}

		// Simple distance check to entity center
		if (const float dist_sq = glm::distance2(transform.position, center); dist_sq <= radius_sq) {
			result.push_back(entity);
		}
	});

	return result;
}

// Progress the world (run systems)
void ECSWorld::Progress(const float delta_time) const { world_.progress(delta_time); }

// Submit render commands for all renderable entities
void ECSWorld::SubmitRenderCommands(const rendering::Renderer& renderer) {
	// Static default camera to avoid recreation every frame
	static const Camera default_camera = []() {
		Camera cam;
		constexpr glm::vec3 default_position(0.0f, 0.0f, 10.0f);
		cam.view_matrix = glm::lookAt(default_position, cam.target, cam.up);
		cam.projection_matrix =
				glm::perspective(glm::radians(cam.fov), cam.aspect_ratio, cam.near_plane, cam.far_plane);
		return cam;
	}();

	const flecs::entity camera_entity = GetActiveCamera();

	// Determine which camera to use: active camera or default fallback
	const Camera* active_camera = &default_camera;
	Camera camera_data;
	if (camera_entity.is_valid() && camera_entity.has<Camera>()) {
		camera_data = camera_entity.get<Camera>();
		active_camera = &camera_data;
	}

	// Set up lighting uniforms from first Light component in scene
	auto& shader_mgr = renderer.GetShaderManager();
	glm::vec3 light_dir{0.2f, -1.0f, -0.3f}; // Default fallback
	world_.query<const Light>().each([&light_dir](const Light& light) { light_dir = glm::normalize(light.direction); });

	// Single query loop for all renderables
	const auto renderable_query = world_.query<const WorldTransform, const rendering::Renderable>();
	renderable_query.each(
			[&](flecs::iter, size_t, const WorldTransform& transform, const rendering::Renderable& renderable) {
				if (!renderable.visible) {
					return;
				}
				rendering::RenderCommand cmd{
						.mesh = renderable.mesh,
						.shader = renderable.shader,
						.material = renderable.material,
						.render_state_stack = renderable.render_state_stack,
						.camera_view = active_camera->view_matrix};

				// Short term: set light direction uniform here
				const auto& shader = shader_mgr.GetShader(renderable.shader);
				shader.Use();
				shader.SetUniform("u_LightDir", light_dir);

				cmd.transform = transform.matrix;

				renderer.SubmitRenderCommand(cmd);
			});
}

void ECSWorld::SetupMovementSystem() const {
	// System to update positions based on velocity
	world_.system<Transform, const Velocity>().each(
			[](const flecs::iter itr, const size_t index, Transform& transform, const Velocity& velocity) {
				transform.position += velocity.linear * itr.delta_time();
				transform.rotation += velocity.angular * itr.delta_time();
				itr.entity(index).modified<Transform>();
			});
}

void ECSWorld::SetupRotationSystem() const {
	// System for entities with Rotating tag - simple rotation animation
	world_.system<Transform, Rotating>().each(
			[](const flecs::iter itr, const size_t index, Transform& transform, Rotating) {
				transform.rotation.y += 1.0F * itr.delta_time(); // 1 radian per second
				itr.entity(index).modified<Transform>();
			});
}

void ECSWorld::SetupCameraSystem() const {
	// System to update camera matrices when dirty
	world_.observer<Transform, Camera>("CameraTransformUpdate")
			.event(flecs::OnSet)
			.each([](flecs::iter, size_t, const Transform& transform, Camera& camera) {
				// Update view matrix
				camera.view_matrix = glm::lookAt(transform.position, camera.target, camera.up);

				// Update projection matrix
				camera.projection_matrix = glm::perspective(
						glm::radians(camera.fov), camera.aspect_ratio, camera.near_plane, camera.far_plane);

				camera.dirty = false;
			});
}

void ECSWorld::SetupSpatialSystem() const {
	// System to update spatial bounds when transforms change
	world_.observer<const Transform, Spatial>("SpatialBoundsUpdate")
			.event(flecs::OnSet)
			.each([](flecs::entity, const Transform&, Spatial& spatial) { spatial.bounds_dirty = true; });
}

void ECSWorld::SetupTransformSystem() const {
	world_.observer<const Transform, WorldTransform>("TransformPropagation")
			.term_at(0)
			.self()
			.up()
			.event(flecs::OnSet)
			.each([](const flecs::entity entity, const Transform& transform, WorldTransform& world_transform) {
				glm::mat4 world_matrix = component_helpers::ComputeTransformMatrix(transform);
				if (const auto parent = entity.parent(); parent.is_valid() && parent.has<WorldTransform>()) {
					const auto [parent_world_matrix] = parent.get<WorldTransform>();
					world_matrix = parent_world_matrix * world_matrix;
				}

				world_transform.matrix = world_matrix;

				// Cascade to children: trigger their Transform observers
				entity.children([](const flecs::entity child) {
					if (child.has<Transform>()) {
						child.modified<Transform>();
					}
				});
			});
}

// === SHADER REFERENCE INTEGRATION ===
// Co-located shader-related ECS setup: ShaderRef component, With trait, and bidirectional observers

void ECSWorld::SetupShaderRefIntegration() {
	auto& registry = ComponentRegistry::Instance();

	// Register ShaderRef component - stores shader asset name for serialization
	registry.Register<ShaderRef>("ShaderRef", world_)
			.Category("Rendering")
			.Field("name", &ShaderRef::name)
			.AssetRef(scene::ShaderAssetInfo::TYPE_NAME)
			.Build();

	// Add (With, ShaderRef) trait to Renderable - auto-adds ShaderRef when Renderable is added
	if (!world_.component<Renderable>().add(flecs::With, world_.component<ShaderRef>())) {
		return;
	}

	// Observer: ShaderRef.name → Renderable.shader (resolve name to ID)
	// Updates Renderable shader ID: valid name → lookup ID, empty name → INVALID_SHADER
	world_.observer<ShaderRef, Renderable>("ShaderRefToRenderable")
			.event(flecs::OnSet)
			.each([](flecs::entity, const ShaderRef& ref, Renderable& renderable) {
				if (ref.name.empty()) {
					renderable.shader = INVALID_SHADER;
					return;
				}

				if (const ShaderId id = GetRenderer().GetShaderManager().FindShader(ref.name); id != INVALID_SHADER) {
					renderable.shader = id;
				}
			});

	// Observer: Renderable.shader → ShaderRef.name (sync ID back to name)
	// Respects empty ref.name as intentional (user chose "(None)")
	world_.observer<Renderable, ShaderRef>("RenderableToShaderRef")
			.event(flecs::OnSet)
			.each([](flecs::entity, const Renderable& renderable, ShaderRef& ref) {
				// If user cleared the name, respect that choice - don't overwrite
				if (ref.name.empty()) {
					return;
				}

				if (renderable.shader == INVALID_SHADER) {
					return;
				}

				if (const std::string name = GetRenderer().GetShaderManager().GetShaderName(renderable.shader);
					!name.empty() && ref.name != name) {
					ref.name = name;
				}
			});
}

// === MESH REFERENCE INTEGRATION ===
// Co-located mesh-related ECS setup: MeshRef component, With trait, and observers

void ECSWorld::SetupMeshRefIntegration() {
	auto& registry = ComponentRegistry::Instance();

	// Register MeshRef component - stores mesh asset name for serialization
	registry.Register<MeshRef>("MeshRef", world_)
			.Category("Rendering")
			.Field("name", &MeshRef::name)
			.AssetRef(scene::MeshAssetInfo::TYPE_NAME)
			.Build();

	// Add (With, MeshRef) trait to Renderable - auto-adds MeshRef when Renderable is added
	if (!world_.component<Renderable>().add(flecs::With, world_.component<MeshRef>())) {
		return;
	}

	// Observer: MeshRef.name → Renderable.mesh (resolve name to mesh ID)
	// Updates Renderable mesh ID: valid name → lookup ID, empty name → INVALID_MESH
	world_.observer<MeshRef, Renderable>("MeshRefToRenderable")
			.event(flecs::OnSet)
			.each([](flecs::entity, const MeshRef& ref, Renderable& renderable) {
				if (ref.name.empty()) {
					renderable.mesh = INVALID_MESH;
					return;
				}

				// Look up mesh by name in MeshManager (global, not scene-specific)
				if (const MeshId id = GetRenderer().GetMeshManager().FindMesh(ref.name); id != INVALID_MESH) {
					renderable.mesh = id;
				}
			});

	// Observer: Renderable.mesh → MeshRef.name (sync ID back to name)
	// Uses MeshManager's global name registry
	// Respects empty ref.name as intentional (user chose "(None)")
	world_.observer<Renderable, MeshRef>("RenderableToMeshRef")
			.event(flecs::OnSet)
			.each([](flecs::entity, const Renderable& renderable, MeshRef& ref) {
				// If user cleared the name, respect that choice - don't overwrite
				if (ref.name.empty()) {
					return;
				}

				if (renderable.mesh == INVALID_MESH) {
					return;
				}

				// Look up mesh name by ID in MeshManager
				if (const std::string name = GetRenderer().GetMeshManager().GetMeshName(renderable.mesh);
					!name.empty() && ref.name != name) {
					ref.name = name;
				}
			});
}
} // namespace engine::ecs
