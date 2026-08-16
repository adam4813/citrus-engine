#include "build/asset_packager.h"

#include "build/build_reporter.h"
#include "build/project_model.h"

#include <system_error>

namespace editor::build {

bool CopyAssetPackager::Package(
	const ProjectModel& /*project*/,
	const std::filesystem::path& source_assets,
	const std::filesystem::path& dest_assets,
	BuildReporter& reporter
) {
	namespace fs = std::filesystem;
	std::error_code ec;

	if (!fs::exists(source_assets, ec)) {
		reporter.AppendLine("[assets] Source assets directory not found: " + source_assets.string());
		fs::create_directories(dest_assets, ec);
		return true;
	}

	fs::create_directories(dest_assets, ec);
	if (ec) {
		reporter.AppendLine("[assets] Failed to create dest directory: " + ec.message());
		return false;
	}

	reporter.AppendLine("[assets] Copying assets: " + source_assets.string() + " -> " + dest_assets.string());

	// Manually walk the tree so we can skip VCS-only marker files (.gitkeep, .gitignore)
	// that have no business landing in a shipped build.
	auto is_excluded = [](const std::string& name) {
		return name == ".gitkeep" || name == ".gitignore" || name == ".DS_Store" || name == "Thumbs.db";
	};

	size_t copied = 0;
	for (const auto& entry : fs::recursive_directory_iterator(source_assets, ec)) {
		if (ec) {
			reporter.AppendLine("[assets] Walk error: " + ec.message());
			return false;
		}
		const auto rel = fs::relative(entry.path(), source_assets, ec);
		if (ec) continue;
		const auto dst = dest_assets / rel;
		if (entry.is_directory()) {
			fs::create_directories(dst, ec);
		}
		else if (entry.is_regular_file()) {
			if (is_excluded(entry.path().filename().string())) continue;
			fs::create_directories(dst.parent_path(), ec);
			fs::copy_file(entry.path(), dst, fs::copy_options::overwrite_existing, ec);
			if (ec) {
				reporter.AppendLine("[assets] Copy failed for " + rel.string() + ": " + ec.message());
				return false;
			}
			++copied;
		}
	}

	reporter.AppendLine("[assets] Copied " + std::to_string(copied) + " file(s).");
	return true;
}

std::unique_ptr<IAssetPackager> MakeDefaultAssetPackager() { return std::make_unique<CopyAssetPackager>(); }

} // namespace editor::build
