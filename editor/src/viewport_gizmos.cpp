#include "viewport_panel.h"

#include <algorithm>
#include <cmath>

namespace editor {

void ViewportPanel::HandleGizmoInput() {
	using namespace engine::input;
	using engine::input::KeyCode;

	// Gizmo mode switching (W/E/R keys) - use IsKeyJustPressed to avoid conflicts with camera hold
	if (Input::IsKeyJustPressed(KeyCode::W)) {
		gizmo_mode_ = GizmoMode::Translate;
	}
	if (Input::IsKeyJustPressed(KeyCode::E)) {
		gizmo_mode_ = GizmoMode::Rotate;
	}
	if (Input::IsKeyJustPressed(KeyCode::R)) {
		gizmo_mode_ = GizmoMode::Scale;
	}

	// Local/World space toggle (X key)
	if (Input::IsKeyJustPressed(KeyCode::X)) {
		gizmo_space_ = (gizmo_space_ == GizmoSpace::World) ? GizmoSpace::Local : GizmoSpace::World;
	}
}

void ViewportPanel::RenderTransformGizmo(
		const flecs::entity selected_entity,
		const flecs::entity editor_camera,
		const ImVec2& viewport_min,
		const ImVec2& viewport_size) {
	// Dispatch to appropriate gizmo based on current mode
	switch (gizmo_mode_) {
	case GizmoMode::Translate: RenderTranslateGizmo(selected_entity, editor_camera, viewport_min, viewport_size); break;
	case GizmoMode::Rotate: RenderRotationGizmo(selected_entity, editor_camera, viewport_min, viewport_size); break;
	case GizmoMode::Scale: RenderScaleGizmo(selected_entity, editor_camera, viewport_min, viewport_size); break;
	}
}

void ViewportPanel::RenderTranslateGizmo(
		const flecs::entity selected_entity,
		const flecs::entity editor_camera,
		const ImVec2& viewport_min,
		const ImVec2& viewport_size) {
	if (!editor_camera.is_valid() || !editor_camera.has<engine::components::Camera>()) {
		return;
	}

	const auto& camera = editor_camera.get<engine::components::Camera>();
	const auto& entity_transform = selected_entity.get<engine::components::Transform>();
	const glm::mat4 vp = camera.projection_matrix * camera.view_matrix;

	// Project a 3D world position to 2D viewport screen coordinates
	auto project = [&](const glm::vec3& world_pos) -> ImVec2 {
		const glm::vec4 clip = vp * glm::vec4(world_pos, 1.0f);
		if (clip.w <= 0.0f) {
			return {-1.0f, -1.0f};
		}
		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		return {viewport_min.x + (ndc.x * 0.5f + 0.5f) * viewport_size.x,
				viewport_min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport_size.y};
	};

	const glm::vec3 origin = entity_transform.position;
	const ImVec2 origin_2d = project(origin);
	if (origin_2d.x < 0.0f) {
		return;
	}

	// Compute axis endpoint scale based on distance to camera for consistent screen size
	const auto& cam_transform = editor_camera.get<engine::components::Transform>();
	const float dist_to_camera = glm::length(origin - cam_transform.position);
	const float axis_world_len = std::max(dist_to_camera * 0.15f, 0.01f);

	// Determine axis directions based on gizmo space (local or world)
	glm::vec3 axes[3];
	if (gizmo_space_ == GizmoSpace::Local && entity_transform.rotation != glm::vec3(0.0f)) {
		// Local space: transform axes by entity's rotation
		const glm::quat entity_quat = glm::quat(entity_transform.rotation);
		axes[0] = entity_quat * glm::vec3(axis_world_len, 0.0f, 0.0f);
		axes[1] = entity_quat * glm::vec3(0.0f, axis_world_len, 0.0f);
		axes[2] = entity_quat * glm::vec3(0.0f, 0.0f, axis_world_len);
	}
	else {
		// World space: use world axes
		axes[0] = {axis_world_len, 0.0f, 0.0f};
		axes[1] = {0.0f, axis_world_len, 0.0f};
		axes[2] = {0.0f, 0.0f, axis_world_len};
	}

	ImVec2 axis_ends_2d[3];
	for (int i = 0; i < 3; ++i) {
		axis_ends_2d[i] = project(origin + axes[i]);
	}

	// Distance from point to line segment
	auto point_to_segment_dist = [](const ImVec2& p, const ImVec2& a, const ImVec2& b) -> float {
		const ImVec2 ab = {b.x - a.x, b.y - a.y};
		const ImVec2 ap = {p.x - a.x, p.y - a.y};
		const float ab_sq = ab.x * ab.x + ab.y * ab.y;
		if (ab_sq < 1e-6f) {
			return std::sqrt(ap.x * ap.x + ap.y * ap.y);
		}
		const float t = std::clamp((ap.x * ab.x + ap.y * ab.y) / ab_sq, 0.0f, 1.0f);
		const float dx = ap.x - ab.x * t;
		const float dy = ap.y - ab.y * t;
		return std::sqrt(dx * dx + dy * dy);
	};

	const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	const bool right_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);
	const bool left_pressed = !right_down && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool left_down = !right_down && ImGui::IsMouseDown(ImGuiMouseButton_Left);

