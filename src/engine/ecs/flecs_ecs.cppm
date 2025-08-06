module;

#include <string>
#include <vector>
#include <functional>
#include <glm/gtx/norm.hpp>
#include <flecs.h>

namespace engine::ecs {
    void RegisterTransformSystem(const flecs::world &ecs);
}

export module engine.ecs.flecs;

import engine.components;
import engine.rendering;
import glm;

export namespace engine::ecs {
    // Re-export flecs types for convenience
    using World = flecs::world;
    using Entity = flecs::entity;
    using Query = flecs::query_base;
    using Camera = components::Camera;

    // === CORE COMPONENTS ===

    // Transform component for position, rotation, scale
    struct Transform {
        glm::vec3 position{0.0F};
        glm::vec3 rotation{0.0F}; // Euler angles in radians
        glm::vec3 scale{1.0F};
        glm::mat4 world_matrix{1.0F}; // Cached world transform matrix
        bool dirty = true;

        // Helper methods
        glm::mat4 GetMatrix() const { return world_matrix; }

        void Translate(const glm::vec3 &offset) {
            position += offset;
            dirty = true;
        }

        void Rotate(const glm::vec3 &euler_angles) {
            rotation += euler_angles;
            dirty = true;
        }

        void SetScale(const glm::vec3 &new_scale) {
            scale = new_scale;
            dirty = true;
        }

        void SetScale(float uniform_scale) {
            scale = glm::vec3(uniform_scale);
            dirty = true;
        }
    };

    struct WorldTransform {
        glm::mat4 matrix{1.0F};
    };

    // Velocity component for movement
    struct Velocity {
        glm::vec3 linear{0.0F};
        glm::vec3 angular{0.0F}; // Angular velocity in radians/sec
    };

    // === RENDERING COMPONENTS ===

    // Core rendering component - every renderable entity needs this
    struct Renderable {
        engine::rendering::MeshId mesh{0};
        engine::rendering::MaterialId material{0};
        bool visible{true};
        uint32_t render_layer{0};
        float alpha{1.0F};
    };


    // Sprite component for 2D rendering
    struct Sprite {
        engine::rendering::TextureId texture{0};
        glm::vec2 size{1.0F};
        glm::vec2 pivot{0.5F}; // 0,0 = bottom-left, 1,1 = top-right
        engine::rendering::Color tint{1.0F, 1.0F, 1.0F, 1.0F};
        bool flip_x{false};
        bool flip_y{false};
    };

    // Light component for lighting calculations
    struct Light {
        enum class Type : int {
            Directional = 0,
            Point = 1,
            Spot = 2
        };

        Type type{Type::Directional};
        engine::rendering::Color color{1.0F, 1.0F, 1.0F, 1.0F};
        float intensity{1.0F};
        // Point/Spot light parameters
        float range{10.0F};
        float attenuation{1.0F};
        // Spot light parameters
        float spot_angle{45.0F};
        float spot_falloff{1.0F};
        // Directional light direction
        glm::vec3 direction{0.0F, -1.0F, 0.0F};
    };

    // Animation component for animated meshes/sprites
    struct Animation {
        char current_animation[64] = ""; // Fixed-size string for POD compatibility
        float animation_time{0.0F};
        float animation_speed{1.0F};
        bool looping{true};
        bool playing{false};
    };

    // Particle system component
    struct ParticleSystem {
        uint32_t max_particles{100};
        uint32_t active_particles{0};
        float emission_rate{10.0F};
        float particle_lifetime{1.0F};
        glm::vec3 velocity{0.0F};
        glm::vec3 gravity{0.0F, -9.81F, 0.0F};
        engine::rendering::Color start_color{1.0F, 1.0F, 1.0F, 1.0F};
        engine::rendering::Color end_color{1.0F, 1.0F, 1.0F, 1.0F};
        float start_size{1.0F};
        float end_size{0.0F};
    };

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

        void UpdateBounds(const glm::vec3 &center, const glm::vec3 &extents) {
            bounding_min = center - extents;
            bounding_max = center + extents;
            bounds_dirty = true;
        }

        [[nodiscard]] bool ContainsPoint(const glm::vec3 &point) const {
            return point.x >= bounding_min.x && point.x <= bounding_max.x &&
                   point.y >= bounding_min.y && point.y <= bounding_max.y &&
                   point.z >= bounding_min.z && point.z <= bounding_max.z;
        }

        [[nodiscard]] float DistanceToPoint(const glm::vec3 &point) const {
            const glm::vec3 center = (bounding_min + bounding_max) * 0.5f;
            return glm::distance(center, point);
        }

