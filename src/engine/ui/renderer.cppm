module;

#include <vector>
#include <memory>

export module engine.ui:uirenderer;

import :uitypes;

export namespace engine::ui {
    class UIRenderer {
    public:
        UIRenderer() = default;

        ~UIRenderer();

        // Add a sprite to be rendered this frame
        void AddSprite(const Sprite &sprite);

        // Clear all sprites (typically called each frame)
        void ClearSprites();

        // Render all added sprites by submitting them to the main renderer
        void Render();

        // Initialize the UI rendering system
        bool Initialize();

        // Shutdown and cleanup resources
        void Shutdown();

    private:
        std::vector<Sprite> sprites_;
        bool initialized_ = false;
    };
} // namespace engine::ui
