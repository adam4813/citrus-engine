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

        GLint GetGLFilterMode(const TextureFilter filter) {
            switch (filter) {
                case TextureFilter::Nearest: return GL_NEAREST;
                case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
                case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
                case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
                case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
                case TextureFilter::Linear:
                default: return GL_LINEAR;
            }
        }

        GLint GetGLWrapMode(const TextureWrap wrap) {
            switch (wrap) {
                case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
                case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
                case TextureWrap::Repeat:
                default: return GL_REPEAT;
            }
        }

        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
        GLint GetGLInternalFormat(const TextureFormat format) {
            switch (format) {
                case TextureFormat::R8: return GL_R8;
                case TextureFormat::RG8: return GL_RG8;
                case TextureFormat::RGB8: return GL_RGB8;
                case TextureFormat::R16F: return GL_R16F;
                case TextureFormat::RG16F: return GL_RG16F;
                case TextureFormat::RGB16F: return GL_RGB16F;
                case TextureFormat::RGBA16F: return GL_RGBA16F;
                case TextureFormat::RGBA8:
                default: return GL_RGBA8;
            }
        }

        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
        GLenum GetGLFormat(const TextureFormat format) {
            switch (format) {
                case TextureFormat::R16F:
                case TextureFormat::R8:
                    return GL_RED;
                case TextureFormat::RG16F:
                case TextureFormat::RG8:
                    return GL_RG;
                case TextureFormat::RGB16F:
                case TextureFormat::RGB8:
                    return GL_RGB;
                case TextureFormat::RGBA16F:
                case TextureFormat::RGBA8:
                default: return GL_RGBA;
            }
        }
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

    TextureId TextureManager::CreateTexture(const std::string &name, const TextureCreateInfo &info) const {
        const TextureId id = pimpl_->next_id++;
        if (id != INVALID_TEXTURE) {
            pimpl_->textures[id] = info;
            pimpl_->texture_cache[name] = id;
        }

        GLTexture gl_tex;
        gl_tex.width = info.width;
        gl_tex.height = info.height;
        gl_tex.format = info.format;

        const GLint gl_internal_format = GetGLInternalFormat(gl_tex.format);
        const GLenum gl_format = GetGLFormat(gl_tex.format);

        glGenTextures(1, &gl_tex.handle);
        glBindTexture(GL_TEXTURE_2D, gl_tex.handle);
        glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, gl_tex.width, gl_tex.height, 0, gl_format, GL_UNSIGNED_BYTE,
                     info.data);
        glBindTexture(GL_TEXTURE_2D, 0);

        g_texture_gl[id] = gl_tex;
        SetTextureParameters(id, info.parameters);

        return id;
    }

    TextureId TextureManager::LoadTexture(const platform::fs::Path &path, const TextureParameters &parameter) const {
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

        return CreateTexture(image, parameter);
    }

    void TextureManager::UpdateTexture(const TextureId id, const void *data,
                                       const uint32_t x, const uint32_t y,
                                       const uint32_t width,
                                       const uint32_t height) {
        const auto *gl_tex = GetGLTexture(id);
        if (!gl_tex) { return; }

        if (x + width > gl_tex->width || y + height > gl_tex->height || x + width == 0 || y + height == 0) {
            // Out of bounds or no area to update
            return;
        }

        const GLenum gl_format = GetGLFormat(gl_tex->format);

        glBindTexture(GL_TEXTURE_2D, gl_tex->handle);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, gl_format, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void TextureManager::SetTextureParameters(const TextureId id, const TextureParameters &parameters) const {
        const auto *gl_tex = GetGLTexture(id);
        if (!gl_tex) { return; }

        pimpl_->textures[id].parameters = parameters;
        glBindTexture(GL_TEXTURE_2D, gl_tex->handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetGLFilterMode(parameters.min_filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetGLFilterMode(parameters.mag_filter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetGLWrapMode(parameters.wrap_s));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetGLWrapMode(parameters.wrap_t));
        if (parameters.generate_mipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
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

    TextureId TextureManager::CreateTexture(const std::shared_ptr<assets::Image> &image,
                                            const TextureParameters &parameters) const {
        if (!image || !image->IsValid()) {
            return INVALID_TEXTURE;
        }
        TextureCreateInfo info;
        info.width = static_cast<uint32_t>(image->width);
        info.height = static_cast<uint32_t>(image->height);
        info.data = image->pixel_data.data();
        info.parameters = parameters;
        return CreateTexture(image->name, info);
    }
} // namespace engine::rendering
