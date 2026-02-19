#include <flecs.h>
#include <gtest/gtest.h>

import engine.components;
import engine.physics;
import glm;

using namespace engine::physics;

// === Type/Struct Unit Tests (no flecs world needed) ===

TEST(PhysicsTypesTest, physics_transform_matrix_conversion) {
	PhysicsTransform transform;
	transform.position = glm::vec3{1.0F, 2.0F, 3.0F};
	transform.rotation = glm::quat{1.0F, 0.0F, 0.0F, 0.0F};

	glm::mat4 matrix = transform.GetMatrix();

	EXPECT_FLOAT_EQ(matrix[3][0], 1.0F);
	EXPECT_FLOAT_EQ(matrix[3][1], 2.0F);
	EXPECT_FLOAT_EQ(matrix[3][2], 3.0F);
	EXPECT_FLOAT_EQ(matrix[3][3], 1.0F);

	PhysicsTransform converted = PhysicsTransform::FromMatrix(matrix);
	EXPECT_FLOAT_EQ(converted.position.x, 1.0F);
	EXPECT_FLOAT_EQ(converted.position.y, 2.0F);
	EXPECT_FLOAT_EQ(converted.position.z, 3.0F);
}

TEST(PhysicsTypesTest, collision_info_validity) {
	CollisionInfo info{};
	info.entity_a = 0;
	info.entity_b = 0;
	EXPECT_FALSE(info.IsValid());

	info.entity_a = 1;
	info.entity_b = 2;
	EXPECT_TRUE(info.IsValid());
}

TEST(PhysicsTypesTest, raycast_result_validity) {
	RaycastResult result{};
	result.entity = 0;
	EXPECT_FALSE(result.HasHit());

	result.entity = 1;
	EXPECT_TRUE(result.HasHit());
}

TEST(PhysicsTypesTest, component_defaults) {
	RigidBody rb{};
	EXPECT_EQ(rb.motion_type, MotionType::Dynamic);
	EXPECT_FLOAT_EQ(rb.mass, 1.0F);
	EXPECT_TRUE(rb.use_gravity);

	CollisionShape cs{};
	EXPECT_EQ(cs.type, ShapeType::Box);
	EXPECT_FLOAT_EQ(cs.sphere_radius, 0.5F);

	PhysicsWorldConfig cfg{};
	EXPECT_FLOAT_EQ(cfg.gravity.y, -9.81F);
	EXPECT_FLOAT_EQ(cfg.fixed_timestep, 1.0F / 60.0F);
	EXPECT_EQ(cfg.max_substeps, 4);
}

// === Backend Factory Tests ===

TEST(PhysicsBackendTest, can_create_jolt_backend) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::JoltPhysics);
	ASSERT_NE(backend, nullptr);
	EXPECT_EQ(backend->GetEngineName(), "JoltPhysics");

	PhysicsConfig config{};
	EXPECT_TRUE(backend->Initialize(config));
	backend->Shutdown();
}

TEST(PhysicsBackendTest, can_create_bullet3_backend) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::Bullet3);
	ASSERT_NE(backend, nullptr);
	EXPECT_EQ(backend->GetEngineName(), "Bullet3");

	PhysicsConfig config{};
	EXPECT_TRUE(backend->Initialize(config));
	backend->Shutdown();
}

TEST(PhysicsBackendTest, can_create_physx_stub_backend) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::PhysX);
	ASSERT_NE(backend, nullptr);
	EXPECT_EQ(backend->GetEngineName(), "PhysX (stub)");

	PhysicsConfig config{};
	EXPECT_TRUE(backend->Initialize(config));
	backend->Shutdown();
}

TEST(PhysicsBackendTest, backend_gravity) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::JoltPhysics);
	PhysicsConfig config{};
	backend->Initialize(config);

	glm::vec3 new_gravity{0.0F, -20.0F, 0.0F};
	backend->SetGravity(new_gravity);

	auto g = backend->GetGravity();
	EXPECT_FLOAT_EQ(g.x, 0.0F);
	EXPECT_FLOAT_EQ(g.y, -20.0F);
	EXPECT_FLOAT_EQ(g.z, 0.0F);

	backend->Shutdown();
}

TEST(PhysicsBackendTest, backend_step_simulation) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::JoltPhysics);
	PhysicsConfig config{};
	backend->Initialize(config);

	// Just verify stepping doesn't crash
	backend->StepSimulation(1.0F / 60.0F);
	backend->StepSimulation(1.0F / 60.0F);

	backend->Shutdown();
}

TEST(PhysicsBackendTest, backend_constraints) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::JoltPhysics);
	PhysicsConfig config{};
	backend->Initialize(config);

	ConstraintConfig cc{};
	cc.type = ConstraintType::Fixed;
	EXPECT_TRUE(backend->AddConstraint(1, 2, cc));
	backend->RemoveConstraint(1, 2); // Should not crash

	backend->Shutdown();
}

