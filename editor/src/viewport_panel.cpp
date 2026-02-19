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

	// Get the scene root to iterate entities
	if (!scene) {
		return;
	}

	const flecs::entity scene_root = scene->GetSceneRoot();
	if (!scene_root.is_valid()) {
		return;
	}

	// Try physics raycasting first if physics backend is available
	bool used_physics = false;
	if (scene_root.world().has<engine::physics::PhysicsBackendPtr>()) {
		const auto backend_ptr = scene_root.world().get<engine::physics::PhysicsBackendPtr>();
		if (backend_ptr.backend) {
			// Construct physics ray
			engine::physics::Ray ray;
			ray.origin = ray_origin;
			ray.direction = ray_world;
			ray.max_distance = 10000.0f; // Large distance for viewport picking
			ray.collision_mask = 0xFFFFFFFF; // Pick all layers

			// Perform raycast
			if (auto result = backend_ptr.backend->Raycast(ray)) {
				if (result->HasHit()) {
					auto hit_entity = scene_root.world().entity(result->entity);
					if (hit_entity != scene_root) {
						picked_entity = hit_entity;
						used_physics = true;
					}
				}
			}
		}
	}

	// Fallback to distance-based picking if physics didn't find anything
	// (e.g., entities without collision shapes)
	if (!used_physics || !picked_entity.is_valid()) {
		float closest_distance = std::numeric_limits<float>::max();

		// Iterate all entities with WorldTransform and either Renderable or Sprite
		scene_root.world().each([&](flecs::entity entity, const engine::components::WorldTransform& world_transform) {
			// Never select the scene root
			if (entity == scene_root) {
				return;
			}

			// Skip if entity is not visible (check if it has Renderable and is not visible)
			if (entity.has<engine::rendering::Renderable>()) {
				const auto& renderable = entity.get<engine::rendering::Renderable>();
				if (!renderable.visible) {
					return;
				}
			}

			// Extract entity position from world transform
			const glm::vec3 entity_pos = world_transform.position;

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
	}

	// Call the selection callback with the picked entity (or invalid entity if nothing picked)
	callbacks_.on_entity_selected(picked_entity);
}

} // namespace editor
