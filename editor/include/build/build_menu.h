#pragma once

#include "build/build_dialog.h"
#include "build/project_model.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace editor::build {

/// Owns the build subsystem state used by the editor: the loaded project (if any) and
/// the modal build dialog. Provides the helpers EditorScene calls from its File menu.
class BuildMenu {
public:
	/// Called when New Project succeeds, with the absolute path to the freshly created
	/// startup scene file (so EditorScene can open it).
	using OpenSceneCallback = std::function<void(const std::string& scene_path)>;

	void SetOpenSceneCallback(OpenSceneCallback cb) { open_scene_cb_ = std::move(cb); }

	/// Render `File > Build` submenu items + `File > New Project...` / `Open Project...`.
	/// Call inside `if (ImGui::BeginMenu("File"))` ... `EndMenu()`.
	void RenderFileMenuItems();

	/// Render the modal dialogs each frame (no-op when closed).
	void RenderDialog();

	/// Set or clear the current project. Called when EditorScene opens/saves a scene.
	void SetProject(std::optional<ProjectModel> project);

	[[nodiscard]] bool HasProject() const { return project_.has_value(); }
	[[nodiscard]] const ProjectModel* Project() const { return project_ ? &*project_ : nullptr; }

private:
	void StartBuild(const BuildTarget& target);
	void RenderNewProjectModal();
	bool CreateProjectFromTemplate(
			const std::filesystem::path& parent_dir,
			const std::string& project_name,
			std::string& out_error,
			std::filesystem::path& out_scene);

	std::optional<ProjectModel> project_;
	BuildDialog dialog_;
	OpenSceneCallback open_scene_cb_;

	bool open_new_project_modal_ = false;
	char new_project_name_[128]{'\0'};
	char new_project_parent_[512]{'\0'};
	std::string new_project_error_;
};

} // namespace editor::build
