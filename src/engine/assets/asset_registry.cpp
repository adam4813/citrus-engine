module;

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

module engine.asset_registry;

import engine.rendering;
import engine.platform;
import engine.assets;

namespace engine::assets {

AssetRegistry& AssetRegistry::Instance() {
	static AssetRegistry instance;

	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		ShaderAssetInfo::RegisterType();
		MeshAssetInfo::RegisterType();
		TextureAssetInfo::RegisterType();
		MaterialAssetInfo::RegisterType();
		AnimationAssetInfo::RegisterType();
		SoundAssetInfo::RegisterType();
		DataTableAssetInfo::RegisterType();
		PrefabAssetInfo::RegisterType();
	}

	return instance;
}

std::shared_ptr<AssetInfo> AssetRegistry::FromJson(const nlohmann::json& j) const {
	const std::string type_str = j.value("type", "");
	if (type_str.empty()) {
		std::cerr << "AssetRegistry: Missing asset type \n";
		return nullptr;
	}
	const auto* type_info = GetTypeInfo(type_str);
	if (!type_info || !type_info->create_default_factory) {
		std::cerr << "AssetRegistry: Unknown asset type '" << type_str << "'" << '\n';
		return nullptr;
	}

	auto asset = type_info->create_default_factory();
	if (!asset) {
		std::cerr << "AssetRegistry: Failed to create asset of type '" << type_str << "'" << '\n';
	}

	asset->FromJson(j); // Populate common fields like name and type
	return asset;
}

void AssetRegistry::ToJson(nlohmann::json& j, const std::shared_ptr<AssetInfo>& asset) const {
	if (!asset) {
		return;
	}

	if (const auto* type_info = GetTypeInfo(asset->type)) {
		j["type"] = type_info->type_name; // Ensure type is included in JSON
	}
	else {
		std::cerr << "AssetRegistry: Nothing registered for asset named '" << asset->name << "' serializing what we can"
				  << '\n';
	}

	asset->ToJson(j);
	j["name"] = asset->name; // Include common name field
}

std::shared_ptr<AssetInfo> AssetRegistry::CreateDefault(const AssetType type) const {
	for (const auto& info : types_) {
		if (info.asset_type == type && info.create_default_factory) {
			return info.create_default_factory();
		}
	}
	return nullptr;
}

const AssetTypeInfo* AssetRegistry::GetTypeInfo(const AssetType type) const {
	for (const auto& info : types_) {
		if (info.asset_type == type) {
			return &info;
		}
	}
	return nullptr;
}

const AssetTypeInfo* AssetRegistry::GetTypeInfo(const std::string& type_name) const {
	for (const auto& info : types_) {
		if (info.type_name == type_name) {
			return &info;
		}
	}
	return nullptr;
}

void AssetRegistry::AddTypeInfo(AssetTypeInfo info) { types_.push_back(std::move(info)); }

void AssetInfo::Initialize() {
	if (initialized_) {
		return; // Already initialized
	}
	DoInitialize();
	initialized_ = true;
}

bool AssetInfo::Load() {
	if (loaded_) {
		return true; // Already loaded
	}
	// Ensure initialized before loading
	if (!initialized_) {
		Initialize();
	}
	if (DoLoad()) {
		loaded_ = true;
		return true;
	}
	return false;
}
void AssetInfo::Unload() {
	if (!loaded_) {
		return; // Not loaded
	}
	DoUnload();
	loaded_ = false;
}
void AssetInfo::FromJson(const nlohmann::json& j) {
	name = j.value("name", name);
	if (const auto type_str = j.value("type", ""); !type_str.empty()) {
		if (const auto* type_info = AssetRegistry::Instance().GetTypeInfo(type_str); type_info != nullptr) {
			type = type_info->asset_type;
		}
	}
}

void AssetInfo::ToJson(nlohmann::json& j) {
	j["name"] = name;
	j["type"] = GetTypeName();
}

