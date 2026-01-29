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
		std::cerr << "AssetRegistry: Unknown asset type '" << type_str << "'" << std::endl;
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
	std::cout << "ShaderAssetInfo: Created shader slot '" << name << "' (id=" << id << ")" << std::endl;
}

bool ShaderAssetInfo::DoLoad() {
	if (id == rendering::INVALID_SHADER) {
		std::cerr << "ShaderAssetInfo: Cannot load - shader not initialized" << std::endl;
		return false;
	}

	if (const auto& shader_mgr = rendering::GetRenderer().GetShaderManager();
		!shader_mgr.CompileShader(id, platform::fs::Path(vertex_path), platform::fs::Path(fragment_path))) {
		std::cerr << "ShaderAssetInfo: Failed to compile shader '" << name << "'" << std::endl;
		return false;
	}
	std::cout << "ShaderAssetInfo: Compiled shader '" << name << "' (id=" << id << ")" << std::endl;
	return true;
}

void ShaderAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<ShaderAssetInfo>("shader", AssetType::SHADER)
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

bool MeshAssetInfo::DoLoad() {
	auto& mesh_mgr = rendering::GetRenderer().GetMeshManager();

	if (mesh_type == mesh_types::QUAD) {
		id = mesh_mgr.CreateQuad(params[0], params[1]);
	}
	else if (mesh_type == mesh_types::CUBE) {
		id = mesh_mgr.CreateCube(params[0], params[1], params[2]);
	}
	else if (mesh_type == mesh_types::SPHERE) {
		id = mesh_mgr.CreateSphere(params[0], static_cast<uint32_t>(params[1]));
	}
	else if (mesh_type == mesh_types::CAPSULE) {
		std::cerr << "MeshAssetInfo: Capsule mesh not yet implemented" << std::endl;
		return false;
	}
	else if (mesh_type == mesh_types::FILE) {
		std::cerr << "MeshAssetInfo: File mesh loading not yet implemented: " << file_path << std::endl;
		return false;
	}
	else {
		std::cerr << "MeshAssetInfo: Unknown mesh type: " << mesh_type << std::endl;
		return false;
	}

	if (id == rendering::INVALID_MESH) {
		std::cerr << "MeshAssetInfo: Failed to create mesh '" << name << "'" << std::endl;
		return false;
	}

	std::cout << "MeshAssetInfo: Created mesh '" << name << "' (type=" << mesh_type << ", id=" << id << ")" << std::endl;
	return true;
}

void MeshAssetInfo::RegisterType() {
	AssetRegistry::Instance()
			.RegisterType<MeshAssetInfo>("mesh", AssetType::MESH)
			.DisplayName("Mesh")
			.Category("Rendering")
			.Field("name", &MeshAssetInfo::name, "Name")
			.Field("mesh_type", &MeshAssetInfo::mesh_type, "Mesh Type")
			.Field("file_path", &MeshAssetInfo::file_path, "File Path", AssetFieldType::FilePath)
			.FromJson([](const nlohmann::json& j) -> std::unique_ptr<AssetInfo> {
				auto asset = std::make_unique<MeshAssetInfo>();
				asset->name = j.value("name", "");
				asset->mesh_type = j.value("mesh_type", mesh_types::QUAD);
				if (j.contains("params") && j["params"].is_array()) {
					const auto& arr = j["params"];
					if (arr.size() >= 1) asset->params[0] = arr[0].get<float>();
					if (arr.size() >= 2) asset->params[1] = arr[1].get<float>();
					if (arr.size() >= 3) asset->params[2] = arr[2].get<float>();
				}
				asset->file_path = j.value("file_path", "");
				return asset;
			})
			.CreateDefault([]() -> std::shared_ptr<AssetInfo> {
				return std::make_shared<MeshAssetInfo>("NewMesh", mesh_types::QUAD);
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
