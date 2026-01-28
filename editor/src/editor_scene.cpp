#include "editor_scene.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

namespace editor {

constexpr unsigned int MAX_ENTITY_NAME_CHECK_COUNT = 1000;

EditorScene::EditorScene() = default;

EditorScene::~EditorScene() = default;

void EditorScene::Initialize(engine::Engine& engine) {
	std::cout << "EditorScene: Initializing 2D Scene Editor..." << std::endl;

	// Initialize the scene system
	engine::scene::InitializeSceneSystem(engine.ecs);

	// Create a new empty scene for editing
	const auto& scene_manager = engine::scene::GetSceneManager();
	editor_scene_id_ = scene_manager.CreateScene("UntitledScene");
	scene_manager.SetActiveScene(editor_scene_id_);

	state_.current_file_path = "";
	state_.is_dirty = false;

	// Wire up panel callbacks
	EditorCallbacks callbacks;
	callbacks.on_entity_selected = [this](engine::ecs::Entity entity) { OnEntitySelected(entity); };
	callbacks.on_entity_deleted = [this](engine::ecs::Entity entity) { OnEntityDeleted(entity); };
	callbacks.on_scene_modified = [this]() { OnSceneModified(); };
	callbacks.on_show_rename_dialog = [this](engine::ecs::Entity entity) { OnShowRenameDialog(entity); };
	callbacks.on_add_child_entity = [this](engine::ecs::Entity parent) { OnAddChildEntity(parent); };

	hierarchy_panel_.SetCallbacks(callbacks);
	properties_panel_.SetCallbacks(callbacks);

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

void EditorScene::Render(engine::Engine& engine) const {
	// Render the active scene if in play mode
	if (state_.is_running) {
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Render();
	}
}

void EditorScene::RenderUI(engine::Engine& engine) {
	const ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Create settings
	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
		ImGuiID dock_id_left = 0;
		ImGuiID dock_id_main = dockspace_id;
		ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
		ImGuiID dock_id_left_top = 0;
		ImGuiID dock_id_left_bottom = 0;
		ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
		ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left_top);
		ImGui::DockBuilderDockWindow("Properties", dock_id_left_bottom);
		ImGui::DockBuilderDockWindow("Viewport", dock_id_main);
		ImGui::DockBuilderFinish(dockspace_id);
	}

	// Submit dockspace
	ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	RenderMenuBar();
	hierarchy_panel_.Render(editor_scene_id_, selected_entity_);
	properties_panel_.Render(selected_entity_);
	viewport_panel_.Render(state_.is_running);

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

	if (state_.show_rename_entity_dialog) {
		ImGui::OpenPopup("RenameEntityPopup");
		const auto& scene_entity = selected_entity_.get<engine::ecs::SceneEntity>();
		std::strncpy(rename_entity_buffer_, scene_entity.name.c_str(), sizeof(rename_entity_buffer_) - 1);
		state_.show_rename_entity_dialog = false;
	}

	if (selected_entity_ && ImGui::BeginPopupModal("RenameEntityPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("##rename", rename_entity_buffer_, sizeof(rename_entity_buffer_));
		if (ImGui::Button("Ok", ImVec2(120, 0)) && selected_entity_.set_name(rename_entity_buffer_)) {
			auto& scene_entity = selected_entity_.get_mut<engine::ecs::SceneEntity>();
			scene_entity.name = std::string(rename_entity_buffer_);
			rename_entity_buffer_[0] = '\0';
			state_.is_dirty = true;
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
	constexpr float MENU_PADDING = 6.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {MENU_PADDING, MENU_PADDING});
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
			ImGui::MenuItem("Hierarchy", nullptr, &hierarchy_panel_.VisibleRef());
			ImGui::MenuItem("Properties", nullptr, &properties_panel_.VisibleRef());
			ImGui::MenuItem("Viewport", nullptr, &viewport_panel_.VisibleRef());
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("Add Entity")) {
				// Add a new entity to the scene
				auto& scene_manager = engine::scene::GetSceneManager();
				if (const auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
					std::string entity_name = "NewEntity";
					auto count = 1;
					while (count <= MAX_ENTITY_NAME_CHECK_COUNT
						   && scene->GetSceneRoot().lookup(entity_name.c_str()) != flecs::entity::null()) {
						entity_name = "NewEntity_" + std::to_string(count++);
					}
					if (count <= MAX_ENTITY_NAME_CHECK_COUNT && scene->CreateEntity(entity_name)) {
						state_.is_dirty = true;
					}
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
		if (state_.is_dirty) {
			title += " *";
		}
		const float text_width = ImGui::CalcTextSize(title.c_str()).x;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width - 20);
		ImGui::TextDisabled("%s", title.c_str());

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar();
}

// ========================================================================
// Callback Handlers
// ========================================================================

void EditorScene::OnEntitySelected(engine::ecs::Entity entity) {
	selected_entity_ = entity;
}

void EditorScene::OnEntityDeleted(engine::ecs::Entity entity) {
	if (selected_entity_ == entity) {
		selected_entity_ = {};
	}
}

void EditorScene::OnSceneModified() {
	state_.is_dirty = true;
}

void EditorScene::OnShowRenameDialog(engine::ecs::Entity entity) {
	selected_entity_ = entity;
	state_.show_rename_entity_dialog = true;
}

void EditorScene::OnAddChildEntity(engine::ecs::Entity parent) {
	auto& scene_manager = engine::scene::GetSceneManager();
	if (const auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
		std::string entity_name = "NewEntity";
		auto count = 1;
		while (count <= MAX_ENTITY_NAME_CHECK_COUNT
			   && scene->GetSceneRoot().lookup(entity_name.c_str()) != flecs::entity::null()) {
			entity_name = "NewEntity_" + std::to_string(count++);
		}
		if (count <= MAX_ENTITY_NAME_CHECK_COUNT) {
			if (auto new_entity = scene->CreateEntity(entity_name, parent)) {
				state_.is_dirty = true;
				selected_entity_ = new_entity;
			}
		}
	}
}

// ========================================================================
// File Operations
// ========================================================================

void EditorScene::NewScene() {
	std::cout << "EditorScene: Creating new scene..." << std::endl;

	const auto& scene_manager = engine::scene::GetSceneManager();

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
	rename_entity_buffer_[0] = '\0';
	hierarchy_panel_.ClearNodeState();

	std::cout << "EditorScene: New scene created" << std::endl;
}

void EditorScene::OpenScene(const std::string& path) {
	std::cout << "EditorScene: Opening scene from: " << path << std::endl;

	// TODO: Implement actual scene loading from file
	// For now, just create a new scene with the given name

	const auto& scene_manager = engine::scene::GetSceneManager();

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
	hierarchy_panel_.ClearNodeState();

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
	if (const auto* scene = scene_manager.TryGetScene(editor_scene_id_)) {
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