void ShaderAssetInfo::DoInitialize() {
	const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
	id = shader_mgr.CreateShader(name);
	std::cout << "ShaderAssetInfo: Created shader slot '" << name << "' (id=" << id << ")" << '\n';
}

bool ShaderAssetInfo::DoLoad() {
	if (id == rendering::INVALID_SHADER) {
		std::cerr << "ShaderAssetInfo: Cannot load - shader not initialized" << '\n';
		return false;
	}

	if (const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
		!shader_mgr.CompileShader(id, platform::fs::Path(vertex_path), platform::fs::Path(fragment_path))) {
		std::cerr << "ShaderAssetInfo: Failed to compile shader '" << name << "'" << '\n';
		return false;
	}
	std::cout << "ShaderAssetInfo: Compiled shader '" << name << "' (id=" << id << ")" << '\n';
	return true;
}

void ShaderAssetInfo::DoUnload() {
	const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
	shader_mgr.DestroyShader(id);
	std::cout << "ShaderAssetInfo: Unloaded shader '" << name << "' (id=" << id << ")" << '\n';
}

void ShaderAssetInfo::FromJson(const nlohmann::json& j) {
	vertex_path = j.value("vertex_path", "");
	fragment_path = j.value("fragment_path", "");
	AssetInfo::FromJson(j);
}

void ShaderAssetInfo::ToJson(nlohmann::json& j) {
	j["vertex_path"] = vertex_path;
	j["fragment_path"] = fragment_path;
	AssetInfo::ToJson(j);
}

void ShaderAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<ShaderAssetInfo>(ShaderAssetInfo::TYPE_NAME, AssetType::SHADER)
			.DisplayName("Shader")
			.Category("Rendering")
			.Field("name", &ShaderAssetInfo::name, "Name")
			.Field("vertex_path", &ShaderAssetInfo::vertex_path, "Vertex Shader", ecs::FieldType::FilePath)
			.Field("fragment_path", &ShaderAssetInfo::fragment_path, "Fragment Shader", ecs::FieldType::FilePath)
			.Build();
}

void MeshAssetInfo::DoInitialize() {
	const auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();
	id = mesh_mgr.CreateNamedMesh(name);
	std::cout << "MeshAssetInfo: Reserved mesh slot '" << name << "' (id=" << id << ")" << '\n';
}

bool MeshAssetInfo::DoLoad() {
	auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();

	bool success = false;
	if (mesh_type == mesh_types::QUAD) {
		success = mesh_mgr.GenerateQuad(id, params[0], params[1]);
	}
	else if (mesh_type == mesh_types::CUBE) {
		success = mesh_mgr.GenerateCube(id, params[0], params[1], params[2]);
	}
	else if (mesh_type == mesh_types::SPHERE) {
		success = mesh_mgr.GenerateSphere(id, params[0], static_cast<uint32_t>(params[1]));
	}
	else if (mesh_type == mesh_types::CAPSULE) {
		std::cerr << "MeshAssetInfo: Capsule mesh not yet implemented" << '\n';
		return false;
	}
	else if (mesh_type == mesh_types::FILE) {
		std::cerr << "MeshAssetInfo: File mesh loading not yet implemented: " << file_path << '\n';
		return false;
	}
	else {
		std::cerr << "MeshAssetInfo: Unknown mesh type: " << mesh_type << '\n';
		return false;
	}

	if (!success) {
		std::cerr << "MeshAssetInfo: Failed to generate mesh geometry for '" << name << "'" << '\n';
		return false;
	}

	std::cout << "MeshAssetInfo: Generated mesh '" << name << "' (type=" << mesh_type << ", id=" << id << ")" << '\n';
	return true;
}

void MeshAssetInfo::DoUnload() {
	const auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();
	mesh_mgr.DestroyMesh(id);
	std::cout << "MeshAssetInfo: Unloaded mesh '" << name << "' (id=" << id << ")" << '\n';
}