	// Determine hovered axis (suppress during mouse look)
	int hovered_axis = -1;
	if (dragging_axis_ < 0 && !right_down) {
		float best_dist = kGizmoHitRadius;
		for (int i = 0; i < 3; ++i) {
			if (axis_ends_2d[i].x < 0.0f) {
				continue;
			}
			const float d = point_to_segment_dist(mouse_pos, origin_2d, axis_ends_2d[i]);
			if (d < best_dist) {
				best_dist = d;
				hovered_axis = i;
			}
		}
	}

	// Begin drag — capture screen-space axis direction and scale at start
	if (hovered_axis >= 0 && left_pressed) {
		const ImVec2& axis_end = axis_ends_2d[hovered_axis];
		ImVec2 axis_dir = {axis_end.x - origin_2d.x, axis_end.y - origin_2d.y};
		const float axis_screen_len = std::sqrt(axis_dir.x * axis_dir.x + axis_dir.y * axis_dir.y);
		if (axis_screen_len > 1e-4f) {
			dragging_axis_ = hovered_axis;
			drag_start_mouse_ = mouse_pos;
			drag_start_position_ = entity_transform.position;
			drag_start_rotation_ = entity_transform.rotation; // Capture rotation for local space
			drag_axis_screen_dir_ = {axis_dir.x / axis_screen_len, axis_dir.y / axis_screen_len};
			drag_world_per_pixel_ = axis_world_len / axis_screen_len;
		}
	}

	// Process drag using fixed start-of-drag values
	if (dragging_axis_ >= 0) {
		if (left_down) {
			const ImVec2 mouse_delta = {mouse_pos.x - drag_start_mouse_.x, mouse_pos.y - drag_start_mouse_.y};
			const float projected = mouse_delta.x * drag_axis_screen_dir_.x + mouse_delta.y * drag_axis_screen_dir_.y;

			auto& transform = selected_entity.get_mut<engine::components::Transform>();

			// Compute axis direction in world space
			glm::vec3 axis_dir(0.0f);
			if (gizmo_space_ == GizmoSpace::Local && drag_start_rotation_ != glm::vec3(0.0f)) {
				// Local space: axis is rotated by entity's rotation at drag start
				const glm::quat entity_quat = glm::quat(drag_start_rotation_);
				glm::vec3 local_axis(0.0f);
				local_axis[dragging_axis_] = 1.0f;
				axis_dir = entity_quat * local_axis;
			}
			else {
				// World space: use world axes
				axis_dir[dragging_axis_] = 1.0f;
			}

			float movement = projected * drag_world_per_pixel_;

			// Apply snapping if Ctrl is held
			if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT_CONTROL)
				|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT_CONTROL)) {
				movement = std::round(movement / translate_snap_) * translate_snap_;
			}

			transform.position = drag_start_position_ + axis_dir * movement;
			selected_entity.modified<engine::components::Transform>();
		}
		else {
			dragging_axis_ = -1;
		}
	}

	// Draw axis lines and arrowheads
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	for (int i = 0; i < 3; ++i) {
		if (axis_ends_2d[i].x < 0.0f) {
			continue;
		}
		const bool highlight = (i == dragging_axis_ || i == hovered_axis);
		DrawAxisLine(
				draw_list,
				{.origin = origin_2d,
				 .end = axis_ends_2d[i],
				 .color = highlight ? kAxisHoverColors[i] : kAxisColors[i],
				 .thickness = highlight ? kGizmoThickness + 1.5f : kGizmoThickness,
				 .arrow_size = kArrowHeadSize});
	}

	// Draw origin circle
	draw_list->AddCircleFilled(origin_2d, 4.0f, IM_COL32(255, 255, 255, 200));
}

