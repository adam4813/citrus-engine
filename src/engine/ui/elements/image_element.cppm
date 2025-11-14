module;

#include <memory>

export module engine.ui:elements.image;

import :ui_element;
import :uitypes;
import glm;

export namespace engine::ui::elements {
using namespace engine::rendering;
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
		 * @brief Construct Image with bounds
		 * @param x Relative X position
		 * @param y Relative Y position
		 * @param width Image width
		 * @param height Image height
		 */
	Image(float x, float y, float width, float height) : UIElement(x, y, width, height) {}

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

		// Skip if not visible
		if (!IsVisible()) {
			return;
		}

		// Render sprite if present
		if (sprite_) {
			const auto bounds = GetAbsoluteBounds();

			// Create sprite command with UIElement bounds
			SpriteRenderCommand command;
			command.texture = sprite_->texture;
			command.position = glm::vec2(bounds.x, bounds.y);
			command.size = glm::vec2(bounds.width, bounds.height);
			command.rotation = sprite_->rotation;
			command.color = sprite_->color;
			command.texture_offset = sprite_->texture_offset;
			command.texture_scale = sprite_->texture_scale;
			command.layer = sprite_->layer;

			// Apply UI-specific transformations
			if (sprite_->flip_x) {
				command.texture_scale.x *= -1.0f;
			}
			if (sprite_->flip_y) {
				command.texture_scale.y *= -1.0f;
			}

			// Submit to renderer
			auto& renderer = GetRenderer();
			renderer.SubmitSprite(command);
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
