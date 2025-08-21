module;

#include <string>
#include <vector>
#include <functional>
#include <glm/gtx/norm.hpp>
#include <flecs.h>

module engine.ecs;
import engine.components;
import engine.rendering;
import engine.ui;
import glm;

using namespace engine::components;
using namespace engine::rendering;

namespace engine::ecs {
    ECSWorld::ECSWorld() {
        // Register core components with flecs
        world_.component<Transform>();
        world_.component<WorldTransform>();
        world_.component<Velocity>();
        // Register rendering components
        world_.component<engine::rendering::Renderable>();
        world_.component<Camera>();
        world_.component<ui::Sprite>();
        world_.component<engine::rendering::Light>();
        world_.component<engine::rendering::Animation>();
        world_.component<engine::rendering::ParticleSystem>();

        // Register scene components
        world_.component<SceneEntity>();
        world_.component<Spatial>();

        // Register tag components
        world_.component<Rotating>();

        // Register relationship tags
        world_.component<SceneRoot>();
        world_.component<ActiveCamera>();

        // Register WorldTransform and propagation system
        RegisterTransformSystem();

        // Set up built-in systems
        SetupMovementSystem();
        SetupRotationSystem();
        SetupCameraSystem();
        SetupHierarchySystem();
        SetupSpatialSystem();
        SetupTransformSystem();
    }

    // Get the underlying flecs world
    flecs::world &ECSWorld::GetWorld() { return world_; }
    [[nodiscard]] const flecs::world &ECSWorld::GetWorld() const { return world_; }

    // === ENTITY CREATION ===

    // Create an entity
    [[nodiscard]] flecs::entity ECSWorld::CreateEntity() const {
        const auto entity = world_.entity();
        entity.set<Transform>({});
        entity.set<WorldTransform>({});
        return entity;
    }

    // Create an entity with a name
    flecs::entity ECSWorld::CreateEntity(const char *name) const {
        const auto entity = world_.entity(name);
        entity.set<SceneEntity>({.name = name});
        entity.set<Transform>({});
        entity.set<WorldTransform>({});
        return entity;
    }

    // Create a scene root entity
    flecs::entity ECSWorld::CreateSceneRoot(const char *name) const {
        const auto entity = CreateEntity(name);
        entity.add<SceneRoot>();
        return entity;
    }

    // === HIERARCHY MANAGEMENT ===

    // Set parent-child relationship using flecs built-in ChildOf
    void ECSWorld::SetParent(const flecs::entity child, const flecs::entity parent) {
        child.child_of(parent);
    }

    // Remove parent relationship
    void ECSWorld::RemoveParent(const flecs::entity child) {
        child.remove(flecs::ChildOf, flecs::Wildcard);
    }

    // Get parent entity (returns invalid entity if no parent)
    flecs::entity ECSWorld::GetParent(const flecs::entity entity) {
        return entity.parent();
    }