        [[nodiscard]] bool IntersectsWith(const Spatial &other) const {
            return bounding_min.x <= other.bounding_max.x && bounding_max.x >= other.bounding_min.x &&
                   bounding_min.y <= other.bounding_max.y && bounding_max.y >= other.bounding_min.y &&
                   bounding_min.z <= other.bounding_max.z && bounding_max.z >= other.bounding_min.z;
        }
    };

    // === TAG COMPONENTS ===

    // Simple tag component for testing
    struct Rotating {
    };

    // === FLECS RELATIONSHIP TAGS ===

    // Built-in flecs relationships we'll use
    // ChildOf - for parent-child hierarchy (built into flecs)
    // DependsOn - for dependency relationships (built into flecs)

    // Custom relationship tags
    struct SceneRoot {
    }; // Tag for scene root entities
    struct ActiveCamera {
    }; // Tag for the currently active camera

    // === COMPONENT HELPER FUNCTIONS ===
    namespace component_helpers {
        // Update transform matrix
        inline void UpdateTransformMatrix(Transform &transform) {
            if (!transform.dirty) return;
            // Create transformation matrix: T * R * S
            glm::mat4 translation = glm::gtc::translate(glm::mat4(1.0F), transform.position);
            glm::mat4 rotation_x = glm::gtc::rotate(glm::mat4(1.0F), transform.rotation.x, glm::vec3(1, 0, 0));
            glm::mat4 rotation_y = glm::gtc::rotate(glm::mat4(1.0F), transform.rotation.y, glm::vec3(0, 1, 0));
            glm::mat4 rotation_z = glm::gtc::rotate(glm::mat4(1.0F), transform.rotation.z, glm::vec3(0, 0, 1));
            glm::mat4 scale_mat = glm::gtc::scale(glm::mat4(1.0F), transform.scale);
            transform.world_matrix = translation * rotation_z * rotation_y * rotation_x * scale_mat;
            transform.dirty = false;
        }

        // Update camera view matrix
        inline void UpdateCameraViewMatrix(Camera &camera, const glm::vec3 &position) {
            camera.view_matrix = glm::gtc::lookAt(position, camera.target, camera.up);
        }

        // Update camera projection matrix
        inline void UpdateCameraProjectionMatrix(Camera &camera) {
            camera.projection_matrix = glm::gtc::perspective(glm::radians(camera.fov), camera.aspect_ratio,
                                                             camera.near_plane, camera.far_plane);
        }

        // Get view-projection matrix
        inline glm::mat4 GetViewProjectionMatrix(const Camera &camera) {
            return camera.projection_matrix * camera.view_matrix;
        }
    }

    // Flecs world wrapper with engine-specific functionality
    class ECSWorld {
    private:
        flecs::world world_;
        flecs::entity active_camera_;

    public:
        ECSWorld() {
            // Register core components with flecs
            world_.component<Transform>();
            world_.component<WorldTransform>();
            world_.component<Velocity>();
            // Register rendering components
            world_.component<Renderable>();
            world_.component<Camera>();
            world_.component<Sprite>();
            world_.component<Light>();
            world_.component<Animation>();
            world_.component<ParticleSystem>();

            // Register scene components
            world_.component<SceneEntity>();
            world_.component<Spatial>();

            // Register tag components
            world_.component<Rotating>();

            // Register relationship tags
            world_.component<SceneRoot>();
            world_.component<ActiveCamera>();

            // Register WorldTransform and propagation system
            RegisterTransformSystem(world_);

            // Set up built-in systems
            SetupMovementSystem();
            SetupRotationSystem();
            SetupCameraSystem();
            SetupHierarchySystem();
            SetupSpatialSystem();
            SetupTransformSystem();
        }

        // Get the underlying flecs world
        flecs::world &GetWorld() { return world_; }
        [[nodiscard]] const flecs::world &GetWorld() const { return world_; }

        // === ENTITY CREATION ===

        // Create an entity
        [[nodiscard]] flecs::entity CreateEntity() const {
            const auto entity = world_.entity();
            entity.set<Transform>({});
            entity.set<WorldTransform>({});
            return entity;
        }

        // Create an entity with a name
        flecs::entity CreateEntity(const char *name) const {
            const auto entity = world_.entity(name);
            entity.set<SceneEntity>({.name = name});
            entity.set<Transform>({});
            entity.set<WorldTransform>({});
            return entity;
        }

        // Create a scene root entity
        flecs::entity CreateSceneRoot(const char *name) const {
            const auto entity = CreateEntity(name);
            entity.add<SceneRoot>();
            return entity;
        }

        // === HIERARCHY MANAGEMENT ===

        // Set parent-child relationship using flecs built-in ChildOf
        void SetParent(const flecs::entity child, const flecs::entity parent) {
            child.child_of(parent);
        }

        // Remove parent relationship
        void RemoveParent(const flecs::entity child) {
            child.remove(flecs::ChildOf, flecs::Wildcard);
        }

        // Get parent entity (returns invalid entity if no parent)
        flecs::entity GetParent(const flecs::entity entity) {
            return entity.parent();
        }

        // Get all children of an entity
        std::vector<flecs::entity> GetChildren(const flecs::entity parent) {
            std::vector<flecs::entity> children;
            parent.children([&](const flecs::entity child) {
                children.push_back(child);
            });
            return children;
        }

        // Get all descendants (recursive)
        std::vector<flecs::entity> GetDescendants(const flecs::entity root) {
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
        flecs::entity FindEntityByName(const char *name, const flecs::entity root = {}) {
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
        bool IsDescendantOf(const flecs::entity entity, const flecs::entity ancestor) {
            flecs::entity current = entity.parent();
            while (current.is_valid()) {
                if (current == ancestor) { return true; }
                current = current.parent();
            }
            return false;
        }

        // === CAMERA MANAGEMENT ===

        void SetActiveCamera(const flecs::entity camera) {
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

        [[nodiscard]] flecs::entity GetActiveCamera() const {
            return active_camera_;
        }

        // === SPATIAL QUERIES ===

        // Find entities at a point
        [[nodiscard]] std::vector<flecs::entity> QueryPoint(const glm::vec3 &point,
                                                            const uint32_t layer_mask = 0xFFFFFFFF) const {
            std::vector<flecs::entity> result;

            const auto query = world_.query<const Transform, const Spatial>();
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
        [[nodiscard]] std::vector<flecs::entity> QuerySphere(const glm::vec3 &center, const float radius,
                                                             const uint32_t layer_mask = 0xFFFFFFFF) const {
            std::vector<flecs::entity> result;
            const float radius_sq = radius * radius;

            const auto query = world_.query<const Transform, const Spatial>();
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
        void Progress(const float delta_time) const {
            world_.progress(delta_time);
        }

        // Query entities with specific components
        template<typename... Components>
        auto Query() const {
            return world_.query<Components...>();
        }

        // Submit render commands for all renderable entities
        void SubmitRenderCommands(const rendering::Renderer &renderer) {
            const flecs::entity camera_entity = GetActiveCamera();
            const auto camera_comp = camera_entity.get<Camera>();

            const auto renderable_query = world_.query<const WorldTransform, const Renderable>();
            renderable_query.each(
                [&](flecs::iter, size_t, const WorldTransform &transform, const Renderable &renderable) {
                    if (!renderable.visible) {
                        return;
                    }
                    rendering::RenderCommand cmd{.mesh = renderable.mesh, .material = renderable.material};

                    // Create model matrix from transform
                    /*auto model = glm::mat4(1.0F);
                    model = glm::gtc::translate(model, transform.position);
                    model = glm::gtc::rotate(model, transform.rotation.x, glm::vec3(1, 0, 0));
                    model = glm::gtc::rotate(model, transform.rotation.y, glm::vec3(0, 1, 0));
                    model = glm::gtc::rotate(model, transform.rotation.z, glm::vec3(0, 0, 1));
                    model = glm::gtc::scale(model, transform.scale);*/

                    const auto model = camera_comp.projection_matrix * camera_comp.view_matrix * transform.matrix;

                    cmd.transform = model;

                    renderer.SubmitRenderCommand(cmd);
                });
        }

    private:
        void SetupMovementSystem() const {
            // System to update positions based on velocity
            world_.system<Transform, const Velocity>()
                    .each([](const flecs::iter itr, size_t, Transform &transform, const Velocity &velocity) {
                        transform.position += velocity.linear * itr.delta_time();
                        transform.rotation += velocity.angular * itr.delta_time();
                        transform.dirty = true;
                    });
        }

        void SetupRotationSystem() const {
            // System for entities with Rotating tag - simple rotation animation
            world_.system<Transform, Rotating>()
                    .each([](const flecs::iter itr, size_t, Transform &transform, Rotating) {
                        transform.rotation.y += 1.0F * itr.delta_time(); // 1 radian per second
                        transform.dirty = true;
                    });
        }

        void SetupCameraSystem() const {
            // System to update camera matrices when dirty
            world_.system<Transform, Camera>()
                    .each([](flecs::iter, size_t, const Transform &transform, Camera &camera) {
                        if (!camera.dirty && !transform.dirty) {
                            return;
                        }
                        // Update view matrix
                        camera.view_matrix = glm::gtc::lookAt(transform.position, camera.target, camera.up);

                        // Update projection matrix
                        camera.projection_matrix = glm::gtc::perspective(
                            glm::radians(camera.fov),
                            camera.aspect_ratio,
                            camera.near_plane,
                            camera.far_plane
                        );

                        camera.dirty = false;
                    });
        }

        void SetupHierarchySystem() const {
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

        void SetupSpatialSystem() const {
            // System to update spatial bounds when transforms change
            world_.system<const Transform, Spatial>()
                    .each([](flecs::entity, const Transform &transform, Spatial &spatial) {
                        if (transform.dirty) {
                            spatial.bounds_dirty = true;
                        }
                    });
        }

        void SetupTransformSystem() const {
            // System to update transform matrix
            world_.system<Transform>()
                    .each([](const flecs::iter, size_t, Transform &transform) {
                        component_helpers::UpdateTransformMatrix(transform);
                    });
        }
    };
}
