export module engine.ui:uitypes;

import engine.rendering;
import glm;

export namespace engine::ui {
    // Sprite component for 2D rendering
    struct Sprite {
        rendering::TextureId texture{0};
        glm::vec2 position{0.0F}; // Screen coordinates
        glm::vec2 size{1.0F};
        float rotation{0.0F};
        rendering::Color color{1.0F, 1.0F, 1.0F, 1.0F};
        glm::vec2 texture_offset{0.0F, 0.0F};
        glm::vec2 texture_scale{1.0F, 1.0F};
        int layer{0};
        // UI-specific properties
        glm::vec2 pivot{0.5F}; // 0,0 = bottom-left, 1,1 = top-right
        bool flip_x{false};
        bool flip_y{false};
    };
}
