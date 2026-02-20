module;

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

module engine.scene.assets;

import engine.rendering;
import engine.platform;

namespace engine::scene {

AssetRegistry& AssetRegistry::Instance() {
	static AssetRegistry instance;
	return instance;
}

void AssetRegistry::Register(const std::string& type_name, FactoryFn factory) {
	factories_[type_name] = std::move(factory);
}

std::unique_ptr<AssetInfo> AssetRegistry::Create(const nlohmann::json& j) const {
	const std::string type_str = j.value("type", "");
	if (type_str.empty()) {
		return nullptr;
	}

	const auto it = factories_.find(type_str);
	if (it == factories_.end()) {
		std::cerr << "AssetRegistry: Unknown asset type '" << type_str << "'" << '\n';
		return nullptr;
	}

	return it->second(j);
}

bool AssetRegistry::IsRegistered(const std::string& type_name) const { return factories_.contains(type_name); }

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

void AssetInfo::ToJson(nlohmann::json& j) const { j["name"] = name; }

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

std::unique_ptr<AssetInfo> AssetInfo::FromJson(const nlohmann::json& j) { return AssetRegistry::Instance().Create(j); }

void ShaderAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "shader";
	j["vertex_path"] = vertex_path;
	j["fragment_path"] = fragment_path;
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

void ShaderAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<ShaderAssetInfo>(ShaderAssetInfo::TYPE_NAME, AssetType::SHADER)
			.DisplayName("Shader")
			.Category("Rendering")
			.Field("name", &ShaderAssetInfo::name, "Name")
			.Field("vertex_path", &ShaderAssetInfo::vertex_path, "Vertex Shader", AssetFieldType::FilePath)
			.Field("fragment_path", &ShaderAssetInfo::fragment_path, "Fragment Shader", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<ShaderAssetInfo>();
				asset->name = j.value("name", "");
				asset->vertex_path = j.value("vertex_path", "");
				asset->fragment_path = j.value("fragment_path", "");
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<ShaderAssetInfo>("NewShader", "", "");
			})
			.Build();
}

void MeshAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "mesh";
	j["mesh_type"] = mesh_type;
	j["params"] = {params[0], params[1], params[2]};
	j["file_path"] = file_path;
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

void MeshAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<MeshAssetInfo>(MeshAssetInfo::TYPE_NAME, AssetType::MESH)
			.DisplayName("Mesh")
			.Category("Rendering")
			.Field("name", &MeshAssetInfo::name, "Name")
			.Field("mesh_type", &MeshAssetInfo::mesh_type, "Mesh Type", AssetFieldType::Selection)
			.Options({mesh_types::QUAD, mesh_types::CUBE, mesh_types::SPHERE, mesh_types::CAPSULE, mesh_types::FILE})
			.Field("file_path", &MeshAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<MeshAssetInfo>();
				asset->name = j.value("name", "");
				asset->mesh_type = j.value("mesh_type", mesh_types::QUAD);
				if (j.contains("params") && j["params"].is_array()) {
					const auto& arr = j["params"];
					if (!arr.empty()) {
						asset->params[0] = arr[0].get<float>();
					}
					if (arr.size() >= 2) {
						asset->params[1] = arr[1].get<float>();
					}
					if (arr.size() >= 3) {
						asset->params[2] = arr[2].get<float>();
					}
				}
				asset->file_path = j.value("file_path", "");
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<MeshAssetInfo>("NewMesh", mesh_types::QUAD);
			})
			.Build();
}

// === TextureAssetInfo ===

void TextureAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "texture";
	j["file_path"] = file_path;
}

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

void TextureAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<TextureAssetInfo>(TextureAssetInfo::TYPE_NAME, AssetType::TEXTURE)
			.DisplayName("Texture")
			.Category("Rendering")
			.Field("name", &TextureAssetInfo::name, "Name")
			.Field("file_path", &TextureAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<TextureAssetInfo>();
				asset->name = j.value("name", "");
				asset->file_path = j.value("file_path", "");
				return asset;
			})
			.CreateDefault(
					[]() -> std::shared_ptr<AssetInfo> { return std::make_shared<TextureAssetInfo>("NewTexture", ""); })
			.Build();
}

// === MaterialAssetInfo ===

void MaterialAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "material";
	j["shader"] = shader_name;

	// PBR textures
	if (!albedo_texture.empty()) { j["albedo_texture"] = albedo_texture; }
	if (!normal_texture.empty()) { j["normal_texture"] = normal_texture; }
	if (!metallic_texture.empty()) { j["metallic_texture"] = metallic_texture; }
	if (!roughness_texture.empty()) { j["roughness_texture"] = roughness_texture; }
	if (!ao_texture.empty()) { j["ao_texture"] = ao_texture; }
	if (!emissive_texture.empty()) { j["emissive_texture"] = emissive_texture; }
	if (!height_texture.empty()) { j["height_texture"] = height_texture; }

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
}

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
void BindTextureSlot(
		rendering::Material& material,
		const std::string& texture_name,
		const char* uniform_name) {
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
	BindTextureSlot(material, albedo_texture, "u_AlbedoMap");
	BindTextureSlot(material, normal_texture, "u_NormalMap");
	BindTextureSlot(material, metallic_texture, "u_MetallicMap");
	BindTextureSlot(material, roughness_texture, "u_RoughnessMap");
	BindTextureSlot(material, ao_texture, "u_AOMap");
	BindTextureSlot(material, emissive_texture, "u_EmissiveMap");
	BindTextureSlot(material, height_texture, "u_HeightMap");

	std::cout << "MaterialAssetInfo: Loaded material '" << name << "' (id=" << id << ")" << '\n';
	return true;
}

void MaterialAssetInfo::DoUnload() {
	auto& mat_mgr = rendering::GetRenderer().GetMaterialManager();
	mat_mgr.DestroyMaterial(id);
	std::cout << "MaterialAssetInfo: Unloaded material '" << name << "' (id=" << id << ")" << '\n';
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
			.Field("base_color", &MaterialAssetInfo::base_color, "Base Color", AssetFieldType::Color)
			.Field("emissive_color", &MaterialAssetInfo::emissive_color, "Emissive Color", AssetFieldType::Color)
			// PBR textures
			.Field("albedo_texture", &MaterialAssetInfo::albedo_texture, "Albedo Texture")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("normal_texture", &MaterialAssetInfo::normal_texture, "Normal Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("metallic_texture", &MaterialAssetInfo::metallic_texture, "Metallic Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("roughness_texture", &MaterialAssetInfo::roughness_texture, "Roughness Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("ao_texture", &MaterialAssetInfo::ao_texture, "AO Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("emissive_texture", &MaterialAssetInfo::emissive_texture, "Emissive Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			.Field("height_texture", &MaterialAssetInfo::height_texture, "Height Map")
			.AssetRef(TextureAssetInfo::TYPE_NAME)
			.FileExtensions({".png", ".jpg", ".jpeg", ".tga", ".bmp"})
			// PBR scalars
			.Field("metallic_factor", &MaterialAssetInfo::metallic_factor, "Metallic")
			.Field("roughness_factor", &MaterialAssetInfo::roughness_factor, "Roughness")
			.Field("ao_strength", &MaterialAssetInfo::ao_strength, "AO Strength")
			.Field("emissive_intensity", &MaterialAssetInfo::emissive_intensity, "Emissive Intensity")
			.Field("normal_strength", &MaterialAssetInfo::normal_strength, "Normal Strength")
			.Field("alpha_cutoff", &MaterialAssetInfo::alpha_cutoff, "Alpha Cutoff")
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto a = std::make_unique<MaterialAssetInfo>();
				a->name = j.value("name", "");
				a->shader_name = j.value("shader", "");
				// PBR textures
				a->albedo_texture = j.value("albedo_texture", "");
				a->normal_texture = j.value("normal_texture", "");
				a->metallic_texture = j.value("metallic_texture", "");
				a->roughness_texture = j.value("roughness_texture", "");
				a->ao_texture = j.value("ao_texture", "");
				a->emissive_texture = j.value("emissive_texture", "");
				a->height_texture = j.value("height_texture", "");
				// PBR scalars
				a->metallic_factor = j.value("metallic_factor", 0.0f);
				a->roughness_factor = j.value("roughness_factor", 0.5f);
				a->ao_strength = j.value("ao_strength", 1.0f);
				a->emissive_intensity = j.value("emissive_intensity", 0.0f);
				a->normal_strength = j.value("normal_strength", 1.0f);
				a->alpha_cutoff = j.value("alpha_cutoff", 0.5f);
				// PBR colors
				if (j.contains("base_color") && j["base_color"].is_array() && j["base_color"].size() >= 4) {
					const auto& c = j["base_color"];
					a->base_color = rendering::Vec4(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
				}
				if (j.contains("emissive_color") && j["emissive_color"].is_array() && j["emissive_color"].size() >= 4) {
					const auto& c = j["emissive_color"];
					a->emissive_color = rendering::Vec4(c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>());
				}
				return a;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<MaterialAssetInfo>("NewMaterial", "");
			})
			.Build();
}

// === AnimationAssetInfo ===

void AnimationAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "animation_clip";
	j["clip_path"] = clip_path;
}

void AnimationAssetInfo::DoInitialize() {
	// Stub: no initialization needed yet
}

bool AnimationAssetInfo::DoLoad() {
	// Stub: load animation clip data when animation system is available
	return true;
}

void AnimationAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<AnimationAssetInfo>(AnimationAssetInfo::TYPE_NAME, AssetType::ANIMATION_CLIP)
			.DisplayName("Animation Clip")
			.Category("Animation")
			.Field("name", &AnimationAssetInfo::name, "Name")
			.Field("clip_path", &AnimationAssetInfo::clip_path, "Clip Path", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<AnimationAssetInfo>();
				asset->name = j.value("name", "");
				asset->clip_path = j.value("clip_path", "");
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<AnimationAssetInfo>("NewAnimation", "");
			})
			.Build();
}

