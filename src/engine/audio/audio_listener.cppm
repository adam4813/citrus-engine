module;

export module engine.audio.listener;

import glm;

export namespace engine::audio {

// AudioListener component for ECS - represents the "ears" for spatial audio
// Typically attached to the camera or player entity
struct AudioListener {
	glm::vec3 position{0.0f, 0.0f, 0.0f};   // Position in 3D space
	glm::vec3 forward{0.0f, 0.0f, -1.0f};   // Forward direction vector
	glm::vec3 up{0.0f, 1.0f, 0.0f};         // Up direction vector

	AudioListener() = default;

	AudioListener(const glm::vec3& pos, const glm::vec3& fwd, const glm::vec3& up_vec)
		: position(pos), forward(fwd), up(up_vec) {}
};

} // namespace engine::audio
