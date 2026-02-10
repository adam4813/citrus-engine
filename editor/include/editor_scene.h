#pragma once

#include <flecs.h>
#include <memory>
#include <string>

#include "animation_editor_panel.h"
#include "asset_browser_panel.h"
#include "command.h"
#include "graph_editor_panel.h"
#include "hierarchy_panel.h"
#include "properties_panel.h"
#include "viewport_panel.h"

import engine;

namespace editor {

/**
 * @brief Selection state for tracking what's selected in the editor
 *
 * Shared between panels to coordinate entity vs asset selection.
 */
enum class SelectionType { None, Entity, Asset };

/**
 * @brief Editor state for tracking scene modifications and file path
 */
struct EditorState {
	std::string current_file_path;
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

	/**
	 * Check if the scene is in play mode
	 * @return true if the scene is running, false if in edit mode
	 */
	[[nodiscard]] bool IsRunning() const { return state_.is_running; }

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

	// Graph editor setup
	void RegisterExampleGraphNodes();
	void CreateExampleGraph();

	// ========================================================================
	// Clipboard Operations
	// ========================================================================

	void CopyEntity();
	void CutEntity();
	void PasteEntity();
	void DuplicateEntity();

	// ========================================================================
	// Callback Handlers
	// ========================================================================

	void OnEntitySelected(engine::ecs::Entity entity);
	void OnEntityDeleted(engine::ecs::Entity entity);
	void OnSceneModified();
	void OnShowRenameDialog(engine::ecs::Entity entity);
	void OnAddChildEntity(engine::ecs::Entity parent);
	void OnAddComponent(engine::ecs::Entity entity, const std::string& component_name);
	void OnAssetSelected(engine::scene::AssetType type, const std::string& name);
	void OnAssetDeleted(engine::scene::AssetType type, const std::string& name);

	// ========================================================================
	// State
	// ========================================================================

	engine::Engine* engine_ = nullptr;
	EditorState state_;
	engine::scene::SceneId editor_scene_id_ = engine::scene::INVALID_SCENE;
	float last_delta_time_ = 0.0f; // Cached for RenderUI camera controls

	// Editor camera (separate from scene cameras, used for viewport navigation in edit mode)
	flecs::entity editor_camera_;

	// Scene's intended active camera (stored separately from ECS active camera)
	// This is what gets serialized - the editor camera remains active in ECS for rendering
	flecs::entity scene_active_camera_;

	// Selection state (mutually exclusive: entity or asset)
	SelectionType selection_type_ = SelectionType::None;
	engine::ecs::Entity selected_entity_;
	AssetSelection selected_asset_;

	// Panels (composition)
	HierarchyPanel hierarchy_panel_;
	PropertiesPanel properties_panel_;
	ViewportPanel viewport_panel_;
	AssetBrowserPanel asset_browser_panel_;
	GraphEditorPanel graph_editor_panel_;
	DataTableEditorPanel data_table_editor_panel_;
	AnimationEditorPanel animation_editor_panel_;

	// Command history for undo/redo
	CommandHistory command_history_;

	// Play mode snapshot — stores serialized scene state to restore on Stop
	std::string play_mode_snapshot_;

	// Clipboard for copy/paste operations
	std::string clipboard_json_;

	// Input buffer for dialogs
	char file_path_buffer_[256] = "";
	char rename_entity_buffer_[256] = "";
};

} // namespace editor
