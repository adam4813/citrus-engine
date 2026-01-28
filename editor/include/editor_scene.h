#pragma once

#include "hierarchy_panel.h"
#include "properties_panel.h"
#include "viewport_panel.h"

#include <flecs.h>
#include <memory>
#include <string>

import engine;

namespace editor {

/**
 * @brief Editor state for tracking scene modifications and file path
 */
struct EditorState {
	std::string current_file_path;
	bool is_dirty = false;
	bool show_new_scene_dialog = false;
	bool show_open_dialog = false;
	bool show_save_as_dialog = false;
	bool is_running = false; // Whether the scene is in "play" mode
	bool show_rename_entity_dialog = false;
};

/**
 * @brief Main 2D Scene Editor scene
 *
 * Provides a scaffolded editor interface using ImGui for prototyping.
 * Features:
 * - File menu (New, Open, Save, Save As)
 * - Scene hierarchy panel
 * - Properties panel (placeholder)
 * - Viewport panel (placeholder for 2D canvas)
 * - Play/Stop controls for running the scene
 */
class EditorScene {
public:
	EditorScene();
	~EditorScene();

	/**
	 * Initialize the editor scene
	 * @param engine Reference to the engine instance
	 */
	void Initialize(engine::Engine& engine);

	/**
	 * Shutdown and cleanup the editor scene
	 * @param engine Reference to the engine instance
	 */
	void Shutdown(engine::Engine& engine);

	/**
	 * Update the editor scene
	 * @param engine Reference to the engine instance
	 * @param delta_time Time elapsed since last frame
	 */
	void Update(engine::Engine& engine, float delta_time);

	/**
	 * Render the editor viewport
	 * @param engine Reference to the engine instance
	 */
	void Render(engine::Engine& engine) const;

	/**
	 * Render the ImGui editor UI
	 * @param engine Reference to the engine instance
	 */
	void RenderUI(engine::Engine& engine);

private:
	// ========================================================================
	// UI Rendering Methods
	// ========================================================================

	void RenderMenuBar();

	// ========================================================================
	// File Operations (stubs for now)
	// ========================================================================

	void NewScene();
	void OpenScene(const std::string& path);
	void SaveScene();
	void SaveSceneAs(const std::string& path);

	// ========================================================================
	// Scene Control
	// ========================================================================

	void PlayScene();
	void StopScene();

	// ========================================================================
	// Callback Handlers
	// ========================================================================

	void OnEntitySelected(engine::ecs::Entity entity);
	void OnEntityDeleted(engine::ecs::Entity entity);
	void OnSceneModified();
	void OnShowRenameDialog(engine::ecs::Entity entity);
	void OnAddChildEntity(engine::ecs::Entity parent);

	// ========================================================================
	// State
	// ========================================================================

	EditorState state_;
	engine::scene::SceneId editor_scene_id_ = engine::scene::INVALID_SCENE;

	// Selected entity in hierarchy
	engine::ecs::Entity selected_entity_;

	// Panels (composition)
	HierarchyPanel hierarchy_panel_;
	PropertiesPanel properties_panel_;
	ViewportPanel viewport_panel_;

	// Input buffer for dialogs
	char file_path_buffer_[256] = "";
	char rename_entity_buffer_[256] = "";
};

} // namespace editor
