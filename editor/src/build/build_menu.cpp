#include "build/build_menu.h"

#include "build/asset_packager.h"
#include "build/build_job.h"
#include "build/build_stager.h"

#include <imgui.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

namespace editor::build {

namespace {

std::string ReadAll(const std::filesystem::path& p) {
	std::ifstream in(p, std::ios::binary);
	std::string out((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return out;
}

bool WriteAll(const std::filesystem::path& p, const std::string& content) {
	std::error_code ec;
	std::filesystem::create_directories(p.parent_path(), ec);
	std::ofstream out(p, std::ios::binary);
	if (!out) return false;
	out.write(content.data(), static_cast<std::streamsize>(content.size()));
	return out.good();
}

} // namespace

void BuildMenu::SetProject(std::optional<ProjectModel> project) { project_ = std::move(project); }

void BuildMenu::RenderFileMenuItems() {
	if (ImGui::MenuItem("New Project...")) {
		open_new_project_modal_ = true;
		new_project_error_.clear();
		if (new_project_name_[0] == '\0') std::strcpy(new_project_name_, "my-game");
		if (new_project_parent_[0] == '\0') {
			const auto cwd = std::filesystem::current_path().string();
			std::strncpy(new_project_parent_, cwd.c_str(), sizeof(new_project_parent_) - 1);
		}
	}

	const bool has_project = project_.has_value();
	if (ImGui::BeginMenu("Build", has_project)) {
		if (project_) {
			for (const auto& target : project_->targets) {
				const auto label = target.DisplayName();
				if (ImGui::MenuItem(label.c_str())) {
					StartBuild(target);
				}
			}
			if (project_->targets.empty()) {
				ImGui::MenuItem("(no targets defined in project.json)", nullptr, false, false);
			}
		}
		ImGui::EndMenu();
	}
	if (!has_project && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Open a scene inside a project (with project.json) to enable builds.");
	}
}

void BuildMenu::RenderDialog() {
	dialog_.Render();
	RenderNewProjectModal();
}

void BuildMenu::StartBuild(const BuildTarget& target) {
	if (!project_) return;
	auto job = std::make_unique<BuildJob>();
	if (!job->Start(*project_, target, MakeDefaultAssetPackager())) return;
	dialog_.Open(std::move(job));
}

void BuildMenu::RenderNewProjectModal() {
	constexpr const char* kPopupId = "New Project##build_new_project";
	if (open_new_project_modal_) {
		ImGui::OpenPopup(kPopupId);
		open_new_project_modal_ = false;
	}

	const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(560, 0), ImGuiCond_Appearing);

	if (ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextWrapped(
			"Creates a new project from the bundled game-project template. "
			"A new folder named after the project will be created inside the parent directory."
		);
		ImGui::Separator();

		ImGui::InputText("Project name", new_project_name_, sizeof(new_project_name_));
		ImGui::InputText("Parent directory", new_project_parent_, sizeof(new_project_parent_));

		if (!new_project_error_.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "%s", new_project_error_.c_str());
		}

		ImGui::Separator();
		if (ImGui::Button("Create", ImVec2(120, 0))) {
			std::filesystem::path scene;
			std::string err;
			if (CreateProjectFromTemplate(new_project_parent_, new_project_name_, err, scene)) {
				ImGui::CloseCurrentPopup();
				if (open_scene_cb_ && !scene.empty()) {
					open_scene_cb_(scene.string());
				}
			}
			else {
				new_project_error_ = err;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

bool BuildMenu::CreateProjectFromTemplate(
	const std::filesystem::path& parent_dir,
	const std::string& project_name,
	std::string& out_error,
	std::filesystem::path& out_scene
) {
	namespace fs = std::filesystem;
	std::error_code ec;

	if (project_name.empty()) {
		out_error = "Project name is required.";
		return false;
	}
	if (parent_dir.empty() || !fs::is_directory(parent_dir, ec)) {
		out_error = "Parent directory does not exist: " + parent_dir.string();
		return false;
	}
	const auto template_dir = BuildStager::LocateTemplateDir();
	if (template_dir.empty()) {
		out_error = "Could not locate templates/game-project. Set CITRUS_TEMPLATE_DIR.";
		return false;
	}

	const auto project_root = parent_dir / project_name;
	if (fs::exists(project_root, ec)) {
		out_error = "Target directory already exists: " + project_root.string();
		return false;
	}

	// Sanitize the project name for use as an identifier in CMakeLists.txt project() and
	// vcpkg.json "name": lowercase, alphanumeric, spaces -> '-', drop everything else.
	// Collapse runs of '-' and trim trailing dashes so we never produce names like "----".
	auto sanitize_id = [](const std::string& in) {
		std::string out;
		out.reserve(in.size());
		bool last_dash = false;
		for (char c : in) {
			if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
			const bool is_alnum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
			if (is_alnum) {
				out.push_back(c);
				last_dash = false;
			}
			else if (c == ' ' || c == '-' || c == '_') {
				if (!last_dash && !out.empty()) {
					out.push_back('-');
					last_dash = true;
				}
			}
			// Anything else (punctuation, unicode) is dropped.
		}
		while (!out.empty() && out.back() == '-') out.pop_back();
		if (out.empty()) out = "my-game";
		return out;
	};

	const std::string sanitized_name = sanitize_id(project_name);

	// Copy template tree (excluding any pre-existing build/ output and VCS marker files).
	// Substitute the @TOKEN@ placeholders so the on-disk project tree is immediately
	// buildable - we no longer re-substitute or re-copy at build time.
	fs::create_directories(project_root, ec);

	// Seed a ProjectModel + default BuildTarget for substitution. The real values are
	// loaded back at the end of this function.
	ProjectModel seed;
	seed.name = sanitized_name; // -> @PROJECT_NAME@  (CMake/vcpkg identifier)
	seed.version = "0.1.0";
	seed.description = "A game built with Citrus Engine";
	seed.window.title = project_name; // -> @WINDOW_TITLE@  (human-readable)
	BuildTarget seed_target;          // defaults are fine; per-build targets override later

	// Files that contain @TOKEN@ placeholders. Anything else copies verbatim.
	const std::array<std::string, 5> substitutable = {
		"CMakeLists.txt",
		"vcpkg.json",
		"vcpkg-configuration.json",
		"src/main.cpp",
		"project.json",
	};
	auto is_substitutable = [&](const fs::path& rel) {
		const auto rel_str = rel.generic_string();
		for (const auto& s : substitutable) {
			if (rel_str == s) return true;
		}
		return false;
	};

	for (const auto& entry : fs::recursive_directory_iterator(template_dir, ec)) {
		if (ec) {
			out_error = "Failed to scan template: " + ec.message();
			return false;
		}
		const auto rel = fs::relative(entry.path(), template_dir, ec);
		if (ec) continue;
		if (!rel.empty() && rel.begin()->string() == "build") continue;
		const auto dst = project_root / rel;
		if (entry.is_directory()) {
			fs::create_directories(dst, ec);
		}
		else if (entry.is_regular_file()) {
			fs::create_directories(dst.parent_path(), ec);
			if (is_substitutable(rel)) {
				const auto content =
					BuildStager::SubstituteTokens(ReadAll(entry.path()), seed, seed_target, template_dir);
				if (!WriteAll(dst, content)) {
					out_error = "Failed to write: " + dst.string();
					return false;
				}
			}
			else {
				fs::copy_file(entry.path(), dst, fs::copy_options::overwrite_existing, ec);
				if (ec) {
					out_error = "Copy failed for " + rel.string() + ": " + ec.message();
					return false;
				}
			}
		}
	}

	const auto project_json = project_root / "project.json";

	// Ensure a startup scene file exists. The template references assets/scenes/main.json.
	const auto scenes_dir = project_root / "assets" / "scenes";
	fs::create_directories(scenes_dir, ec);
	const auto startup_scene = scenes_dir / "main.json";
	if (!fs::exists(startup_scene, ec)) {
		// Minimal empty-scene JSON the engine serializer understands.
		WriteAll(startup_scene, "{\n  \"name\": \"main\",\n  \"entities\": []\n}\n");
	}

	// Load the new project into the menu so File > Build is immediately available.
	std::string load_err;
	if (auto loaded = LoadProject(project_json, load_err)) {
		project_ = std::move(loaded);
	}

	out_scene = startup_scene;
	return true;
}

} // namespace editor::build
