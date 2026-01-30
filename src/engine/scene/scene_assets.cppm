module;

#include <cstdint>
#include <functional>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

export module engine.scene.assets;

import engine.rendering;

export namespace engine::scene {

/// Type of asset in the scene
enum class AssetType : uint8_t {
	SHADER,
	MESH,
	// Future: TEXTURE, MATERIAL, etc.
};

// === ASSET FIELD REFLECTION SYSTEM ===

/**
 * @brief Supported field types for asset editing (mirrors component FieldType)
 */
enum class AssetFieldType {
	String,
	FilePath, // String displayed with file browser hint
	Int,
	Float,
	Bool,
	ReadOnly // Display-only
};

/**
 * @brief Metadata about a single field within an asset
 */
struct AssetFieldInfo {
	std::string name;
	std::string display_name; // Human-readable label
	AssetFieldType type{AssetFieldType::ReadOnly};
	size_t offset{}; // Byte offset into asset struct
	size_t size{}; // Size of the field in bytes
};

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
	std::function<std::unique_ptr<AssetInfo>(const nlohmann::json&)> from_json_factory;
	std::function<std::shared_ptr<AssetInfo>()> create_default_factory;
	std::vector<AssetFieldInfo> fields;
};

/**
 * @brief Builder for registering asset types with fluent API
 *
 * Usage:
 *   AssetRegistry::Instance().RegisterType<ShaderAssetInfo>("shader", AssetType::SHADER)
 *       .DisplayName("Shader")
 *       .Category("Rendering")
 *       .Field("name", &ShaderAssetInfo::name, "Name")
 *       .Field("vertex_path", &ShaderAssetInfo::vertex_path, "Vertex Shader", AssetFieldType::FilePath)
 *       .FromJson([](const json& j) { ... })
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

	/// Register a field with automatic type deduction
	template <typename FieldT, typename ClassT>
	AssetTypeRegistration&
	Field(const std::string& field_name, FieldT ClassT::* member_ptr, const std::string& display_name) {
		static_assert(
				std::is_base_of_v<ClassT, T> || std::is_same_v<ClassT, T>,
				"Member pointer must be from T or a base class of T");
		AssetFieldInfo field;
		field.name = field_name;
		field.display_name = display_name;
		field.type = DeduceFieldType<FieldT>();
		field.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member_ptr));
		field.size = sizeof(FieldT);
		info_.fields.push_back(std::move(field));
		return *this;
	}

	/// Register a field with explicit type override
	template <typename FieldT, typename ClassT>
	AssetTypeRegistration&
	Field(const std::string& field_name,
		  FieldT ClassT::* member_ptr,
		  const std::string& display_name,
		  AssetFieldType type_override) {
		static_assert(
				std::is_base_of_v<ClassT, T> || std::is_same_v<ClassT, T>,
				"Member pointer must be from T or a base class of T");
		AssetFieldInfo field;
		field.name = field_name;
		field.display_name = display_name;
		field.type = type_override;
		field.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member_ptr));
		field.size = sizeof(FieldT);
		info_.fields.push_back(std::move(field));
		return *this;
	}

	/// Set the JSON deserialization factory
	AssetTypeRegistration& FromJson(std::function<std::unique_ptr<AssetInfo>(const nlohmann::json&)> factory) {
		info_.from_json_factory = std::move(factory);
		return *this;
	}

	/// Set the default creation factory (for "New Asset" in editor)
	AssetTypeRegistration& CreateDefault(std::function<std::shared_ptr<AssetInfo>()> factory) {
		info_.create_default_factory = std::move(factory);
		return *this;
	}

	void Build();

private:
	template <typename FieldT> static constexpr AssetFieldType DeduceFieldType() {
		if constexpr (std::is_same_v<FieldT, std::string>) {
			return AssetFieldType::String;
		}
		else if constexpr (std::is_same_v<FieldT, int> || std::is_same_v<FieldT, uint32_t>) {
			return AssetFieldType::Int;
		}
		else if constexpr (std::is_same_v<FieldT, float>) {
			return AssetFieldType::Float;
		}
		else if constexpr (std::is_same_v<FieldT, bool>) {
			return AssetFieldType::Bool;
		}
		else {
			return AssetFieldType::ReadOnly;
		}
	}

	AssetRegistry& registry_;
	AssetTypeInfo info_;
};

/// Registry for asset type factories and editor metadata
/// Asset types self-register their deserialization factories and field info
class AssetRegistry {
public:
	using FactoryFn = std::function<std::unique_ptr<AssetInfo>(const nlohmann::json&)>;

	/// Get singleton instance
	static AssetRegistry& Instance();

	/// Start registering an asset type with builder pattern
	template <typename T> AssetTypeRegistration<T> RegisterType(const std::string& type_name, AssetType asset_type) {
		return AssetTypeRegistration<T>(*this, type_name, asset_type);
	}

	/// Legacy: Register an asset type factory only (for backward compat)
	/// @param type_name The "type" string in JSON (e.g., "shader")
	/// @param factory Function that creates the asset from JSON
	void Register(const std::string& type_name, FactoryFn factory);

	/// Create an asset from JSON using registered factory
	/// @return nullptr if type is unknown
	[[nodiscard]] std::unique_ptr<AssetInfo> Create(const nlohmann::json& j) const;

	/// Create a new default asset of the specified type
	/// @return nullptr if type has no default factory
	[[nodiscard]] std::shared_ptr<AssetInfo> CreateDefault(AssetType type) const;

	/// Check if a type is registered
	[[nodiscard]] bool IsRegistered(const std::string& type_name) const;

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
	std::unordered_map<std::string, FactoryFn> factories_; // Legacy
	std::vector<AssetTypeInfo> types_;
};

// Template implementation
template <typename T>
AssetTypeRegistration<T>::AssetTypeRegistration(
		AssetRegistry& registry, const std::string& type_name, AssetType asset_type) : registry_(registry) {
	info_.type_name = type_name;
	info_.display_name = type_name; // Default, override with DisplayName()
	info_.category = "Other";
	info_.asset_type = asset_type;
}

template <typename T> void AssetTypeRegistration<T>::Build() {
	// Also register legacy factory if from_json_factory is set
	if (info_.from_json_factory) {
		registry_.Register(info_.type_name, info_.from_json_factory);
	}
	registry_.AddTypeInfo(std::move(info_));
}

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

	/// Check if this asset has been loaded
	[[nodiscard]] bool IsLoaded() const { return loaded_; }

	/// Serialize this asset to JSON
	virtual void ToJson(nlohmann::json& j) const;

	/// Create an asset from JSON (delegates to AssetRegistry)
	static std::unique_ptr<AssetInfo> FromJson(const nlohmann::json& j);

protected:
	/// Override to allocate resources before Load (e.g., reserve shader ID)
	virtual void DoInitialize() {}

	/// Override to implement asset-specific loading logic
	virtual bool DoLoad() { return true; }

	virtual void DoUnload() {}
};

/// Shader asset definition
struct ShaderAssetInfo : AssetInfo {
	std::string vertex_path;
	std::string fragment_path;
	rendering::ShaderId id{rendering::INVALID_SHADER}; // Populated in DoInitialize, compiled in DoLoad

	ShaderAssetInfo() : AssetInfo("", AssetType::SHADER) {}
	ShaderAssetInfo(std::string asset_name, std::string vert, std::string frag) :
			AssetInfo(std::move(asset_name), AssetType::SHADER), vertex_path(std::move(vert)),
			fragment_path(std::move(frag)) {}

	void ToJson(nlohmann::json& j) const override;

	/// Register this asset type with the AssetRegistry
	static void RegisterType();

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
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
	std::string mesh_type; // "quad", "cube", "sphere", "capsule", "file"
	float params[3]{1.0f, 1.0f, 1.0f}; // Interpreted based on mesh_type
	std::string file_path; // Only used when mesh_type == "file"
	rendering::MeshId id{rendering::INVALID_MESH}; // Populated in DoInitialize, geometry in DoLoad

	MeshAssetInfo() : AssetInfo("", AssetType::MESH), mesh_type(mesh_types::QUAD) {}
	MeshAssetInfo(std::string asset_name, std::string type) :
			AssetInfo(std::move(asset_name), AssetType::MESH), mesh_type(std::move(type)) {}

	void ToJson(nlohmann::json& j) const override;

	/// Register this asset type with the AssetRegistry
	static void RegisterType();

protected:
	void DoInitialize() override;
	bool DoLoad() override;
	void DoUnload() override;
};

/// Polymorphic asset storage using shared_ptr (allows editor to hold references)
using AssetPtr = std::shared_ptr<AssetInfo>;

/// Asset list owned by a scene
class SceneAssets {
public:
	/// Add an asset (shares ownership)
	void Add(AssetPtr asset);

	/// Remove an asset by name and type
	bool Remove(const std::string& name, AssetType type);

	/// Find asset by name and type (returns shared_ptr for safe reference holding)
	AssetPtr Find(const std::string& name, AssetType type);
	[[nodiscard]] std::shared_ptr<const AssetInfo> Find(const std::string& name, AssetType type) const;

	/// Get all assets
	[[nodiscard]] const std::vector<AssetPtr>& GetAll() const { return assets_; }
	std::vector<AssetPtr>& GetAll() { return assets_; }

	/// Get all assets of a specific type (returns shared_ptrs for safe reference holding)
	template <typename T> std::vector<std::shared_ptr<T>> GetAllOfType() {
		std::vector<std::shared_ptr<T>> result;
		for (auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<T>(asset)) {
				result.push_back(typed);
			}
		}
		return result;
	}

	template <typename T> std::vector<std::shared_ptr<const T>> GetAllOfType() const {
		std::vector<std::shared_ptr<const T>> result;
		for (const auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<const T>(asset)) {
				result.push_back(typed);
			}
		}
		return result;
	}

	/// Find typed asset by name (returns shared_ptr for safe reference holding)
	template <typename T> std::shared_ptr<T> FindTyped(const std::string& name) {
		for (auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<T>(asset); typed && typed->name == name) {
				return typed;
			}
		}
		return nullptr;
	}

	template <typename T> std::shared_ptr<const T> FindTyped(const std::string& name) const {
		for (const auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<const T>(asset); typed && typed->name == name) {
				return typed;
			}
		}
		return nullptr;
	}

	/// Find typed asset matching a predicate (returns shared_ptr for safe reference holding)
	template <typename T, typename Predicate> std::shared_ptr<T> FindTypedIf(Predicate pred) {
		for (auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<T>(asset); typed && pred(*typed)) {
				return typed;
			}
		}
		return nullptr;
	}

	template <typename T, typename Predicate> std::shared_ptr<const T> FindTypedIf(Predicate pred) const {
		for (const auto& asset : assets_) {
			if (auto typed = std::dynamic_pointer_cast<const T>(asset); typed && pred(*typed)) {
				return typed;
			}
		}
		return nullptr;
	}

	/// Clear all assets
	void Clear() { assets_.clear(); }

	/// Check if empty
	[[nodiscard]] bool Empty() const { return assets_.empty(); }

	/// Get count
	[[nodiscard]] size_t Size() const { return assets_.size(); }

private:
	std::vector<AssetPtr> assets_;
};

} // namespace engine::scene
