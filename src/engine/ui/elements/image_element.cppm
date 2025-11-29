module;

#include <memory>

export module engine.ui:elements.image;

import :ui_element;
import :uitypes;
import engine.ui.batch_renderer;
import engine.rendering;

export namespace engine::ui::elements {

using rendering::Sprite;

/**
	 * @brief UI Image element - renders a sprite within a UIElement
	 *
	 * Image is a UIElement that displays a sprite/texture. It follows the
	 * declarative UI pattern: set sprite once, render many times.
	 *
	 * **Usage Pattern:**
	 * @code
	 * // Create image with bounds
	 * auto image = std::make_unique<Image>(10, 10, 64, 64);
	 *
	 * // Set sprite to render
	 * auto sprite = std::make_shared<Sprite>();
	 * sprite->texture = texture_id;
	 * image->SetSprite(sprite);
	 *
	 * // Render (uses UIElement bounds for positioning)
	 * image->Render();
	 * @endcode
	 *
	 * The sprite's position is automatically set to match the UIElement's
	 * absolute bounds, so the sprite renders at the correct screen location.
	 */
class Image : public UIElement {
public:
	/**
		 * @brief Default constructor for layout-managed images
		 *
		 * Creates an image with zero position and size. Use when the image
		 * will be sized by a parent layout container.
		 */
	Image() = default;

	/**
		 * @brief Construct Image for layout container (position determined by layout)
		 * @param width Image width
		 * @param height Image height
		 */
	Image(const float width, const float height) : UIElement(0, 0, width, height) {}

	/**
		 * @brief Construct Image with bounds
		 * @param x Relative X position
		 * @param y Relative Y position
		 * @param width Image width
		 * @param height Image height
		 */
	Image(const float x, const float y, const float width, const float height) : UIElement(x, y, width, height) {}

	~Image() override = default;

	/**
		 * @brief Set the sprite to be rendered
		 * @param sprite Shared pointer to sprite (can be shared across multiple Images)
		 */
	void SetSprite(const std::shared_ptr<Sprite>& sprite) { sprite_ = sprite; }

	/**
		 * @brief Clear the current sprite
		 */
	void ClearSprite() { sprite_.reset(); }

	/**
		 * @brief Render the sprite by submitting to the renderer
		 *
		 * Uses the UIElement's absolute bounds for sprite positioning,
		 * ensuring the sprite renders at the correct screen location.
		 * Children are rendered regardless of whether a sprite is present.
		 */
	void Render() const override {
		using namespace engine::ui::batch_renderer;

		// Skip if not visible
		if (!IsVisible()) {
			return;
		}

		// Render sprite if present using BatchRenderer (UI coordinate system)
		if (sprite_) {
			const auto bounds = GetAbsoluteBounds();

			// Use BatchRenderer::SubmitQuad with texture instead of world sprite system
			// This keeps everything in UI coordinate space (top-left origin)
			const Rectangle uv_rect{
					sprite_->texture_offset.x,
					sprite_->texture_offset.y,
					sprite_->texture_scale.x,
					sprite_->texture_scale.y};

			// Convert rendering::Color (glm::vec4) to ui::batch_renderer::Color
			const Color ui_color{sprite_->color.r, sprite_->color.g, sprite_->color.b, sprite_->color.a};

			BatchRenderer::SubmitQuad(
					bounds, // Screen-space rectangle (UI coords)
					ui_color, // Tint color (converted to UI color type)
					uv_rect, // Texture coordinates
					sprite_->texture // Texture ID
			);
		}

		// Always render children, even if no sprite
		for (const auto& child : GetChildren()) {
			child->Render();
		}
	}

private:
	std::shared_ptr<Sprite> sprite_;
};
} // namespace engine::ui::elements