void ViewportPanel::RenderRotationGizmo(
		const flecs::entity selected_entity,
		const flecs::entity editor_camera,
		const ImVec2& viewport_min,
		const ImVec2& viewport_size) {
	if (!editor_camera.is_valid() || !editor_camera.has<engine::components::Camera>()) {
		return;
	}

	const auto& camera = editor_camera.get<engine::components::Camera>();
	const auto& entity_transform = selected_entity.get<engine::components::Transform>();
	const glm::mat4 vp = camera.projection_matrix * camera.view_matrix;

	// Project a 3D world position to 2D viewport screen coordinates
	auto project = [&](const glm::vec3& world_pos) -> ImVec2 {
		const glm::vec4 clip = vp * glm::vec4(world_pos, 1.0f);
		if (clip.w <= 0.0f) {
			return {-1.0f, -1.0f};
		}
		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		return {viewport_min.x + (ndc.x * 0.5f + 0.5f) * viewport_size.x,
				viewport_min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport_size.y};
	};

	const glm::vec3 origin = entity_transform.position;
	const ImVec2 origin_2d = project(origin);
	if (origin_2d.x < 0.0f) {
		return;
	}

	// Compute ring radius based on distance to camera
	const auto& cam_transform = editor_camera.get<engine::components::Transform>();
	const float dist_to_camera = glm::length(origin - cam_transform.position);
	const float ring_world_radius = std::max(dist_to_camera * 0.15f, 0.01f);

	// Define rotation plane normals (axes of rotation) - in local or world space
	glm::vec3 rotation_axes[3];
	if (gizmo_space_ == GizmoSpace::Local && entity_transform.rotation != glm::vec3(0.0f)) {
		// Local space: rotate axes by entity's rotation
		const glm::quat entity_quat = glm::quat(entity_transform.rotation);
		rotation_axes[0] = entity_quat * glm::vec3(1.0f, 0.0f, 0.0f); // X axis
		rotation_axes[1] = entity_quat * glm::vec3(0.0f, 1.0f, 0.0f); // Y axis
		rotation_axes[2] = entity_quat * glm::vec3(0.0f, 0.0f, 1.0f); // Z axis
	}
	else {
		// World space
		rotation_axes[0] = {1.0f, 0.0f, 0.0f};
		rotation_axes[1] = {0.0f, 1.0f, 0.0f};
		rotation_axes[2] = {0.0f, 0.0f, 1.0f};
	}

	const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	const bool right_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);
	const bool left_pressed = !right_down && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool left_down = !right_down && ImGui::IsMouseDown(ImGuiMouseButton_Left);

	// Draw rotation rings as arc segments
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	constexpr int kNumSegments = 64;

	// Determine hovered ring
	int hovered_axis = -1;
	if (dragging_axis_ < 0 && !right_down) {
		float best_dist = kGizmoHitRadius;
		for (int ring_idx = 0; ring_idx < 3; ++ring_idx) {
			const glm::vec3& axis_normal = rotation_axes[ring_idx];

			// Build two perpendicular vectors in the rotation plane
			glm::vec3 tangent1 = glm::abs(axis_normal.y) < 0.9f
										 ? glm::normalize(glm::cross(axis_normal, glm::vec3(0.0f, 1.0f, 0.0f)))
										 : glm::normalize(glm::cross(axis_normal, glm::vec3(1.0f, 0.0f, 0.0f)));
			glm::vec3 tangent2 = glm::normalize(glm::cross(axis_normal, tangent1));

			// Check distance from mouse to ring arc
			for (int seg = 0; seg < kNumSegments; ++seg) {
				const float angle1 = (seg * 2.0f * glm::pi<float>()) / kNumSegments;
				const float angle2 = ((seg + 1) * 2.0f * glm::pi<float>()) / kNumSegments;

				const glm::vec3 p1_world =
						origin + ring_world_radius * (std::cos(angle1) * tangent1 + std::sin(angle1) * tangent2);
				const glm::vec3 p2_world =
						origin + ring_world_radius * (std::cos(angle2) * tangent1 + std::sin(angle2) * tangent2);

				const ImVec2 p1_2d = project(p1_world);
				const ImVec2 p2_2d = project(p2_world);

				if (p1_2d.x >= 0.0f && p2_2d.x >= 0.0f) {
					// Distance from mouse to segment
					const ImVec2 seg_dir = {p2_2d.x - p1_2d.x, p2_2d.y - p1_2d.y};
					const ImVec2 mouse_dir = {mouse_pos.x - p1_2d.x, mouse_pos.y - p1_2d.y};
					const float seg_len_sq = seg_dir.x * seg_dir.x + seg_dir.y * seg_dir.y;
					if (seg_len_sq > 1e-6f) {
						const float t = std::clamp(
								(mouse_dir.x * seg_dir.x + mouse_dir.y * seg_dir.y) / seg_len_sq, 0.0f, 1.0f);
						const float dx = mouse_dir.x - seg_dir.x * t;
						const float dy = mouse_dir.y - seg_dir.y * t;
						const float d = std::sqrt(dx * dx + dy * dy);
						if (d < best_dist) {
							best_dist = d;
							hovered_axis = ring_idx;
						}
					}
				}
			}
		}
	}

	// Begin drag — capture initial rotation
	if (hovered_axis >= 0 && left_pressed) {
		dragging_axis_ = hovered_axis;
		drag_start_mouse_ = mouse_pos;
		drag_start_rotation_ = entity_transform.rotation;
		drag_start_angle_ = 0.0f; // Will accumulate angle during drag
	}

	// Process drag — compute rotation angle from mouse movement
	if (dragging_axis_ >= 0) {
		if (left_down) {
			const glm::vec3& axis_normal = rotation_axes[dragging_axis_];

			// Project mouse delta onto screen-space tangent of rotation
			// Simplified: use perpendicular mouse movement to estimate rotation angle
			const ImVec2 mouse_delta = {mouse_pos.x - drag_start_mouse_.x, mouse_pos.y - drag_start_mouse_.y};
			const float mouse_dist = std::sqrt(mouse_delta.x * mouse_delta.x + mouse_delta.y * mouse_delta.y);

			// Compute screen-space direction and convert to angle delta
			// Approximate: 1 pixel ~ 0.005 radians (tunable sensitivity)
			float angle_delta = mouse_dist * 0.005f;

			// Determine sign based on mouse direction (simplified heuristic)
			const float cross = mouse_delta.x * (origin_2d.y - drag_start_mouse_.y)
								- mouse_delta.y * (origin_2d.x - drag_start_mouse_.x);
			if (cross < 0.0f) {
				angle_delta = -angle_delta;
			}

			// Apply snapping if Ctrl is held
			if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT_CONTROL)
				|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT_CONTROL)) {
				const float snap_radians = glm::radians(rotate_snap_);
				angle_delta = std::round(angle_delta / snap_radians) * snap_radians;
			}

			// Apply rotation to entity
			auto& transform = selected_entity.get_mut<engine::components::Transform>();
			const glm::quat rotation_quat = glm::angleAxis(angle_delta, axis_normal);
			const glm::quat start_quat = glm::quat(drag_start_rotation_);
			const glm::quat new_quat = rotation_quat * start_quat;
			transform.rotation = glm::eulerAngles(new_quat);
			selected_entity.modified<engine::components::Transform>();
		}
		else {
			dragging_axis_ = -1;
		}
	}

	// Draw rotation rings
	for (int ring_idx = 0; ring_idx < 3; ++ring_idx) {
		const bool highlight = (ring_idx == dragging_axis_ || ring_idx == hovered_axis);
		const ImU32 color = highlight ? kAxisHoverColors[ring_idx] : kAxisColors[ring_idx];
		const float thickness = highlight ? kGizmoThickness + 1.0f : kGizmoThickness;

		const glm::vec3& axis_normal = rotation_axes[ring_idx];

		// Build two perpendicular vectors in the rotation plane
		glm::vec3 tangent1 = glm::abs(axis_normal.y) < 0.9f
									 ? glm::normalize(glm::cross(axis_normal, glm::vec3(0.0f, 1.0f, 0.0f)))
									 : glm::normalize(glm::cross(axis_normal, glm::vec3(1.0f, 0.0f, 0.0f)));
		glm::vec3 tangent2 = glm::normalize(glm::cross(axis_normal, tangent1));

		// Draw ring as line segments
		for (int seg = 0; seg < kNumSegments; ++seg) {
			const float angle1 = (seg * 2.0f * glm::pi<float>()) / kNumSegments;
			const float angle2 = ((seg + 1) * 2.0f * glm::pi<float>()) / kNumSegments;

			const glm::vec3 p1_world =
					origin + ring_world_radius * (std::cos(angle1) * tangent1 + std::sin(angle1) * tangent2);
			const glm::vec3 p2_world =
					origin + ring_world_radius * (std::cos(angle2) * tangent1 + std::sin(angle2) * tangent2);

			const ImVec2 p1_2d = project(p1_world);
			const ImVec2 p2_2d = project(p2_world);

			if (p1_2d.x >= 0.0f && p2_2d.x >= 0.0f) {
				draw_list->AddLine(p1_2d, p2_2d, color, thickness);
			}
		}
	}

	// Draw origin circle
	draw_list->AddCircleFilled(origin_2d, 4.0f, IM_COL32(255, 255, 255, 200));
}

