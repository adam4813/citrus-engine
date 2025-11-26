#include <gtest/gtest.h>

import engine.physics;
import glm;

using namespace engine::physics;

class PhysicsSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Test basic initialization with JoltPhysics backend
TEST_F(PhysicsSystemTest, can_initialize_jolt_backend) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::JoltPhysics);
    EXPECT_EQ(physics.GetEngineName(), "JoltPhysics (stub)");
}

// Test initialization with Bullet3 backend
TEST_F(PhysicsSystemTest, can_initialize_bullet3_backend) {
    PhysicsSystem physics(PhysicsEngineType::Bullet3);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::Bullet3);
    EXPECT_EQ(physics.GetEngineName(), "Bullet3 (stub)");
}

// Test initialization with PhysX backend
TEST_F(PhysicsSystemTest, can_initialize_physx_backend) {
    PhysicsSystem physics(PhysicsEngineType::PhysX);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::PhysX);
    EXPECT_EQ(physics.GetEngineName(), "PhysX (stub)");
}

// Test gravity setting and getting
TEST_F(PhysicsSystemTest, can_set_and_get_gravity) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    glm::vec3 new_gravity{0.0F, -20.0F, 0.0F};
    physics.SetGravity(new_gravity);
    
    glm::vec3 retrieved_gravity = physics.GetGravity();
    EXPECT_FLOAT_EQ(retrieved_gravity.x, new_gravity.x);
    EXPECT_FLOAT_EQ(retrieved_gravity.y, new_gravity.y);
    EXPECT_FLOAT_EQ(retrieved_gravity.z, new_gravity.z);
}

// Test adding and removing rigid bodies
TEST_F(PhysicsSystemTest, can_add_and_remove_rigid_body) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    config.motion_type = MotionType::Dynamic;
    config.mass = 5.0F;
    
    EXPECT_FALSE(physics.HasRigidBody(entity));
    
    bool added = physics.AddRigidBody(entity, config);
    EXPECT_TRUE(added);
    EXPECT_TRUE(physics.HasRigidBody(entity));
    
    physics.RemoveRigidBody(entity);
    EXPECT_FALSE(physics.HasRigidBody(entity));
}

// Test setting and getting transform
TEST_F(PhysicsSystemTest, can_set_and_get_transform) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    physics.AddRigidBody(entity, config);
    
    glm::vec3 position{10.0F, 20.0F, 30.0F};
    glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
    
    physics.SetTransform(entity, position, rotation);
    
    glm::vec3 retrieved_pos = physics.GetPosition(entity);
    EXPECT_FLOAT_EQ(retrieved_pos.x, position.x);
    EXPECT_FLOAT_EQ(retrieved_pos.y, position.y);
    EXPECT_FLOAT_EQ(retrieved_pos.z, position.z);
}

// Test velocity management
TEST_F(PhysicsSystemTest, can_set_and_get_velocity) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    physics.AddRigidBody(entity, config);
    
    glm::vec3 velocity{5.0F, 10.0F, 15.0F};
    physics.SetLinearVelocity(entity, velocity);
    
    glm::vec3 retrieved_vel = physics.GetLinearVelocity(entity);
    EXPECT_FLOAT_EQ(retrieved_vel.x, velocity.x);
    EXPECT_FLOAT_EQ(retrieved_vel.y, velocity.y);
    EXPECT_FLOAT_EQ(retrieved_vel.z, velocity.z);
}

// Test adding and removing colliders
TEST_F(PhysicsSystemTest, can_add_and_remove_collider) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    ColliderConfig config{};
    config.shape.type = ShapeType::Box;
    config.shape.box_half_extents = glm::vec3{1.0F, 2.0F, 3.0F};
    
    EXPECT_FALSE(physics.HasCollider(entity));
    
    bool added = physics.AddCollider(entity, config);
    EXPECT_TRUE(added);
    EXPECT_TRUE(physics.HasCollider(entity));
    
    physics.RemoveCollider(entity);
    EXPECT_FALSE(physics.HasCollider(entity));
}

// Test CCD enable/disable
TEST_F(PhysicsSystemTest, can_enable_and_disable_ccd) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    config.enable_ccd = false;
    physics.AddRigidBody(entity, config);
    
    EXPECT_FALSE(physics.IsCCDEnabled(entity));
    
    physics.EnableCCD(entity, true);
    EXPECT_TRUE(physics.IsCCDEnabled(entity));
    
    physics.EnableCCD(entity, false);
    EXPECT_FALSE(physics.IsCCDEnabled(entity));
}

