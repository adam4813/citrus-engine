#include "viewport_panel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <imgui.h>
#include <limits>
#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

namespace editor {

std::string_view ViewportPanel::GetPanelName() const { return "Viewport"; }

void ViewportPanel::SetCallbacks(const EditorCallbacks& callbacks) { callbacks_ = callbacks; }

void ViewportPanel::Render(
		engine::Engine& engine,
		engine::scene::Scene* scene,
		const bool is_running,
		const flecs::entity editor_camera,
		const float delta_time,
		const flecs::entity selected_entity) {
	if (!IsVisible()) {
		return;
	}

	ImGui::Begin("Viewport", &VisibleRef());

	// Track focus state for camera controls
	is_focused_ = ImGui::IsWindowFocused();

	// Handle gizmo mode switching (when viewport is focused and not in play mode)
	if (is_focused_ && !is_running) {
		HandleGizmoInput();
	}

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
			// TODO: Set up sending viewport size to ECS system and have it update all cameras, instead of just setting a fixed 16:9 aspect ratio here
			camera.aspect_ratio =
					16.0F / 9.0F; //static_cast<float>(viewport_width) / static_cast<float>(viewport_height);
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

		// Handle object picking in edit mode
		if (!is_running && is_focused_ && editor_camera.is_valid()) {
			const ImVec2 viewport_min = ImGui::GetItemRectMin();
			HandleObjectPicking(editor_camera, viewport_min, content_size, scene);
		}

		// Draw transform gizmo overlay on selected entity
		if (!is_running && selected_entity.is_valid() && selected_entity.has<engine::components::Transform>()) {
			const ImVec2 viewport_min = ImGui::GetItemRectMin();
			RenderTransformGizmo(selected_entity, editor_camera, viewport_min, content_size);

			// Draw gizmo mode indicator
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			const char* mode_text = gizmo_mode_ == GizmoMode::Translate ? "Translate (W)"
									: gizmo_mode_ == GizmoMode::Rotate  ? "Rotate (E)"
																		: "Scale (R)";
			const char* space_text = gizmo_space_ == GizmoSpace::World ? "World (X)" : "Local (X)";

			const ImVec2 mode_text_size = ImGui::CalcTextSize(mode_text);
			const ImVec2 space_text_size = ImGui::CalcTextSize(space_text);
			const float padding = 8.0f;
			const float spacing = 2.0f;

			// Draw mode indicator
			draw_list->AddRectFilled(
					ImVec2(viewport_min.x + padding, viewport_min.y + padding),
					ImVec2(viewport_min.x + padding + mode_text_size.x + 10.0f,
						   viewport_min.y + padding + mode_text_size.y + 6.0f),
					IM_COL32(40, 40, 40, 200));
			draw_list->AddText(
					ImVec2(viewport_min.x + padding + 5.0f, viewport_min.y + padding + 3.0f),
					IM_COL32(255, 255, 255, 255),
					mode_text);

			// Draw space indicator below mode
			const float space_y_offset = mode_text_size.y + 6.0f + spacing;
			draw_list->AddRectFilled(
					ImVec2(viewport_min.x + padding, viewport_min.y + padding + space_y_offset),
					ImVec2(viewport_min.x + padding + space_text_size.x + 10.0f,
						   viewport_min.y + padding + space_y_offset + space_text_size.y + 6.0f),
					IM_COL32(40, 40, 40, 200));
			draw_list->AddText(
					ImVec2(viewport_min.x + padding + 5.0f, viewport_min.y + padding + space_y_offset + 3.0f),
					IM_COL32(255, 255, 255, 255),
					space_text);
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

void ViewportPanel::HandleObjectPicking(
		const flecs::entity editor_camera,
		const ImVec2& viewport_min,
		const ImVec2& viewport_size,
		engine::scene::Scene* scene) {
	// Only handle picking when we have a valid callback
	if (!callbacks_.on_entity_selected) {
		return;
	}

	// Get mouse state
	const ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	const bool left_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	const bool right_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);

	// Don't pick during right-click camera movement
	if (right_down) {
		left_mouse_was_down_ = left_down;
		return;
	}

	// Track when left mouse button goes down to detect click vs drag
	if (left_down && !left_mouse_was_down_) {
		left_mouse_down_pos_ = mouse_pos;
	}

	// Detect click (mouse released without dragging)
	if (!left_down && left_mouse_was_down_) {
		// Check if mouse moved significantly (drag) or stayed in place (click)
		const float dx = mouse_pos.x - left_mouse_down_pos_.x;
		const float dy = mouse_pos.y - left_mouse_down_pos_.y;
		const float drag_dist = std::sqrt(dx * dx + dy * dy);

		// Only pick if it was a click (not a drag) and we're not dragging a gizmo
		if (drag_dist < kPickDragThreshold && dragging_axis_ < 0) {
			// Convert mouse position to viewport-relative coordinates
			const ImVec2 viewport_mouse = {mouse_pos.x - viewport_min.x, mouse_pos.y - viewport_min.y};

			// Check if mouse is within viewport bounds
			if (viewport_mouse.x >= 0 && viewport_mouse.x < viewport_size.x && viewport_mouse.y >= 0
				&& viewport_mouse.y < viewport_size.y) {
				// Perform picking
				PickEntityAtMousePosition(editor_camera, viewport_mouse, viewport_size, scene);
			}
		}
	}

	left_mouse_was_down_ = left_down;
}

void ViewportPanel::PickEntityAtMousePosition(
		const flecs::entity editor_camera,
		const ImVec2& viewport_mouse,
		const ImVec2& viewport_size,
		engine::scene::Scene* scene) {
	if (!editor_camera.is_valid() || !editor_camera.has<engine::components::Camera>()) {
		return;
	}

	const auto& camera = editor_camera.get<engine::components::Camera>();
	const auto& cam_transform = editor_camera.get<engine::components::Transform>();

	// Convert viewport coordinates to normalized device coordinates (NDC)
	// viewport_mouse is in [0, viewport_size], NDC is in [-1, 1]
	const float ndc_x = (viewport_mouse.x / viewport_size.x) * 2.0f - 1.0f;
	const float ndc_y = 1.0f - (viewport_mouse.y / viewport_size.y) * 2.0f; // Flip Y

	// Create a ray in world space from the camera through the mouse position
	const glm::mat4 inv_proj = glm::inverse(camera.projection_matrix);
	const glm::mat4 inv_view = glm::inverse(camera.view_matrix);

	// Point on near plane in clip space
	glm::vec4 ray_clip(ndc_x, ndc_y, -1.0f, 1.0f);

	// Transform to view space
	glm::vec4 ray_view = inv_proj * ray_clip;
	ray_view = glm::vec4(ray_view.x, ray_view.y, -1.0f, 0.0f);

	// Transform to world space
	const glm::vec3 ray_world = glm::normalize(glm::vec3(inv_view * ray_view));
	const glm::vec3 ray_origin = cam_transform.position;

	// Find the closest entity under the ray
	flecs::entity picked_entity;
	float closest_distance = std::numeric_limits<float>::max();

	// Get the scene root to iterate entities
	if (!scene) {
		return;
	}

	const flecs::entity scene_root = scene->GetSceneRoot();
	if (!scene_root.is_valid()) {
		return;
	}

	// Iterate all entities with WorldTransform and either Renderable or Sprite
	scene_root.world().each([&](flecs::entity entity, const engine::components::WorldTransform& world_transform) {
		// Skip if entity is not visible (check if it has Renderable and is not visible)
		if (entity.has<engine::rendering::Renderable>()) {
			const auto& renderable = entity.get<engine::rendering::Renderable>();
			if (!renderable.visible) {
				return;
			}
		}

		// Extract entity position from world transform matrix
		const glm::vec3 entity_pos(
				world_transform.matrix[3][0], world_transform.matrix[3][1], world_transform.matrix[3][2]);

		// Simple distance-based picking - compute distance from ray to entity position
		// Using point-to-line distance formula
		const glm::vec3 ray_to_entity = entity_pos - ray_origin;
		const float t = glm::dot(ray_to_entity, ray_world);

		// Only consider entities in front of the camera
		if (t < 0.0f) {
			return;
		}

		// Point on ray closest to entity
		const glm::vec3 closest_point = ray_origin + ray_world * t;
		const float distance = glm::length(entity_pos - closest_point);

		// Use a simple picking radius for 2D-like picking
		// This could be improved with bounding boxes or sprite sizes
		constexpr float pick_radius = 1.0f;

		if (distance < pick_radius && t < closest_distance) {
			picked_entity = entity;
			closest_distance = t;
		}
	});

	// Call the selection callback with the picked entity (or invalid entity if nothing picked)
	callbacks_.on_entity_selected(picked_entity);
}

} // namespace editor
