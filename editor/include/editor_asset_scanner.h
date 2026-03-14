#pragma once

#include <string>
#include <vector>

namespace editor {

/// Scan a project directory for asset files and register them in the AssetCache.
///
/// This is an editor/tooling concern — the engine itself does not scan filesystems.
/// Handles both JSON asset files (filtered by extensions) and raw asset files
/// (matched by registered file importers, e.g., .wav → SoundAssetInfo).
///
/// Assets are registered (metadata parsed, GUID assigned) but NOT loaded —
/// no system resources are allocated until Load() is called on each asset.
///
/// @param directory Path to scan (e.g., "assets/")
/// @param extensions File extensions to match for JSON files (e.g., {".material.json"}).
///                   If empty, scans all .json files. Raw file importers run regardless.
/// @return Number of newly registered assets.
size_t ScanAssetsDirectory(const std::string& directory, const std::vector<std::string>& extensions = {});

} // namespace editor
