#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace editor {

/**
 * @brief Registry mapping asset type strings to editor open handlers
 *
 * Replaces per-type callbacks (on_open_tileset, on_open_sprite_atlas, etc.)
 * with a generic dispatch system. Asset files include an "asset_type" field
 * in their JSON, and the registry routes to the correct editor panel.
 *
 * Usage:
 *   registry.Register("tileset", [&](const std::string& path) { tileset_panel.OpenTileset(path); });
 *   registry.Register("sprite_atlas", [&](const std::string& path) { sprite_panel.OpenAtlas(path); });
 *   // Later, when opening a file:
 *   registry.OpenAssetFile(path);  // Reads JSON, dispatches by "asset_type"
 */
class AssetEditorRegistry {
public:
	using OpenHandler = std::function<void(const std::string& path)>;

	/**
	 * @brief Register a handler for an asset type
	 * @param asset_type The "asset_type" value in JSON files (e.g., "tileset", "sprite_atlas")
	 * @param handler Function to call when opening this asset type
	 */
	void Register(const std::string& asset_type, OpenHandler handler) { handlers_[asset_type] = std::move(handler); }

	/**
	 * @brief Register a file extension handler for non-JSON files
	 * @param extension The extension including dot (e.g., ".lua", ".glsl")
	 * @param handler Function to call when opening files with this extension
	 */
	void RegisterExtension(const std::string& extension, OpenHandler handler) {
		extension_handlers_[extension] = std::move(handler);
	}

	/**
	 * @brief Try to open an asset file by reading its "asset_type" field or matching extension
	 * @param path Path to the asset file
	 * @return true if a handler was found and invoked
	 */
	bool TryOpen(const std::string& path) const;

	/**
	 * @brief Check if a handler exists for the given asset type
	 */
	[[nodiscard]] bool HasHandler(const std::string& asset_type) const { return handlers_.contains(asset_type); }

	/**
	 * @brief The standard JSON key used to identify asset types in files
	 */
	static constexpr const char* TYPE_KEY = "asset_type";

private:
	std::unordered_map<std::string, OpenHandler> handlers_;
	std::unordered_map<std::string, OpenHandler> extension_handlers_;
};

} // namespace editor
