module;

#include <algorithm>
#include <cctype>
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

AssetTypeRegistry& AssetTypeRegistry::Instance() {
	static AssetTypeRegistry instance;
	return instance;
}

void AssetTypeRegistry::Initialize(flecs::world& world) {
	ShaderAssetInfo::RegisterType();
	MeshAssetInfo::RegisterType();
	TextureAssetInfo::RegisterType();
	MaterialAssetInfo::RegisterType();
	AnimationAssetInfo::RegisterType();
	SoundAssetInfo::RegisterType();
	DataTableAssetInfo::RegisterType();
	PrefabAssetInfo::RegisterType();

	// Set up ECS ref component bindings (observers that resolve name → runtime ID), after all types are registered
	ShaderAssetInfo::SetupRefBinding(world);
	MeshAssetInfo::SetupRefBinding(world);
	MaterialAssetInfo::SetupRefBinding(world);
	SoundAssetInfo::SetupRefBinding(world);
	TextureAssetInfo::SetupRefBinding(world);
}

const AssetTypeInfo* AssetTypeRegistry::GetTypeInfo(const AssetType type) const {
	for (const auto& info : types_) {
		if (info.asset_type == type) {
			return &info;
		}
	}
	return nullptr;
}

const AssetTypeInfo* AssetTypeRegistry::GetTypeInfo(const std::string& type_name) const {
	for (const auto& info : types_) {
		if (info.type_name == type_name) {
			return &info;
		}
	}
	return nullptr;
}

void AssetTypeRegistry::AddTypeInfo(AssetTypeInfo info) { types_.push_back(std::move(info)); }

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
	guid = j.value("guid", 0U);
	name = j.value("name", name);
	if (const auto type_str = j.value("type", ""); !type_str.empty()) {
		if (const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(type_str); type_info != nullptr) {
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
AssetPtr AssetCache::Create(const AssetType type, const std::string& name) {
	if (const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(type);
		type_info && type_info->create_default_factory) {
		if (auto asset = type_info->create_default_factory()) {
			asset->name = name;
			Add(asset);
			return asset;
		}
	}
	return nullptr;
}

void AssetCache::Add(const AssetPtr& asset) {
	if (!asset) {
		return;
	}
	if (asset->guid == 0) {
		asset->guid = GenerateGuid();
	}
	cache_[asset->guid] = asset;
	if (!asset->name.empty()) {
		name_index_[asset->name] = asset->guid;
	}
}

bool AssetCache::Remove(const std::string& name, const AssetType type) {
	const auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return false;
	}
	if (const auto it = cache_.find(nit->second); it != cache_.end() && it->second && it->second->type == type) {
		cache_.erase(it);
		name_index_.erase(nit);
		return true;
	}
	return false;
}

AssetPtr AssetCache::Find(const uint32_t guid) {
	const auto it = cache_.find(guid);
	return (it != cache_.end()) ? it->second : nullptr;
}

std::shared_ptr<const AssetInfo> AssetCache::Find(const uint32_t guid) const {
	const auto it = cache_.find(guid);
	return (it != cache_.end()) ? it->second : nullptr;
}

AssetPtr AssetCache::Find(const std::string& name, const AssetType type) {
	const auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return nullptr;
	}
	if (const auto it = cache_.find(nit->second); it != cache_.end() && it->second && it->second->type == type) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<const AssetInfo> AssetCache::Find(const std::string& name, const AssetType type) const {
	const auto nit = name_index_.find(name);
	if (nit == name_index_.end()) {
		return nullptr;
	}
	if (const auto it = cache_.find(nit->second); it != cache_.end() && it->second && it->second->type == type) {
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
		const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(type_str);
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

	if (const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(asset->type); !type_info) {
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

void AssetCache::RegisterFileImporter(const std::vector<std::string>& file_extensions, FileImportFactory factory) {
	for (auto ext : file_extensions) {
		if (ext.empty()) {
			continue;
		}
		// Ensure extension starts with a dot and is lowercase
		if (ext[0] != '.') {
			ext = '.' + ext;
		}
		std::ranges::transform(ext, ext.begin(), ::tolower);
		file_importers_[ext] = factory;
	}
}

AssetPtr AssetCache::TryFileImport(const std::string& filename, const std::string& file_path) {
	// Extract extension from filename
	const auto dot = filename.rfind('.');
	if (dot == std::string::npos) {
		return nullptr;
	}
	std::string ext = filename.substr(dot);
	// Lowercase the extension for case-insensitive matching
	std::ranges::transform(ext, ext.begin(), ::tolower);

	const auto it = file_importers_.find(ext);
	if (it == file_importers_.end()) {
		return nullptr;
	}

	// Derive asset name from filename stem
	const std::string name = filename.substr(0, dot);
	return it->second(name, file_path);
}

size_t AssetCache::ScanDirectory(const std::string& directory, const std::vector<std::string>& extensions) {
	size_t registered = 0;
	const auto dir_path = platform::fs::Path(directory);

	for (const auto entries = platform::fs::ListDirectory(dir_path); const auto& entry : entries) {
		// Recursively scan subdirectories
		if (platform::fs::IsDirectory(entry)) {
			registered += ScanDirectory(entry.string(), extensions);
			continue;
		}

		const auto filename = entry.filename().string();
		const auto path_str = entry.string();

		// Skip if already registered by this path
		if (path_to_guid_.contains(path_str)) {
			continue;
		}

		// Try file importers first (handles raw asset files like .wav, .png, etc.)
		if (auto asset = TryFileImport(filename, path_str)) {
			// Skip if an asset with this name already exists (JSON asset takes priority)
			if (!asset->name.empty() && name_index_.contains(asset->name)) {
				continue;
			}
			if (asset->guid == 0) {
				asset->guid = GenerateGuid();
			}
			cache_[asset->guid] = asset;
			if (!asset->name.empty()) {
				name_index_[asset->name] = asset->guid;
			}
			path_to_guid_[path_str] = asset->guid;
			++registered;
			continue;
		}

		// Filter by extensions for JSON asset files
		if (!extensions.empty()) {
			bool matched = false;
			for (const auto& ext : extensions) {
				if (filename.length() >= ext.length() && filename.substr(filename.length() - ext.length()) == ext) {
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

		const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(type_str);
		if (!type_info || !type_info->create_default_factory) {
			continue;
		}

		const auto asset = type_info->create_default_factory();
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
