#pragma once

#include <flecs.h>
#include <functional>
#include <memory>
#include <string>

import engine;

namespace editor {

class ICommand;

using EntityCallback = std::function<void(engine::ecs::Entity)>;
using VoidCallback = std::function<void()>;
using AssetCallback = std::function<void(engine::scene::AssetType, const std::string&)>;
using CommandCallback = std::function<void(std::unique_ptr<ICommand>)>;
using PrefabCallback = std::function<void(const std::string&)>;
using FilePathCallback = std::function<void(const std::string&)>;
using StringCallback = std::function<void(const std::string&)>;

/**
 * @brief Callbacks for panel-to-editor communication
 *
 * Panels use these callbacks to notify the editor of user actions
 * without directly depending on EditorScene.
 */ 
struct EditorCallbacks {
	EntityCallback on_entity_selected;
	EntityCallback on_entity_deleted;
	VoidCallback on_scene_modified; // Continuous/non-undoable change (field edits, property tweaks)
	EntityCallback on_show_rename_dialog;
	AssetCallback on_asset_selected; // An asset was selected for editing
	AssetCallback on_asset_deleted; // An asset was deleted
	EntityCallback on_scene_camera_changed; // Scene's active camera selection changed
	CommandCallback on_execute_command; // Execute a command through the command history (undoable)
	PrefabCallback on_instantiate_prefab; // Instantiate a prefab by file path
	VoidCallback on_copy_entity; // Copy the selected entity
	VoidCallback on_paste_entity; // Paste from clipboard
	VoidCallback on_duplicate_entity; // Duplicate the selected entity
	FilePathCallback on_open_asset_file; // Open any asset file via AssetEditorRegistry
	StringCallback on_open_file; // Open a file in the code editor (legacy, for non-JSON)
	FilePathCallback on_file_selected; // A file was single-clicked in asset browser
};

} // namespace editor
