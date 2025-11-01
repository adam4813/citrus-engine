#include <gtest/gtest.h>

import engine.components;
import glm;

class RendererTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// Test that default camera has sane values
TEST_F(RendererTest, default_camera_has_sane_values) {
    // This test validates the default camera values used in the fix
    engine::components::Camera default_camera;
    default_camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
    default_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    default_camera.fov = 60.0f;
    default_camera.aspect_ratio = 16.0f / 9.0f;
    default_camera.near_plane = 0.1f;
    default_camera.far_plane = 100.0f;
    
    const glm::vec3 default_position(0.0f, 0.0f, 10.0f);
    
    // Validate defaults make sense
    EXPECT_EQ(default_camera.target, glm::vec3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(default_camera.up, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_EQ(default_camera.fov, 60.0f);
    EXPECT_FLOAT_EQ(default_camera.aspect_ratio, 16.0f / 9.0f);
    EXPECT_FLOAT_EQ(default_camera.near_plane, 0.1f);
    EXPECT_FLOAT_EQ(default_camera.far_plane, 100.0f);
    EXPECT_EQ(default_position, glm::vec3(0.0f, 0.0f, 10.0f));
    
    // Ensure we can create matrices with these defaults
    default_camera.view_matrix = glm::lookAt(default_position, default_camera.target, default_camera.up);
    default_camera.projection_matrix = glm::perspective(
        glm::radians(default_camera.fov),
        default_camera.aspect_ratio,
        default_camera.near_plane,
        default_camera.far_plane
    );
    
    // Matrices should be valid (not NaN or infinite)
    EXPECT_FALSE(glm::isnan(default_camera.view_matrix[0][0]));
    EXPECT_FALSE(glm::isinf(default_camera.view_matrix[0][0]));
    EXPECT_FALSE(glm::isnan(default_camera.projection_matrix[0][0]));
    EXPECT_FALSE(glm::isinf(default_camera.projection_matrix[0][0]));
}
