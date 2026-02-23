module;

#include <flecs.h>
#include <iostream>
#include <string>
#include <vector>

module engine.ecs;
import engine;
import engine.physics;
import glm;

using namespace engine::components;
using namespace engine::rendering;
using namespace engine::assets;

namespace engine::ecs {

// Submit render commands for all renderable entities
void ECSWorld::SubmitRenderCommands(const Renderer& renderer) {
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
	const auto renderable_query = world_.query<const WorldTransform, const Renderable>();
	renderable_query.each([&](flecs::iter, size_t, const WorldTransform& transform, const Renderable& renderable) {
		if (!renderable.visible) {
			return;
		}
		RenderCommand cmd{
				.mesh = renderable.mesh,
				.shader = renderable.shader,
				.material = renderable.material,
				.render_state_stack = renderable.render_state_stack,
				.camera_view = active_camera->view_matrix};

		cmd.transform = transform.matrix;

		// Set lighting uniforms for lit shaders
		if (const auto& shader = shader_mgr.GetShader(renderable.shader); shader.IsValid()) {
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
		if (const auto& physics_config = world_.get<physics::PhysicsWorldConfig>();
			physics_config.show_debug_physics && world_.has<physics::PhysicsBackendPtr>()) {
			renderer.SetDebugCamera(active_camera->view_matrix, active_camera->projection_matrix);

			if (const auto& [backend] = world_.get<physics::PhysicsBackendPtr>(); backend) {
				physics::RendererDebugAdapter adapter(renderer);
				backend->DebugDraw(adapter);
			}

			renderer.FlushDebugLines();
		}
	}
}

} // namespace engine::ecs