// Test character controller
TEST_F(PhysicsSystemTest, can_add_and_remove_character_controller) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    CharacterControllerConfig config{};
    config.height = 1.8F;
    config.radius = 0.3F;
    
    EXPECT_FALSE(physics.HasCharacterController(entity));
    
    bool added = physics.AddCharacterController(entity, config);
    EXPECT_TRUE(added);
    EXPECT_TRUE(physics.HasCharacterController(entity));
    
    physics.RemoveCharacterController(entity);
    EXPECT_FALSE(physics.HasCharacterController(entity));
}

// Test convenience method: CreateDynamicBox
TEST_F(PhysicsSystemTest, can_create_dynamic_box) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    glm::vec3 position{0.0F, 10.0F, 0.0F};
    glm::vec3 half_extents{1.0F, 1.0F, 1.0F};
    
    bool created = physics.CreateDynamicBox(entity, position, half_extents, 2.0F);
    EXPECT_TRUE(created);
    EXPECT_TRUE(physics.HasRigidBody(entity));
    EXPECT_TRUE(physics.HasCollider(entity));
    
    glm::vec3 retrieved_pos = physics.GetPosition(entity);
    EXPECT_FLOAT_EQ(retrieved_pos.x, position.x);
    EXPECT_FLOAT_EQ(retrieved_pos.y, position.y);
    EXPECT_FLOAT_EQ(retrieved_pos.z, position.z);
}

// Test convenience method: CreateStaticBox
TEST_F(PhysicsSystemTest, can_create_static_box) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    glm::vec3 position{0.0F, 0.0F, 0.0F};
    glm::vec3 half_extents{10.0F, 0.5F, 10.0F};
    
    bool created = physics.CreateStaticBox(entity, position, half_extents);
    EXPECT_TRUE(created);
    EXPECT_TRUE(physics.HasRigidBody(entity));
    EXPECT_TRUE(physics.HasCollider(entity));
}

// Test convenience method: CreateDynamicSphere
TEST_F(PhysicsSystemTest, can_create_dynamic_sphere) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    glm::vec3 position{0.0F, 5.0F, 0.0F};
    float radius = 0.5F;
    
    bool created = physics.CreateDynamicSphere(entity, position, radius, 1.0F);
    EXPECT_TRUE(created);
    EXPECT_TRUE(physics.HasRigidBody(entity));
    EXPECT_TRUE(physics.HasCollider(entity));
}

// Test RemovePhysics removes all physics components
TEST_F(PhysicsSystemTest, can_remove_all_physics_from_entity) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    
    physics.CreateDynamicBox(entity, glm::vec3{0.0F}, glm::vec3{1.0F});
    physics.AddCharacterController(entity);
    
    EXPECT_TRUE(physics.HasRigidBody(entity));
    EXPECT_TRUE(physics.HasCollider(entity));
    EXPECT_TRUE(physics.HasCharacterController(entity));
    
    physics.RemovePhysics(entity);
    
    EXPECT_FALSE(physics.HasRigidBody(entity));
    EXPECT_FALSE(physics.HasCollider(entity));
    EXPECT_FALSE(physics.HasCharacterController(entity));
}

// Test feature support query
TEST_F(PhysicsSystemTest, can_query_feature_support) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    // JoltPhysics supports these
    EXPECT_TRUE(physics.SupportsFeature("ccd"));
    EXPECT_TRUE(physics.SupportsFeature("convex_hull"));
    EXPECT_TRUE(physics.SupportsFeature("character_controller"));
    EXPECT_TRUE(physics.SupportsFeature("multithreading"));
    
    // JoltPhysics does not support these
    EXPECT_FALSE(physics.SupportsFeature("ccd_concave_mesh"));
    EXPECT_FALSE(physics.SupportsFeature("gpu_acceleration"));
}

// Test switching between physics engines at runtime
TEST_F(PhysicsSystemTest, can_switch_physics_engine) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::JoltPhysics);
    
    // Switch to Bullet3
    bool switched = physics.SetEngine(PhysicsEngineType::Bullet3);
    EXPECT_TRUE(switched);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::Bullet3);
    
    // Switch to PhysX
    switched = physics.SetEngine(PhysicsEngineType::PhysX);
    EXPECT_TRUE(switched);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::PhysX);
    
    // Switch back to JoltPhysics
    switched = physics.SetEngine(PhysicsEngineType::JoltPhysics);
    EXPECT_TRUE(switched);
    EXPECT_EQ(physics.GetEngineType(), PhysicsEngineType::JoltPhysics);
}

