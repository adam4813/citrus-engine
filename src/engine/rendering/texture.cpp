// Texture implementation stub
module;

#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import :types;           // Need this for TextureId, TextureCreateInfo, etc.
import :texture;         // Import own interface
import engine.platform;  // Need this for fs::Path
import engine.assets;

namespace engine::rendering {
    namespace {
        std::unordered_map<TextureId, GLTexture> g_texture_gl;
    }

    GLTexture *rendering::GetGLTexture(const TextureId id) {
        const auto it = g_texture_gl.find(id);
        return it != g_texture_gl.end() ? &it->second : nullptr;
    }

    // TextureManager implementation
    struct TextureManager::Impl {
        std::unordered_map<TextureId, TextureCreateInfo> textures;
        std::unordered_map<std::string, TextureId> texture_cache; // Cache textures by file path
        TextureId next_id = 1;
        TextureId white_texture = INVALID_TEXTURE;
        TextureId black_texture = INVALID_TEXTURE;
        TextureId default_normal_texture = INVALID_TEXTURE;
    };

    TextureManager::TextureManager() : pimpl_(std::make_unique<Impl>()) {
    }

    TextureManager::~TextureManager() = default;

    TextureId TextureManager::CreateTexture(const TextureCreateInfo &info) const {
        const TextureId id = pimpl_->next_id++;
        pimpl_->textures[id] = info;

        // OpenGL texture creation
        GLTexture gl_tex;
        glGenTextures(1, &gl_tex.handle);
        glBindTexture(GL_TEXTURE_2D, gl_tex.handle);
        gl_tex.width = info.width;
        gl_tex.height = info.height;
        gl_tex.format = info.format;

        constexpr GLenum gl_type = GL_UNSIGNED_BYTE;
        GLenum gl_format = GL_RGBA;
        switch (info.format) {
            case TextureFormat::RGB8: gl_format = GL_RGB;
                break;
            case TextureFormat::RGBA8: gl_format = GL_RGBA;
                break;
            default: break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, gl_format, info.width, info.height, 0, gl_format, gl_type, info.data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        g_texture_gl[id] = gl_tex;
        return id;
    }

    TextureId TextureManager::LoadTexture(const platform::fs::Path &path) const {
        const std::string path_str = path.string();

        // Check cache first
        if (const auto cache_it = pimpl_->texture_cache.find(path_str); cache_it != pimpl_->texture_cache.end()) {
            // Return cached texture if it's still valid
            if (IsValid(cache_it->second)) {
                return cache_it->second;
            }
            pimpl_->texture_cache.erase(cache_it);
        }

        // Load image and create texture
        const auto image = assets::AssetManager::Instance().LoadImage(path_str);
        if (!image || !image->IsValid()) {
            return INVALID_TEXTURE;
        }

        const TextureId texture_id = CreateTexture(image);

        // Cache the texture
        if (texture_id != INVALID_TEXTURE) {
            pimpl_->texture_cache[path_str] = texture_id;
        }

        return texture_id;
    }

    TextureId TextureManager::LoadTexture([[maybe_unused]] const std::string &name, [[maybe_unused]] const void *data,
                                          [[maybe_unused]] size_t size) {
        // TODO: Load texture from memory
        return INVALID_TEXTURE;
    }

    void TextureManager::UpdateTexture([[maybe_unused]] TextureId id, [[maybe_unused]] const void *data,
                                       [[maybe_unused]] uint32_t x, [[maybe_unused]] uint32_t y,
                                       [[maybe_unused]] uint32_t width,
                                       [[maybe_unused]] uint32_t height) {
        // TODO: Update texture data
    }

    void TextureManager::GenerateMipmaps([[maybe_unused]] TextureId id) {
        // TODO: Generate mipmaps
    }

    uint32_t TextureManager::GetWidth(const TextureId id) const {
        const auto it = pimpl_->textures.find(id);
        return it != pimpl_->textures.end() ? it->second.width : 0;
    }

    uint32_t TextureManager::GetHeight(const TextureId id) const {
        const auto it = pimpl_->textures.find(id);
        return it != pimpl_->textures.end() ? it->second.height : 0;
    }

    TextureFormat TextureManager::GetFormat(const TextureId id) const {
        const auto it = pimpl_->textures.find(id);
        return it != pimpl_->textures.end() ? it->second.format : TextureFormat::RGBA8;
    }

    void TextureManager::DestroyTexture(const TextureId id) const {
        for (auto it = pimpl_->texture_cache.begin(); it != pimpl_->texture_cache.end(); ++it) {
            if (it->second == id) {
                pimpl_->texture_cache.erase(it);
                break;
            }
        }
        pimpl_->textures.erase(id);

        if (const auto it = g_texture_gl.find(id); it != g_texture_gl.end()) {
            glDeleteTextures(1, &it->second.handle);
            g_texture_gl.erase(it);
        }
    }

    bool TextureManager::IsValid(const TextureId id) const {
        return pimpl_->textures.contains(id);
    }

    void TextureManager::Clear() const {
        pimpl_->textures.clear();
        pimpl_->texture_cache.clear();
        for (auto &gl_tex: g_texture_gl | std::views::values) {
            glDeleteTextures(1, &gl_tex.handle);
        }
        g_texture_gl.clear();
    }

    TextureId TextureManager::GetWhiteTexture() const {
        return pimpl_->white_texture;
    }

    TextureId TextureManager::GetBlackTexture() const {
        return pimpl_->black_texture;
    }

    TextureId TextureManager::GetDefaultNormalTexture() const {
        return pimpl_->default_normal_texture;
    }

    TextureId TextureManager::CreateTexture(const std::shared_ptr<assets::Image> &image) const {
        if (!image || !image->IsValid()) {
            return INVALID_TEXTURE;
        }
        TextureCreateInfo info;
        info.width = static_cast<uint32_t>(image->width);
        info.height = static_cast<uint32_t>(image->height);
        info.format = TextureFormat::RGBA8;
        info.data = image->pixel_data.data();
        return CreateTexture(info);
    }
} // namespace engine::rendering
