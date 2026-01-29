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
	// Future: TEXTURE, MESH, MATERIAL, etc.
};

// Forward declaration
struct AssetInfo;

/// Registry for asset type factories (Factory + Registry pattern)
/// Asset types self-register their deserialization factories
class AssetRegistry {
public:
	using FactoryFn = std::function<std::unique_ptr<AssetInfo>(const nlohmann::json&)>;

	/// Get singleton instance
	static AssetRegistry& Instance();

	/// Register an asset type factory
	/// @param type_name The "type" string in JSON (e.g., "shader")
	/// @param factory Function that creates the asset from JSON
	void Register(const std::string& type_name, FactoryFn factory);

	/// Create an asset from JSON using registered factory
	/// @return nullptr if type is unknown
	[[nodiscard]] std::unique_ptr<AssetInfo> Create(const nlohmann::json& j) const;

	/// Check if a type is registered
	[[nodiscard]] bool IsRegistered(const std::string& type_name) const;

private:
	AssetRegistry() = default;
	std::unordered_map<std::string, FactoryFn> factories_;
};

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
