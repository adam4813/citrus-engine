#include "editor_scene.h"

#include <imgui.h>
#include <iostream>

namespace editor {

EditorScene::EditorScene() = default;

EditorScene::~EditorScene() = default;

void EditorScene::Initialize(engine::Engine& engine) {
	std::cout << "EditorScene: Initializing 2D Scene Editor..." << std::endl;

	// Initialize the scene system
	engine::scene::InitializeSceneSystem(engine.ecs);

	// Create a new empty scene for editing
	auto& scene_manager = engine::scene::GetSceneManager();
	editor_scene_id_ = scene_manager.CreateScene("UntitledScene");
	scene_manager.SetActiveScene(editor_scene_id_);

	state_.current_file_path = "";
	state_.is_dirty = false;

	std::cout << "EditorScene: Initialized with new scene" << std::endl;
}

void EditorScene::Shutdown(engine::Engine& engine) {
	std::cout << "EditorScene: Shutting down..." << std::endl;

	// Cleanup scene system
	engine::scene::ShutdownSceneSystem();

	std::cout << "EditorScene: Shutdown complete" << std::endl;
}

void EditorScene::Update(engine::Engine& engine, float delta_time) {
	// Update the active scene if in play mode
	if (state_.is_running) {
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Update(delta_time);
	}
}

void EditorScene::Render(engine::Engine& engine) {
	// Render the active scene if in play mode
	if (state_.is_running) {
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Render();
	}
}

void EditorScene::RenderUI(engine::Engine& engine) {
	RenderMenuBar();
	RenderToolbar();
	RenderHierarchyPanel();
	RenderPropertiesPanel();
	RenderViewportPanel();

	// Handle dialogs
	if (state_.show_new_scene_dialog) {
		ImGui::OpenPopup("New Scene");
		state_.show_new_scene_dialog = false;
	}

	if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create a new scene?");
		ImGui::Text("Any unsaved changes will be lost.");

		if (ImGui::Button("Create", ImVec2(120, 0))) {
			NewScene();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state_.show_open_dialog) {
		ImGui::OpenPopup("Open Scene");
		state_.show_open_dialog = false;
	}

	if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter scene file path:");
		ImGui::InputText("##filepath", file_path_buffer_, sizeof(file_path_buffer_));

		if (ImGui::Button("Open", ImVec2(120, 0))) {
			OpenScene(file_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state_.show_save_as_dialog) {
		ImGui::OpenPopup("Save Scene As");
		state_.show_save_as_dialog = false;
	}

	if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Enter save file path:");
		ImGui::InputText("##savepath", file_path_buffer_, sizeof(file_path_buffer_));

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			SaveSceneAs(file_path_buffer_);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void EditorScene::RenderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) {
				state_.show_new_scene_dialog = true;
			}
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				file_path_buffer_[0] = '\0'; // Clear buffer for new input
				state_.show_open_dialog = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				if (state_.current_file_path.empty()) {
					file_path_buffer_[0] = '\0'; // Clear buffer for new input
					state_.show_save_as_dialog = true;
				}
				else {
					SaveScene();
				}
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
				file_path_buffer_[0] = '\0'; // Clear buffer for new input
				state_.show_save_as_dialog = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				// TODO: Handle exit with unsaved changes check
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
				// TODO: Implement undo
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
				// TODO: Implement redo
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X", false, false)) {
				// TODO: Implement cut
			}
			if (ImGui::MenuItem("Copy", "Ctrl+C", false, false)) {
				// TODO: Implement copy
			}
			if (ImGui::MenuItem("Paste", "Ctrl+V", false, false)) {
				// TODO: Implement paste
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Hierarchy", nullptr, &show_hierarchy_);
			ImGui::MenuItem("Properties", nullptr, &show_properties_);
			ImGui::MenuItem("Viewport", nullptr, &show_viewport_);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("Add Entity")) {
				// Add a new entity to the scene
				auto& scene_manager = engine::scene::GetSceneManager();
				if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
					scene->CreateEntity("NewEntity");
					state_.is_dirty = true;
				}
			}
			ImGui::EndMenu();
		}

		// Display current scene info on the right side of menu bar
		std::string title = state_.current_file_path.empty() ? "Untitled" : state_.current_file_path;
		if (state_.is_dirty) {
			title += " *";
		}
		float text_width = ImGui::CalcTextSize(title.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - 20);
		ImGui::TextDisabled("%s", title.c_str());

		ImGui::EndMainMenuBar();
	}
}

