#include "editor_scene.h"

#include <iostream>

namespace editor {

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
	selected_entity_ = {};
	selected_asset_.Clear();
	selection_type_ = SelectionType::None;
	rename_entity_buffer_[0] = '\0';
	hierarchy_panel_.ClearNodeState();
	command_history_.Clear();

	std::cout << "EditorScene: New scene created" << std::endl;
}

void EditorScene::OpenScene(const std::string& path) {
	std::cout << "EditorScene: Opening scene from: " << path << std::endl;

	auto& scene_manager = engine::scene::GetSceneManager();

	// Destroy the old scene
	if (editor_scene_id_ != engine::scene::INVALID_SCENE) {
		scene_manager.DestroyScene(editor_scene_id_);
	}

	// Load scene from file using engine serializer
	const engine::platform::fs::Path file_path(path);
	editor_scene_id_ = scene_manager.LoadSceneFromFile(file_path);

	if (editor_scene_id_ == engine::scene::INVALID_SCENE) {
		std::cerr << "EditorScene: Failed to load scene from: " << path << std::endl;
		// Fall back to creating a new empty scene
		editor_scene_id_ = scene_manager.CreateScene("Untitled");
	}

	scene_manager.SetActiveScene(editor_scene_id_);

	// Store the scene's active camera (loaded from file) before switching to editor camera
	scene_active_camera_ = engine_->ecs.GetActiveCamera();
	// Filter out editor camera in case it was serialized (shouldn't happen but be safe)
	if (scene_active_camera_ == editor_camera_) {
		scene_active_camera_ = {};
	}

	// HACK: Reset to editor camera for viewport rendering
	engine_->ecs.SetActiveCamera(editor_camera_);

	// Update state
	state_.current_file_path = path;
	selected_entity_ = {};
	selected_asset_.Clear();
	selection_type_ = SelectionType::None;
	hierarchy_panel_.ClearNodeState();
	command_history_.Clear();

	std::cout << "EditorScene: Scene loaded from: " << path << std::endl;
}

void EditorScene::SaveScene() {
	if (state_.current_file_path.empty()) {
		std::cout << "EditorScene: No file path set, cannot save" << std::endl;
		return;
	}

	std::cout << "EditorScene: Saving scene to: " << state_.current_file_path << std::endl;

	auto& scene_manager = engine::scene::GetSceneManager();

	// HACK: Before saving, switch active camera from editor camera to scene's intended camera.
	// This prevents the editor camera from being serialized as the active camera.
	// The editor camera entity itself is automatically excluded from serialization because
	// it's not a child of the scene root (see SerializeEntities in scene_serializer.cpp).
	// Use scene_active_camera_ if valid, otherwise set to invalid entity (no active camera).
	engine_->ecs.SetActiveCamera(scene_active_camera_);

	if (const engine::platform::fs::Path file_path(state_.current_file_path);
		scene_manager.SaveScene(editor_scene_id_, file_path)) {
		command_history_.SetSavePosition();
		std::cout << "EditorScene: Scene saved successfully" << std::endl;
	}
	else {
		std::cerr << "EditorScene: Failed to save scene" << std::endl;
	}

	// HACK: Restore editor camera as active after save
	engine_->ecs.SetActiveCamera(editor_camera_);
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

} // namespace editor
