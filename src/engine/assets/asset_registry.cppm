module;

#include <cstdint>
#include <flecs.h>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <ranges>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

export module engine.asset_registry;

import engine.rendering;
import engine.ecs.component_registry;

export namespace engine::assets {

/// Type of asset in the scene
enum class AssetType : uint8_t {
	SHADER,
	MESH,
	TEXTURE,
	MATERIAL,
	ANIMATION_CLIP,
	SOUND,
	DATA_TABLE,
	PREFAB,
};

/// Stable, hardcoded GUIDs for built-in assets (shaders, mesh primitives).
/// Built-ins are created in code rather than loaded from disk, so they need fixed
/// GUIDs to remain referenceable across runs. These live in a reserved high range
/// that deterministic name-hashed GUIDs (see AssetCache) never produce.
namespace builtin_guids {
inline constexpr uint32_t RESERVED_BASE = 0xF0000000U;

inline constexpr uint32_t SHADER_DEFAULT_2D = 0xF0000001U;
inline constexpr uint32_t SHADER_DEFAULT_3D_LIT = 0xF0000002U;
inline constexpr uint32_t SHADER_UNLIT = 0xF0000003U;

inline constexpr uint32_t MESH_QUAD = 0xF0000011U;
inline constexpr uint32_t MESH_CUBE = 0xF0000012U;
inline constexpr uint32_t MESH_SPHERE = 0xF0000013U;
} // namespace builtin_guids

// === ASSET FIELD REFLECTION SYSTEM ===

// Forward declarations
struct AssetInfo;
class AssetTypeRegistry;

/**
 * @brief Information about a registered asset type
 */
struct AssetTypeInfo {
	std::string type_name; // JSON type string (e.g., "shader")
	std::string display_name; // UI display name (e.g., "Shader")
	std::string category; // Grouping category (e.g., "Rendering")
	AssetType asset_type{}; // Enum value
	std::function<std::shared_ptr<AssetInfo>()> create_default_factory;
	std::vector<ecs::FieldInfo> fields;
};

/**
 * @brief Builder for registering asset types with fluent API
 *
 * Usage:
 *   AssetRegistry::Instance().RegisterType<ShaderAssetInfo>("shader", AssetType::SHADER)
 *       .DisplayName("Shader")
 *       .Category("Rendering")
 *       .Field("name", &ShaderAssetInfo::name, "Name")
 *       .Field("vertex_path", &ShaderAssetInfo::vertex_path, "Vertex Shader", ecs::FieldType::FilePath)
 *       .CreateDefault([]() { return std::make_shared<ShaderAssetInfo>(); })
 *       .Build();
 */
template <typename T> class AssetTypeRegistration {
public:
	AssetTypeRegistration(AssetTypeRegistry& registry, const std::string& type_name, AssetType asset_type);

	AssetTypeRegistration& DisplayName(const std::string& name) {
		info_.display_name = name;
		return *this;
	}

	AssetTypeRegistration& Category(const std::string& cat) {
		info_.category = cat;
		return *this;
	}

	template <typename FieldT, typename ClassT>
	AssetTypeRegistration&
	Field(const std::string& field_name,
		  FieldT ClassT::* member_ptr,
		  const std::string& display_name,
		  const ecs::FieldType type_override = DeduceFieldType<FieldT>()) {
		static_assert(
				std::is_base_of_v<ClassT, T> || std::is_same_v<ClassT, T>,
				"Member pointer must be from T or a base class of T");
		ecs::FieldInfo field;
		field.name = field_name;
		field.display_name = display_name;
		field.type = type_override;
		field.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member_ptr));
		field.size = sizeof(FieldT);
		info_.fields.push_back(std::move(field));
		return *this;
	}

	/// Add options to the last registered field (for Selection type)
	AssetTypeRegistration& Options(std::vector<std::string> opts) {
		if (!info_.fields.empty()) {
			info_.fields.back().options = std::move(opts);
		}
		return *this;
	}

	/// Set file extension filters on the last field (for FilePath/AssetRef browse dialogs)
	AssetTypeRegistration& FileExtensions(std::vector<std::string> extensions) {
		if (!info_.fields.empty()) {
			info_.fields.back().file_extensions = std::move(extensions);
		}
		return *this;
	}

	/// Set slider range on the last field (changes type to Slider)
	AssetTypeRegistration& SliderRange(float min_val, float max_val) {
		if (!info_.fields.empty()) {
			info_.fields.back().type = ecs::FieldType::Slider;
			info_.fields.back().slider_min = min_val;
			info_.fields.back().slider_max = max_val;
		}
		return *this;
	}

	/// Mark the last field as an asset reference with the given asset type key.
	/// For list types, only sets asset_type hint (elements rendered as asset pickers).
	AssetTypeRegistration& AssetRef(const std::string& asset_type_key) {
		if (!info_.fields.empty()) {
			auto& field_info = info_.fields.back();
			if (field_info.type != ecs::FieldType::ListInt && field_info.type != ecs::FieldType::ListFloat
				&& field_info.type != ecs::FieldType::ListString && field_info.type != ecs::FieldType::UintAssetRef
				&& field_info.type != ecs::FieldType::AssetReference) {
				field_info.type = ecs::FieldType::AssetRef;
			}
			field_info.asset_type = asset_type_key;
		}
		return *this;
	}
	AssetTypeRegistration& AssetRef(const std::string_view asset_type_key) {
		return AssetRef(std::string(asset_type_key));
	}

	void Build();

