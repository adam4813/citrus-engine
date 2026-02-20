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
#include <spdlog/spdlog.h>

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

Material::Material() : pimpl_(std::make_unique<Impl>()) {}

Material::Material(const ShaderId shader) : pimpl_(std::make_unique<Impl>()) { pimpl_->shader = shader; }

Material::~Material() = default;

void Material::SetShader(const ShaderId shader) { pimpl_->shader = shader; }

void Material::SetProperty(const std::string& name, const int value) { pimpl_->int_properties[name] = value; }

void Material::SetProperty(const std::string& name, const float value) { pimpl_->float_properties[name] = value; }

void Material::SetProperty(const std::string& name, const Vec2& value) { pimpl_->vec2_properties[name] = value; }

void Material::SetProperty(const std::string& name, const Vec3& value) { pimpl_->vec3_properties[name] = value; }

void Material::SetProperty(const std::string& name, const Vec4& value) { pimpl_->vec4_properties[name] = value; }

void Material::SetTexture(const std::string& name, const TextureId texture) {
	pimpl_->texture_properties[name] = texture;
}

TextureId Material::GetTexture(const std::string& name) const {
	const auto it = pimpl_->texture_properties.find(name);
	return it != pimpl_->texture_properties.end() ? it->second : INVALID_TEXTURE;
}

ShaderId Material::GetShader() const { return pimpl_->shader; }

void Material::Apply(const Shader& shader) const {
	// Set all material properties
	for (const auto& [name, value] : pimpl_->int_properties) {
		shader.SetUniform(name, value);
	}
	for (const auto& [name, value] : pimpl_->float_properties) {
		shader.SetUniform(name, value);
	}
	for (const auto& [name, value] : pimpl_->vec2_properties) {
		shader.SetUniform(name, value);
	}
	for (const auto& [name, value] : pimpl_->vec3_properties) {
		shader.SetUniform(name, value);
	}
	for (const auto& [name, value] : pimpl_->vec4_properties) {
		shader.SetUniform(name, value);
	}

	// Bind all textures and set samplers
	int texture_unit = 0;
	for (const auto& [name, tex_id] : pimpl_->texture_properties) {
		if (tex_id == INVALID_TEXTURE) {
			continue;
		}
		const auto* gl_tex = GetGLTexture(tex_id);
		if (!gl_tex) {
			continue;
		}
		// Set the sampler uniform to this texture
		shader.SetTexture(name, gl_tex->handle, texture_unit);
		++texture_unit;
	}
	// Optionally: reset active texture to 0
	glActiveTexture(GL_TEXTURE0);
}

// MaterialManager implementation
struct MaterialManager::Impl {
	std::unordered_map<MaterialId, std::unique_ptr<Material>> materials;
	std::unordered_map<std::string, MaterialId> name_to_id;
	MaterialId next_id = 1;

	// Cached default material IDs
	MaterialId default_material_id = INVALID_MATERIAL;
	MaterialId sprite_material_id = INVALID_MATERIAL;
	MaterialId unlit_material_id = INVALID_MATERIAL;
};

MaterialManager::MaterialManager() : pimpl_(std::make_unique<Impl>()) {}

MaterialManager::~MaterialManager() = default;

