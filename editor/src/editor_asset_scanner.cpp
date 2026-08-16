#include "editor_asset_scanner.h"

#include <filesystem>

import engine;

namespace editor {

size_t ScanAssetsDirectory(const std::string& directory, const std::vector<std::string>& extensions) {
	size_t registered = 0;
	auto& cache = engine::assets::AssetCache::Instance();

	const std::filesystem::path dir_path(directory);
	if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
		return 0;
	}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
		if (!entry.is_regular_file()) {
			continue;
		}

		const auto filename = entry.path().filename().string();
		const auto path_str = entry.path().string();

		// Skip sidecar descriptors: they are metadata for a raw source file, not
		// standalone assets, and are consumed via AssetCache::LoadOrImportSource.
		if (filename.length() >= 10 && filename.substr(filename.length() - 10) == ".meta.json") {
			continue;
		}

		const bool is_json = filename.length() >= 5 && filename.substr(filename.length() - 5) == ".json";

		// Raw asset files (e.g. .wav, .png): import via the registered importer, creating
		// a persistent "<source>.meta.json" descriptor and indexing the source path so the
		// asset resolves by path/GUID later.
		if (!is_json) {
			if (cache.LoadOrImportSource(path_str)) {
				++registered;
			}
			continue;
		}

		// Filter JSON asset files by the requested extensions (e.g. ".material.json").
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

		// JSON-native asset definition: load + cache (path-indexed, GUID recovered/assigned).
		if (cache.LoadFromFile(path_str)) {
			++registered;
		}
	}

	return registered;
}

} // namespace editor