private:
	template <typename FieldT> static constexpr ecs::FieldType DeduceFieldType() {
		if constexpr (std::is_same_v<FieldT, std::string>) {
			return ecs::FieldType::String;
		}
		else if constexpr (std::is_same_v<FieldT, int> || std::is_same_v<FieldT, uint32_t>) {
			return ecs::FieldType::Int;
		}
		else if constexpr (std::is_same_v<FieldT, float>) {
			return ecs::FieldType::Float;
		}
		else if constexpr (std::is_same_v<FieldT, bool>) {
			return ecs::FieldType::Bool;
		}
		else {
			return ecs::FieldType::ReadOnly;
		}
	}

	AssetTypeRegistry& registry_;
	AssetTypeInfo info_;
};

/// Registry for asset type factories and editor metadata
/// Asset types self-register their deserialization factories and field info
class AssetTypeRegistry {
public:
	/// Get singleton instance
	static AssetTypeRegistry& Instance();

	/// Initialize asset registry: register all asset types and set up ECS ref bindings.
	/// Must be called once after the flecs world is created.
	static void Initialize(flecs::world& world);

	/// Start registering an asset type with builder pattern
	template <typename T> AssetTypeRegistration<T> RegisterType(const std::string& type_name, AssetType asset_type) {
		return AssetTypeRegistration<T>(*this, type_name, asset_type);
	}

	template <typename T>
	AssetTypeRegistration<T> RegisterType(const std::string_view type_name, const AssetType asset_type) {
		return RegisterType<T>(std::string(type_name), asset_type);
	}

	/// Get all registered asset types
	[[nodiscard]] const std::vector<AssetTypeInfo>& GetAssetTypes() const { return types_; }

	/// Get asset type info by type enum
	[[nodiscard]] const AssetTypeInfo* GetTypeInfo(AssetType type) const;

	/// Get asset type info by type name string
	[[nodiscard]] const AssetTypeInfo* GetTypeInfo(const std::string& type_name) const;

	// Called by AssetTypeRegistration::Build()
	void AddTypeInfo(AssetTypeInfo info);

private:
	AssetTypeRegistry() = default;
	std::vector<AssetTypeInfo> types_;
};

// Template implementation
template <typename T>
AssetTypeRegistration<T>::AssetTypeRegistration(
		AssetTypeRegistry& registry, const std::string& type_name, const AssetType asset_type) : registry_(registry) {
	info_.type_name = type_name;
	info_.display_name = type_name; // Default, override with DisplayName()
	info_.category = "Other";
	info_.asset_type = asset_type;
	info_.create_default_factory = [type_name] { return std::make_shared<T>("New " + type_name); };
}

template <typename T> void AssetTypeRegistration<T>::Build() { registry_.AddTypeInfo(std::move(info_)); }

