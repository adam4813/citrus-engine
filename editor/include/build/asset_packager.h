#pragma once

#include <filesystem>
#include <memory>

namespace editor::build {

struct ProjectModel;
class BuildReporter;

/// Strategy for moving project assets into a build's staging directory.
/// v1 ships CopyAssetPackager. Future packagers (compression, archive, texture
/// cooking) implement this interface without requiring changes to BuildJob.
class IAssetPackager {
public:
	virtual ~IAssetPackager() = default;
	virtual bool Package(
			const ProjectModel& project,
			const std::filesystem::path& source_assets,
			const std::filesystem::path& dest_assets,
			BuildReporter& reporter) = 0;
};

class CopyAssetPackager : public IAssetPackager {
public:
	bool Package(
			const ProjectModel& project,
			const std::filesystem::path& source_assets,
			const std::filesystem::path& dest_assets,
			BuildReporter& reporter) override;
};

std::unique_ptr<IAssetPackager> MakeDefaultAssetPackager();

} // namespace editor::build