void MeshAssetInfo::FromJson(const nlohmann::json& j) {
	mesh_type = j.value("mesh_type", mesh_types::QUAD);
	if (j.contains("params") && j["params"].is_array()) {
		const auto& arr = j["params"];
		if (!arr.empty()) {
			params[0] = arr[0].get<float>();
		}
		if (arr.size() >= 2) {
			params[1] = arr[1].get<float>();
		}
		if (arr.size() >= 3) {
			params[2] = arr[2].get<float>();
		}
	}
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void MeshAssetInfo::ToJson(nlohmann::json& j) {
	j["mesh_type"] = mesh_type;
	j["params"] = {params[0], params[1], params[2]};
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void MeshAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<MeshAssetInfo>(MeshAssetInfo::TYPE_NAME, AssetType::MESH)
			.DisplayName("Mesh")
			.Category("Rendering")
			.Field("name", &MeshAssetInfo::name, "Name")
			.Field("mesh_type", &MeshAssetInfo::mesh_type, "Mesh Type", ecs::FieldType::Selection)
			.Options({mesh_types::QUAD, mesh_types::CUBE, mesh_types::SPHERE, mesh_types::CAPSULE, mesh_types::FILE})
			.Field("file_path", &MeshAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

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

// === AnimationAssetInfo ===
void AnimationAssetInfo::DoInitialize() {
	// Stub: no initialization needed yet
}

bool AnimationAssetInfo::DoLoad() {
	// Stub: load animation clip data when animation system is available
	return true;
}

void AnimationAssetInfo::FromJson(const nlohmann::json& j) {
	clip_path = j.value("clip_path", "");
	AssetInfo::FromJson(j);
}

void AnimationAssetInfo::ToJson(nlohmann::json& j) {
	j["clip_path"] = clip_path;
	AssetInfo::ToJson(j);
}

void AnimationAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<AnimationAssetInfo>(AnimationAssetInfo::TYPE_NAME, AssetType::ANIMATION_CLIP)
			.DisplayName("Animation Clip")
			.Category("Animation")
			.Field("name", &AnimationAssetInfo::name, "Name")
			.Field("clip_path", &AnimationAssetInfo::clip_path, "Clip Path", ecs::FieldType::FilePath)
			.Build();
}

// === SoundAssetInfo ===
void SoundAssetInfo::DoInitialize() {
	// Audio clips are loaded on-demand by the SoundRef observer in the ECS system
}

bool SoundAssetInfo::DoLoad() {
	// Audio clips are loaded on-demand by the SoundRef observer in the ECS system
	return true;
}

void SoundAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	volume = j.value("volume", 1.0f);
	loop = j.value("loop", false);
	AssetInfo::FromJson(j);
}

void SoundAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	j["volume"] = volume;
	j["loop"] = loop;
	AssetInfo::ToJson(j);
}

void SoundAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<SoundAssetInfo>(SoundAssetInfo::TYPE_NAME, AssetType::SOUND)
			.DisplayName("Sound")
			.Category("Audio")
			.Field("name", &SoundAssetInfo::name, "Name")
			.Field("file_path", &SoundAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Field("volume", &SoundAssetInfo::volume, "Volume")
			.Field("loop", &SoundAssetInfo::loop, "Loop")
			.Build();
}

// === DataTableAssetInfo ===
void DataTableAssetInfo::DoInitialize() {
	// Stub: no data table system yet
}

bool DataTableAssetInfo::DoLoad() {
	// Stub: load JSON data when data table system is available
	return true;
}

void DataTableAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	schema_name = j.value("schema_name", "");
	AssetInfo::FromJson(j);
}

void DataTableAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	j["schema_name"] = schema_name;
	AssetInfo::ToJson(j);
}

void DataTableAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<DataTableAssetInfo>(DataTableAssetInfo::TYPE_NAME, AssetType::DATA_TABLE)
			.DisplayName("Data Table")
			.Category("Data")
			.Field("name", &DataTableAssetInfo::name, "Name")
			.Field("file_path", &DataTableAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Field("schema_name", &DataTableAssetInfo::schema_name, "Schema Name")
			.Build();
}

// === PrefabAssetInfo ===
void PrefabAssetInfo::DoInitialize() {
	// Stub: prefab initialization handled by PrefabUtility
}

bool PrefabAssetInfo::DoLoad() {
	// Stub: prefab loading handled by PrefabUtility
	return true;
}

void PrefabAssetInfo::FromJson(const nlohmann::json& j) {
	file_path = j.value("file_path", "");
	AssetInfo::FromJson(j);
}

void PrefabAssetInfo::ToJson(nlohmann::json& j) {
	j["file_path"] = file_path;
	AssetInfo::ToJson(j);
}

void PrefabAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<PrefabAssetInfo>(PrefabAssetInfo::TYPE_NAME, AssetType::PREFAB)
			.DisplayName("Prefab")
			.Category("Scene")
			.Field("name", &PrefabAssetInfo::name, "Name")
			.Field("file_path", &PrefabAssetInfo::file_path, "File Path", ecs::FieldType::FilePath)
			.Build();
}

// --- AssetCache ---

AssetCache& AssetCache::Instance() {
	static AssetCache instance;
	return instance;
}

void AssetCache::Add(AssetPtr asset) {
	if (!asset)
		return;
	asset->Initialize();
	cache_[asset->name] = std::move(asset);
}

bool AssetCache::Remove(const std::string& name, const AssetType type) {
	auto it = cache_.find(name);
	if (it != cache_.end() && it->second && it->second->type == type) {
		cache_.erase(it);
		return true;
	}
	return false;
}

AssetPtr AssetCache::Find(const std::string& name, const AssetType type) {
	auto it = cache_.find(name);
	if (it != cache_.end() && it->second && it->second->type == type) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<const AssetInfo> AssetCache::Find(const std::string& name, const AssetType type) const {
	auto it = cache_.find(name);
	if (it != cache_.end() && it->second && it->second->type == type) {
		return it->second;
	}
	return nullptr;
}

std::vector<AssetPtr> AssetCache::GetAll() const {
	std::vector<AssetPtr> result;
	result.reserve(cache_.size());
	for (const auto& asset : cache_ | std::views::values) {
		result.push_back(asset);
	}
	return result;
}

std::vector<AssetPtr> AssetCache::GetByType(const AssetType type) const {
	std::vector<AssetPtr> result;
	for (const auto& asset : cache_ | std::views::values) {
		if (asset && asset->type == type) {
			result.push_back(asset);
		}
	}
	return result;
}

AssetPtr AssetCache::LoadFromFile(const std::string& path) {
	// Check if already cached
	if (const auto existing = cache_.find(path); existing != cache_.end()) {
		return existing->second;
	}

	// Read JSON from disk
	const auto text = AssetManager::LoadTextFile(platform::fs::Path(path));
	if (!text) {
		std::cerr << "AssetCache::LoadFromFile: file not found: " << path << '\n';
		return nullptr;
	}

	try {
		const auto j = nlohmann::json::parse(*text, nullptr, false);
		if (j.is_discarded()) {
			std::cerr << "AssetCache::LoadFromFile: invalid JSON in " << path << '\n';
			return nullptr;
		}

		auto asset = AssetRegistry::Instance().FromJson(j);
		if (!asset) {
			return nullptr;
		}

		asset->Load();

		cache_[path] = asset;
		return asset;
	}
	catch (const std::exception& e) {
		std::cerr << "AssetCache::LoadFromFile: error: " << e.what() << '\n';
		return nullptr;
	}
}

void AssetCache::Clear() { cache_.clear(); }

} // namespace engine::assets
