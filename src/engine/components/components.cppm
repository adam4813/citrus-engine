module;

export module engine.components;

import glm;

export namespace engine::components {
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
}
