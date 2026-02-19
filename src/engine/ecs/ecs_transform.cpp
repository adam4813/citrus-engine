module;

#include <flecs.h>

module engine.ecs;
import engine;
import engine.physics;
import glm;

using namespace engine::components;

namespace engine::ecs {

void ECSWorld::SetupTransformSystem() const {
	world_.observer<const Transform, WorldTransform&>("TransformPropagation")
			.event(flecs::OnAdd)
			.event(flecs::OnSet)
			.each([&sim_phase = simulation_phase_](
						  const flecs::entity entity, const Transform& transform, WorldTransform& world_transform) {
				// In play mode, dynamic physics bodies own WorldTransform — skip recomputing.
				// In edit mode (simulation disabled), always recompute so the user can edit Transform.
				if (sim_phase.enabled() && entity.has<physics::RigidBody>()) {
					if (const auto rigid_body = entity.get<physics::RigidBody>();
						rigid_body.motion_type == physics::MotionType::Dynamic) {
						// Cascade to children only — WorldTransform was set by physics
						entity.children([](const flecs::entity child) {
							if (child.has<Transform>()) {
								child.modified<Transform>();
							}
						});
						return;
					}
				}

				glm::mat4 world_matrix = component_helpers::ComputeTransformMatrix(transform);

				if (const auto parent = entity.parent(); parent.is_valid() && parent.has<WorldTransform>()) {
					const auto& parent_wt = parent.get<WorldTransform>();
					world_matrix = parent_wt.matrix * world_matrix;
				}

				// Decompose the world matrix into position/rotation/scale
				world_transform.position = glm::vec3(world_matrix[3]);

				const glm::vec3 col0(world_matrix[0]);
				const glm::vec3 col1(world_matrix[1]);
				const glm::vec3 col2(world_matrix[2]);
				world_transform.scale = glm::vec3(glm::length(col0), glm::length(col1), glm::length(col2));

				if (constexpr float kEpsilon = 1e-6F; world_transform.scale.x > kEpsilon
													  && world_transform.scale.y > kEpsilon
													  && world_transform.scale.z > kEpsilon) {
					const glm::mat3 rot_mat(
							col0 / world_transform.scale.x,
							col1 / world_transform.scale.y,
							col2 / world_transform.scale.z);
					world_transform.rotation = glm::eulerAngles(glm::quat_cast(rot_mat));
				}
				else {
					world_transform.rotation = transform.rotation;
				}

				world_transform.matrix = world_matrix;

				// Notify physics::RigidBody changed so physics SyncToBackend can update body position
				entity.modified<physics::RigidBody>();

				// Cascade to children: trigger their Transform observers
				entity.children([](const flecs::entity child) {
					if (child.has<Transform>()) {
						child.modified<Transform>();
					}
				});
			});
}

} // namespace engine::ecs
