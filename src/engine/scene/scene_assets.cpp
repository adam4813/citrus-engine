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
	// Stub: reserve texture slot if needed
}

bool TextureAssetInfo::DoLoad() {
	// Stub: load via AssetManager::LoadImage() when available
	return true;
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
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<TextureAssetInfo>("NewTexture", "");
			})
			.Build();
}

// === MaterialAssetInfo ===

void MaterialAssetInfo::ToJson(nlohmann::json& j) const {
	AssetInfo::ToJson(j);
	j["type"] = "material";
	j["shader"] = shader_name;

	if (!float_properties.empty()) {
		j["floats"] = float_properties;
	}
	if (!vec4_properties.empty()) {
		nlohmann::json vec4s;
		for (const auto& [key, v] : vec4_properties) {
			vec4s[key] = {v.x, v.y, v.z, v.w};
		}
		j["vec4s"] = vec4s;
	}
	if (!texture_properties.empty()) {
		j["textures"] = texture_properties;
	}
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

bool MaterialAssetInfo::DoLoad() {
	if (id == rendering::INVALID_MATERIAL) {
		std::cerr << "MaterialAssetInfo: Cannot load - material not initialized" << '\n';
		return false;
	}

	auto& mat_mgr = rendering::GetRenderer().GetMaterialManager();
	auto& material = mat_mgr.GetMaterial(id);

	// Apply float properties
	for (const auto& [key, value] : float_properties) {
		material.SetProperty(key, value);
	}
	// Apply vec4 properties (colors, etc.)
	for (const auto& [key, value] : vec4_properties) {
		material.SetProperty(key, value);
	}
	// Texture properties — texture loading is deferred, names stored for future binding
	// for (const auto& [uniform_name, texture_asset_name] : texture_properties) { }

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
			.Field("shader_name", &MaterialAssetInfo::shader_name, "Shader", AssetFieldType::AssetRef)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<MaterialAssetInfo>();
				asset->name = j.value("name", "");
				asset->shader_name = j.value("shader", "");

				if (j.contains("floats") && j["floats"].is_object()) {
					for (auto& [key, val] : j["floats"].items()) {
						asset->float_properties[key] = val.get<float>();
					}
				}
				if (j.contains("vec4s") && j["vec4s"].is_object()) {
					for (auto& [key, val] : j["vec4s"].items()) {
						if (val.is_array() && val.size() >= 4) {
							asset->vec4_properties[key] = rendering::Vec4(
									val[0].get<float>(), val[1].get<float>(), val[2].get<float>(),
									val[3].get<float>());
						}
					}
				}
				if (j.contains("textures") && j["textures"].is_object()) {
					for (auto& [key, val] : j["textures"].items()) {
						asset->texture_properties[key] = val.get<std::string>();
					}
				}
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				auto mat = std::make_shared<MaterialAssetInfo>("NewMaterial", "");
				mat->vec4_properties["u_Color"] = rendering::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
				mat->float_properties["u_Shininess"] = 32.0f;
				return mat;
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
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<SoundAssetInfo>("NewSound", "");
			})
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
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<PrefabAssetInfo>("NewPrefab", "");
			})
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