// === SoundAssetInfo ===

void SoundAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "sound";
	j["file_path"] = file_path;
	j["volume"] = volume;
	j["loop"] = loop;
}

void SoundAssetInfo::DoInitialize() {
	// Audio clips are loaded on-demand by the SoundRef observer in the ECS system
}

bool SoundAssetInfo::DoLoad() {
	// Audio clips are loaded on-demand by the SoundRef observer in the ECS system
	return true;
}

void SoundAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<SoundAssetInfo>(SoundAssetInfo::TYPE_NAME, AssetType::SOUND)
			.DisplayName("Sound")
			.Category("Audio")
			.Field("name", &SoundAssetInfo::name, "Name")
			.Field("file_path", &SoundAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.Field("volume", &SoundAssetInfo::volume, "Volume")
			.Field("loop", &SoundAssetInfo::loop, "Loop")
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<SoundAssetInfo>();
				asset->name = j.value("name", "");
				asset->file_path = j.value("file_path", "");
				asset->volume = j.value("volume", 1.0f);
				asset->loop = j.value("loop", false);
				return asset;
			})
			.CreateDefault(
					[]() -> std::shared_ptr<AssetInfo> { return std::make_shared<SoundAssetInfo>("NewSound", ""); })
			.Build();
}

// === DataTableAssetInfo ===

void DataTableAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "data_table";
	j["file_path"] = file_path;
	j["schema_name"] = schema_name;
}

void DataTableAssetInfo::DoInitialize() {
	// Stub: no data table system yet
}

bool DataTableAssetInfo::DoLoad() {
	// Stub: load JSON data when data table system is available
	return true;
}

void DataTableAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<DataTableAssetInfo>(DataTableAssetInfo::TYPE_NAME, AssetType::DATA_TABLE)
			.DisplayName("Data Table")
			.Category("Data")
			.Field("name", &DataTableAssetInfo::name, "Name")
			.Field("file_path", &DataTableAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.Field("schema_name", &DataTableAssetInfo::schema_name, "Schema Name")
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<DataTableAssetInfo>();
				asset->name = j.value("name", "");
				asset->file_path = j.value("file_path", "");
				asset->schema_name = j.value("schema_name", "");
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<DataTableAssetInfo>("NewDataTable", "");
			})
			.Build();
}

// === PrefabAssetInfo ===

void PrefabAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "prefab";
	j["file_path"] = file_path;
}

void PrefabAssetInfo::DoInitialize() {
	// Stub: prefab initialization handled by PrefabUtility
}

bool PrefabAssetInfo::DoLoad() {
	// Stub: prefab loading handled by PrefabUtility
	return true;
}

void PrefabAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<PrefabAssetInfo>(PrefabAssetInfo::TYPE_NAME, AssetType::PREFAB)
			.DisplayName("Prefab")
			.Category("Scene")
			.Field("name", &PrefabAssetInfo::name, "Name")
			.Field("file_path", &PrefabAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<PrefabAssetInfo>();
				asset->name = j.value("name", "");
				asset->file_path = j.value("file_path", "");
				return asset;
			})
			.CreateDefault(
					[]() -> std::shared_ptr<AssetInfo> { return std::make_shared<PrefabAssetInfo>("NewPrefab", ""); })
			.Build();
}

void SceneAssets::Add(AssetPtr asset) {
	if (asset) {
		asset->Initialize(); // Allocate resources (e.g., reserve shader ID)
		assets_.push_back(std::move(asset));
	}
}

bool SceneAssets::Remove(const std::string& name, const AssetType type) {
	const auto it = std::ranges::remove_if(assets_, [&](const AssetPtr& asset) {
						return asset && asset->name == name && asset->type == type;
					}).begin();
	if (it != assets_.end()) {
		assets_.erase(it, assets_.end());
		return true;
	}
	return false;
}

AssetPtr SceneAssets::Find(const std::string& name, const AssetType type) {
	for (auto& asset : assets_) {
		if (asset && asset->name == name && asset->type == type) {
			return asset;
		}
	}
	return nullptr;
}

std::shared_ptr<const AssetInfo> SceneAssets::Find(const std::string& name, const AssetType type) const {
	for (const auto& asset : assets_) {
		if (asset && asset->name == name && asset->type == type) {
			return asset;
		}
	}
	return nullptr;
}

} // namespace engine::scene