/// Base asset definition - common fields for all asset types
///
/// Asset lifecycle:
///   Registered → Loaded
///   - Registered: asset metadata is in the cache (name, guid, type, paths).
///     No system resources allocated. Created by Add().
///   - Loaded: subsystem has processed the asset. System resources are allocated
///     (shader compiled, mesh geometry on GPU, audio decoded, etc.).
///     Triggered by calling Load().
struct AssetInfo {
	uint32_t guid{0}; // Stable identifier, persisted in asset JSON files
	std::string name;
	AssetType type{};

	virtual ~AssetInfo() = default;

	AssetInfo() = default;
	AssetInfo(std::string asset_name, const AssetType asset_type) : name(std::move(asset_name)), type(asset_type) {}

	/// Load this asset: allocates system resources (compiles shaders, uploads
	/// geometry, decodes audio, etc.). Returns true if already loaded or success.
	bool Load();

	/// Unload/release system resources for this asset.
	void Unload();

	virtual void FromJson(const nlohmann::json& j);
	virtual void ToJson(nlohmann::json& j);

	/// True if system resources have been allocated via Load().
	[[nodiscard]] bool IsLoaded() const { return loaded_; }

protected:
	bool loaded_{false};
	bool loading_{false}; // Re-entrancy guard: prevents infinite recursion when
						  // resolving nested asset refs (e.g. material -> texture).
	bool initialized_{false}; // Internal: tracks DoInitialize() for two-phase loading

	/// Reserve lightweight system resources (e.g., shader ID slot) before DoLoad
	virtual void DoInitialize() {}

	/// Allocate full system resources (compile, upload, decode)
	virtual bool DoLoad() { return true; }

	virtual void DoUnload() {}

	[[nodiscard]] virtual std::string_view GetTypeName() const = 0;

private:
	/// Internal: calls DoInitialize() once, used by Load()
	void Initialize();
};

/// Shader asset definition
struct ShaderAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "shader";

	std::string vertex_path;
	std::string fragment_path;
	rendering::ShaderId id{rendering::INVALID_SHADER}; // Populated in DoInitialize, compiled in DoLoad

	ShaderAssetInfo() : AssetInfo("", AssetType::SHADER) {}
	ShaderAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::SHADER) {}

	/// Register this asset type with the AssetRegistry
	static void RegisterType();

	/// Register built-in shaders (e.g., default PBR shader) in the asset cache
	static void RegisterBuiltins();

	/// Set up ECS ref component + observer binding
	static void SetupRefBinding(flecs::world& world);

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Unified asset reference value type used by both ECS ref components and
/// asset-internal slots (e.g. material texture maps).
///
/// Exactly one identity is authoritative, encoded by which fields are set:
///   - guid != 0, path empty        → builtin/procedural asset (no source file)
///   - guid != 0 && !path.empty()   → disk-backed asset (guid = stable identity, path = locator)
///   - guid == 0 && !path.empty()   → unresolved import (resolve by path, then assign guid)
///   - guid == 0 && path empty      → null reference
///
/// Resolution precedence (see AssetCache::Resolve): guid → path → content-hash relink.
struct AssetRef {
	uint32_t guid{0};
	std::string path;

	[[nodiscard]] static AssetRef FromGuid(const uint32_t g) { return AssetRef{g, std::string{}}; }
	[[nodiscard]] static AssetRef FromPath(std::string p) { return AssetRef{0, std::move(p)}; }
	[[nodiscard]] bool IsEmpty() const { return guid == 0 && path.empty(); }
};

/// Serialize an AssetRef to its canonical JSON form: {"guid": N, "path": "..."}.
nlohmann::json AssetRefToJson(const AssetRef& ref);

/// Parse an AssetRef from JSON. Accepts the canonical object form {guid, path}.
AssetRef AssetRefFromJson(const nlohmann::json& j);

/// Asset reference component for shader - holds an AssetRef for serialization.
/// An observer resolves the ref to a ShaderId and updates Renderable::shader.
struct ShaderRef {
	AssetRef ref;
};

/// Built-in mesh type names (used as mesh_type field value)
namespace mesh_types {
constexpr auto QUAD = "quad";
constexpr auto CUBE = "cube";
constexpr auto SPHERE = "sphere";
constexpr auto CAPSULE = "capsule"; // Future
constexpr auto FILE = "file";
} // namespace mesh_types

