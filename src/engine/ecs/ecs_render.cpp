module;

#include <flecs.h>
#include <string>
#include <vector>

module engine.ecs;
import engine;
import engine.physics;
import glm;

using namespace engine::components;
using namespace engine::rendering;

namespace engine::ecs {

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

void ECSWorld::SetupSoundRefIntegration() {
	auto& registry = ComponentRegistry::Instance();

	// Register SoundRef component - stores sound asset name for serialization
	registry.Register<audio::SoundRef>("SoundRef", world_)
			.Category("Audio")
			.Field("name", &audio::SoundRef::name)
			.AssetRef(scene::SoundAssetInfo::TYPE_NAME)
			.Build();

	// Add (With, SoundRef) trait to AudioSource - auto-adds SoundRef when AudioSource is added
	if (!world_.component<audio::AudioSource>().add(flecs::With, world_.component<audio::SoundRef>())) {
		return;
	}

	// Observer: SoundRef.name → AudioSource.clip_id (resolve name to clip ID)
	// Lazily loads the clip from the scene's SoundAssetInfo if not yet cached
	world_.observer<audio::SoundRef, audio::AudioSource>("SoundRefToAudioSource")
			.event(flecs::OnSet)
			.each([](flecs::entity, const audio::SoundRef& ref, audio::AudioSource& source) {
				if (ref.name.empty()) {
					source.clip_id = 0;
					return;
				}

				auto& audio_sys = audio::AudioSystem::Get();
				if (!audio_sys.IsInitialized()) {
					return;
				}

				// Check if already loaded by name
				if (uint32_t id = audio_sys.FindClipByName(ref.name); id != 0) {
					source.clip_id = id;
					return;
				}

				// Lazy load: look up SoundAssetInfo from the active scene
				auto& scene_mgr = scene::GetSceneManager();
				const auto active_id = scene_mgr.GetActiveScene();
				if (active_id == scene::INVALID_SCENE) {
					return;
				}
				const auto& scene = scene_mgr.GetScene(active_id);
				if (auto sound_asset = scene.GetAssets().FindTyped<scene::SoundAssetInfo>(ref.name)) {
					if (!sound_asset->file_path.empty()) {
						source.clip_id = audio_sys.LoadClipNamed(ref.name, sound_asset->file_path);
					}
				}
			});
}

} // namespace engine::ecs
