module;

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <flecs.h>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
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
	// Register the unified AssetRef value type with flecs reflection BEFORE any ref
	// component that nests it, so the nested member can be registered by component id.
	world.component<AssetRef>().member<std::uint32_t>("guid").member<std::string>("path");

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
	if (loading_) {
		// Re-entrant load (e.g. a material resolving a texture that resolves back):
		// treat as success to break the cycle; the in-progress load will finish.
		return true;
	}
	loading_ = true;
	// Reset the re-entrancy guard on any exit path (including exceptions from DoLoad).
	struct LoadingGuard {
		bool& flag;
		~LoadingGuard() { flag = false; }
	} loading_guard{loading_};
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
	// Identity lives under "_metadata"; fall back to legacy top-level keys for older files.
	const nlohmann::json& meta = (j.contains("_metadata") && j["_metadata"].is_object()) ? j["_metadata"] : j;
	guid = meta.value("guid", 0U);
	name = meta.value("name", name);
	if (const auto type_str = meta.value("type", std::string{}); !type_str.empty()) {
		if (const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(type_str); type_info != nullptr) {
			type = type_info->asset_type;
		}
	}
}

void AssetInfo::ToJson(nlohmann::json& j) {
	// Group stable identity under "_metadata"; type-specific (opaque) fields stay top-level.
	j["_metadata"]["guid"] = guid;
	j["_metadata"]["name"] = name;
	j["_metadata"]["type"] = GetTypeName();
}

// --- AssetCache ---

namespace {
/// Normalize a path to a stable cache key (forward slashes, no redundant separators).
std::string NormalizePathKey(const std::string& path) {
	if (path.empty()) {
		return path;
	}
	return std::filesystem::path(path).lexically_normal().generic_string();
}

/// Read the asset type name from a descriptor, preferring the grouped "_metadata"
/// block and falling back to a legacy top-level "type" key for older files.
std::string ReadAssetType(const nlohmann::json& j) {
	if (const auto it = j.find("_metadata"); it != j.end() && it->is_object()) {
		return it->value("type", std::string{});
	}
	return j.value("type", std::string{});
}
} // namespace

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
	AssignGuidIfNeeded(asset);
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
	const std::string key = NormalizePathKey(path);
	// Check if this file path was already loaded
	if (const auto it = path_to_guid_.find(key); it != path_to_guid_.end()) {
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
		const std::string type_str = ReadAssetType(j);
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

		// Assign a deterministic GUID if the file didn't have one
		AssignGuidIfNeeded(asset);

		// Assets are loaded on-demand when referenced by an entity (via SetupRefBinding observers)

		// Cache by GUID (primary key) with name and path indices
		cache_[asset->guid] = asset;
		if (!asset->name.empty()) {
			name_index_[asset->name] = asset->guid;
		}
		path_to_guid_[key] = asset->guid;
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
	AssignGuidIfNeeded(asset);

	if (const auto* type_info = AssetTypeRegistry::Instance().GetTypeInfo(asset->type); !type_info) {
		std::cerr << "AssetCache::SaveToFile: unknown type for asset '" << asset->name << "'" << '\n';
		return false;
	}

	nlohmann::json j;
	asset->ToJson(j);

	return AssetManager::SaveTextFile(platform::fs::Path(path), j.dump(2));
}

nlohmann::json AssetRefToJson(const AssetRef& ref) {
	return nlohmann::json{{"guid", ref.guid}, {"path", ref.path}};
}

AssetRef AssetRefFromJson(const nlohmann::json& j) {
	if (j.is_object()) {
		return AssetRef{j.value("guid", 0U), j.value("path", std::string{})};
	}
	return AssetRef{};
}

AssetPtr AssetCache::FindByPath(const std::string& path) {
	if (path.empty()) {
		return nullptr;
	}
	const std::string norm = NormalizePathKey(path);
	if (const auto it = path_to_guid_.find(norm); it != path_to_guid_.end()) {
		if (const auto c = cache_.find(it->second); c != cache_.end()) {
			return c->second;
		}
	}
	return nullptr;
}

