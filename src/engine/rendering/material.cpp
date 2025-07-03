// Material implementation
module;

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

module engine.rendering;

import :material;
import :types;

namespace engine::rendering {
    // Material implementation
    struct Material::Impl {
        ShaderId shader = INVALID_SHADER;
        std::unordered_map<std::string, int> int_properties;
        std::unordered_map<std::string, float> float_properties;
        std::unordered_map<std::string, Vec2> vec2_properties;
        std::unordered_map<std::string, Vec3> vec3_properties;
        std::unordered_map<std::string, Vec4> vec4_properties;
        std::unordered_map<std::string, TextureId> texture_properties;
    };

    Material::Material() : pimpl_(std::make_unique<Impl>()) {
    }

    Material::Material(const ShaderId shader) : pimpl_(std::make_unique<Impl>()) {
        pimpl_->shader = shader;
    }

    Material::~Material() = default;

    void Material::SetShader(const ShaderId shader) {
        pimpl_->shader = shader;
    }

    void Material::SetProperty(const std::string &name, const int value) {
        pimpl_->int_properties[name] = value;
    }

    void Material::SetProperty(const std::string &name, const float value) {
        pimpl_->float_properties[name] = value;
    }

    void Material::SetProperty(const std::string &name, const Vec2 &value) {
        pimpl_->vec2_properties[name] = value;
    }

    void Material::SetProperty(const std::string &name, const Vec3 &value) {
        pimpl_->vec3_properties[name] = value;
    }

    void Material::SetProperty(const std::string &name, const Vec4 &value) {
        pimpl_->vec4_properties[name] = value;
    }

    void Material::SetTexture(const std::string &name, const TextureId texture) {
        pimpl_->texture_properties[name] = texture;
    }

    TextureId Material::GetTexture(const std::string &name) const {
        const auto it = pimpl_->texture_properties.find(name);
        return it != pimpl_->texture_properties.end() ? it->second : INVALID_TEXTURE;
    }

    ShaderId Material::GetShader() const {
        return pimpl_->shader;
    }

    void Material::Apply(const Shader &shader) const {
        // Bind all textures and set uniforms
        int texture_unit = 0;
        for (const auto &[name, tex_id]: pimpl_->texture_properties) {
            if (tex_id == INVALID_TEXTURE) { continue; }
            const auto *gl_tex = GetGLTexture(tex_id);
            if (!gl_tex) { continue; }
            // Set the sampler uniform to this texture
            shader.SetTexture(name, gl_tex->handle, texture_unit);
            ++texture_unit;
        }
        // Optionally: reset active texture to 0
        glActiveTexture(GL_TEXTURE0);
    }

    // MaterialManager implementation
    struct MaterialManager::Impl {
        std::unordered_map<MaterialId, std::unique_ptr<Material> > materials;
        std::unordered_map<std::string, MaterialId> name_to_id;
        MaterialId next_id = 1;
    };

    MaterialManager::MaterialManager() : pimpl_(std::make_unique<Impl>()) {
    }

    MaterialManager::~MaterialManager() = default;

    MaterialId MaterialManager::CreateMaterial(const std::string &name, const MaterialCreateInfo &info) {
        auto material = std::make_unique<Material>(info.shader);

        for (const auto &[key, value]: info.int_properties) {
            material->SetProperty(key, value);
        }
        for (const auto &[key, value]: info.float_properties) {
            material->SetProperty(key, value);
        }
        for (const auto &[key, value]: info.vec2_properties) {
            material->SetProperty(key, value);
        }
        for (const auto &[key, value]: info.vec3_properties) {
            material->SetProperty(key, value);
        }
        for (const auto &[key, value]: info.vec4_properties) {
            material->SetProperty(key, value);
        }
        for (const auto &[key, value]: info.texture_properties) {
            material->SetTexture(key, value);
        }

        const MaterialId id = pimpl_->next_id++;
        pimpl_->name_to_id[name] = id;
        pimpl_->materials[id] = std::move(material);
        return id;
    }

    MaterialId MaterialManager::CreateMaterial(const std::string &name, const ShaderId shader) {
        MaterialCreateInfo info;
        info.shader = shader;
        return CreateMaterial(name, info);
    }

    Material &MaterialManager::GetMaterial(const MaterialId id) {
        const auto it = pimpl_->materials.find(id);
        if (it == pimpl_->materials.end()) {
            throw std::runtime_error("Invalid material ID");
        }
        return *it->second;
    }

    const Material &MaterialManager::GetMaterial(const MaterialId id) const {
        const auto it = pimpl_->materials.find(id);
        if (it == pimpl_->materials.end()) {
            throw std::runtime_error("Invalid material ID");
        }
        return *it->second;
    }

    MaterialId MaterialManager::FindMaterial(const std::string &name) const {
        const auto it = pimpl_->name_to_id.find(name);
        return it != pimpl_->name_to_id.end() ? it->second : INVALID_MATERIAL;
    }

    void MaterialManager::DestroyMaterial(const MaterialId id) {
        pimpl_->materials.erase(id);
    }

    bool MaterialManager::IsValid(const MaterialId id) const {
        return pimpl_->materials.find(id) != pimpl_->materials.end();
    }

    void MaterialManager::Clear() {
        pimpl_->materials.clear();
        pimpl_->name_to_id.clear();
    }

    MaterialId MaterialManager::GetDefaultMaterial() const {
        // TODO: Return default material ID
        return INVALID_MATERIAL;
    }

    MaterialId MaterialManager::GetSpriteMaterial() const {
        // TODO: Return sprite material ID
        return INVALID_MATERIAL;
    }

    MaterialId MaterialManager::GetUnlitMaterial() const {
        // TODO: Return unlit material ID
        return INVALID_MATERIAL;
    }
}