    // Get all children of an entity
    std::vector<flecs::entity> ECSWorld::GetChildren(const flecs::entity parent) {
        std::vector<flecs::entity> children;
        parent.children([&](const flecs::entity child) {
            children.push_back(child);
        });
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
    flecs::entity ECSWorld::FindEntityByName(const char *name, const flecs::entity root) {
        flecs::entity found;

        const auto query = world_.query_builder<const SceneEntity>()
                .build();

        query.each([&](const flecs::entity entity, const SceneEntity &scene_entity) {
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
            if (current == ancestor) { return true; }
            current = current.parent();
        }
        return false;
    }

    // === CAMERA MANAGEMENT ===

    void ECSWorld::SetActiveCamera(const flecs::entity camera) {
        // Remove ActiveCamera tag from all entities
        world_.query_builder<>()
                .with<ActiveCamera>()
                .build()
                .each([](const flecs::entity entity) {
                    entity.remove<ActiveCamera>();
                });

        // Add ActiveCamera tag to new camera
        if (camera.is_valid()) {
            camera.add<ActiveCamera>();
            active_camera_ = camera;
        }
    }

    [[nodiscard]] flecs::entity ECSWorld::GetActiveCamera() const {
        return active_camera_;
    }

    // === SPATIAL QUERIES ===

    // Find entities at a point
    [[nodiscard]] std::vector<flecs::entity> ECSWorld::QueryPoint(const glm::vec3 &point,
                                                                  const uint32_t layer_mask) const {
        std::vector<flecs::entity> result;

        const auto query = world_.query<const
            Transform, const
            Spatial>();
        query.each([&](const flecs::entity entity, const Transform &transform, const Spatial &spatial) {
            if ((spatial.spatial_layer & layer_mask) == 0) { return; }

            // Transform bounding box to world space
            const glm::vec3 world_min = transform.position + spatial.bounding_min;
            const glm::vec3 world_max = transform.position + spatial.bounding_max;

            if (point.x >= world_min.x && point.x <= world_max.x &&
                point.y >= world_min.y && point.y <= world_max.y &&
                point.z >= world_min.z && point.z <= world_max.z) {
                result.push_back(entity);
            }
        });

        return result;
    }

    // Find entities in sphere
    [[nodiscard]] std::vector<flecs::entity> ECSWorld::QuerySphere(const glm::vec3 &center, const float radius,
                                                                   const uint32_t layer_mask) const {
        std::vector<flecs::entity> result;
        const float radius_sq = radius * radius;

        const auto query = world_.query<const
            Transform, const
            Spatial>();
        query.each([&](const flecs::entity entity, const Transform &transform, const Spatial &spatial) {
            if ((spatial.spatial_layer & layer_mask) == 0) { return; }

            // Simple distance check to entity center
            if (const float dist_sq = glm::distance2(transform.position, center); dist_sq <= radius_sq) {
                result.push_back(entity);
            }
        });

        return result;
    }

    // Progress the world (run systems)
    void ECSWorld::Progress(const float delta_time) const {
        world_.progress(delta_time);
    }

    // Submit render commands for all renderable entities
    void ECSWorld::SubmitRenderCommands(const rendering::Renderer &renderer) {
        const flecs::entity camera_entity = GetActiveCamera();
        const auto camera_comp = camera_entity.get<Camera>();

        const auto renderable_query = world_.query<const WorldTransform, const rendering::Renderable>();
        renderable_query.each(
            [&](flecs::iter, size_t, const WorldTransform &transform, const rendering::Renderable &renderable) {
                if (!renderable.visible) {
                    return;
                }
                rendering::RenderCommand cmd{.mesh = renderable.mesh, .material = renderable.material};

                const auto model = camera_comp.projection_matrix * camera_comp.view_matrix * transform.matrix;

                cmd.transform = model;

                renderer.SubmitRenderCommand(cmd);
            });
    }

    void ECSWorld::SetupMovementSystem() const {
        // System to update positions based on velocity
        world_.system<Transform, const Velocity>()
                .each([](const flecs::iter itr, size_t, Transform &transform, const Velocity &velocity) {
                    transform.position += velocity.linear * itr.delta_time();
                    transform.rotation += velocity.angular * itr.delta_time();
                    transform.dirty = true;
                });
    }

    void ECSWorld::SetupRotationSystem() const {
        // System for entities with Rotating tag - simple rotation animation
        world_.system<Transform, Rotating>()
                .each([](const flecs::iter itr, size_t, Transform &transform, Rotating) {
                    transform.rotation.y += 1.0F * itr.delta_time(); // 1 radian per second
                    transform.dirty = true;
                });
    }

    void ECSWorld::SetupCameraSystem() const {
        // System to update camera matrices when dirty
        world_.system<Transform, Camera>()
                .each([](flecs::iter, size_t, const Transform &transform, Camera &camera) {
                    if (!camera.dirty && !transform.dirty) {
                        return;
                    }
                    // Update view matrix
                    camera.view_matrix = glm::lookAt(transform.position, camera.target, camera.up);

                    // Update projection matrix
                    camera.projection_matrix = glm::perspective(
                        glm::radians(camera.fov),
                        camera.aspect_ratio,
                        camera.near_plane,
                        camera.far_plane
                    );

                    camera.dirty = false;
                });
    }

    void ECSWorld::SetupHierarchySystem() const {
        // System to propagate transform changes down the hierarchy
        world_.system<Transform>()
                .with(flecs::ChildOf, flecs::Wildcard)
                .each([](const flecs::entity entity, Transform &transform) {
                    if (const auto parent = entity.parent(); parent.is_valid()) {
                        if (const auto parent_transform = parent.get<Transform>(); parent_transform.dirty) {
                            transform.dirty = true;
                        }
                    }
                });
    }

    void ECSWorld::SetupSpatialSystem() const {
        // System to update spatial bounds when transforms change
        world_.system<const Transform, Spatial>()
                .each([](flecs::entity, const Transform &transform, Spatial &spatial) {
                    if (transform.dirty) {
                        spatial.bounds_dirty = true;
                    }
                });
    }

    void ECSWorld::SetupTransformSystem() const {
        // System to update transform matrix
        world_.system<Transform>()
                .each([](const flecs::iter, size_t, Transform &transform) {
                    component_helpers::UpdateTransformMatrix(transform);
                });
    }

    void ECSWorld::RegisterTransformSystem() const {
        world_.system<const Transform, WorldTransform>("TransformPropagation")
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
}
