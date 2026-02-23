module;

#include <algorithm>
#include <flecs.h>
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

void AssetRegistry::Initialize(flecs::world& world) {
	// Ensure type registration has happened
	Instance();

	// Set up ECS ref component bindings (observers that resolve name → runtime ID)
	ShaderAssetInfo::SetupRefBinding(world);
	MeshAssetInfo::SetupRefBinding(world);
	MaterialAssetInfo::SetupRefBinding(world);
	SoundAssetInfo::SetupRefBinding(world);
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
	guid = j.value("guid", 0u);
	name = j.value("name", name);
	if (const auto type_str = j.value("type", ""); !type_str.empty()) {
		if (const auto* type_info = AssetRegistry::Instance().GetTypeInfo(type_str); type_info != nullptr) {
			type = type_info->asset_type;
		}
	}
}

void AssetInfo::ToJson(nlohmann::json& j) {
	j["guid"] = guid;
	j["name"] = name;
	j["type"] = GetTypeName();
}

// --- AssetCache ---

AssetCache& AssetCache::Instance() {
	static AssetCache instance;
	return instance;
}

void AssetCache::Add(AssetPtr asset) {
	if (!asset)
		return;
	if (asset->guid == 0) {
		asset->guid = GenerateGuid();
	}
	cache_[asset->guid] = asset;
	if (!asset->name.empty()) {
		name_index_[asset->name] = asset->guid;
	}
}

bool AssetCache::Remove(const std::string& name, const AssetType type) {
	auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return false;
	}
	auto it = cache_.find(nit->second);
	if (it != cache_.end() && it->second && it->second->type == type) {
		cache_.erase(it);
		name_index_.erase(nit);
		return true;
	}
	return false;
}

AssetPtr AssetCache::Find(const uint32_t guid) {
	auto it = cache_.find(guid);
	return (it != cache_.end()) ? it->second : nullptr;
}

std::shared_ptr<const AssetInfo> AssetCache::Find(const uint32_t guid) const {
	auto it = cache_.find(guid);
	return (it != cache_.end()) ? it->second : nullptr;
}

AssetPtr AssetCache::Find(const std::string& name, const AssetType type) {
	auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return nullptr;
	}
	auto it = cache_.find(nit->second);
	if (it != cache_.end() && it->second && it->second->type == type) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<const AssetInfo> AssetCache::Find(const std::string& name, const AssetType type) const {
	auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return nullptr;
	}
	auto it = cache_.find(nit->second);
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
	// Check if this file path was already loaded
	if (const auto it = path_to_guid_.find(path); it != path_to_guid_.end()) {
		if (const auto cached = cache_.find(it->second); cached != cache_.end()) {
			return cached->second;
		}
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

		// Look up type and create asset via factory
		const std::string type_str = j.value("type", "");
		if (type_str.empty()) {
			std::cerr << "AssetCache::LoadFromFile: missing 'type' in " << path << '\n';
			return nullptr;
		}
		const auto* type_info = AssetRegistry::Instance().GetTypeInfo(type_str);
		if (!type_info || !type_info->create_default_factory) {
			std::cerr << "AssetCache::LoadFromFile: unknown asset type '" << type_str << "'" << '\n';
			return nullptr;
		}

		auto asset = type_info->create_default_factory();
		if (!asset) {
			std::cerr << "AssetCache::LoadFromFile: factory failed for type '" << type_str << "'" << '\n';
			return nullptr;
		}

		asset->FromJson(j);

		// Assign a GUID if the file didn't have one
		if (asset->guid == 0) {
			asset->guid = GenerateGuid();
		}

		// Assets are loaded on-demand when referenced by an entity (via SetupRefBinding observers)

		// Cache by GUID (primary key) with name and path indices
		cache_[asset->guid] = asset;
		if (!asset->name.empty()) {
			name_index_[asset->name] = asset->guid;
		}
		path_to_guid_[path] = asset->guid;
		return asset;
	}
	catch (const std::exception& e) {
		std::cerr << "AssetCache::LoadFromFile: error: " << e.what() << '\n';
		return nullptr;
	}
}

bool AssetCache::SaveToFile(const AssetPtr& asset, const std::string& path) {
	if (!asset) {
		return false;
	}

	// Ensure asset has a GUID before saving
	if (asset->guid == 0) {
		asset->guid = GenerateGuid();
	}

	const auto* type_info = AssetRegistry::Instance().GetTypeInfo(asset->type);
	if (!type_info) {
		std::cerr << "AssetCache::SaveToFile: unknown type for asset '" << asset->name << "'" << '\n';
		return false;
	}

	nlohmann::json j;
	asset->ToJson(j);

	return AssetManager::SaveTextFile(platform::fs::Path(path), j.dump(2));
}

uint32_t AssetCache::GenerateGuid() {
	while (cache_.contains(next_guid_)) {
		++next_guid_;
	}
	return next_guid_++;
}

void AssetCache::Clear() {
	cache_.clear();
	name_index_.clear();
	path_to_guid_.clear();
	next_guid_ = 1;
}

size_t AssetCache::ScanDirectory(const std::string& directory, const std::vector<std::string>& extensions) {
	size_t registered = 0;
	const auto dir_path = platform::fs::Path(directory);
	const auto entries = platform::fs::ListDirectory(dir_path);

	for (const auto& entry : entries) {
		// Recursively scan subdirectories
		if (platform::fs::IsDirectory(entry)) {
			registered += ScanDirectory(entry.string(), extensions);
			continue;
		}

		const auto filename = entry.filename().string();

		// Filter by extensions
		if (!extensions.empty()) {
			bool matched = false;
			for (const auto& ext : extensions) {
				if (filename.length() >= ext.length()
					&& filename.substr(filename.length() - ext.length()) == ext) {
					matched = true;
					break;
				}
			}
			if (!matched) {
				continue;
			}
		}
		else if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".json") {
			continue; // Default: only .json files
		}

		// Skip if already registered by this path
		const auto path_str = entry.string();
		if (path_to_guid_.contains(path_str)) {
			continue;
		}

		// Read and parse JSON
		const auto text = AssetManager::LoadTextFile(entry);
		if (!text) {
			continue;
		}

		const auto j = nlohmann::json::parse(*text, nullptr, false);
		if (j.is_discarded()) {
			continue;
		}

		const std::string type_str = j.value("type", "");
		if (type_str.empty()) {
			continue;
		}

		const auto* type_info = AssetRegistry::Instance().GetTypeInfo(type_str);
		if (!type_info || !type_info->create_default_factory) {
			continue;
		}

		auto asset = type_info->create_default_factory();
		if (!asset) {
			continue;
		}

		asset->FromJson(j);

		if (asset->guid == 0) {
			asset->guid = GenerateGuid();
		}

		// Register (add to cache) without loading
		cache_[asset->guid] = asset;
		if (!asset->name.empty()) {
			name_index_[asset->name] = asset->guid;
		}
		path_to_guid_[path_str] = asset->guid;
		++registered;
	}

	return registered;
}

} // namespace engine::assets
