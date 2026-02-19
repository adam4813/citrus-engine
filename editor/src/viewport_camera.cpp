#include "viewport_panel.h"

namespace editor {

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

} // namespace editor
