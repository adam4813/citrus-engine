module;

#include <flecs.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

module engine.asset_registry;

import engine.rendering;
import engine.platform;
import engine.ecs.component_registry;

namespace engine::assets {

// === TextureAssetInfo ===

void TextureAssetInfo::DoInitialize() {
	// Reserve texture slot by name (will be populated in DoLoad)
}

bool TextureAssetInfo::DoLoad() {
	if (file_path.empty()) {
		return true; // No file to load (procedural textures etc.)
	}
	auto& tex_mgr = rendering::GetRenderer().GetTextureManager();
	// Check if already loaded by name
	if (id = tex_mgr.FindTexture(name); id != rendering::INVALID_TEXTURE) {
		std::cout << "TextureAssetInfo: Reusing cached texture '" << name << "' (id=" << id << ")" << '\n';
		return true;
	}
	id = tex_mgr.LoadTexture(platform::fs::Path(file_path));
	if (id == rendering::INVALID_TEXTURE) {
		std::cerr << "TextureAssetInfo: Failed to load texture '" << name << "' from " << file_path << '\n';
		return false;
	}
	std::cout << "TextureAssetInfo: Loaded texture '" << name << "' (id=" << id << ")" << '\n';
	return true;
}

void TextureAssetInfo::DoUnload() {
	if (id != rendering::INVALID_TEXTURE) {
		rendering::GetRenderer().GetTextureManager().DestroyTexture(id);
		std::cout << "TextureAssetInfo: Unloaded texture '" << name << "' (id=" << id << ")" << '\n';
		id = rendering::INVALID_TEXTURE;
	}
}

void TextureAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void TextureAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void TextureAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<TextureAssetInfo>(TextureAssetInfo::TYPE_NAME, AssetType::TEXTURE)
			.DisplayName("Texture")
			.Category("Rendering")
			.Field("name", &TextureAssetInfo::name, "Name")
			.Field("file_path", &TextureAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

// === MaterialAssetInfo ===

void MaterialAssetInfo::DoInitialize() {
	auto& mat_mgr = rendering::GetRenderer().GetMaterialManager();
	// Resolve shader by name
	rendering::ShaderId shader_id = rendering::INVALID_SHADER;
	if (!shader_name.empty()) {
		shader_id = rendering::GetRenderer().GetShaderManager().FindShader(shader_name);
	}
	id = mat_mgr.CreateMaterial(name, shader_id);
	std::cout << "MaterialAssetInfo: Created material '" << name << "' (id=" << id << ", shader=" << shader_name << ")"
			  << '\n';
}

namespace {
void BindTextureSlot(rendering::Material& material, const std::string& texture_name, const char* uniform_name) {
	if (texture_name.empty()) {
		return;
	}
	// Try loading through the asset cache first (chains Load on demand)
	if (auto tex_asset = AssetCache::Instance().FindTyped<TextureAssetInfo>(texture_name)) {
		tex_asset->Load();
		if (tex_asset->id != rendering::INVALID_TEXTURE) {
			material.SetTexture(uniform_name, tex_asset->id);
			return;
		}
	}
	// Fallback: load directly via texture manager (e.g., for raw image paths)
	auto& tex_mgr = rendering::GetRenderer().GetTextureManager();
	auto texture_id = tex_mgr.FindTexture(texture_name);
	if (texture_id == rendering::INVALID_TEXTURE) {
		texture_id = tex_mgr.LoadTexture(texture_name);
	}
	if (texture_id != rendering::INVALID_TEXTURE) {
		material.SetTexture(uniform_name, texture_id);
	}
}
} // namespace

bool MaterialAssetInfo::DoLoad() {
	if (id == rendering::INVALID_MATERIAL) {
		std::cerr << "MaterialAssetInfo: Cannot load - material not initialized" << '\n';
		return false;
	}

	auto& mat_mgr = rendering::GetRenderer().GetMaterialManager();
	auto& material = mat_mgr.GetMaterial(id);

	// Apply PBR colors
	material.SetProperty("u_BaseColor", base_color);
	material.SetProperty("u_EmissiveColor", emissive_color);

	// Apply PBR scalars
	material.SetProperty("u_MetallicFactor", metallic_factor);
	material.SetProperty("u_RoughnessFactor", roughness_factor);
	material.SetProperty("u_AOStrength", ao_strength);
	material.SetProperty("u_EmissiveIntensity", emissive_intensity);
	material.SetProperty("u_NormalStrength", normal_strength);
	material.SetProperty("u_AlphaCutoff", alpha_cutoff);

	// Bind texture slots
	BindTextureSlot(material, albedo_map, "u_AlbedoMap");
	BindTextureSlot(material, normal_map, "u_NormalMap");
	BindTextureSlot(material, metallic_map, "u_MetallicMap");
	BindTextureSlot(material, roughness_map, "u_RoughnessMap");
	BindTextureSlot(material, ao_map, "u_AOMap");
	BindTextureSlot(material, emissive_map, "u_EmissiveMap");
	BindTextureSlot(material, height_map, "u_HeightMap");

	// Set texture presence flags for shaders
	material.SetProperty("u_HasAlbedoMap", albedo_map.empty() ? 0 : 1);

	std::cout << "MaterialAssetInfo: Loaded material '" << name << "' (id=" << id << ")" << '\n';
	return true;
}

void MaterialAssetInfo::DoUnload() {
	auto& mat_mgr = rendering::GetRenderer().GetMaterialManager();
	mat_mgr.DestroyMaterial(id);
	std::cout << "MaterialAssetInfo: Unloaded material '" << name << "' (id=" << id << ")" << '\n';
}

void MaterialAssetInfo::FromJson(const nlohmann::json& j) {
	shader_name = j.value("shader", "");
	// PBR texture maps
	albedo_map = j.value("albedo_map", j.value("albedo_texture", ""));
	normal_map = j.value("normal_map", j.value("normal_texture", ""));
	metallic_map = j.value("metallic_map", j.value("metallic_texture", ""));
	roughness_map = j.value("roughness_map", j.value("roughness_texture", ""));
	ao_map = j.value("ao_map", j.value("ao_texture", ""));
	emissive_map = j.value("emissive_map", j.value("emissive_texture", ""));
	height_map = j.value("height_map", j.value("height_texture", ""));
	// PBR scalars
	metallic_factor = j.value("metallic_factor", 0.0f);
	roughness_factor = j.value("roughness_factor", 0.5f);
	ao_strength = j.value("ao_strength", 1.0f);
	emissive_intensity = j.value("emissive_intensity", 0.0f);
	normal_strength = j.value("normal_strength", 1.0f);
	alpha_cutoff = j.value("alpha_cutoff", 0.5f);
	// PBR colors
	if (j.contains("base_color") && j["base_color"].is_array() && j["base_color"].size() >= 4) {
		const auto& c = j["base_color"];
		base_color = rendering::Vec4(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
	}
	if (j.contains("emissive_color") && j["emissive_color"].is_array() && j["emissive_color"].size() >= 4) {
		const auto& c = j["emissive_color"];
		emissive_color = rendering::Vec4(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
	}
	AssetInfo::FromJson(j);
}

void MaterialAssetInfo::ToJson(nlohmann::json& j) {
	j["shader"] = shader_name;
	// PBR texture maps
	j["albedo_map"] = albedo_map;
	j["normal_map"] = normal_map;
	j["metallic_map"] = metallic_map;
	j["roughness_map"] = roughness_map;
	j["ao_map"] = ao_map;
	j["emissive_map"] = emissive_map;
	j["height_map"] = height_map;
	// PBR scalars
	j["metallic_factor"] = metallic_factor;
	j["roughness_factor"] = roughness_factor;
	j["ao_strength"] = ao_strength;
	j["emissive_intensity"] = emissive_intensity;
	j["normal_strength"] = normal_strength;
	j["alpha_cutoff"] = alpha_cutoff;
	// PBR colors
	j["base_color"] = {base_color.x, base_color.y, base_color.z, base_color.w};
	j["emissive_color"] = {emissive_color.x, emissive_color.y, emissive_color.z, emissive_color.w};
	AssetInfo::ToJson(j);
}

void MaterialAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<MaterialAssetInfo>(MaterialAssetInfo::TYPE_NAME, AssetType::MATERIAL)
			.DisplayName("Material")
			.Category("Rendering")
			.Field("name", &MaterialAssetInfo::name, "Name")
			.Field("shader_name", &MaterialAssetInfo::shader_name, "Shader")
			.AssetRef(ShaderAssetInfo::TYPE_NAME)
			// PBR colors
			.Field("base_color", &MaterialAssetInfo::base_color, "Base Color", ecs::FieldType::Color)
			.Field("emissive_color", &MaterialAssetInfo::emissive_color, "Emissive Color", ecs::FieldType::Color)
			// PBR textures
			.Field("albedo_map", &MaterialAssetInfo::albedo_map, "Albedo Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("normal_map", &MaterialAssetInfo::normal_map, "Normal Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("metallic_map", &MaterialAssetInfo::metallic_map, "Metallic Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("roughness_map", &MaterialAssetInfo::roughness_map, "Roughness Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("ao_map", &MaterialAssetInfo::ao_map, "AO Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("emissive_map", &MaterialAssetInfo::emissive_map, "Emissive Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("height_map", &MaterialAssetInfo::height_map, "Height Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			// PBR scalars
			.Field("metallic_factor", &MaterialAssetInfo::metallic_factor, "Metallic")
			.Field("roughness_factor", &MaterialAssetInfo::roughness_factor, "Roughness")
			.Field("ao_strength", &MaterialAssetInfo::ao_strength, "AO Strength")
			.Field("emissive_intensity", &MaterialAssetInfo::emissive_intensity, "Emissive Intensity")
			.Field("normal_strength", &MaterialAssetInfo::normal_strength, "Normal Strength")
			.Field("alpha_cutoff", &MaterialAssetInfo::alpha_cutoff, "Alpha Cutoff")
			.Build();
}

void MaterialAssetInfo::SetupRefBinding(flecs::world& world) {
	SetupRefBindingImpl<MaterialAssetInfo, MaterialRef, rendering::Renderable>(
			world, "MaterialRef", "Rendering", "MaterialRefResolve", MaterialAssetInfo::TYPE_NAME,
			[](const auto& asset, auto& target) { target.material = asset->id; },
			[](auto& target) { target.material = rendering::INVALID_MATERIAL; }, {".material.json"});
}

void TextureAssetInfo::SetupRefBinding(flecs::world& world) {
	auto& registry = ecs::ComponentRegistry::Instance();
	std::string TextureRef::* name_member = &AssetRefBase::name;
	registry.Register<TextureRef>("TextureRef", world)
			.Category("Rendering")
			.Field("name", name_member)
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Build();
}

} // namespace engine::assets
