#pragma once

#include "editor_callbacks.h"

#include <flecs.h>

import engine;

namespace editor {

/**
 * @brief Entity properties inspector panel
 *
 * Displays and allows editing of components on the selected entity.
 */
class PropertiesPanel {
public:
	PropertiesPanel() = default;
	~PropertiesPanel() = default;

	/**
	 * @brief Set callbacks for panel events
	 */
	void SetCallbacks(const EditorCallbacks& callbacks);

	/**
	 * @brief Render the properties panel
	 * @param selected_entity The entity to inspect (may be invalid)
	 */
	void Render(engine::ecs::Entity selected_entity);

	/**
	 * @brief Check if panel is visible
	 */
	[[nodiscard]] bool IsVisible() const { return is_visible_; }

	/**
	 * @brief Set panel visibility
	 */
	void SetVisible(bool visible) { is_visible_ = visible; }

	/**
	 * @brief Get mutable reference to visibility (for ImGui::MenuItem binding)
	 */
	bool& VisibleRef() { return is_visible_; }

private:
	void RenderTransformSection(engine::ecs::Entity entity);
	void RenderAddComponentButton(engine::ecs::Entity entity);

	EditorCallbacks callbacks_;
	bool is_visible_ = true;
};

} // namespace editor
