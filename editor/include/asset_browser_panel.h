#pragma once

#include "editor_callbacks.h"
#include "editor_panel.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

import engine;

namespace editor {

/**
 * @brief File system item in the asset browser
 */
struct FileSystemItem {
	std::filesystem::path path;
	std::string display_name;
	bool is_directory{false};
	std::string type_icon; // Icon to display (e.g., "[T]", "[S]", "[P]")

	FileSystemItem() = default;
	FileSystemItem(std::filesystem::path p, bool is_dir = false) : path(std::move(p)), is_directory(is_dir) {
		display_name = path.filename().string();
	}
};

/**
 * @brief View mode for asset display
 */
enum class AssetViewMode { List, Grid };

/**
 * @brief Asset type for filtering
 */
enum class AssetFileType { All, Scene, Prefab, Texture, Sound, Mesh, Script, Shader, DataTable, Directory };

/**
 * @brief Info about the currently selected asset
 */
struct AssetSelection {
	engine::scene::AssetType type{};
	std::string name;

	void Clear() { name.clear(); }

	[[nodiscard]] bool IsValid() const { return !name.empty(); }
};

/**
 * @brief Asset browser panel for viewing and managing scene assets
 *
 * Displays assets organized by type (Shaders, Textures, etc.) in a tree view.
 * Uses AssetRegistry field metadata to render asset properties dynamically.
 * Supports selection for editing in Properties panel and context menus for
 * creating new assets.
 */
class AssetBrowserPanel : public EditorPanel {
public:
	AssetBrowserPanel() = default;
	~AssetBrowserPanel() override = default;

	[[nodiscard]] std::string_view GetPanelName() const override;

	/**
	 * @brief Set callbacks for panel events
	 */
	void SetCallbacks(const EditorCallbacks& callbacks);

	/**
	 * @brief Render the asset browser panel
	 * @param scene The current scene to display assets from
	 * @param selected_asset Currently selected asset (for highlighting)
	 */
	void Render(engine::scene::Scene* scene, const AssetSelection& selected_asset);

private:
	/**
	 * @brief Render assets for a specific asset type
	 */
	void RenderAssetCategory(
			engine::scene::Scene* scene,
			const engine::scene::AssetTypeInfo& type_info,
			const AssetSelection& selected_asset) const;

	/**
	 * @brief Render the Prefabs section listing .prefab.json files
	 */
	void RenderPrefabSection();

	/**
	 * @brief Scan the assets directory for .prefab.json files
	 */
	void ScanForPrefabs();

	/**
	 * @brief Render the directory tree (left panel)
	 */
	void RenderDirectoryTree();

	/**
	 * @brief Render directory tree node recursively
	 */
	void RenderDirectoryTreeNode(const std::filesystem::path& path);

	/**
	 * @brief Render the content view (right panel)
	 */
	void RenderContentView();

	/**
	 * @brief Render breadcrumb navigation bar
	 */
	void RenderBreadcrumbBar();

	/**
	 * @brief Render search and filter bar
	 */
	void RenderSearchBar();

	/**
	 * @brief Render an asset item in list mode
	 */
	void RenderAssetItemList(const FileSystemItem& item);

	/**
	 * @brief Render an asset item in grid mode
	 */
	void RenderAssetItemGrid(const FileSystemItem& item);

	/**
	 * @brief Show context menu for an asset/directory
	 */
	void ShowItemContextMenu(const FileSystemItem& item);

	/**
	 * @brief Show context menu for empty space
	 */
	void ShowEmptySpaceContextMenu();

	/**
	 * @brief Refresh the current directory contents
	 */
	void RefreshCurrentDirectory();

	/**
	 * @brief Get the icon for a file based on its extension
	 */
	static std::string GetFileIcon(const std::filesystem::path& path);

	/**
	 * @brief Get the asset file type from extension
	 */
	static AssetFileType GetAssetFileType(const std::filesystem::path& path);

	/**
	 * @brief Check if an item passes the current filter
	 */
	bool PassesFilter(const FileSystemItem& item) const;

	EditorCallbacks callbacks_;
	bool prefabs_scanned_ = false;
	std::vector<std::string> prefab_files_; // Cached list of .prefab.json paths

	// New members for enhanced browser
	std::filesystem::path assets_root_{"assets"};
	std::filesystem::path current_directory_{"assets"};
	std::vector<FileSystemItem> current_items_;
	AssetViewMode view_mode_{AssetViewMode::Grid};
	char search_buffer_[256]{};
	AssetFileType filter_type_{AssetFileType::All};
	bool needs_refresh_{true};
	std::filesystem::path selected_item_path_;
};

} // namespace editor
