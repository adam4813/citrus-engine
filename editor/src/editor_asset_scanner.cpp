#include "editor_asset_scanner.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

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

		// Try file importers first (handles raw asset files like .wav, .png, etc.)
		if (auto asset = cache.TryFileImport(filename, path_str)) {
			cache.Add(asset);
			++registered;
			continue;
		}

		// Filter by extensions for JSON asset files
		if (!extensions.empty()) {
			bool matched = false;
			for (const auto& ext : extensions) {
				if (filename.length() >= ext.length()
					&& filename.substr(filename.length() - ext.length()) == ext) {
					matched = true;
					break;
				}
			}
			if (!matched) {
				continue;
			}
		}
		else if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".json") {
			continue; // Default: only .json files
		}

		// Read and parse JSON
		std::ifstream file(entry.path());
		if (!file.is_open()) {
			continue;
		}
		std::ostringstream ss;
		ss << file.rdbuf();
		const auto text = ss.str();

		const auto j = nlohmann::json::parse(text, nullptr, false);
		if (j.is_discarded()) {
			continue;
		}

		const std::string type_str = j.value("type", "");
		if (type_str.empty()) {
			continue;
		}

		const auto* type_info = engine::assets::AssetTypeRegistry::Instance().GetTypeInfo(type_str);
		if (!type_info || !type_info->create_default_factory) {
			continue;
		}

		const auto asset = type_info->create_default_factory();
		if (!asset) {
			continue;
		}

		asset->FromJson(j);
		cache.Add(asset);
		++registered;
	}

	return registered;
}

} // namespace editor
