#include <flecs.h>

import glm;
import engine;

namespace engine::ecs {
    void RegisterTransformSystem(const flecs::world &ecs) {
        ecs.system<const Transform, WorldTransform>("TransformPropagation")
                .kind(flecs::OnUpdate)
                .each([](const flecs::entity entity, const Transform &transform, WorldTransform &world_transform) {
                    const glm::mat4 local = transform.world_matrix;
                    if (const auto parent = entity.parent(); parent.is_valid() && parent.has<WorldTransform>()) {
                        const auto [world_matrix] = parent.get<WorldTransform>();
                        world_transform.matrix = world_matrix * local;
                    } else {
                        world_transform.matrix = local;
                    }
                });
    }
} // namespace engine::ecs
