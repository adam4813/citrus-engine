#include "viewport_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <imgui.h>
#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

namespace editor {

void ViewportPanel::Render(
		engine::Engine& engine,
		engine::scene::Scene* scene,
		const bool is_running,
		const flecs::entity editor_camera,
		const float delta_time,
		const flecs::entity selected_entity) {
	if (!is_visible_) {
		return;
	}

	ImGui::Begin("Viewport", &is_visible_);

	// Track focus state for camera controls
	is_focused_ = ImGui::IsWindowFocused();

	const ImVec2 content_size = ImGui::GetContentRegionAvail();
	const auto viewport_width = static_cast<std::uint32_t>(content_size.x);
	const auto viewport_height = static_cast<uint32_t>(content_size.y);

	// Resize framebuffer if viewport size changed
	if (viewport_width > 0 && viewport_height > 0
		&& (viewport_width != last_width_ || viewport_height != last_height_)) {
		framebuffer_.Resize(viewport_width, viewport_height);
		last_width_ = viewport_width;
		last_height_ = viewport_height;

		// Update camera aspect ratio
		auto active_camera = engine.ecs.GetActiveCamera();
		if (active_camera.is_valid() && active_camera.has<engine::components::Camera>()) {
			auto& camera = active_camera.get_mut<engine::components::Camera>();
			camera.aspect_ratio = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);
		}
	}

	// Handle camera input when viewport is focused and not in play mode
	if (is_focused_ && !is_running && editor_camera.is_valid()) {
		HandleCameraInput(editor_camera, delta_time);
	}

	// Render scene to framebuffer
	if (framebuffer_.IsValid()) {
		framebuffer_.Bind();

		// Clear the framebuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render the scene
		scene->Render();
		// The scene's render method isn't set up to call the ECS render submission directly.
		// So we manually submit the render commands from the ECS world.
		engine.ecs.SubmitRenderCommands(engine::rendering::GetRenderer());

		engine::rendering::Framebuffer::Unbind();

		// Restore main viewport
		uint32_t main_width = 0;
		uint32_t main_height = 0;
		engine.renderer->GetFramebufferSize(main_width, main_height);
		glViewport(0, 0, static_cast<GLsizei>(main_width), static_cast<GLsizei>(main_height));
	}

	// Display the framebuffer texture in ImGui
	if (framebuffer_.IsValid()) {
		// ImGui expects texture ID as void*, OpenGL textures are uint32
		const auto texture_id = static_cast<ImTextureID>(static_cast<uintptr_t>(framebuffer_.GetColorTextureId()));
		// Flip UV vertically because OpenGL textures are bottom-up
		ImGui::Image(texture_id, content_size, ImVec2(0, 1), ImVec2(1, 0));

		// Draw transform gizmo overlay on selected entity
		if (!is_running && selected_entity.is_valid()
			&& selected_entity.has<engine::components::Transform>()) {
			const ImVec2 viewport_min = ImGui::GetItemRectMin();
			RenderTransformGizmo(selected_entity, editor_camera, viewport_min, content_size);
		}

		// Draw orientation gizmo in upper-right corner
		if (editor_camera.is_valid()) {
			const ImVec2 viewport_min = ImGui::GetItemRectMin();
			RenderOrientationGizmo(viewport_min, content_size);
		}
	}

	// Show play mode indicator overlay
	if (is_running) {
		const ImVec2 cursor_pos = ImGui::GetItemRectMin();
		RenderPlayModeIndicator(cursor_pos);
	}

	ImGui::End();
}