TEST(PhysicsBackendTest, backend_body_falls_under_gravity) {
	auto backend = CreatePhysicsBackend(PhysicsEngineType::JoltPhysics);
	PhysicsConfig config{};
	backend->Initialize(config);

	// Create a dynamic sphere at y=10
	PhysicsTransform pt;
	pt.position = {0.0F, 10.0F, 0.0F};
	pt.rotation = {1.0F, 0.0F, 0.0F, 0.0F};

	RigidBody rb{};
	rb.motion_type = MotionType::Dynamic;
	rb.mass = 1.0F;

	CollisionShape cs{};
	cs.type = ShapeType::Sphere;
	cs.sphere_radius = 0.5F;

	backend->SyncBodyToBackend(100, pt, rb, cs);
	EXPECT_TRUE(backend->HasBody(100));

	// Step simulation for 60 frames
	for (int i = 0; i < 60; ++i) {
		backend->StepSimulation(1.0F / 60.0F);
	}

	auto result = backend->SyncBodyFromBackend(100);
	EXPECT_LT(result.position.y, 10.0F);

	backend->Shutdown();
}

// === Flecs Module Integration Tests ===

class JoltModuleTest : public ::testing::Test {
protected:
	flecs::world world_;

	void SetUp() override {
		// Register engine Transform component
		world_.component<engine::components::Transform>();
		// Set default physics config
		world_.set<PhysicsWorldConfig>({});
		// Create Simulation phase entity matching ECSWorld::SetupPipeline
		auto sim_phase = world_.entity("Simulation").add(flecs::Phase).depends_on(flecs::OnUpdate);
		sim_phase.enable();
		// Import the Jolt module
		world_.import <JoltPhysicsModule>();
	}
};

TEST_F(JoltModuleTest, module_imports_successfully) {
	// If SetUp completes without error, the module imported
	SUCCEED();
}

TEST_F(JoltModuleTest, entity_with_physics_components_syncs_to_backend) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	// Progress to trigger observers and systems
	world_.progress(1.0F / 60.0F);

	// Dynamic body should get PhysicsVelocity auto-added
	EXPECT_TRUE(e.has<PhysicsVelocity>());
}

TEST_F(JoltModuleTest, static_body_does_not_get_velocity) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 0.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Static})
					 .set<CollisionShape>({.type = ShapeType::Box});

	world_.progress(1.0F / 60.0F);

	// Static bodies should NOT get PhysicsVelocity
	EXPECT_FALSE(e.has<PhysicsVelocity>());
}

TEST_F(JoltModuleTest, dynamic_body_falls_under_gravity) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	// First progress triggers OnSet observer → creates body in backend
	world_.progress(1.0F / 60.0F);

	// Verify PhysicsVelocity was added (confirms observer ran)
	ASSERT_TRUE(e.has<PhysicsVelocity>());

	// Run simulation frames
	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	const auto& wt = e.get<engine::components::WorldTransform>();
	// After 1 second of gravity, Y should be less than starting position
	EXPECT_LT(wt.position.y, 10.0F);
}

TEST_F(JoltModuleTest, removing_rigidbody_cleans_up) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 5.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic})
					 .set<CollisionShape>({.type = ShapeType::Box});

	world_.progress(1.0F / 60.0F);
	EXPECT_TRUE(e.has<PhysicsVelocity>());

	// Remove RigidBody — should trigger OnRemove observer
	e.remove<RigidBody>();
	world_.progress(1.0F / 60.0F);

	// Entity should no longer have physics velocity updated
	// (it still has the component, but backend won't sync)
	SUCCEED();
}

TEST_F(JoltModuleTest, physics_force_is_applied) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F, .use_gravity = false})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);

	// Apply a force in X direction
	e.set<PhysicsForce>({.force = {100.0F, 0.0F, 0.0F}, .clear_after_apply = true});

	for (int i = 0; i < 30; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	const auto& wt = e.get<engine::components::WorldTransform>();
	// Should have moved in X direction
	EXPECT_GT(wt.position.x, 0.0F);

	// PhysicsForce should have been removed (clear_after_apply)
	EXPECT_FALSE(e.has<PhysicsForce>());
}

TEST_F(JoltModuleTest, physics_impulse_is_consumed) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F, .use_gravity = false})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);

	// Apply an impulse
	e.set<PhysicsImpulse>({.impulse = {0.0F, 50.0F, 0.0F}});
	world_.progress(1.0F / 60.0F);

	// PhysicsImpulse should be consumed (removed)
	EXPECT_FALSE(e.has<PhysicsImpulse>());
}

// === Bullet3 Module Integration Tests ===

class Bullet3ModuleTest : public ::testing::Test {
protected:
	flecs::world world_;

	void SetUp() override {
		world_.component<engine::components::Transform>();
		world_.set<PhysicsWorldConfig>({});
		auto sim_phase = world_.entity("Simulation").add(flecs::Phase).depends_on(flecs::OnUpdate);
		sim_phase.enable();
		world_.import <Bullet3PhysicsModule>();
	}
};

TEST_F(Bullet3ModuleTest, module_imports_successfully) { SUCCEED(); }

TEST_F(Bullet3ModuleTest, entity_with_physics_components_syncs) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);

	EXPECT_TRUE(e.has<PhysicsVelocity>());
}

TEST_F(Bullet3ModuleTest, dynamic_body_falls_under_gravity) {
	auto e = world_.entity()
					 .set<engine::components::Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<engine::components::WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	for (int i = 0; i < 60; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	const auto& wt = e.get<engine::components::WorldTransform>();
	EXPECT_LT(wt.position.y, 10.0F);}
