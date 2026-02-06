#include "viewport_panel.h"

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
		const float delta_time) {
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