void ViewportPanel::HandleCameraInput(const flecs::entity editor_camera, const float delta_time) {
	using namespace engine::input;
	using engine::input::KeyCode;

	if (!editor_camera.has<engine::components::Transform>()) {
		return;
	}

	auto& transform = editor_camera.get_mut<engine::components::Transform>();

	// Calculate movement speed (Shift for fast movement)
	float speed = kMoveSpeed;
	if (Input::IsKeyPressed(KeyCode::LEFT_SHIFT) || Input::IsKeyPressed(KeyCode::RIGHT_SHIFT)) {
		speed *= kFastMoveMultiplier;
	}

	// Derive direction vectors from quaternion orientation
	const glm::vec3 forward = camera_orientation_ * glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 right = camera_orientation_ * glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 movement(0.0f);

	// WASD for horizontal movement
	if (Input::IsKeyPressed(KeyCode::W)) {
		movement += forward;
	}
	if (Input::IsKeyPressed(KeyCode::S)) {
		movement -= forward;
	}
	if (Input::IsKeyPressed(KeyCode::A)) {
		movement -= right;
	}
	if (Input::IsKeyPressed(KeyCode::D)) {
		movement += right;
	}

	// Q/E for vertical movement
	if (Input::IsKeyPressed(KeyCode::Q)) {
		movement -= up;
	}
	if (Input::IsKeyPressed(KeyCode::E)) {
		movement += up;
	}

	// Right-click mouse look (quaternion-based, no gimbal lock)
	bool camera_dirty = false;
	const float mouse_x = Input::GetMouseX();
	const float mouse_y = Input::GetMouseY();

	if (Input::IsMouseButtonDown(MouseButton::RIGHT)) {
		if (is_right_mouse_down_) {
			const float dx = mouse_x - last_mouse_x_;
			const float dy = mouse_y - last_mouse_y_;

			if (dx != 0.0f || dy != 0.0f) {
				// Yaw around world up axis, pitch around camera's local right axis
				const glm::quat yaw_rot = glm::angleAxis(-dx * kMouseSensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
				const glm::vec3 local_right = camera_orientation_ * glm::vec3(1.0f, 0.0f, 0.0f);
				const glm::quat pitch_rot = glm::angleAxis(-dy * kMouseSensitivity, local_right);
				camera_orientation_ = glm::normalize(yaw_rot * pitch_rot * camera_orientation_);
				camera_dirty = true;
			}
		}
		is_right_mouse_down_ = true;
	}
	else {
		is_right_mouse_down_ = false;
	}
	last_mouse_x_ = mouse_x;
	last_mouse_y_ = mouse_y;

	// Apply movement if any
	if (glm::length(movement) > 0.0f) {
		movement = glm::normalize(movement) * speed * delta_time;
		transform.position += movement;
		camera_dirty = true;
	}

	// Update transform rotation and camera target
	if (camera_dirty) {
		transform.rotation = glm::eulerAngles(camera_orientation_);
		editor_camera.modified<engine::components::Transform>();

		if (editor_camera.has<engine::components::Camera>()) {
			const glm::vec3 look_dir = camera_orientation_ * glm::vec3(0.0f, 0.0f, -1.0f);
			auto& camera = editor_camera.get_mut<engine::components::Camera>();
			camera.target = transform.position + look_dir;
			editor_camera.modified<engine::components::Camera>();
		}
	}
}

void ViewportPanel::RenderTransformGizmo(
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
		return {
				viewport_min.x + (ndc.x * 0.5f + 0.5f) * viewport_size.x,
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

	const glm::vec3 axes[3] = {
			{axis_world_len, 0.0f, 0.0f},
			{0.0f, axis_world_len, 0.0f},
			{0.0f, 0.0f, axis_world_len}};

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
			glm::vec3 axis_dir(0.0f);
			axis_dir[dragging_axis_] = 1.0f;
			transform.position = drag_start_position_ + axis_dir * projected * drag_world_per_pixel_;
			selected_entity.modified<engine::components::Transform>();
		} else {
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
		DrawAxisLine(draw_list, {
				.origin = origin_2d,
				.end = axis_ends_2d[i],
				.color = highlight ? kAxisHoverColors[i] : kAxisColors[i],
				.thickness = highlight ? kGizmoThickness + 1.5f : kGizmoThickness,
				.arrow_size = kArrowHeadSize});
	}

	// Draw origin circle
	draw_list->AddCircleFilled(origin_2d, 4.0f, IM_COL32(255, 255, 255, 200));
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
		const ImVec2 end = {
				center.x + rotated.x * kOrientationGizmoSize,
				center.y - rotated.y * kOrientationGizmoSize};
		// Dim axes pointing away from camera (negative Z)
		const ImU32 color = rotated.z < 0.0f
									? IM_COL32(
											  (kAxisColors[i] >> 0) & 0xFF,
											  (kAxisColors[i] >> 8) & 0xFF,
											  (kAxisColors[i] >> 16) & 0xFF,
											  100)
									: kAxisColors[i];
		DrawAxisLine(draw_list, {
				.origin = center,
				.end = end,
				.color = color,
				.thickness = 2.0f,
				.arrow_size = 7.0f});
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
		const ImVec2 left_pt = {tip.x - dir.x * s + perp.x * s * 0.4f,
								tip.y - dir.y * s + perp.y * s * 0.4f};
		const ImVec2 right_pt = {tip.x - dir.x * s - perp.x * s * 0.4f,
								 tip.y - dir.y * s - perp.y * s * 0.4f};
		draw_list->AddTriangleFilled(tip, left_pt, right_pt, params.color);
	}
}

void ViewportPanel::DrawAxisLabel(ImDrawList* draw_list, const ImVec2& pos, const char* label, const ImU32 color) {
	const ImVec2 text_size = ImGui::CalcTextSize(label);
	draw_list->AddText({pos.x - text_size.x * 0.5f + 1.0f, pos.y - text_size.y * 0.5f + 1.0f},
					   IM_COL32(0, 0, 0, 180), label);
	draw_list->AddText({pos.x - text_size.x * 0.5f, pos.y - text_size.y * 0.5f}, color, label);
}

void ViewportPanel::RenderPlayModeIndicator(const ImVec2& cursor_pos) {
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const auto play_text = "PLAYING";
	const ImVec2 play_text_size = ImGui::CalcTextSize(play_text);

	draw_list->AddRectFilled(
			ImVec2(cursor_pos.x + 5, cursor_pos.y + 5),
			ImVec2(cursor_pos.x + play_text_size.x + 15, cursor_pos.y + play_text_size.y + 15),
			IM_COL32(0, 100, 0, 200));
	draw_list->AddText(ImVec2(cursor_pos.x + 10, cursor_pos.y + 10), IM_COL32(255, 255, 255, 255), play_text);
}

} // namespace editor
