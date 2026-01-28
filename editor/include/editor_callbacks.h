#pragma once

#include <flecs.h>
#include <functional>

import engine;

namespace editor {

using EntityCallback = std::function<void(engine::ecs::Entity)>;
using VoidCallback = std::function<void()>;

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
	EntityCallback on_add_child_entity; // Add a new entity as child of the given parent
};

} // namespace editor
