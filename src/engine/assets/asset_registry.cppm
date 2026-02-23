module;

#include <cstdint>
#include <flecs.h>
#include <functional>
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

// === ASSET FIELD REFLECTION SYSTEM ===

// Forward declarations
struct AssetInfo;
class AssetRegistry;

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
	AssetTypeRegistration(AssetRegistry& registry, const std::string& type_name, AssetType asset_type);

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

	/// Mark the last field as an asset reference with the given asset type key.
	/// For list types, only sets asset_type hint (elements rendered as asset pickers).
	AssetTypeRegistration& AssetRef(const std::string& asset_type_key) {
		if (!info_.fields.empty()) {
			auto& field_info = info_.fields.back();
			if (field_info.type != ecs::FieldType::ListInt && field_info.type != ecs::FieldType::ListFloat
				&& field_info.type != ecs::FieldType::ListString) {
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

	AssetRegistry& registry_;
	AssetTypeInfo info_;
};

/// Registry for asset type factories and editor metadata
/// Asset types self-register their deserialization factories and field info
class AssetRegistry {
public:
	/// Get singleton instance
	static AssetRegistry& Instance();

	/// Initialize asset registry: register all asset types and set up ECS ref bindings.
	/// Must be called once after the flecs world is created.
	void Initialize(flecs::world& world);

	/// Start registering an asset type with builder pattern
	template <typename T> AssetTypeRegistration<T> RegisterType(const std::string& type_name, AssetType asset_type) {
		return AssetTypeRegistration<T>(*this, type_name, asset_type);
	}

	template <typename T>
	AssetTypeRegistration<T> RegisterType(const std::string_view type_name, const AssetType asset_type) {
		return RegisterType<T>(std::string(type_name), asset_type);
	}

	/// Create a new default asset of the specified type
	/// @return nullptr if type has no default factory
	[[nodiscard]] std::shared_ptr<AssetInfo> CreateDefault(AssetType type) const;

	/// Get all registered asset types
	[[nodiscard]] const std::vector<AssetTypeInfo>& GetAssetTypes() const { return types_; }

	/// Get asset type info by type enum
	[[nodiscard]] const AssetTypeInfo* GetTypeInfo(AssetType type) const;

	/// Get asset type info by type name string
	[[nodiscard]] const AssetTypeInfo* GetTypeInfo(const std::string& type_name) const;

	// Called by AssetTypeRegistration::Build()
	void AddTypeInfo(AssetTypeInfo info);

private:
	AssetRegistry() = default;
	std::vector<AssetTypeInfo> types_;
};

// Template implementation
template <typename T>
AssetTypeRegistration<T>::AssetTypeRegistration(
		AssetRegistry& registry, const std::string& type_name, const AssetType asset_type) : registry_(registry) {
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
///     No system resources allocated. Created by ScanDirectory or Add.
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
	ShaderAssetInfo(std::string asset_name, std::string vert = "", std::string frag = "") :
			AssetInfo(std::move(asset_name), AssetType::SHADER), vertex_path(std::move(vert)),
			fragment_path(std::move(frag)) {}

	/// Register this asset type with the AssetRegistry
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

/// Common base for asset reference components.
/// Each concrete ref type inherits this so flecs still uses the derived C++ type
/// for component identity, while the shared base eliminates field duplication.
struct AssetRefBase {
	std::string name;
};

/// Asset reference component for shader - stores shader name for serialization.
/// An observer resolves the name to ShaderId and updates Renderable::shader.
struct ShaderRef : AssetRefBase {};

/// Built-in mesh type names (used as mesh_type field value)
namespace mesh_types {
constexpr const char* QUAD = "quad";
constexpr const char* CUBE = "cube";
constexpr const char* SPHERE = "sphere";
constexpr const char* CAPSULE = "capsule"; // Future
constexpr const char* FILE = "file";
} // namespace mesh_types

/// Mesh asset definition
/// Can be a built-in type (quad, cube, sphere) with parameters, or a file reference
struct MeshAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "mesh";

	std::string mesh_type = mesh_types::QUAD; // "quad", "cube", "sphere", "capsule", "file"
	float params[3]{1.0F, 1.0F, 1.0F}; // Interpreted based on mesh_type
	std::string file_path; // Only used when mesh_type == "file"
	rendering::MeshId id{rendering::INVALID_MESH}; // Populated in DoInitialize, geometry in DoLoad

	MeshAssetInfo() : AssetInfo("", AssetType::MESH), mesh_type(mesh_types::QUAD) {}
	MeshAssetInfo(std::string asset_name, std::string type = "") :
			AssetInfo(std::move(asset_name), AssetType::MESH), mesh_type(std::move(type)) {}

	/// Register this asset type with the AssetRegistry
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

/// Asset reference component for mesh - stores mesh asset name for serialization.
/// An observer resolves the name to MeshId and updates Renderable::mesh.
struct MeshRef : AssetRefBase {};

/// Texture asset definition
struct TextureAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "texture";

	std::string file_path;
	rendering::TextureId id{rendering::INVALID_TEXTURE};

	TextureAssetInfo() : AssetInfo("", AssetType::TEXTURE) {}
	TextureAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::TEXTURE), file_path(std::move(path)) {}

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

/// Asset reference component for texture - stores texture asset name for serialization.
struct TextureRef : AssetRefBase {};

/// Material asset definition - PBR material with static texture slots and properties
struct MaterialAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "material";

	uint32_t shader_ref_id; // Custom shader override (empty = default PBR)
	rendering::MaterialId id{rendering::INVALID_MATERIAL};

	// PBR texture map slots (asset names resolved at load time)
	std::string albedo_map;
	std::string normal_map;
	std::string metallic_map;
	std::string roughness_map;
	std::string ao_map;
	std::string emissive_map;
	std::string height_map;

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
	MaterialAssetInfo(std::string asset_name, uint32_t shader_id = 0) :
			AssetInfo(std::move(asset_name), AssetType::MATERIAL), shader_ref_id(shader_id) {}

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

/// Asset reference component for material - stores material asset name for serialization.
/// An observer resolves the name to MaterialId and updates Renderable::material.
struct MaterialRef : AssetRefBase {};

/// Animation clip asset definition
struct AnimationAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "animation_clip";

	std::string clip_path;

	AnimationAssetInfo() : AssetInfo("", AssetType::ANIMATION_CLIP) {}
	AnimationAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::ANIMATION_CLIP), clip_path(std::move(path)) {}

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
	SoundAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::SOUND), file_path(std::move(path)) {}

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