void ViewportPanel::RenderScaleGizmo(
		const flecs::entity selected_entity,
		const flecs::entity editor_camera,
		const ImVec2& viewport_min,
		const ImVec2& viewport_size) {
	if (!editor_camera.is_valid() || !editor_camera.has<engine::components::Camera>()) {
		return;
	}

	const auto& camera = editor_camera.get<engine::components::Camera>();
	const auto& entity_transform = selected_entity.get<engine::components::Transform>();
	const glm::mat4 vp = camera.projection_matrix * camera.view_matrix;

	// Project a 3D world position to 2D viewport screen coordinates
	auto project = [&](const glm::vec3& world_pos) -> ImVec2 {
		const glm::vec4 clip = vp * glm::vec4(world_pos, 1.0f);
		if (clip.w <= 0.0f) {
			return {-1.0f, -1.0f};
		}
		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		return {viewport_min.x + (ndc.x * 0.5f + 0.5f) * viewport_size.x,
				viewport_min.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport_size.y};
	};

	const glm::vec3 origin = entity_transform.position;
	const ImVec2 origin_2d = project(origin);
	if (origin_2d.x < 0.0f) {
		return;
	}

	// Compute axis endpoint scale based on distance to camera
	const auto& cam_transform = editor_camera.get<engine::components::Transform>();
	const float dist_to_camera = glm::length(origin - cam_transform.position);
	const float axis_world_len = std::max(dist_to_camera * 0.15f, 0.01f);

	// Determine axis directions based on gizmo space (local or world)
	glm::vec3 axes[3];
	if (gizmo_space_ == GizmoSpace::Local && entity_transform.rotation != glm::vec3(0.0f)) {
		// Local space: transform axes by entity's rotation
		const glm::quat entity_quat = glm::quat(entity_transform.rotation);
		axes[0] = entity_quat * glm::vec3(axis_world_len, 0.0f, 0.0f);
		axes[1] = entity_quat * glm::vec3(0.0f, axis_world_len, 0.0f);
		axes[2] = entity_quat * glm::vec3(0.0f, 0.0f, axis_world_len);
	}
	else {
		// World space: use world axes
		axes[0] = {axis_world_len, 0.0f, 0.0f};
		axes[1] = {0.0f, axis_world_len, 0.0f};
		axes[2] = {0.0f, 0.0f, axis_world_len};
	}

	ImVec2 axis_ends_2d[3];
	for (int i = 0; i < 3; ++i) {
		axis_ends_2d[i] = project(origin + axes[i]);
	}

	// Distance from point to line segment
	auto point_to_segment_dist = [](const ImVec2& p, const ImVec2& a, const ImVec2& b) -> float {
		const ImVec2 ab = {b.x - a.x, b.y - a.y};
		const ImVec2 ap = {p.x - a.x, p.y - a.y};
		const float ab_sq = ab.x * ab.x + ab.y * ab.y;
		if (ab_sq < 1e-6f) {
			return std::sqrt(ap.x * ap.x + ap.y * ap.y);
		}
		const float t = std::clamp((ap.x * ab.x + ap.y * ab.y) / ab_sq, 0.0f, 1.0f);
		const float dx = ap.x - ab.x * t;
		const float dy = ap.y - ab.y * t;
		return std::sqrt(dx * dx + dy * dy);
	};

	const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	const bool right_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);
	const bool left_pressed = !right_down && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	const bool left_down = !right_down && ImGui::IsMouseDown(ImGuiMouseButton_Left);

	// Check for center cube hover (for uniform scale)
	const float center_dist = std::sqrt(
			(mouse_pos.x - origin_2d.x) * (mouse_pos.x - origin_2d.x)
			+ (mouse_pos.y - origin_2d.y) * (mouse_pos.y - origin_2d.y));
	const bool center_hovered = (center_dist < 10.0f) && dragging_axis_ < 0 && !right_down;

	// Determine hovered axis
	int hovered_axis = -1;
	if (dragging_axis_ < 0 && !right_down && !center_hovered) {
		float best_dist = kGizmoHitRadius;
		for (int i = 0; i < 3; ++i) {
			if (axis_ends_2d[i].x < 0.0f) {
				continue;
			}
			const float d = point_to_segment_dist(mouse_pos, origin_2d, axis_ends_2d[i]);
			if (d < best_dist) {
				best_dist = d;
				hovered_axis = i;
			}
		}
	}

	// Begin drag
	if (left_pressed) {
		if (center_hovered) {
			// Uniform scale mode
			dragging_axis_ = 3; // Special value for uniform scale
			drag_start_mouse_ = mouse_pos;
			drag_start_scale_ = entity_transform.scale;
		}
		else if (hovered_axis >= 0) {
			// Single axis scale
			const ImVec2& axis_end = axis_ends_2d[hovered_axis];
			ImVec2 axis_dir = {axis_end.x - origin_2d.x, axis_end.y - origin_2d.y};
			const float axis_screen_len = std::sqrt(axis_dir.x * axis_dir.x + axis_dir.y * axis_dir.y);
			if (axis_screen_len > 1e-4f) {
				dragging_axis_ = hovered_axis;
				drag_start_mouse_ = mouse_pos;
				drag_start_scale_ = entity_transform.scale;
				drag_start_rotation_ = entity_transform.rotation; // For local space
				drag_axis_screen_dir_ = {axis_dir.x / axis_screen_len, axis_dir.y / axis_screen_len};
				drag_world_per_pixel_ = axis_world_len / axis_screen_len;
			}
		}
	}

	// Process drag
	if (dragging_axis_ >= 0) {
		if (left_down) {
			auto& transform = selected_entity.get_mut<engine::components::Transform>();

			if (dragging_axis_ == 3) {
				// Uniform scale: use vertical mouse movement
				const float mouse_delta_y = mouse_pos.y - drag_start_mouse_.y;
				float scale_factor = 1.0f - mouse_delta_y * 0.01f; // Scale sensitivity

				// Apply snapping if Ctrl is held
				if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT_CONTROL)
					|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT_CONTROL)) {
					scale_factor = std::round(scale_factor / scale_snap_) * scale_snap_;
				}

				transform.scale = drag_start_scale_ * scale_factor;
			}
			else {
				// Single axis scale
				const ImVec2 mouse_delta = {mouse_pos.x - drag_start_mouse_.x, mouse_pos.y - drag_start_mouse_.y};
				const float projected =
						mouse_delta.x * drag_axis_screen_dir_.x + mouse_delta.y * drag_axis_screen_dir_.y;

				float scale_delta = projected * 0.01f; // Scale sensitivity

				// Apply snapping if Ctrl is held
				if (engine::input::Input::IsKeyPressed(engine::input::KeyCode::LEFT_CONTROL)
					|| engine::input::Input::IsKeyPressed(engine::input::KeyCode::RIGHT_CONTROL)) {
					scale_delta = std::round(scale_delta / scale_snap_) * scale_snap_;
				}

				transform.scale = drag_start_scale_;
				transform.scale[dragging_axis_] += scale_delta;
				// Prevent negative or zero scale
				transform.scale[dragging_axis_] = std::max(transform.scale[dragging_axis_], 0.01f);
			}

			selected_entity.modified<engine::components::Transform>();
		}
		else {
			dragging_axis_ = -1;
		}
	}

	// Draw axis lines with cube endpoints
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	constexpr float kCubeSize = 8.0f;

	for (int i = 0; i < 3; ++i) {
		if (axis_ends_2d[i].x < 0.0f) {
			continue;
		}
		const bool highlight = (i == dragging_axis_ || i == hovered_axis);
		const ImU32 color = highlight ? kAxisHoverColors[i] : kAxisColors[i];
		const float thickness = highlight ? kGizmoThickness + 1.0f : kGizmoThickness;

		// Draw line
		draw_list->AddLine(origin_2d, axis_ends_2d[i], color, thickness);

		// Draw cube at endpoint
		const ImVec2& cube_center = axis_ends_2d[i];
		draw_list->AddRectFilled(
				ImVec2(cube_center.x - kCubeSize * 0.5f, cube_center.y - kCubeSize * 0.5f),
				ImVec2(cube_center.x + kCubeSize * 0.5f, cube_center.y + kCubeSize * 0.5f),
				color);
	}

	// Draw center cube for uniform scale
	const bool center_highlight = (dragging_axis_ == 3 || center_hovered);
	const ImU32 center_color = center_highlight ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 200);
	const float center_cube_size = center_highlight ? 10.0f : 8.0f;
	draw_list->AddRectFilled(
			ImVec2(origin_2d.x - center_cube_size * 0.5f, origin_2d.y - center_cube_size * 0.5f),
			ImVec2(origin_2d.x + center_cube_size * 0.5f, origin_2d.y + center_cube_size * 0.5f),
			center_color);
}

