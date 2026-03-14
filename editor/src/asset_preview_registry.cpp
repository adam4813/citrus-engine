#include "asset_preview_registry.h"

#include <algorithm>
#include <imgui.h>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <vector>

namespace editor {

AssetPreviewRegistry& AssetPreviewRegistry::Instance() {
	static AssetPreviewRegistry instance;
	return instance;
}

void AssetPreviewRegistry::RegisterPreview(
		const std::vector<std::string>& extensions, PreviewGenerator generator) {
	for (const auto& ext : extensions) {
		std::string lower_ext = ext;
		std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
		preview_generators_[lower_ext] = generator;
	}
}

void AssetPreviewRegistry::RegisterAppearance(
		const std::vector<std::string>& extensions, AppearanceFn appearance) {
	for (const auto& ext : extensions) {
		std::string lower_ext = ext;
		std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
		appearance_fns_[lower_ext] = appearance;
	}
}

uint32_t AssetPreviewRegistry::GetOrGeneratePreview(const std::filesystem::path& path) {
	const std::string key = path.string();
	if (const auto it = preview_cache_.find(key); it != preview_cache_.end()) {
		return it->second;
	}

	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	const auto gen_it = preview_generators_.find(ext);
	if (gen_it == preview_generators_.end()) {
		return 0;
	}

	const uint32_t tex_id = gen_it->second(path);
	preview_cache_[key] = tex_id;
	return tex_id;
}

bool AssetPreviewRegistry::HasPreviewGenerator(const std::filesystem::path& path) const {
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return preview_generators_.contains(ext);
}

AssetPreviewRegistry::FileTypeAppearance AssetPreviewRegistry::GetAppearance(
		const std::filesystem::path& path) const {
	if (std::filesystem::is_directory(path)) {
		return {"[D]", ImVec4(0.95f, 0.85f, 0.3f, 1.0f)};
	}

	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// Check for compound extensions (e.g. .material.json)
	const auto filename = path.filename().string();
	if (ext == ".json") {
		if (filename.ends_with(".scene.json")) {
			return {"[Sc]", ImVec4(0.3f, 0.85f, 0.4f, 1.0f)};
		}
		if (filename.ends_with(".prefab.json")) {
			return {"[P]", ImVec4(0.4f, 0.6f, 1.0f, 1.0f)};
		}
		if (filename.ends_with(".material.json")) {
			return {"[Mt]", ImVec4(0.7f, 0.4f, 0.9f, 1.0f)};
		}
		if (filename.ends_with(".tileset.json")) {
			return {"[TS]", ImVec4(0.95f, 0.75f, 0.2f, 1.0f)};
		}
		if (filename.ends_with(".data.json")) {
			return {"[Dt]", ImVec4(0.65f, 0.65f, 0.65f, 1.0f)};
		}
		if (filename.ends_with(".sound.json") || filename.ends_with(".sfx.json")) {
			return {"[S]", ImVec4(1.0f, 0.55f, 0.2f, 1.0f)};
		}
		if (filename.ends_with(".shader.json")) {
			return {"[Sh]", ImVec4(0.9f, 0.4f, 0.7f, 1.0f)};
		}
		if (filename.ends_with(".mesh.json")) {
			return {"[M]", ImVec4(0.7f, 0.4f, 0.9f, 1.0f)};
		}
		if (filename.ends_with(".texture.json")) {
			return {"[T]", ImVec4(0.95f, 0.75f, 0.2f, 1.0f)};
		}
		return {"[J]", ImVec4(0.55f, 0.55f, 0.55f, 1.0f)};
	}

	if (const auto it = appearance_fns_.find(ext); it != appearance_fns_.end()) {
		return it->second(path);
	}

	return {"[F]", ImVec4(0.55f, 0.55f, 0.55f, 1.0f)};
}

void AssetPreviewRegistry::ClearCache() {
	for (const auto& [path, tex_id] : preview_cache_) {
		if (tex_id != 0) {
			const auto gl_id = static_cast<unsigned int>(tex_id);
			glDeleteTextures(1, &gl_id);
		}
	}
	preview_cache_.clear();
}

namespace {

uint32_t GenerateImageThumbnail(const std::filesystem::path& path) {
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
	if (!pixels) {
		return 0;
	}

	constexpr int MAX_THUMB = 128;
	int thumb_w = width;
	int thumb_h = height;
	std::vector<unsigned char> thumb_data;

	if (width > MAX_THUMB || height > MAX_THUMB) {
		const float scale =
				static_cast<float>(MAX_THUMB) / static_cast<float>(std::max(width, height));
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

	unsigned int tex_id = 0;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, thumb_w, thumb_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(pixels);
	return tex_id;
}

} // namespace

void RegisterBuiltinPreviews() {
	auto& registry = AssetPreviewRegistry::Instance();

	// Image thumbnails
	registry.RegisterPreview(
			{".png", ".jpg", ".jpeg", ".tga", ".bmp"}, [](const std::filesystem::path& p) -> uint32_t {
				return GenerateImageThumbnail(p);
			});

	// Icon appearances for file types without visual previews
	registry.RegisterAppearance(
			{".png", ".jpg", ".jpeg", ".tga", ".bmp"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[T]", ImVec4(0.95f, 0.75f, 0.2f, 1.0f)};
			});

	registry.RegisterAppearance(
			{".wav", ".ogg", ".mp3", ".flac"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[S]", ImVec4(1.0f, 0.55f, 0.2f, 1.0f)};
			});

	registry.RegisterAppearance(
			{".obj", ".fbx", ".gltf", ".glb"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[M]", ImVec4(0.7f, 0.4f, 0.9f, 1.0f)};
			});

	registry.RegisterAppearance(
			{".lua", ".as", ".js"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[Sc]", ImVec4(0.3f, 0.8f, 0.8f, 1.0f)};
			});

	registry.RegisterAppearance(
			{".glsl", ".vert", ".frag", ".shader"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[Sh]", ImVec4(0.9f, 0.4f, 0.7f, 1.0f)};
			});

	registry.RegisterAppearance({".scene"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[Sc]", ImVec4(0.3f, 0.85f, 0.4f, 1.0f)};
			});

	registry.RegisterAppearance({".prefab"},
			[](const std::filesystem::path&) -> AssetPreviewRegistry::FileTypeAppearance {
				return {"[P]", ImVec4(0.4f, 0.6f, 1.0f, 1.0f)};
			});
}

} // namespace editor
