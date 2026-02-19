#include "editor_scene.h"
#include "commands/entity_commands.h"

#include <imgui.h>

namespace editor {

void EditorScene::RenderMenuBar() {
	constexpr float MENU_PADDING = 6.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {MENU_PADDING, MENU_PADDING});
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				state_.show_new_scene_dialog = true;
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				open_scene_dialog_.Open();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				if (state_.current_file_path.empty()) {
					save_scene_dialog_.Open("scene.json");
				}
				else {
					SaveScene();
				}
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
				save_scene_dialog_.Open("scene.json");
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				// TODO: Handle exit with unsaved changes check
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_history_.CanUndo())) {
				command_history_.Undo();
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, command_history_.CanRedo())) {
				command_history_.Redo();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X", false, selected_entity_.is_valid())) {
				CutEntity();
			}
			if (ImGui::MenuItem("Copy", "Ctrl+C", false, selected_entity_.is_valid())) {
				CopyEntity();
			}
			if (ImGui::MenuItem("Paste", "Ctrl+V", false, !clipboard_json_.empty())) {
				PasteEntity();
			}
			if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected_entity_.is_valid())) {
				DuplicateEntity();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			for (auto* panel : panels_) {
				panel->RenderViewMenuItem();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("Add Entity")) {
				// Add a new entity to the scene using command
				auto& scene_manager = engine::scene::GetSceneManager();
				if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
					auto command = std::make_unique<CreateEntityCommand>(scene, engine::ecs::Entity());
					command_history_.Execute(std::move(command));
				}
			}
			ImGui::EndMenu();
		}

		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - 30); // Centered approx
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {MENU_PADDING / 2.f, MENU_PADDING / 2.f});
		ImGui::SetCursorPosY(MENU_PADDING / 2.f);
		// Play/Stop buttons
		if (!state_.is_running) {
			if (ImGui::Button("Play")) {
				PlayScene();
			}
		}
		else {
			if (ImGui::Button("Stop")) {
				StopScene();
			}
		}
		ImGui::PopStyleVar();

		// Display current scene info on the right side of menu bar
		std::string title = state_.current_file_path.empty() ? "Untitled" : state_.current_file_path;
		if (command_history_.IsDirty()) {
			title += " *";
		}
		const float text_width = ImGui::CalcTextSize(title.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - 20);
		ImGui::TextDisabled("%s", title.c_str());

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();
}

} // namespace editor
