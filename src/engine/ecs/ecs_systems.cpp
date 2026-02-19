module;

#include <flecs.h>

module engine.ecs;
import engine;
import glm;

using namespace engine::components;
using namespace engine::animation;

namespace engine::ecs {

void ECSWorld::SetupMovementSystem() const {
	// System to update positions based on velocity (Simulation phase)
	world_.system<Transform, const Velocity>()
			.kind(simulation_phase_)
			.each([](const flecs::iter itr, const size_t index, Transform& transform, const Velocity& velocity) {
				transform.position += velocity.linear * itr.delta_time();
				transform.rotation += velocity.angular * itr.delta_time();
				itr.entity(index).modified<Transform>();
			});
}

void ECSWorld::SetupRotationSystem() const {
	// System for entities with Rotating tag - simple rotation animation (Simulation phase)
	world_.system<Transform, Rotating>()
			.kind(simulation_phase_)
			.each([](const flecs::iter itr, const size_t index, Transform& transform, Rotating) {
				transform.rotation.y += 1.0F * itr.delta_time(); // 1 radian per second
				itr.entity(index).modified<Transform>();
			});
}

void ECSWorld::SetupCameraSystem() const {
	// System to update camera matrices when dirty
	world_.observer<Transform, Camera>("CameraTransformUpdate")
			.event(flecs::OnSet)
			.each([](flecs::iter, size_t, const Transform& transform, Camera& camera) {
				camera.view_matrix = glm::lookAt(transform.position, camera.target, camera.up);
				camera.projection_matrix = glm::perspective(
						glm::radians(camera.fov), camera.aspect_ratio, camera.near_plane, camera.far_plane);
			});
}

void ECSWorld::SetupSpatialSystem() const {
	// System to update spatial bounds when transforms change
	world_.observer<const Transform, Spatial>("SpatialBoundsUpdate")
			.event(flecs::OnSet)
			.each([](flecs::entity, const Transform&, Spatial& spatial) { spatial.bounds_dirty = true; });
}

void ECSWorld::SetupAnimationSystem() const {
	// Register the animation system with the flecs world
	AnimationSystem::Register(world_);
}

void ECSWorld::SetupAudioSystem() const {
	// Audio ECS system: sync AudioSource/AudioListener components with AudioSystem
	// Runs in simulation phase — only active during play mode, not in editor mode
	world_.system<audio::AudioSource>("AudioSourceSystem")
			.kind(simulation_phase_)
			.each([](flecs::entity entity, audio::AudioSource& source) {
				auto& audio_sys = audio::AudioSystem::Get();
				if (!audio_sys.IsInitialized()) {
					return;
				}

				switch (source.state) {
				case audio::PlayState::Playing:
					if (source.play_handle == 0 && source.clip_id != 0) {
						source.play_handle = audio_sys.PlaySoundClip(source.clip_id, source.volume, source.looping);
					}
					else if (source.play_handle != 0) {
						// Resume if transitioning from Paused→Playing
						audio_sys.ResumeSound(source.play_handle);
					}
					if (source.play_handle != 0) {
						audio_sys.SetVolume(source.play_handle, source.volume);
						audio_sys.SetPitch(source.play_handle, source.pitch);
						if (source.spatial) {
							// Sync position from WorldTransform if available
							if (entity.has<WorldTransform>()) {
								const auto& wt = entity.get<WorldTransform>();
								audio_sys.SetSourcePosition(
										source.play_handle, wt.position.x, wt.position.y, wt.position.z);
							}
							else {
								audio_sys.SetSourcePosition(
										source.play_handle, source.position.x, source.position.y, source.position.z);
							}
						}
					}
					break;
				case audio::PlayState::Paused:
					if (source.play_handle != 0) {
						audio_sys.PauseSound(source.play_handle);
					}
					break;
				case audio::PlayState::Stopped:
					if (source.play_handle != 0) {
						audio_sys.StopSound(source.play_handle);
						source.play_handle = 0;
					}
					break;
				}
			});

	// Sync the listener position with the AudioSystem (also simulation-only)
	world_.system<const audio::AudioListener>("AudioListenerSystem")
			.kind(simulation_phase_)
			.each([]([[maybe_unused]] flecs::entity entity, const audio::AudioListener& listener) {
				auto& audio_sys = audio::AudioSystem::Get();
				if (audio_sys.IsInitialized()) {
					audio_sys.SetListenerPosition(listener);
				}
			});
}

} // namespace engine::ecs
