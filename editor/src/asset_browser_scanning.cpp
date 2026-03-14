#include "asset_browser_panel.h"
#include "asset_preview_registry.h"
#include "file_utils.h"

#include <algorithm>
#include <filesystem>

namespace editor {

void AssetBrowserPanel::RefreshCurrentDirectory() {
	current_items_.clear();
	AssetPreviewRegistry::Instance().ClearCache();

	if (!std::filesystem::exists(current_directory_)) {
		needs_refresh_ = false;
		return;
	}

	try {
		for (const auto& entry : std::filesystem::directory_iterator(current_directory_)) {
			FileSystemItem item(entry.path(), entry.is_directory());
			item.type_icon = GetFileIcon(entry);
			current_items_.push_back(item);
		}

		// Sort: directories first, then alphabetically
		std::sort(current_items_.begin(), current_items_.end(), [](const auto& a, const auto& b) {
			if (a.is_directory != b.is_directory) {
				return a.is_directory > b.is_directory;
			}
			return a.display_name < b.display_name;
		});
	}
	catch (const std::filesystem::filesystem_error&) {
		// Silently handle errors
	}

	needs_refresh_ = false;
}

void AssetBrowserPanel::ScanForPrefabs() {
	prefab_files_.clear();
	prefabs_scanned_ = true;

	// Scan the assets directory for .prefab.json files
	const std::filesystem::path assets_dir("assets");
	if (!std::filesystem::exists(assets_dir)) {
		return;
	}

	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_dir)) {
			if (entry.is_regular_file()) {
				const auto& path = entry.path();
				const std::string filename = path.filename().string();
				if (filename.ends_with(".prefab.json")) {
					prefab_files_.push_back(path.string());
				}
			}
		}
		// Also check the current directory
		if (std::filesystem::exists(".")) {
			for (const auto& entry : std::filesystem::directory_iterator(".")) {
				if (entry.is_regular_file()) {
					const auto& path = entry.path();
					const std::string filename = path.filename().string();
					if (filename.ends_with(".prefab.json")) {
						prefab_files_.push_back(path.string());
					}
				}
			}
		}
	}
	catch (const std::exception&) {
		// Silently handle filesystem errors
	}
}

} // namespace editor
