module;

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

export module engine.rendering:texture;

import :types;
import engine.platform;
import engine.assets;

export namespace engine::rendering {
enum class TextureFormat { R8, RG8, RGB8, RGBA8, R16F, RG16F, RGB16F, RGBA16F };

enum class TextureFilter {
	Nearest,
	Linear,
	NearestMipmapNearest,
	LinearMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapLinear
};

enum class TextureWrap { Repeat, MirroredRepeat, ClampToEdge };

struct TextureParameters {
	TextureFilter min_filter = TextureFilter::Linear;
	TextureFilter mag_filter = TextureFilter::Linear;
	TextureWrap wrap_s = TextureWrap::Repeat;
	TextureWrap wrap_t = TextureWrap::Repeat;
	bool generate_mipmaps = false;
};

struct TextureCreateInfo {
	uint32_t width{};
	uint32_t height{};
	TextureFormat format = TextureFormat::RGBA8;
	TextureParameters parameters = {};
	const void* data = nullptr; // Optional initial data
};

class TextureManager {
public:
	TextureManager();

	~TextureManager();

	// Create textures
	[[nodiscard]] TextureId CreateTexture(const std::string& name, const TextureCreateInfo& info) const;

	// Create a texture from an image
	[[nodiscard]] TextureId
	CreateTexture(const std::shared_ptr<assets::Image>& image, const TextureParameters& parameters = {}) const;

	// Load texture from file
	[[nodiscard]] TextureId LoadTexture(const platform::fs::Path& path, const TextureParameters& parameter = {}) const;

	// Texture operations
	static void UpdateTexture(TextureId id, const void* data, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

	void SetTextureParameters(TextureId id, const TextureParameters& parameters = {}) const;

	// Texture info
	[[nodiscard]] uint32_t GetWidth(TextureId id) const;

	[[nodiscard]] uint32_t GetHeight(TextureId id) const;

	[[nodiscard]] TextureFormat GetFormat(TextureId id) const;

	// Resource management
	void DestroyTexture(TextureId id) const;

	[[nodiscard]] bool IsValid(TextureId id) const;

	// Name-based lookup
	[[nodiscard]] TextureId FindTexture(const std::string& name) const;

	[[nodiscard]] std::string GetTextureName(TextureId id) const;

	void Clear() const;

	// Get default textures
	[[nodiscard]] TextureId GetWhiteTexture() const;

	[[nodiscard]] TextureId GetBlackTexture() const;

	[[nodiscard]] TextureId GetDefaultNormalTexture() const;

private:
	struct Impl;
	std::unique_ptr<Impl> pimpl_;
};

struct GLTexture {
	GLuint handle = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	TextureFormat format = TextureFormat::RGBA8;
};

GLTexture* GetGLTexture(TextureId id);
} // namespace engine::rendering
