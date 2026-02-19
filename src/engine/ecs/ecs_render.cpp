module;

#include <flecs.h>
#include <functional>
#include <string>
#include <vector>

module engine.ecs;
import engine;
import engine.physics;
import glm;

using namespace engine::components;
using namespace engine::rendering;

namespace engine::ecs {

// === GENERIC ASSET REFERENCE BINDING ===
// Template that wires up: component registration, With trait, and bidirectional observers.
// RefComp must have a `std::string name` member.
// TargetComp is the component that holds the runtime ID (e.g., Renderable, AudioSource).
// find_fn: resolves name → ID. name_fn: resolves ID → name (nullptr disables backward sync).
template <typename RefComp, typename TargetComp, typename IdType, typename FindFn, typename NameFn>
void SetupAssetRefBinding(flecs::world& world,
						  const std::string& ref_name,
						  const std::string& category,
						  std::string_view asset_type_name,
						  IdType TargetComp::*id_member,
						  IdType invalid_value,
						  FindFn find_fn,
						  NameFn name_fn) {
	auto& registry = ComponentRegistry::Instance();

	registry.Register<RefComp>(ref_name, world)
			.Category(category)
			.Field("name", &RefComp::name)
			.AssetRef(asset_type_name)
			.Build();

	if (!world.component<TargetComp>().add(flecs::With, world.component<RefComp>())) {
		return;
	}

	// Forward observer: RefComp.name → TargetComp.id_member
	const auto forward_name = ref_name + "Resolve";
	world.observer<RefComp, TargetComp>(forward_name.c_str())
			.event(flecs::OnSet)
			.each([id_member, invalid_value, find_fn](flecs::entity, const RefComp& ref, TargetComp& target) {
				if (ref.name.empty()) {
					target.*id_member = invalid_value;
					return;
				}
				if (const IdType id = find_fn(ref.name); id != invalid_value) {
					target.*id_member = id;
				}
			});

	// Backward observer: TargetComp.id_member → RefComp.name (skip if name_fn is nullptr)
	if constexpr (requires { name_fn(std::declval<IdType>()); }) {
		if (name_fn) {
			const auto backward_name = ref_name + "Sync";
			world.observer<TargetComp, RefComp>(backward_name.c_str())
					.event(flecs::OnSet)
					.each([id_member, invalid_value, name_fn](
								  flecs::entity, const TargetComp& target, RefComp& ref) {
						if (ref.name.empty()) {
							return;
						}
						if (target.*id_member == invalid_value) {
							return;
						}
						if (const std::string name = name_fn(target.*id_member);
							!name.empty() && ref.name != name) {
							ref.name = name;
						}
					});
		}
	}
}

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

	// Collect all lights in the scene (up to 4)
	constexpr int MAX_LIGHTS = 4;
	std::vector<Light> scene_lights;
	std::vector<glm::vec3> light_positions;

	// Query all entities with Light component and Transform
	world_.query<const Light, const Transform>().each(
			[&scene_lights, &light_positions](flecs::entity e, const Light& light, const Transform& transform) {
				if (scene_lights.size() < MAX_LIGHTS) {
					scene_lights.push_back(light);
					light_positions.push_back(transform.position);
				}
			});

	// TEMP: Hold the first light for backward compatability
	glm::vec3 light_dir{0.2f, -1.0f, -0.3f}; // Default fallback
	if (scene_lights.size() > 0) {
		light_dir = glm::normalize(scene_lights[0].direction);
	}

	// Get camera position for specular calculations
	glm::vec3 camera_position{0.0f, 0.0f, 10.0f}; // Default
	if (camera_entity.is_valid() && camera_entity.has<Transform>()) {
		const auto& cam_transform = camera_entity.get<Transform>();
		camera_position = cam_transform.position;
	}

	auto& mat_mgr = renderer.GetMaterialManager();
	auto& shader_mgr = renderer.GetShaderManager();

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

				cmd.transform = transform.matrix;