std::string AssetCache::GetSourcePath(const uint32_t guid) const {
	for (const auto& [path, mapped_guid] : path_to_guid_) {
		if (mapped_guid == guid) {
			return path;
		}
	}
	return {};
}

std::string AssetCache::MetaPathFor(const std::string& source_path) { return source_path + ".meta.json"; }

std::pair<uint64_t, uint64_t> AssetCache::HashFile(const std::string& path) {
	const auto bytes = AssetManager::LoadBinaryFile(platform::fs::Path(path));
	if (!bytes) {
		return {0, 0};
	}
	// FNV-1a 64-bit content hash.
	constexpr uint64_t fnv_offset = 1469598103934665603ULL;
	constexpr uint64_t fnv_prime = 1099511628211ULL;
	uint64_t hash = fnv_offset;
	for (const auto b : *bytes) {
		hash ^= static_cast<uint8_t>(b);
		hash *= fnv_prime;
	}
	return {hash, static_cast<uint64_t>(bytes->size())};
}

bool AssetCache::WriteMeta(const AssetPtr& asset, const std::string& source_path) {
	if (!asset) {
		return false;
	}
	nlohmann::json j;
	asset->ToJson(j);
	const auto [hash, size] = HashFile(source_path);
	// Source locator + import-tracking data belong with the asset identity.
	j["_metadata"]["path"] = NormalizePathKey(source_path);
	j["_metadata"]["source_hash"] = hash;
	j["_metadata"]["source_size"] = size;
	return AssetManager::SaveTextFile(platform::fs::Path(MetaPathFor(source_path)), j.dump(2));
}

