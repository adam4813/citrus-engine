#pragma once

#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <vector>

namespace editor {

/// Basic file system entry used by both the file dialog and asset browser.
struct FileEntry {
	std::filesystem::path path;
	std::string name;
	bool is_directory = false;
	std::string icon; // Short icon label, e.g. "[T]", "[D]"
};

/// Return a short icon label for the given path based on extension.
inline std::string GetFileIcon(const std::filesystem::path& p) {
	if (std::filesystem::is_directory(p)) {
		return "[D]";
	}

	const auto ext = p.extension().string();
	const auto fname = p.filename().string();
	if (ext == ".scene" || fname.ends_with(".scene.json")) {
		return "[Sc]";
	}
	if (ext == ".prefab" || fname.ends_with(".prefab.json")) {
		return "[P]";
	}
	if (fname.ends_with(".tileset.json")) {
		return "[TS]";
	}
	if (fname.ends_with(".data.json")) {
		return "[Dt]";
	}
	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") {
		return "[T]";
	}
	if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
		return "[S]";
	}
	if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
		return "[M]";
	}
	if (ext == ".lua" || ext == ".as" || ext == ".js") {
		return "[Sc]";
	}
	if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".shader") {
		return "[Sh]";
	}
	if (ext == ".json") {
		return "[J]";
	}
	return "[F]";
}

/// List directory contents, optionally filtering by extensions.
/// Directories first, then alphabetical. Empty extensions = no filter.
inline std::vector<FileEntry>
ListDirectory(const std::filesystem::path& dir, const std::vector<std::string>& extensions = {}) {
	std::vector<FileEntry> result;
	if (!std::filesystem::exists(dir)) {
		return result;
	}

	try {
		for (const auto& e : std::filesystem::directory_iterator(dir)) {
			if (e.is_directory()) {
				result.push_back({e.path(), e.path().filename().string(), true, "[D]"});
			}
			else {
				// Extension filter
				if (!extensions.empty()) {
					const auto fname = e.path().filename().string();
					bool match = false;
					for (const auto& ext : extensions) {
						if (fname.size() >= ext.size()
							&& fname.compare(fname.size() - ext.size(), ext.size(), ext) == 0) {
							match = true;
							break;
						}
					}
					if (!match) {
						continue;
					}
				}
				result.push_back({e.path(), e.path().filename().string(), false, GetFileIcon(e.path())});
			}
		}
		std::ranges::sort(result, [](const FileEntry& a, const FileEntry& b) {
			return a.is_directory != b.is_directory ? a.is_directory > b.is_directory : a.name < b.name;
		});
	}
	catch (...) {
	}

	return result;
}

/// Render an ImGui directory tree with selection. Returns true if selection changed.
/// @param dir          The directory to render as a tree node
/// @param current_dir  Currently selected directory (highlighted)
/// @param new_dir      [out] Set to the clicked directory if selection changed
/// @param default_open Whether the root node starts open
inline bool RenderDirectoryTree(
		const std::filesystem::path& dir,
		const std::filesystem::path& current_dir,
		std::filesystem::path& new_dir,
		const bool default_open = false) {
	if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
		return false;

	bool changed = false;
	const std::string name = dir.filename().string();
	const bool is_selected = (dir == current_dir);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (is_selected) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (default_open) {
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	bool has_subdirs = false;
	try {
		for (const auto& e : std::filesystem::directory_iterator(dir)) {
			if (e.is_directory()) {
				has_subdirs = true;
				break;
			}
		}
	}
	catch (...) {
	}

	if (!has_subdirs)
		flags |= ImGuiTreeNodeFlags_Leaf;

	const bool open = ImGui::TreeNodeEx(name.c_str(), flags);
	if (ImGui::IsItemClicked()) {
		new_dir = dir;
		changed = true;
	}

	if (open) {
		if (has_subdirs) {
			try {
				std::vector<std::filesystem::path> subdirs;
				for (const auto& e : std::filesystem::directory_iterator(dir)) {
					if (e.is_directory()) {
						subdirs.push_back(e.path());
					}
				}
				std::ranges::sort(subdirs);
				for (const auto& sd : subdirs) {
					if (RenderDirectoryTree(sd, current_dir, new_dir, false)) {
						changed = true;
					}
				}
			}
			catch (...) {
			}
		}
		ImGui::TreePop();
	}

	return changed;
}

/// Recursively scan the assets/ directory for files matching the given extensions.
/// Returns paths relative to assets/ root with forward slashes.
inline std::vector<std::string> ScanAssetFiles(const std::vector<std::string>& extensions) {
	std::vector<std::string> results;
	const std::filesystem::path assets_root{"assets"};
	if (!std::filesystem::exists(assets_root)) {
		return results;
	}
	try {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_root)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			const auto ext = entry.path().extension().string();
			for (const auto& match_ext : extensions) {
				if (ext == match_ext) {
					auto rel = std::filesystem::relative(entry.path(), assets_root).string();
					std::ranges::replace(rel, '\\', '/');
					results.push_back(rel);
					break;
				}
			}
		}
	}
	catch (...) {
	}
	std::ranges::sort(results);
	return results;
}

} // namespace editor