				// Set lighting uniforms for lit shaders
				const auto& shader = shader_mgr.GetShader(renderable.shader);
				if (shader.IsValid()) {
					shader.Use();
					// TEMP: Use the first light, backward compatability
					shader.SetUniform("u_LightDir", light_dir);

					// Set camera position
					shader.SetUniform("u_CameraPos", camera_position);

					// Set ambient lighting
					shader.SetUniform("u_AmbientColor", glm::vec3(1.0f, 1.0f, 1.0f));
					shader.SetUniform("u_AmbientIntensity", 0.5f);

					// Set material properties from the entity's material (if valid)
					if (mat_mgr.IsValid(renderable.material)) {
						const auto& material = mat_mgr.GetMaterial(renderable.material);
						material.Apply(shader);
					}
					else {
						shader.SetUniform("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
						shader.SetUniform("u_Shininess", 32.0f);
					}

					// Calculate normal matrix (inverse transpose of model matrix)
					glm::mat4 normal_matrix = glm::transpose(glm::inverse(cmd.transform));
					shader.SetUniform("u_NormalMatrix", normal_matrix);

					// Set number of lights
					shader.SetUniform("u_NumLights", static_cast<int>(scene_lights.size()));

					// Set light properties
					for (size_t i = 0; i < scene_lights.size() && i < MAX_LIGHTS; ++i) {
						const Light& light = scene_lights[i];
						const std::string idx = "[" + std::to_string(i) + "]";

						// Light type
						shader.SetUniform("u_LightTypes" + idx, static_cast<int>(light.type));

						// Position/Direction (for directional lights, this is the direction)
						if (light.type == Light::Type::Directional) {
							shader.SetUniform("u_LightPositions" + idx, glm::normalize(light.direction));
						}
						else {
							shader.SetUniform("u_LightPositions" + idx, light_positions[i]);
						}

						// Color and intensity
						glm::vec3 light_color(light.color.r, light.color.g, light.color.b);
						shader.SetUniform("u_LightColors" + idx, light_color);
						shader.SetUniform("u_LightIntensities" + idx, light.intensity);

						// Range and attenuation (for point/spot lights)
						shader.SetUniform("u_LightRanges" + idx, light.range);
						shader.SetUniform("u_LightAttenuations" + idx, light.attenuation);
					}
				}

				renderer.SubmitRenderCommand(cmd);
			});

	// Physics debug drawing — delegate to the active physics backend
	if (world_.has<physics::PhysicsWorldConfig>()) {
		const auto& physics_config = world_.get<physics::PhysicsWorldConfig>();
		if (physics_config.show_debug_physics && world_.has<physics::PhysicsBackendPtr>()) {
			renderer.SetDebugCamera(active_camera->view_matrix, active_camera->projection_matrix);

			const auto& backend_ptr = world_.get<physics::PhysicsBackendPtr>();
			if (backend_ptr.backend) {
				physics::RendererDebugAdapter adapter(renderer);
				backend_ptr.backend->DebugDraw(adapter);
			}

			renderer.FlushDebugLines();
		}
	}
}

// === ASSET REFERENCE INTEGRATIONS ===

void ECSWorld::SetupShaderRefIntegration() {
	SetupAssetRefBinding<ShaderRef, Renderable>(
			world_, "ShaderRef", "Rendering", scene::ShaderAssetInfo::TYPE_NAME, &Renderable::shader, INVALID_SHADER,
			[](const std::string& name) { return GetRenderer().GetShaderManager().FindShader(name); },
			[](ShaderId id) { return GetRenderer().GetShaderManager().GetShaderName(id); });
}

void ECSWorld::SetupMeshRefIntegration() {
	SetupAssetRefBinding<MeshRef, Renderable>(
			world_, "MeshRef", "Rendering", scene::MeshAssetInfo::TYPE_NAME, &Renderable::mesh, INVALID_MESH,
			[](const std::string& name) { return GetRenderer().GetMeshManager().FindMesh(name); },
			[](MeshId id) { return GetRenderer().GetMeshManager().GetMeshName(id); });
}

void ECSWorld::SetupMaterialRefIntegration() {
	SetupAssetRefBinding<MaterialRef, Renderable>(
			world_, "MaterialRef", "Rendering", scene::MaterialAssetInfo::TYPE_NAME, &Renderable::material,
			INVALID_MATERIAL,
			[](const std::string& name) { return GetRenderer().GetMaterialManager().FindMaterial(name); },
			[](MaterialId id) { return GetRenderer().GetMaterialManager().GetMaterialName(id); });
}

void ECSWorld::SetupSoundRefIntegration() {
	SetupAssetRefBinding<audio::SoundRef, audio::AudioSource>(
			world_, "SoundRef", "Audio", scene::SoundAssetInfo::TYPE_NAME, &audio::AudioSource::clip_id,
			static_cast<uint32_t>(0),
			[](const std::string& name) -> uint32_t {
				auto& audio_sys = audio::AudioSystem::Get();
				if (!audio_sys.IsInitialized()) {
					return 0;
				}
				// Check if already loaded by name
				if (uint32_t id = audio_sys.FindClipByName(name); id != 0) {
					return id;
				}
				// Lazy load from active scene
				auto& scene_mgr = scene::GetSceneManager();
				const auto active_id = scene_mgr.GetActiveScene();
				if (active_id == scene::INVALID_SCENE) {
					return 0;
				}
				const auto& scene = scene_mgr.GetScene(active_id);
				if (auto sound_asset = scene.GetAssets().FindTyped<scene::SoundAssetInfo>(name)) {
					if (!sound_asset->file_path.empty()) {
						return audio_sys.LoadClipNamed(name, sound_asset->file_path);
					}
				}
				return 0;
			},
			std::function<std::string(uint32_t)>(nullptr)); // No backward sync for audio
}

} // namespace engine::ecs
