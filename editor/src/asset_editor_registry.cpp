#include "asset_editor_registry.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

import engine;

using json = nlohmann::json;

namespace editor {

bool AssetEditorRegistry::TryOpen(const std::string& path) const {
	const std::filesystem::path file_path(path);

	// For JSON files, try to read the "asset_type" field
	const auto ext = file_path.extension().string();
	if (ext == ".json") {
		try {
			auto text = engine::assets::AssetManager::LoadTextFile(file_path);
			if (text) {
				json j = json::parse(*text);
				if (j.contains(TYPE_KEY) && j[TYPE_KEY].is_string()) {
					const auto asset_type = j[TYPE_KEY].get<std::string>();
					auto it = handlers_.find(asset_type);
					if (it != handlers_.end()) {
						it->second(path);
						return true;
					}
				}
			}
		}
		catch (const std::exception&) {
			// JSON parse failed, fall through to extension matching
		}

		// Fallback: match compound extensions for legacy files without asset_type
		const auto filename = file_path.filename().string();
		if (filename.ends_with(".tileset.json")) {
			auto it = handlers_.find("tileset");
			if (it != handlers_.end()) {
				it->second(path);
				return true;
			}
		}
		if (filename.ends_with(".sprites.json")) {
			auto it = handlers_.find("sprite_atlas");
			if (it != handlers_.end()) {
				it->second(path);
				return true;
			}
		}
		if (filename.ends_with(".data.json")) {
			auto it = handlers_.find("data_table");
			if (it != handlers_.end()) {
				it->second(path);
				return true;
			}
		}
		if (filename.ends_with(".sfx.json")) {
			auto it = handlers_.find("sound");
			if (it != handlers_.end()) {
				it->second(path);
				return true;
			}
		}
		if (filename.ends_with(".prefab.json")) {
			auto it = handlers_.find("prefab");
			if (it != handlers_.end()) {
				it->second(path);
				return true;
			}
		}

		return false;
	}

	// Non-JSON files: match by extension
	auto ext_it = extension_handlers_.find(ext);
	if (ext_it != extension_handlers_.end()) {
		ext_it->second(path);
		return true;
	}

	return false;
}

} // namespace editor