/// Mesh asset definition
/// Can be a built-in type (quad, cube, sphere) with parameters, or a file reference
struct MeshAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "mesh";

	std::string mesh_type = mesh_types::QUAD; // "quad", "cube", "sphere", "capsule", "file"
	float params[3]{1.0F, 1.0F, 1.0F}; // Interpreted based on mesh_type
	std::string file_path; // Only used when mesh_type == "file"
	rendering::MeshId id{rendering::INVALID_MESH}; // Populated in DoInitialize, geometry in DoLoad

	MeshAssetInfo() : AssetInfo("", AssetType::MESH) {}
	MeshAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::MESH) {}

	/// Register this asset type with the AssetRegistry
	static void RegisterType();

	/// Register built-in mesh primitives (quad, cube, sphere) in the asset cache
	static void RegisterBuiltins();

	/// Set up ECS ref component + observer binding
	static void SetupRefBinding(flecs::world& world);

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Asset reference component for mesh - holds an AssetRef for serialization.
/// An observer resolves the ref to a MeshId and updates Renderable::mesh.
struct MeshRef {
	AssetRef ref;
};

/// Texture asset definition
struct TextureAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "texture";

	std::string file_path;
	rendering::TextureId id{rendering::INVALID_TEXTURE};

	TextureAssetInfo() : AssetInfo("", AssetType::TEXTURE) {}
	TextureAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::TEXTURE) {}

	static void RegisterType();

	/// Set up ECS ref component binding (registers TextureRef as a component)
	static void SetupRefBinding(flecs::world& world);

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Asset reference component for texture - holds an AssetRef for serialization.
struct TextureRef {
	AssetRef ref;
};

/// Material asset definition - PBR material with static texture slots and properties
struct MaterialAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "material";

	AssetRef shader; // Custom shader override (empty = default PBR)
	rendering::MaterialId id{rendering::INVALID_MATERIAL};

	// PBR texture map slots (asset references resolved at load time)
	AssetRef albedo_map;
	AssetRef normal_map;
	AssetRef metallic_map;
	AssetRef roughness_map;
	AssetRef ao_map;
	AssetRef emissive_map;
	AssetRef height_map;

	// PBR scalar properties
	float metallic_factor{0.0F};
	float roughness_factor{0.5F};
	float ao_strength{1.0F};
	float emissive_intensity{0.0F};
	float normal_strength{1.0F};
	float alpha_cutoff{0.5F};

	// PBR color properties
	rendering::Vec4 base_color{1.0F, 1.0F, 1.0F, 1.0F};
	rendering::Vec4 emissive_color{0.0F, 0.0F, 0.0F, 1.0F};

	MaterialAssetInfo() : AssetInfo("", AssetType::MATERIAL) {}
	MaterialAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::MATERIAL) {}

	static void RegisterType();

	/// Set up ECS ref component + observer binding
	static void SetupRefBinding(flecs::world& world);

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Asset reference component for material - holds an AssetRef for serialization.
/// An observer resolves the ref to a MaterialId and updates Renderable::material.
struct MaterialRef {
	AssetRef ref;
};

/// Animation clip asset definition
struct AnimationAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "animation_clip";

	std::string file_path;

	AnimationAssetInfo() : AssetInfo("", AssetType::ANIMATION_CLIP) {}
	AnimationAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::ANIMATION_CLIP) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Sound asset definition
struct SoundAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "sound";

	std::string file_path;
	float volume{1.0F};
	bool loop{false};
	uint32_t clip_id{0}; // Runtime: audio clip handle from AudioSystem

	SoundAssetInfo() : AssetInfo("", AssetType::SOUND) {}
	SoundAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::SOUND) {}

	static void RegisterType();

	/// Set up ECS ref component + observer binding
	static void SetupRefBinding(flecs::world& world);

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Asset reference component for sound - holds an AssetRef for serialization.
/// An observer resolves the ref to a clip_id and updates AudioSource::clip_id.
struct SoundRef {
	AssetRef ref;
};

