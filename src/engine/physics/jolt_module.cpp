module;

#include <cstdint>
#include <flecs.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

module engine.physics;

import engine.components;
import glm;

namespace engine::physics {

struct PhysicsAccumulator {
	float accumulated_time{0.0F};
};

JoltPhysicsModule::JoltPhysicsModule(const flecs::world& world) {
	if (!world.module<JoltPhysicsModule>("physics::jolt")) {
		spdlog::error("[JoltPhysicsModule] Failed to create Jolt module");
		return;
	}

	// Create and initialize Jolt backend
	auto backend = std::shared_ptr<IPhysicsBackend>(CreatePhysicsBackend(PhysicsEngineType::JoltPhysics));

	// Ensure PhysicsWorldConfig singleton exists
	if (!world.has<PhysicsWorldConfig>()) {
		world.set<PhysicsWorldConfig>({});
	}

	// Get default config from PhysicsWorldConfig singleton
	PhysicsConfig config;
	const auto& [gravity, fixed_timestep, max_substeps, enable_sleeping, show_debug_physics] =
			world.get<PhysicsWorldConfig>();
	config.gravity = gravity;
	config.fixed_timestep = fixed_timestep;
	config.max_substeps = max_substeps;
	config.enable_sleeping = enable_sleeping;

	if (!backend->Initialize(config)) {
		spdlog::error("[JoltPhysicsModule] Failed to initialize Jolt backend");
		return;
	}

	// Store backend pointer in singleton for external access (e.g., raycasting from editor)
	world.set<PhysicsBackendPtr>({backend});

	// Find the Simulation phase created by ECSWorld
	// Use the Simulation phase if it exists, otherwise fall back to OnUpdate
	// Use root scope lookup since the phase is created at root level by ECSWorld
	auto simulation_phase = world.lookup("::Simulation");
	if (!simulation_phase) {
		simulation_phase = flecs::entity(world, flecs::OnUpdate);
	}

	// Observer: When RigidBody + CollisionShape are set on an entity, sync to backend
	world.observer<const components::WorldTransform, const RigidBody, const CollisionShape>("JoltSyncToBackend")
			.event(flecs::OnSet)
			.each([backend](
						  const flecs::entity e,
						  const components::WorldTransform& wt,
						  const RigidBody& rb,
						  const CollisionShape& cs) {
				backend->SyncBodyToBackend(e.id(), PhysicsTransform::FromMatrix(wt.matrix), rb, cs);

				// Add PhysicsVelocity if not present (for dynamic bodies)
				if (rb.motion_type == MotionType::Dynamic && !e.has<PhysicsVelocity>()) {
					e.set<PhysicsVelocity>({});
				}
			});

	// Observer: When RigidBody is removed, remove from backend
	world.observer<const RigidBody>("JoltRemoveBody")
			.event(flecs::OnRemove)
			.each([backend](const flecs::entity e, const RigidBody&) { backend->RemoveBody(e.id()); });

	// System: Apply forces from PhysicsForce components
	world.system<const PhysicsForce>("JoltApplyForces")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, const PhysicsForce& force) {
				if (backend->HasBody(e.id())) {
					backend->ApplyForce(e.id(), force.force, force.torque);
				}
				if (force.clear_after_apply) {
					e.remove<PhysicsForce>();
				}
			});

	// System: Apply impulses from PhysicsImpulse components (consumed immediately)
	world.system<const PhysicsImpulse>("JoltApplyImpulses")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, const PhysicsImpulse& impulse) {
				if (backend->HasBody(e.id())) {
					backend->ApplyImpulse(e.id(), impulse.impulse, impulse.point);
				}
				e.remove<PhysicsImpulse>();
			});

	// Fixed timestep accumulator stored as a singleton
	world.set<PhysicsAccumulator>({});

	// System: Step physics simulation (runs once per frame, handles fixed timestep)
	world.system("JoltPhysicsStep").kind(simulation_phase).run([backend](const flecs::iter& it) {
		const auto world = it.world();
		auto& [accumulated_time] = world.get_mut<PhysicsAccumulator>();
		const auto& cfg = world.get<PhysicsWorldConfig>();

		backend->SetGravity(cfg.gravity);

		accumulated_time += it.delta_time();

		int steps = 0;
		while (accumulated_time >= cfg.fixed_timestep && steps < cfg.max_substeps) {
			backend->StepSimulation(cfg.fixed_timestep);
			accumulated_time -= cfg.fixed_timestep;
			++steps;
		}

		// Clamp to prevent spiral of death
		if (accumulated_time > cfg.fixed_timestep * cfg.max_substeps) {
			accumulated_time = cfg.fixed_timestep * cfg.max_substeps;
		}
	});

	// System: Sync results from backend back to ECS components
	world.system<components::Transform, PhysicsVelocity, const RigidBody>("JoltSyncFromBackend")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, components::Transform& t, PhysicsVelocity& v, const RigidBody& rb) {
				if (rb.motion_type != MotionType::Dynamic || !backend->HasBody(e.id())) {
					return;
				}

				auto [position, rotation, linear_velocity, angular_velocity] = backend->SyncBodyFromBackend(e.id());
				t.position = position;

				// Convert quaternion back to euler angles
				const glm::vec3 euler = glm::eulerAngles(rotation);
				t.rotation = euler;

				v.linear = linear_velocity;
				v.angular = angular_velocity;
			});

	// System: Clear old collision events, then distribute new ones
	world.system("JoltCollisionEvents").kind(simulation_phase).run([backend](const flecs::iter& it) {
		const auto world = it.world();

		// Clear previous frame's collision events
		world.query<CollisionEvents>().each([](CollisionEvents& ce) { ce.events.clear(); });

		for (const auto events = backend->GetCollisionEvents(); const auto& event : events) {
			auto entity_a = world.entity(event.entity_a);
			auto entity_b = world.entity(event.entity_b);

			if (entity_a.is_valid() && entity_a.has<CollisionEvents>()) {
				entity_a.get_mut<CollisionEvents>().events.push_back(event);
			}
			if (entity_b.is_valid() && entity_b.has<CollisionEvents>()) {
				entity_b.get_mut<CollisionEvents>().events.push_back(event);
			}
		}
	});

	spdlog::info("[JoltPhysicsModule] Registered with flecs");
}

} // namespace engine::physics
