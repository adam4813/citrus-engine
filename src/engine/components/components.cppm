module;

export module engine.components;

import glm;

export namespace engine::components {
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

    // Simple tag component for testing
    struct Rotating {
    };

    // Camera component - shared between ECS and rendering
    struct Camera {
        glm::vec3 target{0.0F, 0.0F, 0.0F};
        glm::vec3 up{0.0F, 1.0F, 0.0F};
        float fov{60.0F};
        float aspect_ratio{16.0F / 9.0F};
        float near_plane{0.1F};
        float far_plane{100.0F};
        glm::mat4 view_matrix{1.0F};
        glm::mat4 projection_matrix{1.0F};
        bool dirty{true};
    };

    struct ActiveCamera {
    }; // Tag for the currently active camera
}
