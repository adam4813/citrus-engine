#pragma once

import engine;

namespace editor {

/// Render a single field editor widget based on its FieldType.
/// data must point directly to the field in memory (caller applies field.offset).
/// Returns true if the value was modified.
bool RenderFieldWidget(const engine::ecs::FieldInfo& field, void* data, engine::scene::Scene* scene = nullptr);

/// Render any pending file picker dialog opened by RenderFieldWidget.
/// Returns true if a dialog completed this frame (a file path was selected).
/// Call once per panel Render() after all fields are drawn.
bool RenderFieldDialogs();

} // namespace editor
