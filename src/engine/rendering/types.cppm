module;

#include <cstdint>

export module engine.rendering:types;

import glm; // Ensure glm is available in the module context

export namespace engine::rendering {
    // Core math types
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;
    using Color = glm::vec4;

    // Common colors
    namespace colors {
        constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};
        constexpr Color black{0.0f, 0.0f, 0.0f, 1.0f};
        constexpr Color red{1.0f, 0.0f, 0.0f, 1.0f};
        constexpr Color green{0.0f, 1.0f, 0.0f, 1.0f};
        constexpr Color blue{0.0f, 0.0f, 1.0f, 1.0f};
        constexpr Color yellow{1.0f, 1.0f, 0.0f, 1.0f};
        constexpr Color magenta{1.0f, 0.0f, 1.0f, 1.0f};
        constexpr Color cyan{0.0f, 1.0f, 1.0f, 1.0f};
        constexpr Color transparent{0.0f, 0.0f, 0.0f, 0.0f};
    }

    // Resource handles
    using TextureId = uint32_t;
    using ShaderId = uint32_t;
    using MeshId = uint32_t;
    using MaterialId = uint32_t;
    using RenderTargetId = uint32_t;

    constexpr TextureId INVALID_TEXTURE = 0;
    constexpr ShaderId INVALID_SHADER = 0;
    constexpr MeshId INVALID_MESH = 0;
    constexpr MaterialId INVALID_MATERIAL = 0;
    constexpr RenderTargetId INVALID_RENDER_TARGET = 0;

    struct RenderCommand {
        MeshId mesh;
        ShaderId shader;
        MaterialId material;
        Mat4 transform;
        Color tint = colors::white;
        int layer = 0;
    };

    struct SpriteRenderCommand {
        TextureId texture;
        Vec2 position;
        Vec2 size;
        float rotation = 0.0f;
        Color color = colors::white;
        Vec2 texture_offset{0.0f, 0.0f};
        Vec2 texture_scale{1.0f, 1.0f};
        int layer = 0;
    };
}