void ViewportPanel::RenderOrientationGizmo(const ImVec2& viewport_min, const ImVec2& viewport_size) {
	const ImVec2 center = {
			viewport_min.x + viewport_size.x - kOrientationGizmoSize - kOrientationGizmoMargin,
			viewport_min.y + kOrientationGizmoSize + kOrientationGizmoMargin};

	// Rotate world axes by inverse camera orientation to get screen-space directions
	const glm::quat inv = glm::inverse(camera_orientation_);
	const glm::vec3 world_axes[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Background circle
	draw_list->AddCircleFilled(center, kOrientationGizmoSize + 4.0f, IM_COL32(30, 30, 30, 160));
	draw_list->AddCircle(center, kOrientationGizmoSize + 4.0f, IM_COL32(80, 80, 80, 200));

	for (int i = 0; i < 3; ++i) {
		const glm::vec3 rotated = inv * world_axes[i];
		// Project to 2D: X maps to screen right, Y maps to screen up
		const ImVec2 end = {center.x + rotated.x * kOrientationGizmoSize, center.y - rotated.y * kOrientationGizmoSize};
		// Dim axes pointing away from camera (negative Z)
		const ImU32 color = rotated.z < 0.0f ? IM_COL32(
													   (kAxisColors[i] >> 0) & 0xFF,
													   (kAxisColors[i] >> 8) & 0xFF,
													   (kAxisColors[i] >> 16) & 0xFF,
													   100)
											 : kAxisColors[i];
		DrawAxisLine(draw_list, {.origin = center, .end = end, .color = color, .thickness = 2.0f, .arrow_size = 7.0f});
		DrawAxisLabel(draw_list, end, kAxisLabels[i], color);
	}
}

void ViewportPanel::DrawAxisLine(ImDrawList* draw_list, const AxisDrawParams& params) {
	draw_list->AddLine(params.origin, params.end, params.color, params.thickness);

	ImVec2 dir = {params.end.x - params.origin.x, params.end.y - params.origin.y};
	const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
	if (len > 1e-4f) {
		dir.x /= len;
		dir.y /= len;
		const ImVec2 perp = {-dir.y, dir.x};
		const ImVec2& tip = params.end;
		const float s = params.arrow_size;
		const ImVec2 left_pt = {tip.x - dir.x * s + perp.x * s * 0.4f, tip.y - dir.y * s + perp.y * s * 0.4f};
		const ImVec2 right_pt = {tip.x - dir.x * s - perp.x * s * 0.4f, tip.y - dir.y * s - perp.y * s * 0.4f};
		draw_list->AddTriangleFilled(tip, left_pt, right_pt, params.color);
	}
}

void ViewportPanel::DrawAxisLabel(ImDrawList* draw_list, const ImVec2& pos, const char* label, const ImU32 color) {
	const ImVec2 text_size = ImGui::CalcTextSize(label);
	draw_list->AddText(
			{pos.x - text_size.x * 0.5f + 1.0f, pos.y - text_size.y * 0.5f + 1.0f}, IM_COL32(0, 0, 0, 180), label);
	draw_list->AddText({pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f}, color, label);
}

} // namespace editor