/// Data table asset definition
struct DataTableAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "data_table";

	std::string file_path;
	std::string schema_name;

	DataTableAssetInfo() : AssetInfo("", AssetType::DATA_TABLE) {}
	DataTableAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::DATA_TABLE) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Prefab asset definition
struct PrefabAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "prefab";

	std::string file_path;

	PrefabAssetInfo() : AssetInfo("", AssetType::PREFAB) {}
	PrefabAssetInfo(std::string asset_name) : AssetInfo(std::move(asset_name), AssetType::PREFAB) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Factory function for importing raw asset files (e.g., .wav → SoundAssetInfo).
/// Given a filename stem and file path, creates an asset with default settings.
using FileImportFactory = std::function<std::shared_ptr<AssetInfo>(const std::string& name, const std::string& path)>;

/// Polymorphic asset storage using shared_ptr (allows editor to hold references)
using AssetPtr = std::shared_ptr<AssetInfo>;

/// Global asset cache — project-level storage for all assets.
/// Assets are cached by GUID (primary key) with a name index for convenience lookups.
class AssetCache {
public:
	static AssetCache& Instance();

	/// Creates a new default asset of the specified type, adds it to the cache, and returns it.
	AssetPtr Create(AssetType type, const std::string& name);
	template <typename T> std::shared_ptr<T> Create(AssetType type, const std::string& name);

	/// Add an asset to the cache as "registered" (metadata only, not loaded).
	/// Assigns a GUID if the asset doesn't have one (guid == 0).
	/// Call asset->Load() separately when system resources are needed.
	void Add(const AssetPtr& asset);

	/// Remove a cached asset by name and type.
	bool Remove(const std::string& name, AssetType type);

	/// Find asset by GUID (primary lookup).
	AssetPtr Find(uint32_t guid);
	[[nodiscard]] std::shared_ptr<const AssetInfo> Find(uint32_t guid) const;

	/// Find asset by name and type (secondary lookup via name index).
	AssetPtr Find(const std::string& name, AssetType type);
	[[nodiscard]] std::shared_ptr<const AssetInfo> Find(const std::string& name, AssetType type) const;

	/// Find typed asset by name.
	template <typename T> std::shared_ptr<T> FindTyped(const uint32_t guid) {
		if (const auto cached = cache_.find(guid); cached != cache_.end()) {
			if (auto typed = std::dynamic_pointer_cast<T>(cached->second)) {
				return typed;
			}
		}
		return nullptr;
	}

	/// Resolve an AssetRef to a cached asset, loading/importing from disk if needed.
	/// Resolution precedence: guid (if non-zero and cached) → path (cached, then load
	/// from disk / import raw + sidecar) → content-hash relink. Returns nullptr if the
	/// reference is empty or cannot be resolved.
	AssetPtr Resolve(const AssetRef& ref);

	/// Resolve an AssetRef and cast to the requested concrete asset type.
	template <typename T> std::shared_ptr<T> ResolveTyped(const AssetRef& ref) {
		if (auto asset = Resolve(ref)) {
			return std::dynamic_pointer_cast<T>(asset);
		}
		return nullptr;
	}

	/// Find a cached asset by its source/definition file path (normalized).
	AssetPtr FindByPath(const std::string& path);

	/// Return the source/definition file path a cached asset was loaded from, or an
	/// empty string for built-in/procedural assets that have no backing file.
	[[nodiscard]] std::string GetSourcePath(uint32_t guid) const;
	/// Find typed asset by name.
	template <typename T> std::shared_ptr<T> FindTyped(const std::string& name) {
		if (const auto it = name_index_.find(name); it != name_index_.end()) {
			if (auto typed = FindTyped<T>(it->second)) {
				return typed;
			}
		}
		return nullptr;
	}

	template <typename T> std::shared_ptr<const T> FindTyped(const std::string& name) const {
		if (const auto it = name_index_.find(name); it != name_index_.end()) {
			if (const auto cached = cache_.find(it->second); cached != cache_.end()) {
				if (auto typed = std::dynamic_pointer_cast<const T>(cached->second)) {
					return typed;
				}
			}
		}
		return nullptr;
	}

