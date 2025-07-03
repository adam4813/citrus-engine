module;

#include <memory>
#include <vector>
#include <string>
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
    enum class TextureFormat {
        R8,
        RG8,
        RGB8,
        RGBA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F
    };

    enum class TextureFilter {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    enum class TextureWrap {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    struct TextureCreateInfo {
        uint32_t width;
        uint32_t height;
        TextureFormat format = TextureFormat::RGBA8;
        TextureFilter min_filter = TextureFilter::Linear;
        TextureFilter mag_filter = TextureFilter::Linear;
        TextureWrap wrap_s = TextureWrap::Repeat;
        TextureWrap wrap_t = TextureWrap::Repeat;
        bool generate_mipmaps = false;
        const void *data = nullptr; // Optional initial data
    };

    class TextureManager {
    public:
        TextureManager();

        ~TextureManager();

        // Create textures
        TextureId CreateTexture(const TextureCreateInfo &info) const;

        TextureId CreateTexture(const std::shared_ptr<assets::Image> &image) const; // New overload
        TextureId LoadTexture(const platform::fs::Path &path) const;

        TextureId LoadTexture(const std::string &name, const void *data, size_t size);

        // Texture operations
        void UpdateTexture(TextureId id, const void *data, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        void GenerateMipmaps(TextureId id);

        // Texture info
        uint32_t GetWidth(TextureId id) const;

        uint32_t GetHeight(TextureId id) const;

        TextureFormat GetFormat(TextureId id) const;

        // Resource management
        void DestroyTexture(TextureId id) const;

        bool IsValid(TextureId id) const;

        void Clear() const;

        // Get default textures
        TextureId GetWhiteTexture() const;

        TextureId GetBlackTexture() const;

        TextureId GetDefaultNormalTexture() const;

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

    GLTexture *GetGLTexture(TextureId id);
}
