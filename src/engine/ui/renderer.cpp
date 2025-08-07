module;

#include <vector>
#include <memory>
#include <cassert>

module engine.ui;

import :uitypes;
import engine.rendering;

namespace engine::ui {
    namespace {
        rendering::SpriteRenderCommand CreateSpriteCommand(const Sprite &sprite) {
            rendering::SpriteRenderCommand command;
            command.texture = sprite.texture;
            command.position = sprite.position;
            command.size = sprite.size;
            command.rotation = sprite.rotation;
            command.color = sprite.color;
            command.texture_offset = sprite.texture_offset;
            command.texture_scale = sprite.texture_scale;
            command.layer = sprite.layer;

            // Apply UI-specific transformations
            if (sprite.flip_x) {
                command.texture_scale.x *= -1.0f;
            }
            if (sprite.flip_y) {
                command.texture_scale.y *= -1.0f;
            }

            // TODO: Apply pivot transformation to position
            // This would adjust the position based on the pivot point

            return command;
        }
    }

    UIRenderer::~UIRenderer() {
        if (initialized_) {
            Shutdown();
        }
    }

    bool UIRenderer::Initialize() {
        assert(!initialized_);

        // No OpenGL resources needed - we use the main renderer
        initialized_ = true;
        return initialized_;
    }

    void UIRenderer::Shutdown() {
        assert(initialized_);
        sprites_.clear();
        initialized_ = false;
    }

    void UIRenderer::AddSprite(const Sprite &sprite) {
        assert(initialized_);
        sprites_.push_back(sprite);
    }

    void UIRenderer::ClearSprites() {
        sprites_.clear();
    }

    void UIRenderer::Render() {
        assert(initialized_);

        if (sprites_.empty()) {
            return;
        }

        // Submit all sprites to the main renderer
        auto &renderer = rendering::GetRenderer();
        for (const auto &sprite: sprites_) {
            auto command = CreateSpriteCommand(sprite);
            renderer.SubmitSprite(command);
        }
    }
} // namespace engine::ui