void EditorScene::RenderToolbar() {
	ImGui::Begin("Toolbar", nullptr,
				 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

	ImGui::SameLine();
	ImGui::TextDisabled("|");
	ImGui::SameLine();

	// Quick save button
	if (ImGui::Button("Save")) {
		if (state_.current_file_path.empty()) {
			file_path_buffer_[0] = '\0'; // Clear buffer for new input
			state_.show_save_as_dialog = true;
		}
		else {
			SaveScene();
		}
	}

	ImGui::SameLine();

	// Scene info
	ImGui::TextDisabled("|");
	ImGui::SameLine();
	ImGui::Text("Scene: %s",
				state_.current_file_path.empty() ? "Untitled" : state_.current_file_path.c_str());

	if (state_.is_dirty) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "(unsaved)");
	}

	ImGui::End();
}

void EditorScene::RenderHierarchyPanel() {
	if (!show_hierarchy_)
		return;

	ImGui::Begin("Hierarchy", &show_hierarchy_);

	ImGui::Text("Scene Entities");
	ImGui::Separator();

	// Get all entities from the current scene
	auto& scene_manager = engine::scene::GetSceneManager();
	auto* scene = scene_manager.TryGetScene(editor_scene_id_);

	if (scene) {
		auto entities = scene->GetAllEntities();

		if (entities.empty()) {
			ImGui::TextDisabled("No entities in scene");
			ImGui::TextDisabled("Use Scene > Add Entity to create one");
		}
		else {
			for (const auto& entity : entities) {
				std::string name = entity.name().c_str();
				if (name.empty()) {
					name = "Entity_" + std::to_string(entity.id());
				}

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (entity == selected_entity_) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				if (ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entity.id())), flags, "%s",
									  name.c_str())) {
					ImGui::TreePop();
				}

				if (ImGui::IsItemClicked()) {
					selected_entity_ = entity;
				}

				// Right-click context menu
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Delete")) {
						scene->DestroyEntity(entity);
						if (selected_entity_ == entity) {
							selected_entity_ = {};
						}
						state_.is_dirty = true;
					}
					if (ImGui::MenuItem("Rename")) {
						// TODO: Implement rename dialog
					}
					ImGui::EndPopup();
				}
			}
		}
	}
	else {
		ImGui::TextDisabled("No active scene");
	}

	ImGui::End();
}

void EditorScene::RenderPropertiesPanel() {
	if (!show_properties_)
		return;

	ImGui::Begin("Properties", &show_properties_);

	if (selected_entity_.is_valid()) {
		std::string name = selected_entity_.name().c_str();
		if (name.empty()) {
			name = "Entity_" + std::to_string(selected_entity_.id());
		}

		ImGui::Text("Entity: %s", name.c_str());
		ImGui::Separator();

		// Transform component (placeholder - will need proper component inspection)
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Position: (0, 0, 0)");
			ImGui::Text("Rotation: (0, 0, 0)");
			ImGui::Text("Scale: (1, 1, 1)");
			ImGui::TextDisabled("(Component editing coming soon)");
		}

		// Add component button
		ImGui::Spacing();
		if (ImGui::Button("Add Component")) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			ImGui::TextDisabled("Available Components:");
			ImGui::Separator();
			if (ImGui::MenuItem("Transform")) {
				// TODO: Add transform component
				state_.is_dirty = true;
			}
			if (ImGui::MenuItem("Sprite Renderer")) {
				// TODO: Add sprite renderer component
				state_.is_dirty = true;
			}
			if (ImGui::MenuItem("Camera")) {
				// TODO: Add camera component
				state_.is_dirty = true;
			}
			ImGui::EndPopup();
		}
	}
	else {
		ImGui::TextDisabled("No entity selected");
		ImGui::TextDisabled("Select an entity in the Hierarchy panel");
	}

	ImGui::End();
}

