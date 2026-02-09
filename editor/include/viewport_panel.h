#pragma once

#include <flecs.h>
#include <imgui.h>

import engine;
import glm;

namespace editor {

/**
 * @brief Transform gizmo modes
 */
enum class GizmoMode {
	Translate,
	Rotate,
	Scale
};

/**
 * @brief Coordinate space for gizmos
 */
enum class GizmoSpace {
	World,
	Local
};

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
	 * @param selected_entity Currently selected entity for gizmo display
	 */
	void
	Render(engine::Engine& engine,
		   engine::scene::Scene* scene,
		   bool is_running,
		   flecs::entity editor_camera,
		   float delta_time,
		   flecs::entity selected_entity);

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
	void HandleGizmoInput();
	void RenderTransformGizmo(flecs::entity selected_entity, flecs::entity editor_camera,
							  const ImVec2& viewport_min, const ImVec2& viewport_size);
	void RenderTranslateGizmo(flecs::entity selected_entity, flecs::entity editor_camera,
							  const ImVec2& viewport_min, const ImVec2& viewport_size);
	void RenderRotationGizmo(flecs::entity selected_entity, flecs::entity editor_camera,
							 const ImVec2& viewport_min, const ImVec2& viewport_size);
	void RenderScaleGizmo(flecs::entity selected_entity, flecs::entity editor_camera,
						  const ImVec2& viewport_min, const ImVec2& viewport_size);
	void RenderOrientationGizmo(const ImVec2& viewport_min, const ImVec2& viewport_size);

	struct AxisDrawParams {
		ImVec2 origin;
		ImVec2 end;
		ImU32 color;
		float thickness;
		float arrow_size;
	};
	static void DrawAxisLine(ImDrawList* draw_list, const AxisDrawParams& params);
	static void DrawAxisLabel(ImDrawList* draw_list, const ImVec2& pos, const char* label, ImU32 color);

	engine::rendering::Framebuffer framebuffer_;
	bool is_visible_ = true;
	bool is_focused_ = false;
	uint32_t last_width_ = 0;
	uint32_t last_height_ = 0;

	// Camera movement settings
	static constexpr float kMoveSpeed = 5.0f;
	static constexpr float kFastMoveMultiplier = 3.0f;
	static constexpr float kMouseSensitivity = 0.003f;
	static constexpr float kGizmoLength = 80.0f;
	static constexpr float kGizmoThickness = 3.0f;
	static constexpr float kGizmoHitRadius = 8.0f;
	static constexpr float kArrowHeadSize = 10.0f;
	static constexpr float kOrientationGizmoSize = 40.0f;
	static constexpr float kOrientationGizmoMargin = 15.0f;
	static constexpr ImU32 kAxisColors[3] = {
			IM_COL32(230, 50, 50, 255),
			IM_COL32(50, 200, 50, 255),
			IM_COL32(50, 100, 230, 255)};
	static constexpr ImU32 kAxisHoverColors[3] = {
			IM_COL32(255, 130, 130, 255),
			IM_COL32(130, 255, 130, 255),
			IM_COL32(130, 180, 255, 255)};
	static constexpr const char* kAxisLabels[3] = {"X", "Y", "Z"};

	// Mouse look state
	bool is_right_mouse_down_ = false;
	float last_mouse_x_ = 0.0f;
	float last_mouse_y_ = 0.0f;
	glm::quat camera_orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// Gizmo interaction state
	int dragging_axis_ = -1; // -1=none, 0=X, 1=Y, 2=Z
	ImVec2 drag_start_mouse_{};
	glm::vec3 drag_start_position_{};
	glm::vec3 drag_start_rotation_{}; // For rotation gizmo
	glm::vec3 drag_start_scale_{1.0f, 1.0f, 1.0f}; // For scale gizmo
	ImVec2 drag_axis_screen_dir_{};
	float drag_world_per_pixel_ = 0.0f;
	float drag_start_angle_ = 0.0f; // For rotation gizmo

	// Gizmo mode and settings
	GizmoMode gizmo_mode_ = GizmoMode::Translate;
	GizmoSpace gizmo_space_ = GizmoSpace::World;
	
	// Snap settings
	float translate_snap_ = 0.5f; // Default snap increment for translation
	float rotate_snap_ = 15.0f;   // Default snap increment for rotation (degrees)
	float scale_snap_ = 0.1f;     // Default snap increment for scale
};

} // namespace editor
