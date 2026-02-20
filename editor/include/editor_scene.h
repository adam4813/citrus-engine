#pragma once

#include <flecs.h>
#include <memory>
#include <string>
#include <vector>

#include "animation_editor_panel.h"
#include "asset_browser_panel.h"
#include "asset_editor_registry.h"
#include "behavior_tree_editor_panel.h"
#include "code_editor_panel.h"
#include "command.h"
#include "data_table_editor_panel.h"
#include "file_dialog.h"
#include "graph_editor_panel.h"
#include "hierarchy_panel.h"
#include "material_editor_panel.h"
#include "properties_panel.h"
#include "shader_editor_panel.h"
#include "sound_editor_panel.h"
#include "sprite_editor_panel.h"
#include "texture_editor_panel.h"
#include "tileset_editor_panel.h"
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
	engine::ecs::Entity selected_prefab_entity_; // Tracks when a prefab template is selected

	// Panels (composition)
	HierarchyPanel hierarchy_panel_;
	PropertiesPanel properties_panel_;
	ViewportPanel viewport_panel_;
	AssetBrowserPanel asset_browser_panel_;
	GraphEditorPanel graph_editor_panel_;
	ShaderEditorPanel shader_editor_panel_;
	DataTableEditorPanel data_table_editor_panel_;
	SoundEditorPanel sound_editor_panel_;
	TextureEditorPanel texture_editor_panel_;
	CodeEditorPanel code_editor_panel_;
	AnimationEditorPanel animation_editor_panel_;
	SpriteEditorPanel sprite_editor_panel_;
	TilesetEditorPanel tileset_editor_panel_;
	BehaviorTreeEditorPanel behavior_tree_editor_panel_;
	MaterialEditorPanel material_editor_panel_;

	// All panels for iteration (View menu, asset handler registration)
	std::vector<EditorPanel*> panels_;

	// Asset editor registry for generic file dispatch
	AssetEditorRegistry asset_editor_registry_;

	// Command history for undo/redo
	CommandHistory command_history_;

	// Play mode snapshot — stores serialized scene state to restore on Stop
	std::string play_mode_snapshot_;

	// Clipboard for copy/paste operations
	std::string clipboard_json_;

	// Input buffer for dialogs
	char rename_entity_buffer_[256] = "";

	// File dialogs
	FileDialogPopup open_scene_dialog_{"Open Scene", FileDialogMode::Open, {".json"}};
	FileDialogPopup save_scene_dialog_{"Save Scene As", FileDialogMode::Save, {".json"}};
};

} // namespace editor
