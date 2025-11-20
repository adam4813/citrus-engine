/**
 * @file input_mouse_test.cpp
 * @brief Tests for mouse input functionality
 */

#include <gtest/gtest.h>

import engine.input;

using namespace engine::input;

/**
 * @brief Test mouse state structure initialization
 */
TEST(InputMouseTest, MouseStateInitialization) {
    MouseState state;
    
    // Position defaults to origin
    EXPECT_EQ(state.x, 0.0f);
    EXPECT_EQ(state.y, 0.0f);
    
    // All buttons default to not pressed
    EXPECT_FALSE(state.left_down);
    EXPECT_FALSE(state.right_down);
    EXPECT_FALSE(state.middle_down);
    EXPECT_FALSE(state.left_pressed);
    EXPECT_FALSE(state.right_pressed);
    EXPECT_FALSE(state.middle_pressed);
    EXPECT_FALSE(state.left_released);
    EXPECT_FALSE(state.right_released);
    EXPECT_FALSE(state.middle_released);
    
    // Scroll delta defaults to zero
    EXPECT_EQ(state.scroll_delta, 0.0f);
}

/**
 * @brief Test mouse button enum values
 */
TEST(InputMouseTest, MouseButtonEnum) {
    EXPECT_EQ(static_cast<uint8_t>(MouseButton::LEFT), 0);
    EXPECT_EQ(static_cast<uint8_t>(MouseButton::RIGHT), 1);
    EXPECT_EQ(static_cast<uint8_t>(MouseButton::MIDDLE), 2);
}

/**
 * @brief Test that input module exports mouse functions
 * This is a compile-time test - if it compiles, the API is exported correctly
 */
TEST(InputMouseTest, APIExportedCorrectly) {
    // These should compile without errors
    auto check_api = []() {
        [[maybe_unused]] auto state = Input::GetMouseState();
        [[maybe_unused]] auto x = Input::GetMouseX();
        [[maybe_unused]] auto y = Input::GetMouseY();
        [[maybe_unused]] auto left = Input::IsMouseButtonDown(MouseButton::LEFT);
        [[maybe_unused]] auto pressed = Input::IsMouseButtonPressed(MouseButton::LEFT);
        [[maybe_unused]] auto released = Input::IsMouseButtonReleased(MouseButton::LEFT);
    };
    
    SUCCEED() << "API exports verified at compile time";
}
