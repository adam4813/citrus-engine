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
using ComponentCallback = std::function<void(engine::ecs::Entity, const std::string&)>;
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
	VoidCallback on_scene_modified;
	EntityCallback on_show_rename_dialog;
	EntityCallback on_add_child_entity;      // Add a new entity as child of the given parent
	ComponentCallback on_add_component;      // Add a component by name to an entity
	AssetCallback on_asset_selected;         // An asset was selected for editing
	AssetCallback on_asset_deleted;          // An asset was deleted
	EntityCallback on_scene_camera_changed;  // Scene's active camera selection changed
	CommandCallback on_execute_command;      // Execute a command through the command history
	PrefabCallback on_instantiate_prefab;    // Instantiate a prefab by file path
	VoidCallback on_copy_entity;             // Copy the selected entity
	VoidCallback on_paste_entity;            // Paste from clipboard
	VoidCallback on_duplicate_entity;        // Duplicate the selected entity
	FilePathCallback on_open_tileset;        // Open a tileset file in the tileset editor
	FilePathCallback on_open_data_table;     // Open a data table file in the data table editor
	StringCallback on_open_file;             // Open a file in the code editor
};

} // namespace editor
