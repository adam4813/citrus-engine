#pragma once

#include <flecs.h>
#include <imgui.h>

import engine;

namespace editor {

/**
 * @brief Scene viewport panel
 *
 * Renders the scene to a framebuffer and displays it in the viewport.
 * Uses the editor camera for rendering in edit mode.
 */
class ViewportPanel {
public:
	ViewportPanel() = default;
	~ViewportPanel() = default;

	/**
	 * @brief Render the viewport panel
	 * @param engine Reference to the engine for scene rendering
	 * @param scene
	 * @param is_running Whether the scene is in play mode
	 */
	void Render(engine::Engine& engine, engine::scene::Scene* scene, bool is_running);

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
	void RenderPlayModeIndicator(const ImVec2& cursor_pos);

	engine::rendering::Framebuffer framebuffer_;
	bool is_visible_ = true;
	uint32_t last_width_ = 0;
	uint32_t last_height_ = 0;
};

} // namespace editor
