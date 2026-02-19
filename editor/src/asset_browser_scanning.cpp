#include "asset_browser_panel.h"
#include "file_utils.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace editor {

void AssetBrowserPanel::ClearThumbnailCache() {
	for (const auto& [path, tex_id] : thumbnail_cache_) {
		if (tex_id != 0) {
			glDeleteTextures(1, &tex_id);
		}
	}
	thumbnail_cache_.clear();
}

uint32_t AssetBrowserPanel::GetOrLoadThumbnail(const std::filesystem::path& path) {
	const std::string key = path.string();
	if (const auto it = thumbnail_cache_.find(key); it != thumbnail_cache_.end()) {
		return it->second;
	}

	// Load image with stb_image
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
	if (!pixels) {
		thumbnail_cache_[key] = 0;
		return 0;
	}

	// Downscale to thumbnail size if needed
	constexpr int MAX_THUMB = 128;
	int thumb_w = width;
	int thumb_h = height;
	std::vector<unsigned char> thumb_data;

	if (width > MAX_THUMB || height > MAX_THUMB) {
		const float scale = static_cast<float>(MAX_THUMB) / static_cast<float>(std::max(width, height));
		thumb_w = std::max(1, static_cast<int>(static_cast<float>(width) * scale));
		thumb_h = std::max(1, static_cast<int>(static_cast<float>(height) * scale));
		thumb_data.resize(static_cast<size_t>(thumb_w) * thumb_h * 4);
		for (int y = 0; y < thumb_h; ++y) {
			for (int x = 0; x < thumb_w; ++x) {
				const int src_x = x * width / thumb_w;
				const int src_y = y * height / thumb_h;
				const int src_idx = (src_y * width + src_x) * 4;
				const int dst_idx = (y * thumb_w + x) * 4;
				std::memcpy(&thumb_data[dst_idx], &pixels[src_idx], 4);
			}
		}
	}

	const unsigned char* tex_pixels = thumb_data.empty() ? pixels : thumb_data.data();

	// Create GL texture
	GLuint tex_id = 0;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thumb_w, thumb_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);

	thumbnail_cache_[key] = tex_id;
	return tex_id;
}

void AssetBrowserPanel::RefreshCurrentDirectory() {
	current_items_.clear();
	ClearThumbnailCache();

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
