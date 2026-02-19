#include <flecs.h>
#include <gtest/gtest.h>

import engine.components;
import engine.physics;
import glm;

using namespace engine::physics;
using engine::components::Transform;
using engine::components::WorldTransform;

// Helper: create a WorldTransform from a position
static WorldTransform MakeWorldTransform(const glm::vec3& pos) {
	WorldTransform wt;
	wt.position = pos;
	wt.ComputeMatrix();
	return wt;
}

// ============================================================
// Jolt – nested physics body tests (Unity-style: physics owns world space)
// ============================================================

class JoltParentingTest : public ::testing::Test {
protected:
	flecs::world world_;

	void SetUp() override {
		world_.component<Transform>();
		world_.component<WorldTransform>();
		world_.set<PhysicsWorldConfig>({});
		auto sim_phase = world_.entity("Simulation").add(flecs::Phase).depends_on(flecs::OnUpdate);
		sim_phase.enable();
		world_.import <JoltPhysicsModule>();
	}
};

// 1. No parent (baseline) – sync writes world position to WorldTransform
TEST_F(JoltParentingTest, no_parent_baseline) {
	auto e = world_.entity()
					 .set<Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(e.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// Physics writes to WorldTransform, not Transform
	const auto& wt = e.get<WorldTransform>();
	EXPECT_LT(wt.position.y, 10.0F);

	// Transform stays at initial local value
	const auto& t = e.get<Transform>();
	EXPECT_NEAR(t.position.y, 10.0F, 0.01F);
}

// 2. Parent dynamic + child dynamic – physics owns child's WorldTransform
//    Transform stays local, WorldTransform has world-space physics results.
TEST_F(JoltParentingTest, parent_dynamic_child_dynamic) {
	auto parent = world_.entity("parent")
						  .set<Transform>({{5.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({5.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						  .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	// Child starts at local (0, 2, 0) relative to parent at world (5, 0, 0)
	auto child = world_.entity("child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 2.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({5.0F, 2.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F, .use_gravity = false})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 29; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform.position is world-space from physics
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 5.0F, 1.0F);

	// Transform stays local (not modified by physics)
	const auto& child_t = child.get<Transform>();
	EXPECT_NEAR(child_t.position.x, 0.0F, 0.01F);
	EXPECT_NEAR(child_t.position.y, 2.0F, 0.01F);
}

// 3. Parent static + child dynamic – child WorldTransform has world-space position
TEST_F(JoltParentingTest, parent_static_child_dynamic) {
	auto parent = world_.entity("static_parent")
						  .set<Transform>({{3.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({3.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Static})
						  .set<CollisionShape>({.type = ShapeType::Box});

	// Child starts at local (0, 10, 0) relative to parent at (3, 0, 0)
	auto child = world_.entity("dyn_child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 10.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({3.0F, 10.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform tracks the physics world position
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 3.0F, 0.5F);
	EXPECT_LT(child_wt.position.y, 10.0F);

	// Transform stays at initial local offset
	const auto& child_t = child.get<Transform>();
	EXPECT_NEAR(child_t.position.x, 0.0F, 0.01F);
	EXPECT_NEAR(child_t.position.y, 10.0F, 0.01F);
}

// 4. Deeply nested: grandparent → parent → child (child is dynamic, ancestors static)
TEST_F(JoltParentingTest, deeply_nested_grandparent_parent_child) {
	auto grandparent = world_.entity("grandparent")
							   .set<Transform>({{2.0F, 0.0F, 0.0F}})
							   .set<WorldTransform>(MakeWorldTransform({2.0F, 0.0F, 0.0F}))
							   .set<RigidBody>({.motion_type = MotionType::Static})
							   .set<CollisionShape>({.type = ShapeType::Box});

	auto parent = world_.entity("mid_parent")
						  .child_of(grandparent)
						  .set<Transform>({{3.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({5.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Static})
						  .set<CollisionShape>({.type = ShapeType::Box});

	// Dynamic child starts at local (0, 10, 0) under parent at world (5, 0, 0)
	auto child = world_.entity("deep_child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 10.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({5.0F, 10.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform tracks the physics world position
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 5.0F, 0.5F);
	EXPECT_LT(child_wt.position.y, 10.0F);

	// Transform stays at initial local offset
	const auto& child_t = child.get<Transform>();
	EXPECT_NEAR(child_t.position.x, 0.0F, 0.01F);
	EXPECT_NEAR(child_t.position.y, 10.0F, 0.01F);
}

// ============================================================
// Bullet3 – nested physics body tests (Unity-style)
// ============================================================

class Bullet3ParentingTest : public ::testing::Test {
protected:
	flecs::world world_;

	void SetUp() override {
		world_.component<Transform>();
		world_.component<WorldTransform>();
		world_.set<PhysicsWorldConfig>({});
		auto sim_phase = world_.entity("Simulation").add(flecs::Phase).depends_on(flecs::OnUpdate);
		sim_phase.enable();
		world_.import <Bullet3PhysicsModule>();
	}
};

// 1. No parent (baseline)
TEST_F(Bullet3ParentingTest, no_parent_baseline) {
	auto e = world_.entity()
					 .set<Transform>({{0.0F, 10.0F, 0.0F}})
					 .set<WorldTransform>({})
					 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
					 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(e.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// Physics writes to WorldTransform, not Transform
	const auto& wt = e.get<WorldTransform>();
	EXPECT_LT(wt.position.y, 10.0F);
}

// 2. Parent dynamic + child dynamic
TEST_F(Bullet3ParentingTest, parent_dynamic_child_dynamic) {
	auto parent = world_.entity("b3_parent")
						  .set<Transform>({{5.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({5.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						  .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	auto child = world_.entity("b3_child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 2.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({5.0F, 2.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F, .use_gravity = false})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 29; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform.position is world-space from physics
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 5.0F, 1.0F);

	// Transform stays local
	const auto& child_t = child.get<Transform>();
	EXPECT_NEAR(child_t.position.x, 0.0F, 0.01F);
	EXPECT_NEAR(child_t.position.y, 2.0F, 0.01F);
}

// 3. Parent static + child dynamic
TEST_F(Bullet3ParentingTest, parent_static_child_dynamic) {
	auto parent = world_.entity("b3_static_parent")
						  .set<Transform>({{3.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({3.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Static})
						  .set<CollisionShape>({.type = ShapeType::Box});

	auto child = world_.entity("b3_dyn_child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 10.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({3.0F, 10.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform tracks the physics world position
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 3.0F, 0.5F);
	EXPECT_LT(child_wt.position.y, 10.0F);
}

// 4. Deeply nested
TEST_F(Bullet3ParentingTest, deeply_nested_grandparent_parent_child) {
	auto grandparent = world_.entity("b3_grandparent")
							   .set<Transform>({{2.0F, 0.0F, 0.0F}})
							   .set<WorldTransform>(MakeWorldTransform({2.0F, 0.0F, 0.0F}))
							   .set<RigidBody>({.motion_type = MotionType::Static})
							   .set<CollisionShape>({.type = ShapeType::Box});

	auto parent = world_.entity("b3_mid_parent")
						  .child_of(grandparent)
						  .set<Transform>({{3.0F, 0.0F, 0.0F}})
						  .set<WorldTransform>(MakeWorldTransform({5.0F, 0.0F, 0.0F}))
						  .set<RigidBody>({.motion_type = MotionType::Static})
						  .set<CollisionShape>({.type = ShapeType::Box});

	auto child = world_.entity("b3_deep_child")
						 .child_of(parent)
						 .set<Transform>({{0.0F, 10.0F, 0.0F}})
						 .set<WorldTransform>(MakeWorldTransform({5.0F, 10.0F, 0.0F}))
						 .set<RigidBody>({.motion_type = MotionType::Dynamic, .mass = 1.0F})
						 .set<CollisionShape>({.type = ShapeType::Sphere, .sphere_radius = 0.5F});

	world_.progress(1.0F / 60.0F);
	ASSERT_TRUE(child.has<PhysicsVelocity>());

	for (int i = 0; i < 59; ++i) {
		world_.progress(1.0F / 60.0F);
	}

	// WorldTransform tracks the physics world position
	const auto& child_wt = child.get<WorldTransform>();
	EXPECT_NEAR(child_wt.position.x, 5.0F, 0.5F);
	EXPECT_LT(child_wt.position.y, 10.0F);
}
