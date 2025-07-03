module;

#include <vector>
#include <algorithm>

export module engine.rendering:render_system;

import :components;
import :types;
import :renderer;
import :camera;
import engine.ecs;
import glm;

export namespace engine::rendering {
    // Render system that processes ECS components
    class RenderSystem : public engine::ecs::System<RenderSystem> {
    public:
        explicit RenderSystem(Renderer &renderer) : renderer_(renderer) {
        }

        void Update(engine::ecs::EntityManager &entity_manager, float delta_time) override {
            // Find the active camera
            CameraComponent *active_camera = FindActiveCamera(entity_manager);
            if (!active_camera) {
                return; // No camera, nothing to render
            }

            // Update camera matrices if dirty
            if (active_camera->dirty) {
                component_helpers::UpdateCameraViewMatrix(*active_camera);
                component_helpers::UpdateCameraProjectionMatrix(*active_camera);
                active_camera->dirty = false;
            }

            renderer_.BeginFrame();

            // Render 3D objects
            Render3DObjects(entity_manager, *active_camera);

            // Render 2D sprites (after 3D, no depth testing)
            Render2DSprites(entity_manager, *active_camera);

            renderer_.EndFrame();
        }

        // Set which camera entity is active
        void SetActiveCamera(engine::ecs::Entity camera_entity) {
            active_camera_entity_ = camera_entity;
        }

    private:
        Renderer &renderer_;
        engine::ecs::Entity active_camera_entity_ = engine::ecs::INVALID_ENTITY;

        // Find the active camera component
        CameraComponent *FindActiveCamera(engine::ecs::EntityManager &entity_manager) {
            if (active_camera_entity_ != engine::ecs::INVALID_ENTITY) {
                return entity_manager.TryGetComponent<CameraComponent>(active_camera_entity_);
            }

            // If no active camera set, find the first one that has both camera and transform components
            auto camera_query = entity_manager.Query<CameraComponent, TransformComponent>();
            for (auto [entity, camera, transform]: camera_query) {
                active_camera_entity_ = entity;
                return &camera;
            }

            return nullptr;
        }

        // Render all 3D objects with RenderableComponent and TransformComponent
        void Render3DObjects(engine::ecs::EntityManager &entity_manager, CameraComponent &camera) {
            // Update camera matrices if dirty
            if (camera.dirty) {
                component_helpers::UpdateCameraViewMatrix(camera);
                component_helpers::UpdateCameraProjectionMatrix(camera);
                camera.dirty = false;
            }

            // Collect all renderable objects
            std::vector<RenderData> render_queue;
            auto renderable_query = entity_manager.Query<RenderableComponent, TransformComponent>();
            for (auto [entity, renderable, transform]: renderable_query) {
                if (!renderable.visible) continue;

                // Update transform matrix if dirty
                component_helpers::UpdateTransformMatrix(transform);

                RenderData render_data;
                render_data.entity = entity;
                render_data.mesh = renderable.mesh;
                render_data.material = renderable.material;
                render_data.transform = component_helpers::GetViewProjectionMatrix(camera) * transform.world_matrix;
                render_data.layer = renderable.render_layer;
                render_data.alpha = renderable.alpha;

                render_queue.push_back(render_data);
            }

            // Sort by layer, then by material for batching
            std::sort(render_queue.begin(), render_queue.end(), [](const RenderData &a, const RenderData &b) {
                if (a.layer != b.layer) return a.layer < b.layer;
                return a.material < b.material; // Batch by material
            });

            // Submit render commands
            for (const auto &render_data: render_queue) {
                RenderCommand command;
                command.mesh = render_data.mesh;
                command.material = render_data.material;
                command.transform = render_data.transform;

                renderer_.SubmitRenderCommand(command);
            }
        }

        // Render 2D sprites
        void Render2DSprites(engine::ecs::EntityManager &entity_manager, const CameraComponent &camera) {
            auto sprite_query = entity_manager.Query<SpriteComponent, TransformComponent>();
            for (auto [entity, sprite, transform]: sprite_query) {
                // Create sprite render command
                SpriteRenderCommand command;
                command.texture = sprite.texture;
                command.position = glm::vec2(transform.position.x, transform.position.y);
                command.size = sprite.size;
                command.rotation = transform.rotation.z; // 2D rotation
                command.color = sprite.tint;
                //command.flip_x = sprite.flip_x;
                // command.flip_y = sprite.flip_y;

                renderer_.SubmitSprite(command);
            }
        }

        // Internal data structure for render queue
        struct RenderData {
            engine::ecs::Entity entity;
            MeshId mesh;
            MaterialId material;
            glm::mat4 transform;
            int layer;
            float alpha;
        };
    };

    // Camera system for updating camera transforms
    class CameraSystem : public engine::ecs::System<CameraSystem> {
    public:
        void Update(engine::ecs::EntityManager &entity_manager, float delta_time) override {
            auto camera_query = entity_manager.Query<CameraComponent, TransformComponent>();
            for (auto [entity, camera, transform]: camera_query) {
                // Update camera position if transform changed
                if (transform.dirty) {
                    camera.position = transform.position;
                    camera.dirty = true;
                }

                // Update camera matrices if needed
                if (camera.dirty) {
                    component_helpers::UpdateCameraViewMatrix(camera);
                    component_helpers::UpdateCameraProjectionMatrix(camera);
                    camera.dirty = false;
                }
            }
        }
    };

    // Animation system for animated objects
    /*class AnimationSystem : public engine::ecs::System<AnimationSystem> {
    public:
        void Update(engine::ecs::EntityManager &entity_manager, float delta_time) override {
            auto animation_query = entity_manager.Query<AnimationComponent, TransformComponent>();
            for (auto [entity, animation, transform]: animation_query) {
                if (!animation.playing) continue;

                animation.animation_time += delta_time * animation.animation_speed;

                // Simple rotation animation example
                if (animation.current_animation == "rotate") {
                    transform.SetRotation(glm::vec3(0, animation.animation_time, 0));
                }

                // Handle looping
                if (animation.looping && animation.animation_time > 2.0f * glm::gtc::pi<float>()) {
                    animation.animation_time = 0.0f;
                }
            }
        }
    };*/

    // Particle system
    /*class ParticleSystem : public engine::ecs::System<ParticleSystem> {
    public:
        void Update(engine::ecs::EntityManager &entity_manager, float delta_time) override {
            auto particle_query = entity_manager.Query<ParticleSystemComponent, TransformComponent>();
            for (auto [entity, particles, transform]: particle_query) {
                // Update particle simulation
                UpdateParticles(particles, transform, delta_time);
            }
        }

    private:
        void UpdateParticles(ParticleSystemComponent &particles, const TransformComponent &transform,
                             float delta_time) {
            // Simple particle simulation - this would be much more complex in a real system
            // For now, just update the active particle count based on emission rate
            float new_particles = particles.emission_rate * delta_time;
            particles.active_particles = std::min(particles.max_particles,
                                                  particles.active_particles + static_cast<uint32_t>(new_particles));
        }
    };*/
}
