#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <imgui.h>

namespace editor {

/**
 * @brief Registry for asset thumbnail/preview generation.
 *
 * Asset types register preview generators keyed by file extension.
 * The asset browser queries this to get thumbnails for any recognized file type
 * instead of hardcoding preview logic per type.
 *
 * Generators return a GL texture ID (cached internally by the registry).
 *
 * Usage:
 *   registry.Register({".png", ".jpg"}, [](const path& p) -> uint32_t { ... });
 *   uint32_t tex = registry.GetOrGeneratePreview(some_file_path);
 */
class AssetPreviewRegistry {
public:
	/// Generates a GL texture ID for a given file path. Returns 0 on failure.
	using PreviewGenerator = std::function<uint32_t(const std::filesystem::path& path)>;

	/// Returns color + icon label for a file type. Used for non-thumbnail display.
	struct FileTypeAppearance {
		std::string icon;   // Short icon label, e.g. "[T]", "[S]"
		ImVec4 color;       // Tint color for the icon
		FileTypeAppearance() : color(0.55f, 0.55f, 0.55f, 1.0f) {}
		FileTypeAppearance(std::string i, ImVec4 c) : icon(std::move(i)), color(c) {}
	};

	using AppearanceFn = std::function<FileTypeAppearance(const std::filesystem::path& path)>;

	static AssetPreviewRegistry& Instance();

	/**
	 * @brief Register a preview generator for file extensions
	 * @param extensions Extensions including dot, e.g. {".png", ".jpg"}
	 * @param generator Function that creates a GL texture from a file path
	 */
	void RegisterPreview(const std::vector<std::string>& extensions, PreviewGenerator generator);

	/**
	 * @brief Register an icon/color appearance for file extensions (non-thumbnail fallback)
	 * @param extensions Extensions including dot
	 * @param appearance Function returning icon + color for the file type
	 */
	void RegisterAppearance(const std::vector<std::string>& extensions, AppearanceFn appearance);

	/**
	 * @brief Get or generate a thumbnail texture for a file
	 * @return GL texture ID, or 0 if no generator registered or generation failed
	 */
	uint32_t GetOrGeneratePreview(const std::filesystem::path& path);

	/**
	 * @brief Check if a preview generator exists for this file's extension
	 */
	[[nodiscard]] bool HasPreviewGenerator(const std::filesystem::path& path) const;

	/**
	 * @brief Get icon/color appearance for a file. Falls back to generic if unregistered.
	 */
	[[nodiscard]] FileTypeAppearance GetAppearance(const std::filesystem::path& path) const;

	/**
	 * @brief Free all cached preview GL textures
	 */
	void ClearCache();

private:
	AssetPreviewRegistry() = default;

	std::unordered_map<std::string, PreviewGenerator> preview_generators_;
	std::unordered_map<std::string, AppearanceFn> appearance_fns_;
	std::unordered_map<std::string, uint32_t> preview_cache_; // path -> GL texture ID
};

/// Register built-in preview generators for common asset types (images, sounds, etc.)
void RegisterBuiltinPreviews();

} // namespace editor