/// Asset reference component for sound - stores sound asset name for serialization.
/// An observer resolves the name to a clip_id and updates AudioSource::clip_id.
struct SoundRef : AssetRefBase {};

/// Data table asset definition
struct DataTableAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "data_table";

	std::string file_path;
	std::string schema_name;

	DataTableAssetInfo() : AssetInfo("", AssetType::DATA_TABLE) {}
	DataTableAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::DATA_TABLE), file_path(std::move(path)) {}

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
	PrefabAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::PREFAB), file_path(std::move(path)) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Polymorphic asset storage using shared_ptr (allows editor to hold references)
using AssetPtr = std::shared_ptr<AssetInfo>;

/// Global asset cache — project-level storage for all assets.
/// Assets are cached by GUID (primary key) with a name index for convenience lookups.
class AssetCache {
public:
	static AssetCache& Instance();

	/// Add an asset to the cache as "registered" (metadata only, not loaded).
	/// Assigns a GUID if the asset doesn't have one (guid == 0).
	/// Call asset->Load() separately when system resources are needed.
	void Add(AssetPtr asset);

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

	/// Scan a directory for asset JSON files and register them in the cache.
	/// Assets are registered (metadata parsed, GUID assigned) but NOT loaded —
	/// no system resources are allocated until Load() is called on each asset.
	/// @param directory Path to scan (e.g., "assets/materials")
	/// @param extensions File extensions to match (e.g., {".material.json", ".shader.json"}).
	///                   If empty, scans all .json files.
	/// @return Number of newly registered assets.
	size_t ScanDirectory(const std::string& directory, const std::vector<std::string>& extensions = {});

	/// Clear all cached assets.
	void Clear();

	/// Get count.
	[[nodiscard]] size_t Size() const { return cache_.size(); }

	/// Generate a unique GUID not currently in the cache.
	uint32_t GenerateGuid();

private:
	AssetCache() = default;
	std::unordered_map<uint32_t, AssetPtr> cache_; // guid → asset (primary)
	std::unordered_map<std::string, uint32_t> name_index_; // name → guid (secondary)
	std::unordered_map<std::string, uint32_t> path_to_guid_; // file path → guid
	uint32_t next_guid_{1}; // Counter for GUID generation
};

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
	std::string RefT::* name_member = &AssetRefBase::name;
	auto reg = registry.Register<RefT>(ref_name, world)
					   .Category(category)
					   .Field("name", name_member)
					   .AssetRef(asset_type_name);
	if (!file_extensions.empty()) {
		reg.FileExtensions(std::move(file_extensions));
	}
	reg.Build();

	world.component<TargetT>().add(flecs::With, world.component<RefT>());

	world.observer<RefT, TargetT>(observer_name)
			.event(flecs::OnSet)
			.each([assign_fn, clear_fn](flecs::entity, const RefT& ref, TargetT& target) {
				if (ref.name.empty()) {
					clear_fn(target);
					return;
				}
				if (auto asset = AssetCache::Instance().FindTyped<AssetInfoT>(ref.name)) {
					asset->Load();
					assign_fn(asset, target);
				}
			});
}

} // namespace engine::assets
