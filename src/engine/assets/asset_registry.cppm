module;

#include <cstdint>
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
 *       .FromJson([](const json& j) { ... })
 *       .ToJson([](json& j, shared_ptr<AssetInfo> asset) {
 *           if (auto shader_asset = dynamic_pointer_cast<ShaderAssetInfo>(asset)) { ... }
 *        })
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

	/// Start registering an asset type with builder pattern
	template <typename T> AssetTypeRegistration<T> RegisterType(const std::string& type_name, AssetType asset_type) {
		return AssetTypeRegistration<T>(*this, type_name, asset_type);
	}

	template <typename T>
	AssetTypeRegistration<T> RegisterType(const std::string_view type_name, const AssetType asset_type) {
		return RegisterType<T>(std::string(type_name), asset_type);
	}

	/// Create an asset from JSON
	/// @return nullptr if type is unknown
	[[nodiscard]] std::shared_ptr<AssetInfo> FromJson(const nlohmann::json& j) const;

	void ToJson(nlohmann::json& j, const std::shared_ptr<AssetInfo>& asset) const;

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
struct AssetInfo {
	std::string name;
	AssetType type{};
	bool loaded_{false}; // Track whether this asset has been loaded
	bool initialized_{false}; // Track whether OnAdd has been called

	virtual ~AssetInfo() = default;

	AssetInfo() = default;
	AssetInfo(std::string asset_name, const AssetType asset_type) : name(std::move(asset_name)), type(asset_type) {}

	/// Called when asset is added to a SceneAssets container
	/// Use this to allocate resources (e.g., reserve shader IDs) before Load()
	void Initialize();

	/// Returns true if Initialize() has been called
	[[nodiscard]] bool IsInitialized() const { return initialized_; }

	/// Load/compile this asset. Returns true if already loaded or successfully loaded.
	bool Load();
	/// Unload/release this asset
	void Unload();

	virtual void FromJson(const nlohmann::json& j);
	virtual void ToJson(nlohmann::json& j);

	/// Check if this asset has been loaded
	[[nodiscard]] bool IsLoaded() const { return loaded_; }

protected:
	/// Override to allocate resources before Load (e.g., reserve shader ID)
	virtual void DoInitialize() {}

	/// Override to implement asset-specific loading logic
	virtual bool DoLoad() { return true; }

	virtual void DoUnload() {}

	[[nodiscard]] virtual std::string_view GetTypeName() const = 0;
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

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

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

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Texture asset definition
struct TextureAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "texture";

	std::string file_path;
	rendering::TextureId id{rendering::INVALID_TEXTURE};

	TextureAssetInfo() : AssetInfo("", AssetType::TEXTURE) {}
	TextureAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::TEXTURE), file_path(std::move(path)) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

/// Material asset definition - PBR material with static texture slots and properties
struct MaterialAssetInfo : AssetInfo {
	static constexpr std::string_view TYPE_NAME = "material";

	std::string shader_name; // Custom shader override (empty = default PBR)
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
	MaterialAssetInfo(std::string asset_name, std::string shader = "") :
			AssetInfo(std::move(asset_name), AssetType::MATERIAL), shader_name(std::move(shader)) {}

	static void RegisterType();
	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

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

	SoundAssetInfo() : AssetInfo("", AssetType::SOUND) {}
	SoundAssetInfo(std::string asset_name, std::string path = "") :
			AssetInfo(std::move(asset_name), AssetType::SOUND), file_path(std::move(path)) {}

	static void RegisterType();

	void FromJson(const nlohmann::json& j) override;
	void ToJson(nlohmann::json& j) override;

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	[[nodiscard]] std::string_view GetTypeName() const override { return TYPE_NAME; }
};

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

/// Global asset cache — project-level storage replacing per-scene SceneAssets.
/// Assets are cached by (name, type) key for O(1) lookup.
class AssetCache {
public:
	static AssetCache& Instance();

	/// Add an asset to the cache (shares ownership). Overwrites existing if same name+type.
	void Add(AssetPtr asset);

	/// Remove a cached asset by name and type.
	bool Remove(const std::string& name, AssetType type);

	/// Find asset by name and type.
	AssetPtr Find(const std::string& name, AssetType type);
	[[nodiscard]] std::shared_ptr<const AssetInfo> Find(const std::string& name, AssetType type) const;

	/// Find typed asset by name.
	template <typename T> std::shared_ptr<T> FindTyped(const std::string& name) {
		auto it = cache_.find(name);
		if (it != cache_.end()) {
			if (auto typed = std::dynamic_pointer_cast<T>(it->second)) {
				return typed;
			}
		}
		return nullptr;
	}

	template <typename T> std::shared_ptr<const T> FindTyped(const std::string& name) const {
		if (const auto it = cache_.find(name); it != cache_.end()) {
			if (auto typed = std::dynamic_pointer_cast<const T>(it->second)) {
				return typed;
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
	/// Uses AssetRegistry::from_json_factory to create the correct type, calls Load(), and caches.
	/// Returns the loaded asset, or nullptr on failure.
	AssetPtr LoadFromFile(const std::string& path);

	/// Clear all cached assets.
	void Clear();

	/// Get count.
	[[nodiscard]] size_t Size() const { return cache_.size(); }

private:
	AssetCache() = default;
	std::unordered_map<std::string, AssetPtr> cache_;
};

} // namespace engine::assets