	/// Get all cached assets.
	[[nodiscard]] std::vector<AssetPtr> GetAll() const;

	/// Get all cached assets of a specific type.
	[[nodiscard]] std::vector<AssetPtr> GetByType(AssetType type) const;

	template <typename T> std::vector<std::shared_ptr<T>> GetAllOfType() {
		std::vector<std::shared_ptr<T>> result;
		for (auto& asset : cache_ | std::views::values) {
			if (auto typed = std::dynamic_pointer_cast<T>(asset)) {
				result.push_back(typed);
			}
		}
		return result;
	}

	/// Load an asset from a JSON file on disk.
	/// Creates the correct type via AssetRegistry, calls Load(), and caches by GUID.
	/// Returns the loaded asset, or nullptr on failure.
	AssetPtr LoadFromFile(const std::string& path);

	/// Save an asset to a JSON file on disk.
	/// Works on any AssetPtr — the asset does not need to be in the cache.
	/// Assigns a GUID if the asset doesn't have one (guid == 0).
	bool SaveToFile(const AssetPtr& asset, const std::string& path);

	/// Register a file importer for raw asset files (non-JSON).
	/// When scanning encounters a file matching one of the given extensions,
	/// the factory is called to create an asset entry with default settings.
	/// @param file_extensions Extensions to match (e.g., {".wav", ".mp3", ".ogg"})
	/// @param factory Function that creates an asset from (name, file_path)
	void RegisterFileImporter(const std::vector<std::string>& file_extensions, FileImportFactory factory);

	/// Try to import a raw file using registered file importers.
	/// Returns the created asset, or nullptr if no importer matched.
	/// The asset is NOT added to the cache — caller should call Add() if wanted.
	AssetPtr TryFileImport(const std::string& filename, const std::string& file_path);

	/// Clear all cached assets.
	void Clear();

	/// Get count.
	[[nodiscard]] size_t Size() const { return cache_.size(); }

	/// Generate a unique GUID not currently in the cache (runtime counter fallback).
	uint32_t GenerateGuid();

	/// Compute a stable, deterministic GUID for an asset from its type and name.
	/// Returns a value in the non-reserved range (< builtin_guids::RESERVED_BASE),
	/// so it never collides with hardcoded built-in GUIDs. Returns 0 if name is empty.
	[[nodiscard]] static uint32_t ComputeStableGuid(AssetType type, const std::string& name);

	/// Load an asset from a raw source file (e.g. .png, .wav), using a sibling
	/// "<source>.meta.json" descriptor to recover the persistent GUID + import
	/// options. If no descriptor exists, the file is imported with defaults and a
	/// descriptor is written so the GUID stays stable across runs.
	AssetPtr LoadOrImportSource(const std::string& source_path);

	/// Path of the sidecar descriptor for a raw source file ("<source>.meta.json").
	[[nodiscard]] static std::string MetaPathFor(const std::string& source_path);

	/// Compute a content hash + byte size for a file (used for move/relink detection).
	/// Returns {0,0} if the file cannot be read.
	[[nodiscard]] static std::pair<uint64_t, uint64_t> HashFile(const std::string& path);

	/// Write a "<source>.meta.json" descriptor capturing the asset's GUID, type,
	/// import settings, and source content hash/size.
	static bool WriteMeta(const AssetPtr& asset, const std::string& source_path);

private:
	AssetCache() = default;

	/// Assign a GUID to an asset that doesn't have one (guid == 0), preferring the
	/// deterministic name-hash and probing for a free slot on the rare collision.
	void AssignGuidIfNeeded(const AssetPtr& asset);

	std::unordered_map<uint32_t, AssetPtr> cache_; // guid → asset (primary)
	std::unordered_map<std::string, uint32_t> name_index_; // name → guid (secondary)
	std::unordered_map<std::string, uint32_t> path_to_guid_; // file/source path → guid
	uint32_t next_guid_{1}; // Counter for GUID generation

	/// Registered file importers: extension → factory
	std::unordered_map<std::string, FileImportFactory> file_importers_;
};

/// Generate a fresh, persistent asset GUID in the non-reserved range
/// (1 .. builtin_guids::RESERVED_BASE-1). Used by editor save paths that manage
/// their own descriptor files outside the AssetCache.
uint32_t GenerateAssetGuid();

