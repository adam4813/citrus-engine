#pragma once

#include <flecs.h>
#include <imgui.h>

import engine;
import glm;

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
	 * @param scene The scene to render
	 * @param is_running Whether the scene is in play mode
	 * @param editor_camera The editor camera entity for fly-mode controls
	 * @param delta_time Time since last frame for movement speed
	 */
	void
	Render(engine::Engine& engine,
		   engine::scene::Scene* scene,
		   bool is_running,
		   flecs::entity editor_camera,
		   float delta_time);

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
	void HandleCameraInput(flecs::entity editor_camera, float delta_time);

	engine::rendering::Framebuffer framebuffer_;
	bool is_visible_ = true;
	bool is_focused_ = false;
	uint32_t last_width_ = 0;
	uint32_t last_height_ = 0;

	// Camera movement settings
	static constexpr float kMoveSpeed = 5.0f;
	static constexpr float kFastMoveMultiplier = 3.0f;
	static constexpr float kMouseSensitivity = 0.003f;

	// Mouse look state
	bool is_right_mouse_down_ = false;
	float last_mouse_x_ = 0.0f;
	float last_mouse_y_ = 0.0f;
	glm::quat camera_orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

} // namespace editor
