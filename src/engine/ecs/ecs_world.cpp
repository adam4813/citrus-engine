module;

#include <flecs.h>
#include <functional>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <string>
#include <vector>

module engine.ecs;
import engine;
import engine.physics;
import glm;

using namespace engine::components;
using namespace engine::rendering;
using namespace engine::ui;
using namespace engine::animation;

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
	// Set up custom pipeline phases FIRST (before any systems)
	SetupPipeline();

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

	registry.Register<WorldTransform>("WorldTransform", world_)
			.Category("Core")
			.Field("position", &WorldTransform::position)
			.Field("rotation", &WorldTransform::rotation)
			.Field("scale", &WorldTransform::scale)
			.Build();

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

	// Register the new animation system Animator component
	registry.Register<Animator>("Animator", world_).Category("Animation").Build();

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

	// Register scene organization components
	registry.Register<Group>("Group", world_).Category("Scene").Build();
	registry.Register<Tags>("Tags", world_).Category("Scene").Field("tags", &Tags::tags).Build();
	registry.Register<PrefabInstance>("PrefabInstance", world_)
			.Category("Scene")
			.Field("prefab_path", &PrefabInstance::prefab_path)
			.Build();

	// Register audio components (basic registration - SoundRef integration added in SetupSoundRefIntegration)
	// Register PlayState enum for proper serialization
	world_.component<audio::PlayState>();

	registry.Register<audio::AudioSource>("AudioSource", world_)
			.Category("Audio")
			.Field("volume", &audio::AudioSource::volume)
			.Field("pitch", &audio::AudioSource::pitch)
			.Field("looping", &audio::AudioSource::looping)
			.Field("spatial", &audio::AudioSource::spatial)
			.Field("position", &audio::AudioSource::position)
			.Field("state", &audio::AudioSource::state)
			.EnumLabels({"Stopped", "Playing", "Paused"})
			.Build();

	registry.Register<audio::AudioListener>("AudioListener", world_)
			.Category("Audio")
			.Field("position", &audio::AudioListener::position)
			.Field("forward", &audio::AudioListener::forward)
			.Field("up", &audio::AudioListener::up)
			.Build();

	// Register AI components
	registry.Register<BehaviorTreeComponent>("BehaviorTreeComponent", world_)
			.Category("AI")
			.Field("behavior_tree_asset", &BehaviorTreeComponent::behavior_tree_asset)
			.Build();

	// Register physics components
	registry.Register<physics::RigidBody>("RigidBody", world_)
			.Category("Physics")
			.Field("motion_type", &physics::RigidBody::motion_type)
			.EnumLabels({"Static", "Kinematic", "Dynamic"})
			.EnumTooltips(
					{"Immovable object (floors, walls) — zero mass, infinite inertia",
					 "Script-controlled motion (moving platforms, elevators) — not affected by forces",
					 "Physics-simulated (falling objects, projectiles) — affected by gravity and forces"})
			.Field("mass", &physics::RigidBody::mass)
			.Field("linear_damping", &physics::RigidBody::linear_damping)
			.Field("angular_damping", &physics::RigidBody::angular_damping)
			.Field("friction", &physics::RigidBody::friction)
			.Field("restitution", &physics::RigidBody::restitution)
			.Field("enable_ccd", &physics::RigidBody::enable_ccd)
			.Field("use_gravity", &physics::RigidBody::use_gravity)
			.Field("gravity_scale", &physics::RigidBody::gravity_scale)
			.Build();

	registry.Register<physics::CollisionShape>("CollisionShape", world_)
			.Category("Physics")
			.Field("type", &physics::CollisionShape::type)
			.EnumLabels({"Box", "Sphere", "Capsule", "Cylinder", "ConvexHull", "Mesh", "Compound"})
			.EnumTooltips(
					{"Rectangular box collider",
					 "Spherical collider",
					 "Capsule collider (cylinder with rounded ends)",
					 "Cylindrical collider",
					 "Convex hull from mesh vertices",
					 "Triangle mesh collider (static only)",
					 "Multiple shapes combined"})
			.Field("box_half_extents", &physics::CollisionShape::box_half_extents)
			.VisibleWhen("type", {0})
			.Field("sphere_radius", &physics::CollisionShape::sphere_radius)
			.VisibleWhen("type", {1})
			.Field("capsule_radius", &physics::CollisionShape::capsule_radius)
			.VisibleWhen("type", {2})
			.Field("capsule_height", &physics::CollisionShape::capsule_height)
			.VisibleWhen("type", {2})
			.Field("cylinder_radius", &physics::CollisionShape::cylinder_radius)
			.VisibleWhen("type", {3})
			.Field("cylinder_height", &physics::CollisionShape::cylinder_height)
			.VisibleWhen("type", {3})
			.Field("offset", &physics::CollisionShape::offset)
			.Build();

	registry.Register<physics::PhysicsVelocity>("PhysicsVelocity", world_)
			.Category("Physics")
			.Field("linear", &physics::PhysicsVelocity::linear)
			.Field("angular", &physics::PhysicsVelocity::angular)
			.Build();

	registry.Register<physics::PhysicsWorldConfig>("PhysicsWorldConfig", world_)
			.Category("Physics")
			.Hidden()
			.Field("gravity", &physics::PhysicsWorldConfig::gravity)
			.Field("fixed_timestep", &physics::PhysicsWorldConfig::fixed_timestep)
			.Field("max_substeps", &physics::PhysicsWorldConfig::max_substeps)
			.Field("enable_sleeping", &physics::PhysicsWorldConfig::enable_sleeping)
			.Field("show_debug_physics", &physics::PhysicsWorldConfig::show_debug_physics)
			.Build();

	// Tag components
	registry.Register<physics::IsTrigger>("IsTrigger", world_).Category("Physics").Build();
	registry.Register<physics::IsSleeping>("IsSleeping", world_).Category("Physics").Build();

	// Set up shader reference integration (ShaderRef component, With trait, observers)
	SetupShaderRefIntegration();

	// Set up mesh reference integration (MeshRef component, With trait, observers)
	SetupMeshRefIntegration();

	// Set up sound reference integration (SoundRef component, With trait, observers)
	SetupSoundRefIntegration();

	// Set up built-in systems
	SetupMovementSystem();
	SetupRotationSystem();
	SetupCameraSystem();
	SetupSpatialSystem();
	SetupTransformSystem();
	SetupAnimationSystem();
	SetupAudioSystem();
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

void ECSWorld::SetupPipeline() {
	simulation_phase_ = world_.entity("Simulation").add(flecs::Phase).depends_on(flecs::OnUpdate);
}

// Progress all phases (standard full update)
void ECSWorld::ProgressAll(const float delta_time) const {
	simulation_phase_.enable();
	world_.progress(delta_time);
}

// Progress edit mode (skip simulation, run post-simulation and pre-render)
void ECSWorld::ProgressEditMode(const float delta_time) const {
	simulation_phase_.disable();
	world_.progress(delta_time);
}

// Legacy method - kept for backwards compatibility
void ECSWorld::Progress(const float delta_time) const { ProgressAll(delta_time); }

} // namespace engine::ecs