AssetPtr AssetCache::LoadOrImportSource(const std::string& source_path) {
	if (source_path.empty()) {
		return nullptr;
	}
	const std::string norm = NormalizePathKey(source_path);
	if (auto cached = FindByPath(norm)) {
		return cached;
	}

	std::string ext = std::filesystem::path(source_path).extension().string();
	std::ranges::transform(ext, ext.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// JSON-native asset definition (e.g. .material.json): load directly.
	if (ext == ".json") {
		return LoadFromFile(source_path);
	}

	// Raw source file: recover persistent identity + import options from a sibling
	// "<source>.meta.json" descriptor if present, otherwise import with defaults.
	AssetPtr asset;
	const std::string meta_path = MetaPathFor(source_path);
	bool had_meta = false;
	if (const auto meta_text = AssetManager::LoadTextFile(platform::fs::Path(meta_path))) {
		if (const auto mj = nlohmann::json::parse(*meta_text, nullptr, false); !mj.is_discarded()) {
			if (const std::string type_str = ReadAssetType(mj); !type_str.empty()) {
				if (const auto* ti = AssetTypeRegistry::Instance().GetTypeInfo(type_str);
					ti && ti->create_default_factory) {
					asset = ti->create_default_factory();
					asset->FromJson(mj);
					had_meta = true;
				}
			}
		}
	}

	if (!asset) {
		const std::string filename = std::filesystem::path(source_path).filename().string();
		asset = TryFileImport(filename, source_path);
		if (!asset) {
			std::cerr << "AssetCache::LoadOrImportSource: no importer for " << source_path << '\n';
			return nullptr;
		}
	}

	AssignGuidIfNeeded(asset);
	cache_[asset->guid] = asset;
	if (!asset->name.empty()) {
		name_index_[asset->name] = asset->guid;
	}
	path_to_guid_[norm] = asset->guid;

	// Persist a descriptor on first import so the GUID stays stable across runs.
	if (!had_meta) {
		WriteMeta(asset, source_path);
	}
	return asset;
}

AssetPtr AssetCache::Resolve(const AssetRef& ref) {
	if (ref.IsEmpty()) {
		return nullptr;
	}
	// 1. GUID (authoritative identity) — only if already cached.
	if (ref.guid != 0) {
		if (auto found = Find(ref.guid)) {
			return found;
		}
	}
	// 2. Path (locator) — cached, else load/import from disk. Path-based resolution
	//    survives GUID drift (e.g. a deleted descriptor) and triggers on-demand loads.
	if (!ref.path.empty()) {
		if (auto found = LoadOrImportSource(ref.path)) {
			return found;
		}
	}
	return nullptr;
}

uint32_t AssetCache::GenerateGuid() {
	while (cache_.contains(next_guid_)) {
		++next_guid_;
	}
	return next_guid_++;
}

uint32_t AssetCache::ComputeStableGuid(const AssetType type, const std::string& name) {
	if (name.empty()) {
		return 0;
	}
	// FNV-1a 32-bit hash over a type-namespaced key so assets of different types
	// that share a name don't collide on the global GUID key space.
	constexpr uint32_t fnv_offset = 2166136261U;
	constexpr uint32_t fnv_prime = 16777619U;
	uint32_t hash = fnv_offset;
	const auto mix = [&hash](const unsigned char byte) {
		hash ^= byte;
		hash *= fnv_prime;
	};
	mix(static_cast<unsigned char>(type));
	mix(static_cast<unsigned char>(':'));
	for (const char c : name) {
		mix(static_cast<unsigned char>(c));
	}
	// Constrain to the non-reserved range so we never collide with built-in GUIDs.
	hash %= builtin_guids::RESERVED_BASE;
	if (hash == 0) {
		hash = 1;
	}
	return hash;
}

void AssetCache::AssignGuidIfNeeded(const AssetPtr& asset) {
	if (!asset || asset->guid != 0) {
		return;
	}
	// Persistent random identity: GUIDs are written to the asset's JSON file or its
	// "<source>.meta.json" descriptor, so a fresh random value stays stable across runs
	// once persisted. Random (not name-hash) keeps identity stable across renames.
	static std::mt19937 rng{std::random_device{}()};
	std::uniform_int_distribution<uint32_t> dist(1, builtin_guids::RESERVED_BASE - 1);
	uint32_t guid = dist(rng);
	while (cache_.contains(guid) && cache_.at(guid) != asset) {
		guid = dist(rng);
	}
	asset->guid = guid;
}

uint32_t GenerateAssetGuid() {
	static std::mt19937 rng{std::random_device{}()};
	std::uniform_int_distribution<uint32_t> dist(1, builtin_guids::RESERVED_BASE - 1);
	return dist(rng);
}

std::string ReadAssetMetadataType(const nlohmann::json& j) {
	if (const auto it = j.find("_metadata"); it != j.end() && it->is_object()) {
		if (auto type = it->value("type", std::string{}); !type.empty()) {
			return type;
		}
	}
	// Legacy fallbacks for files written before the unified _metadata block.
	if (auto legacy = j.value("asset_type", std::string{}); !legacy.empty()) {
		return legacy;
	}
	return j.value("type", std::string{});
}

void StampAssetMetadata(
		nlohmann::json& j, const std::string& type, const std::string& path, const std::string& name) {
	uint32_t guid = 0;
	if (const auto text = AssetManager::LoadTextFile(platform::fs::Path(path))) {
		if (const auto existing = nlohmann::json::parse(*text, nullptr, false); !existing.is_discarded()) {
			if (const auto it = existing.find("_metadata"); it != existing.end() && it->is_object()) {
				guid = it->value("guid", 0U);
			}
		}
	}
	if (guid == 0) {
		guid = GenerateAssetGuid();
	}
	j.erase("asset_type");
	j["_metadata"]["guid"] = guid;
	j["_metadata"]["type"] = type;
	if (!name.empty()) {
		j["_metadata"]["name"] = name;
	}
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

} // namespace engine::assets
