#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Button Tests
// ============================================================================

class ButtonTest : public ::testing::Test {
protected:
    void SetUp() override {
        button = std::make_unique<Button>(10, 10, 120, 40, "Click Me");
    }

    void TearDown() override {
        button.reset();
    }

    std::unique_ptr<Button> button;
};

TEST_F(ButtonTest, Constructor_SetsInitialBounds) {
    EXPECT_EQ(button->GetRelativeBounds().x, 10);
    EXPECT_EQ(button->GetRelativeBounds().y, 10);
    EXPECT_EQ(button->GetWidth(), 120);
    EXPECT_EQ(button->GetHeight(), 40);
}

TEST_F(ButtonTest, Constructor_SetsLabel) {
    EXPECT_EQ(button->GetLabel(), "Click Me");
}

TEST_F(ButtonTest, SetLabel_UpdatesLabel) {
    button->SetLabel("New Label");
    EXPECT_EQ(button->GetLabel(), "New Label");
}

TEST_F(ButtonTest, SetFontSize_UpdatesSize) {
    button->SetFontSize(20.0f);
    EXPECT_EQ(button->GetFontSize(), 20.0f);
}

TEST_F(ButtonTest, IsEnabled_DefaultsToTrue) {
    EXPECT_TRUE(button->IsEnabled());
}

TEST_F(ButtonTest, SetEnabled_UpdatesState) {
    button->SetEnabled(false);
    EXPECT_FALSE(button->IsEnabled());
    
    button->SetEnabled(true);
    EXPECT_TRUE(button->IsEnabled());
}

TEST_F(ButtonTest, SetEnabled_False_ClearsPressedAndHovered) {
    // Simulate button being pressed
    button->SetHovered(true);
    // Note: IsPressed() requires mouse event handling, tested separately
    
    button->SetEnabled(false);
    
    EXPECT_FALSE(button->IsHovered());
}

TEST_F(ButtonTest, OnClick_WithinBounds_TriggersCallback) {
    bool callback_triggered = false;
    button->SetClickCallback([&](const MouseEvent& event) {
        callback_triggered = true;
        return true;
    });
    
    MouseEvent event{50, 25, false, false, true, false, false, false, false, false, false, false, 0.0f};
    button->OnClick(event);
    
    EXPECT_TRUE(callback_triggered);
}

TEST_F(ButtonTest, OnClick_OutsideBounds_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    button->SetClickCallback([&](const MouseEvent& event) {
        callback_triggered = true;
        return true;
    });
    
    // Click at (200, 200) which is outside button bounds
    MouseEvent event{200, 200, false, false, true, false, false, false, false, false, false, false, 0.0f};
    button->OnClick(event);
    
    EXPECT_FALSE(callback_triggered);
}

TEST_F(ButtonTest, OnClick_Disabled_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    button->SetClickCallback([&](const MouseEvent& event) {
        callback_triggered = true;
        return true;
    });
    
    button->SetEnabled(false);
    
    MouseEvent event{50, 25, false, false, true, false, false, false, false, false, false, false, 0.0f};
    button->OnClick(event);
    
    EXPECT_FALSE(callback_triggered);
}

TEST_F(ButtonTest, OnHover_UpdatesHoverState) {
    // Mouse enters button bounds
    MouseEvent hover_event{50, 25, false, false, false, false, false, false, false, false, false, false, 0.0f};
    button->OnHover(hover_event);
    
    EXPECT_TRUE(button->IsHovered());
    
    // Mouse exits button bounds
    MouseEvent exit_event{200, 200, false, false, false, false, false, false, false, false, false, false, 0.0f};
    button->OnHover(exit_event);
    
    EXPECT_FALSE(button->IsHovered());
}

TEST_F(ButtonTest, SetNormalColor_UpdatesColor) {
    button->SetNormalColor(Colors::GOLD);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetHoverColor_UpdatesColor) {
    button->SetHoverColor(Colors::ORANGE);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetPressedColor_UpdatesColor) {
    button->SetPressedColor(Colors::DARK_GRAY);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetDisabledColor_UpdatesColor) {
    button->SetDisabledColor(Colors::GRAY);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetTextColor_UpdatesColor) {
    button->SetTextColor(Colors::GOLD);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetBorderColor_UpdatesColor) {
    button->SetBorderColor(Colors::GOLD);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(ButtonTest, SetBorderWidth_UpdatesWidth) {
    button->SetBorderWidth(2.0f);
    // Note: Can't directly test width retrieval as it's private
    // This test ensures the method doesn't crash
}