void MaterialManager::Initialize(const ShaderManager& shader_manager) {
	// We need access to the renderer to get the white texture
	// For now, we'll set a default u_Texture property in materials

	// Create default 3D material (lit, with basic properties)
	if (const ShaderId default_3d_shader = shader_manager.GetDefault3DShader(); default_3d_shader != INVALID_SHADER) {
		pimpl_->default_material_id = CreateMaterial("__default_material", default_3d_shader);

		// Set default material properties
		auto& default_mat = GetMaterial(pimpl_->default_material_id);
		default_mat.SetProperty("u_Color", Vec4(1.0f, 1.0f, 1.0f, 1.0f));
		default_mat.SetProperty("u_BaseColor", Vec4(1.0f, 1.0f, 1.0f, 1.0f));
		default_mat.SetProperty("u_HasAlbedoMap", 0);
		default_mat.SetProperty("u_Shininess", 32.0f);
	}
	else {
		spdlog::warn("Default 3D shader not available, default material will not be created");
	}

	// Create sprite material suitable for 2D sprite rendering
	if (const ShaderId sprite_shader = shader_manager.GetDefault2DShader(); sprite_shader != INVALID_SHADER) {
		pimpl_->sprite_material_id = CreateMaterial("__sprite_material", sprite_shader);
	}
	else {
		spdlog::warn("Default 2D shader not available, sprite material will not be created");
	}

	// Create unlit material with no lighting calculations
	if (const ShaderId unlit_shader = shader_manager.GetUnlitShader(); unlit_shader != INVALID_SHADER) {
		pimpl_->unlit_material_id = CreateMaterial("__unlit_material", unlit_shader);

		// Set default unlit material properties
		auto& unlit_mat = GetMaterial(pimpl_->unlit_material_id);
		unlit_mat.SetProperty("u_Color", Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else {
		spdlog::warn("Unlit shader not available, unlit material will not be created");
	}
}

MaterialId MaterialManager::CreateMaterial(const std::string& name, const MaterialCreateInfo& info) const {
	if (info.shader == INVALID_SHADER) {
		spdlog::warn("Creating material '{}' with invalid shader", name);
	}

	auto material = std::make_unique<Material>(info.shader);

	for (const auto& [key, value] : info.int_properties) {
		material->SetProperty(key, value);
	}
	for (const auto& [key, value] : info.float_properties) {
		material->SetProperty(key, value);
	}
	for (const auto& [key, value] : info.vec2_properties) {
		material->SetProperty(key, value);
	}
	for (const auto& [key, value] : info.vec3_properties) {
		material->SetProperty(key, value);
	}
	for (const auto& [key, value] : info.vec4_properties) {
		material->SetProperty(key, value);
	}
	for (const auto& [key, value] : info.texture_properties) {
		material->SetTexture(key, value);
	}

	const MaterialId id = pimpl_->next_id++;
	pimpl_->name_to_id[name] = id;
	pimpl_->materials[id] = std::move(material);
	return id;
}

MaterialId MaterialManager::CreateMaterial(const std::string& name, const ShaderId shader) {
	MaterialCreateInfo info;
	info.shader = shader;
	return CreateMaterial(name, info);
}

Material& MaterialManager::GetMaterial(const MaterialId id) {
	const auto it = pimpl_->materials.find(id);
	if (it == pimpl_->materials.end()) {
		throw std::runtime_error("Invalid material ID");
	}
	return *it->second;
}

const Material& MaterialManager::GetMaterial(const MaterialId id) const {
	const auto it = pimpl_->materials.find(id);
	if (it == pimpl_->materials.end()) {
		throw std::runtime_error("Invalid material ID");
	}
	return *it->second;
}

MaterialId MaterialManager::FindMaterial(const std::string& name) const {
	const auto it = pimpl_->name_to_id.find(name);
	return it != pimpl_->name_to_id.end() ? it->second : INVALID_MATERIAL;
}

std::string MaterialManager::GetMaterialName(const MaterialId id) const {
	for (const auto& [name, mat_id] : pimpl_->name_to_id) {
		if (mat_id == id) {
			return name;
		}
	}
	return {};
}

void MaterialManager::DestroyMaterial(const MaterialId id) { pimpl_->materials.erase(id); }

bool MaterialManager::IsValid(const MaterialId id) const {
	return pimpl_->materials.find(id) != pimpl_->materials.end();
}

void MaterialManager::Clear() {
	pimpl_->materials.clear();
	pimpl_->name_to_id.clear();
}

MaterialId MaterialManager::GetDefaultMaterial() const { return pimpl_->default_material_id; }

MaterialId MaterialManager::GetSpriteMaterial() const { return pimpl_->sprite_material_id; }

MaterialId MaterialManager::GetUnlitMaterial() const { return pimpl_->unlit_material_id; }
} // namespace engine::rendering
