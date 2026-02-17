module;

#include <cstdint>

export module engine.rendering:framebuffer;

export namespace engine::rendering {

/**
 * @brief Framebuffer for render-to-texture operations
 *
 * Provides off-screen rendering capabilities for:
 * - Editor viewport rendering
 * - Picture-in-picture camera views
 * - Screenshots
 * - Asset previews
 * - Post-processing effects
 */
class Framebuffer {
public:
	Framebuffer();
	~Framebuffer();

	// Non-copyable, movable
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;
	Framebuffer(Framebuffer&& other) noexcept;
	Framebuffer& operator=(Framebuffer&& other) noexcept;

	/**
	 * @brief Create the framebuffer with specified dimensions
	 * @param width Width in pixels
	 * @param height Height in pixels
	 * @return true if creation succeeded
	 */
	bool Create(uint32_t width, uint32_t height);

	/**
	 * @brief Destroy the framebuffer and release GPU resources
	 */
	void Destroy();

	/**
	 * @brief Resize the framebuffer (recreates attachments)
	 * @param width New width in pixels
	 * @param height New height in pixels
	 */
	void Resize(uint32_t width, uint32_t height);

	/**
	 * @brief Bind this framebuffer for rendering
	 */
	void Bind() const;

	/**
	 * @brief Unbind framebuffer (bind default framebuffer)
	 */
	static void Unbind();

	/**
	 * @brief Get the color texture ID
	 * @return OpenGL texture ID of the color attachment
	 */
	[[nodiscard]] uint32_t GetColorTextureId() const;

	/**
	 * @brief Get framebuffer width
	 */
	[[nodiscard]] uint32_t GetWidth() const;

	/**
	 * @brief Get framebuffer height
	 */
	[[nodiscard]] uint32_t GetHeight() const;

	/**
	 * @brief Check if framebuffer is valid and ready for use
	 */
	[[nodiscard]] bool IsValid() const;

private:
	uint32_t fbo_id_ = 0;
	uint32_t color_texture_id_ = 0;
	uint32_t depth_rbo_id_ = 0;
	uint32_t width_ = 0;
	uint32_t height_ = 0;
};

} // namespace engine::rendering
