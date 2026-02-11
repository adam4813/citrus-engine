module;

#include <flecs.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

module engine.physics;

import engine.components;
import glm;

namespace engine::physics {

// Private accumulator for Bullet3 fixed timestep
struct Bullet3Accumulator {
	float accumulated_time{0.0F};
};

Bullet3PhysicsModule::Bullet3PhysicsModule(const flecs::world& world) {
	if (!world.module<Bullet3PhysicsModule>("physics::bullet3")) {
		spdlog::error("[Bullet3PhysicsModule] Failed to create Bullet3 module");
		return;
	}

	// Create and initialize Bullet3 backend
	auto backend = std::shared_ptr(CreatePhysicsBackend(PhysicsEngineType::Bullet3));

	// Ensure PhysicsWorldConfig singleton exists
	if (!world.has<PhysicsWorldConfig>()) {
		world.set<PhysicsWorldConfig>({});
	}

	// Get config from PhysicsWorldConfig singleton
	PhysicsConfig config;
	const auto& [gravity, fixed_timestep, max_substeps, enable_sleeping] = world.get<PhysicsWorldConfig>();
	config.gravity = gravity;
	config.fixed_timestep = fixed_timestep;
	config.max_substeps = max_substeps;
	config.enable_sleeping = enable_sleeping;

	if (!backend->Initialize(config)) {
		spdlog::error("[Bullet3PhysicsModule] Failed to initialize Bullet3 backend");
		return;
	}

	// Use the Simulation phase if it exists, otherwise fall back to OnUpdate
	auto simulation_phase = world.lookup("::Simulation");
	if (!simulation_phase) {
		simulation_phase = flecs::entity(world, flecs::OnUpdate);
	}

	// Observer: When RigidBody + CollisionShape are set, sync to backend
	world.observer<const components::Transform, const RigidBody, const CollisionShape>("Bullet3SyncToBackend")
			.event(flecs::OnSet)
			.each([backend](
						  const flecs::entity e,
						  const components::Transform& t,
						  const RigidBody& rb,
						  const CollisionShape& cs) {
				PhysicsTransform pt;
				pt.position = t.position;
				const glm::quat qx = glm::angleAxis(t.rotation.x, glm::vec3(1, 0, 0));
				const glm::quat qy = glm::angleAxis(t.rotation.y, glm::vec3(0, 1, 0));
				const glm::quat qz = glm::angleAxis(t.rotation.z, glm::vec3(0, 0, 1));
				pt.rotation = qz * qy * qx;

				backend->SyncBodyToBackend(e.id(), pt, rb, cs);

				if (rb.motion_type == MotionType::Dynamic && !e.has<PhysicsVelocity>()) {
					e.set<PhysicsVelocity>({});
				}
			});

	// Observer: Remove body when RigidBody component is removed
	world.observer<const RigidBody>("Bullet3RemoveBody")
			.event(flecs::OnRemove)
			.each([backend](const flecs::entity e, const RigidBody&) { backend->RemoveBody(e.id()); });

	// System: Apply forces
	world.system<const PhysicsForce>("Bullet3ApplyForces")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, const PhysicsForce& force) {
				if (backend->HasBody(e.id())) {
					backend->ApplyForce(e.id(), force.force, force.torque);
				}
				if (force.clear_after_apply) {
					e.remove<PhysicsForce>();
				}
			});

	// System: Apply impulses (consumed immediately)
	world.system<const PhysicsImpulse>("Bullet3ApplyImpulses")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, const PhysicsImpulse& impulse) {
				if (backend->HasBody(e.id())) {
					backend->ApplyImpulse(e.id(), impulse.impulse, impulse.point);
				}
				e.remove<PhysicsImpulse>();
			});

	// Accumulator singleton
	world.set<Bullet3Accumulator>({});

	// System: Step simulation with fixed timestep
	world.system("Bullet3PhysicsStep").kind(simulation_phase).run([backend](const flecs::iter& it) {
		const auto world = it.world();
		auto& [accumulated_time] = world.get_mut<Bullet3Accumulator>();
		const auto& cfg = world.get<PhysicsWorldConfig>();

		backend->SetGravity(cfg.gravity);

		accumulated_time += it.delta_time();

		int steps = 0;
		while (accumulated_time >= cfg.fixed_timestep && steps < cfg.max_substeps) {
			backend->StepSimulation(cfg.fixed_timestep);
			accumulated_time -= cfg.fixed_timestep;
			++steps;
		}

		if (accumulated_time > cfg.fixed_timestep * cfg.max_substeps) {
			accumulated_time = cfg.fixed_timestep * cfg.max_substeps;
		}
	});

	// System: Sync results back to ECS
	world.system<components::Transform, PhysicsVelocity, const RigidBody>("Bullet3SyncFromBackend")
			.kind(simulation_phase)
			.each([backend](const flecs::entity e, components::Transform& t, PhysicsVelocity& v, const RigidBody& rb) {
				if (rb.motion_type != MotionType::Dynamic || !backend->HasBody(e.id())) {
					return;
				}

				const auto [position, rotation, linear_velocity, angular_velocity] =
						backend->SyncBodyFromBackend(e.id());
				t.position = position;
				t.rotation = glm::eulerAngles(rotation);
				v.linear = linear_velocity;
				v.angular = angular_velocity;
			});

	// System: Distribute collision events
	world.system("Bullet3CollisionEvents").kind(simulation_phase).run([backend](const flecs::iter& it) {
		const auto world = it.world();

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

	spdlog::info("[Bullet3PhysicsModule] Registered with flecs");
}

} // namespace engine::physics