// Test that switching to same engine is a no-op
TEST_F(PhysicsSystemTest, switching_to_same_engine_is_noop) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    // Add a rigid body
    EntityId entity = 1;
    physics.AddRigidBody(entity, RigidBodyConfig{});
    EXPECT_TRUE(physics.HasRigidBody(entity));
    
    // Switch to same engine - should preserve state
    bool switched = physics.SetEngine(PhysicsEngineType::JoltPhysics);
    EXPECT_TRUE(switched);
    
    // Body should still exist (no reinitialization occurred)
    EXPECT_TRUE(physics.HasRigidBody(entity));
}

// Test simulation step
TEST_F(PhysicsSystemTest, can_step_simulation) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    config.motion_type = MotionType::Dynamic;
    config.use_gravity = true;
    physics.AddRigidBody(entity, config);
    
    glm::vec3 initial_pos{0.0F, 10.0F, 0.0F};
    physics.SetTransform(entity, initial_pos);
    
    // Step simulation
    physics.Update(1.0F / 60.0F);
    
    // Position should have changed due to gravity (in stub implementation)
    glm::vec3 new_pos = physics.GetPosition(entity);
    // In the stub implementation, gravity is applied each step
    EXPECT_NE(new_pos.y, initial_pos.y);
}

// Test applying impulse
TEST_F(PhysicsSystemTest, can_apply_impulse) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity = 1;
    RigidBodyConfig config{};
    config.motion_type = MotionType::Dynamic;
    config.mass = 1.0F;
    config.use_gravity = false; // Disable gravity for cleaner test
    physics.AddRigidBody(entity, config);
    
    glm::vec3 initial_velocity = physics.GetLinearVelocity(entity);
    EXPECT_FLOAT_EQ(initial_velocity.x, 0.0F);
    EXPECT_FLOAT_EQ(initial_velocity.y, 0.0F);
    EXPECT_FLOAT_EQ(initial_velocity.z, 0.0F);
    
    // Apply impulse
    glm::vec3 impulse{10.0F, 0.0F, 0.0F};
    physics.ApplyImpulse(entity, impulse);
    
    // Velocity should have changed
    glm::vec3 new_velocity = physics.GetLinearVelocity(entity);
    EXPECT_GT(new_velocity.x, 0.0F);
}

// Test PhysicsTransform matrix conversion
TEST_F(PhysicsSystemTest, physics_transform_matrix_conversion) {
    PhysicsTransform transform;
    transform.position = glm::vec3{1.0F, 2.0F, 3.0F};
    transform.rotation = glm::quat{1.0F, 0.0F, 0.0F, 0.0F}; // Identity rotation
    
    glm::mat4 matrix = transform.GetMatrix();
    
    // Check position in matrix
    EXPECT_FLOAT_EQ(matrix[3][0], 1.0F);
    EXPECT_FLOAT_EQ(matrix[3][1], 2.0F);
    EXPECT_FLOAT_EQ(matrix[3][2], 3.0F);
    EXPECT_FLOAT_EQ(matrix[3][3], 1.0F);
    
    // Convert back
    PhysicsTransform converted = PhysicsTransform::FromMatrix(matrix);
    EXPECT_FLOAT_EQ(converted.position.x, 1.0F);
    EXPECT_FLOAT_EQ(converted.position.y, 2.0F);
    EXPECT_FLOAT_EQ(converted.position.z, 3.0F);
}

// Test collision info validity check
TEST_F(PhysicsSystemTest, collision_info_validity) {
    CollisionInfo info{};
    info.entity_a = 0;
    info.entity_b = 0;
    EXPECT_FALSE(info.IsValid());
    
    info.entity_a = 1;
    info.entity_b = 2;
    EXPECT_TRUE(info.IsValid());
}

// Test raycast result validity check
TEST_F(PhysicsSystemTest, raycast_result_validity) {
    RaycastResult result{};
    result.entity = 0;
    EXPECT_FALSE(result.HasHit());
    
    result.entity = 1;
    EXPECT_TRUE(result.HasHit());
}

// Test constraints
TEST_F(PhysicsSystemTest, can_add_and_remove_constraints) {
    PhysicsSystem physics(PhysicsEngineType::JoltPhysics);
    
    EntityId entity_a = 1;
    EntityId entity_b = 2;
    
    physics.AddRigidBody(entity_a);
    physics.AddRigidBody(entity_b);
    
    ConstraintConfig config{};
    config.type = ConstraintType::Fixed;
    
    bool added = physics.AddConstraint(entity_a, entity_b, config);
    EXPECT_TRUE(added);
    
    // Remove constraint (no assertion needed, just ensure it doesn't crash)
    physics.RemoveConstraint(entity_a, entity_b);
}