/// Read the asset type name from a descriptor json, preferring the grouped
/// "_metadata.type" block and falling back to the legacy top-level "asset_type"
/// then "type" keys. Returns an empty string if none is present.
std::string ReadAssetMetadataType(const nlohmann::json& j);

/// Stamp a unified "_metadata" block (guid + type [+ name]) onto a serialized
/// asset json before it is written to `path`. If a descriptor already exists at
/// `path` with a "_metadata.guid", that GUID is preserved so re-saving keeps the
/// asset's identity; otherwise a fresh GUID is generated. Any legacy top-level
/// "asset_type" key is removed.
void StampAssetMetadata(
		nlohmann::json& j, const std::string& type, const std::string& path, const std::string& name = {});

template <typename T> std::shared_ptr<T> AssetCache::Create(const AssetType type, const std::string& name) {
	if (const auto assetPtr = Create(type, name)) {
		if (auto asset = std::dynamic_pointer_cast<T>(assetPtr)) {
			return asset;
		}
		std::cerr << "AssetCache::Create: Asset was created but unable to cast to requested type\n";
	}

	return nullptr;
}

// --- Data-driven SetupRefBinding template ---
// Eliminates per-asset-type copy-paste for ref component registration + observer wiring.
// Defined in the module interface so all implementation units can instantiate it.

template <typename AssetInfoT, typename RefT, typename TargetT, typename AssignFn, typename ClearFn>
void SetupRefBindingImpl(
		flecs::world& world,
		const char* ref_name,
		const char* category,
		const char* observer_name,
		std::string_view asset_type_name,
		AssignFn assign_fn,
		ClearFn clear_fn,
		std::vector<std::string> file_extensions = {}) {
	auto& registry = ecs::ComponentRegistry::Instance();
	// Each ref component holds a single nested AssetRef member. Register it by the
	// AssetRef flecs component id (registered in AssetTypeRegistry::Initialize) so the
	// component_registry module need not depend on the asset_registry module.
	const size_t ref_offset = reinterpret_cast<size_t>(&(static_cast<RefT*>(nullptr)->ref));
	auto reg = registry.Register<RefT>(ref_name, world)
					   .Category(category)
					   .Hidden()
					   .StructField(
							   "ref",
							   ref_offset,
							   sizeof(AssetRef),
							   world.component<AssetRef>().id(),
							   ecs::FieldType::AssetReference,
							   std::string(asset_type_name));
	if (!file_extensions.empty()) {
		reg.FileExtensions(std::move(file_extensions));
	}
	reg.Build();

	world.component<TargetT>().add(flecs::With, world.component<RefT>());

	world.observer<RefT, TargetT>(observer_name)
			.event(flecs::OnSet)
			.each([assign_fn, clear_fn](flecs::entity, const RefT& ref, TargetT& target) {
				if (ref.ref.IsEmpty()) {
					clear_fn(target);
					return;
				}
				if (auto asset = AssetCache::Instance().ResolveTyped<AssetInfoT>(ref.ref)) {
					asset->Load();
					assign_fn(asset, target);
				}
				else {
					// Reference points at an asset that can't be resolved: clear the target
					// so stale/broken references don't keep using a previously assigned asset.
					clear_fn(target);
				}
			});

	// Prevent removing the ref component if the target still has it, to ensure the binding stays consistent.
	// The ref can only be removed if the target component is removed
	world.observer<RefT>((std::string(observer_name) + "_ReAdd").c_str())
			.event(flecs::OnRemove)
			.each([clear_fn](flecs::entity e, RefT ref) {
				if (e.has<TargetT>()) {
					// Cleanup since we cannot re-add the same component
					if (ref.ref.IsEmpty()) {
						clear_fn(e.get_mut<TargetT>());
					}
					e.add<RefT>();
				}
			});

	// Remove refs if the target component is removed, to prevent dangling references
	world.observer<TargetT>((std::string(observer_name) + "_Clear").c_str())
			.event(flecs::OnRemove)
			.each([](flecs::entity e, TargetT) { e.remove<RefT>(); });
}

} // namespace engine::assets