void EditorScene::RenderViewportPanel() {
	if (!show_viewport_)
		return;

	ImGui::Begin("Viewport", &show_viewport_);

	ImVec2 content_size = ImGui::GetContentRegionAvail();

	// Placeholder viewport content
	ImGui::BeginChild("ViewportContent", content_size, true);

	// Draw a placeholder grid or message
	ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw background
	draw_list->AddRectFilled(
			cursor_pos, ImVec2(cursor_pos.x + content_size.x, cursor_pos.y + content_size.y),
			IM_COL32(30, 30, 30, 255));

	// Draw grid lines
	float grid_size = 50.0f;
	ImU32 grid_color = IM_COL32(50, 50, 50, 255);

	for (float x = 0; x < content_size.x; x += grid_size) {
		draw_list->AddLine(
				ImVec2(cursor_pos.x + x, cursor_pos.y),
				ImVec2(cursor_pos.x + x, cursor_pos.y + content_size.y), grid_color);
	}

	for (float y = 0; y < content_size.y; y += grid_size) {
		draw_list->AddLine(
				ImVec2(cursor_pos.x, cursor_pos.y + y),
				ImVec2(cursor_pos.x + content_size.x, cursor_pos.y + y), grid_color);
	}

	// Draw center text
	const char* text = "2D Scene Viewport";
	ImVec2 text_size = ImGui::CalcTextSize(text);
	draw_list->AddText(
			ImVec2(cursor_pos.x + (content_size.x - text_size.x) / 2,
				   cursor_pos.y + (content_size.y - text_size.y) / 2),
			IM_COL32(100, 100, 100, 255), text);

	// If running, show play mode indicator
	if (state_.is_running) {
		const char* play_text = "PLAYING";
		ImVec2 play_text_size = ImGui::CalcTextSize(play_text);
		draw_list->AddRectFilled(ImVec2(cursor_pos.x + 5, cursor_pos.y + 5),
								 ImVec2(cursor_pos.x + play_text_size.x + 15, cursor_pos.y + play_text_size.y + 15),
								 IM_COL32(0, 100, 0, 200));
		draw_list->AddText(ImVec2(cursor_pos.x + 10, cursor_pos.y + 10), IM_COL32(255, 255, 255, 255), play_text);
	}

	ImGui::EndChild();

	ImGui::End();
}

// ========================================================================
// File Operations
// ========================================================================

void EditorScene::NewScene() {
	std::cout << "EditorScene: Creating new scene..." << std::endl;

	auto& scene_manager = engine::scene::GetSceneManager();

	// Destroy the old scene
	if (editor_scene_id_ != engine::scene::INVALID_SCENE) {
		scene_manager.DestroyScene(editor_scene_id_);
	}

	// Create a new scene
	editor_scene_id_ = scene_manager.CreateScene("UntitledScene");
	scene_manager.SetActiveScene(editor_scene_id_);

	// Reset state
	state_.current_file_path = "";
	state_.is_dirty = false;
	selected_entity_ = {};
	file_path_buffer_[0] = '\0';

	std::cout << "EditorScene: New scene created" << std::endl;
}

void EditorScene::OpenScene(const std::string& path) {
	std::cout << "EditorScene: Opening scene from: " << path << std::endl;

	// TODO: Implement actual scene loading from file
	// For now, just create a new scene with the given name

	auto& scene_manager = engine::scene::GetSceneManager();

	// Destroy the old scene
	if (editor_scene_id_ != engine::scene::INVALID_SCENE) {
		scene_manager.DestroyScene(editor_scene_id_);
	}

	// Create a new scene (placeholder - would load from file)
	editor_scene_id_ = scene_manager.CreateScene(path);
	scene_manager.SetActiveScene(editor_scene_id_);

	// Update state
	state_.current_file_path = path;
	state_.is_dirty = false;
	selected_entity_ = {};

	std::cout << "EditorScene: Scene opened (stub implementation)" << std::endl;
}

void EditorScene::SaveScene() {
	if (state_.current_file_path.empty()) {
		std::cout << "EditorScene: No file path set, cannot save" << std::endl;
		return;
	}

	std::cout << "EditorScene: Saving scene to: " << state_.current_file_path << std::endl;

	// TODO: Implement actual scene saving
	// auto& scene_manager = engine::scene::GetSceneManager();
	// scene_manager.SaveScene(editor_scene_id_, state_.current_file_path);

	state_.is_dirty = false;

	std::cout << "EditorScene: Scene saved (stub implementation)" << std::endl;
}

void EditorScene::SaveSceneAs(const std::string& path) {
	std::cout << "EditorScene: Saving scene as: " << path << std::endl;

	// Update the file path
	state_.current_file_path = path;

	// Update the scene file path
	auto& scene_manager = engine::scene::GetSceneManager();
	if (auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		scene->SetFilePath(path);
	}

	// Save
	SaveScene();
}

// ========================================================================
// Scene Control
// ========================================================================

void EditorScene::PlayScene() {
	std::cout << "EditorScene: Playing scene..." << std::endl;
	state_.is_running = true;
}

void EditorScene::StopScene() {
	std::cout << "EditorScene: Stopping scene..." << std::endl;
	state_.is_running = false;
}

} // namespace editor
