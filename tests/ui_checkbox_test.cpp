#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Checkbox Tests
// ============================================================================

class CheckboxTest : public ::testing::Test {
protected:
    void SetUp() override {
        checkbox = std::make_unique<Checkbox>(10, 10, "Enable Sound");
    }

    void TearDown() override {
        checkbox.reset();
    }

    std::unique_ptr<Checkbox> checkbox;
};

TEST_F(CheckboxTest, Constructor_SetsLabel) {
    EXPECT_EQ(checkbox->GetLabel(), "Enable Sound");
}

TEST_F(CheckboxTest, Constructor_DefaultsToUnchecked) {
    EXPECT_FALSE(checkbox->IsChecked());
}

TEST_F(CheckboxTest, Constructor_WithInitialChecked_SetsChecked) {
    auto checkbox2 = std::make_unique<Checkbox>(10, 10, "Test", 16.0f, true);
    EXPECT_TRUE(checkbox2->IsChecked());
}

TEST_F(CheckboxTest, SetChecked_UpdatesState) {
    checkbox->SetChecked(true);
    EXPECT_TRUE(checkbox->IsChecked());
    
    checkbox->SetChecked(false);
    EXPECT_FALSE(checkbox->IsChecked());
}

TEST_F(CheckboxTest, SetChecked_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    checkbox->SetToggleCallback([&](bool checked) {
        callback_triggered = true;
    });
    
    checkbox->SetChecked(true);
    
    EXPECT_FALSE(callback_triggered);  // Programmatic change doesn't trigger callback
}

TEST_F(CheckboxTest, Toggle_FlipsState) {
    EXPECT_FALSE(checkbox->IsChecked());
    
    checkbox->Toggle();
    EXPECT_TRUE(checkbox->IsChecked());
    
    checkbox->Toggle();
    EXPECT_FALSE(checkbox->IsChecked());
}

TEST_F(CheckboxTest, Toggle_TriggersCallback) {
    bool callback_triggered = false;
    bool callback_value = false;
    
    checkbox->SetToggleCallback([&](bool checked) {
        callback_triggered = true;
        callback_value = checked;
    });
    
    checkbox->Toggle();
    
    EXPECT_TRUE(callback_triggered);
    EXPECT_TRUE(callback_value);
}

TEST_F(CheckboxTest, SetLabel_UpdatesLabel) {
    checkbox->SetLabel("New Label");
    EXPECT_EQ(checkbox->GetLabel(), "New Label");
}

TEST_F(CheckboxTest, SetLabel_EmptyString_RemovesLabel) {
    checkbox->SetLabel("");
    EXPECT_EQ(checkbox->GetLabel(), "");
}

TEST_F(CheckboxTest, OnClick_LeftPressed_TogglesCheckbox) {
    EXPECT_FALSE(checkbox->IsChecked());
    
    // Click within checkbox bounds
    MouseEvent event{15, 15, false, false, true, true, false, false, false, false, false, false, 0.0f};
    checkbox->OnClick(event);
    
    EXPECT_TRUE(checkbox->IsChecked());
}

TEST_F(CheckboxTest, OnClick_LeftPressed_TriggersCallback) {
    bool callback_triggered = false;
    bool callback_value = false;
    
    checkbox->SetToggleCallback([&](bool checked) {
        callback_triggered = true;
        callback_value = checked;
    });
    
    // Click within checkbox bounds
    MouseEvent event{15, 15, false, false, true, true, false, false, false, false, false, false, 0.0f};
    checkbox->OnClick(event);
    
    EXPECT_TRUE(callback_triggered);
    EXPECT_TRUE(callback_value);
}

TEST_F(CheckboxTest, OnClick_OutsideBounds_DoesNotToggle) {
    EXPECT_FALSE(checkbox->IsChecked());
    
    // Click outside checkbox bounds
    MouseEvent event{500, 500, false, false, true, true, false, false, false, false, false, false, 0.0f};
    checkbox->OnClick(event);
    
    EXPECT_FALSE(checkbox->IsChecked());
}

TEST_F(CheckboxTest, OnClick_OutsideBounds_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    
    checkbox->SetToggleCallback([&](bool checked) {
        callback_triggered = true;
    });
    
    // Click outside checkbox bounds
    MouseEvent event{500, 500, false, false, true, true, false, false, false, false, false, false, 0.0f};
    checkbox->OnClick(event);
    
    EXPECT_FALSE(callback_triggered);
}

TEST_F(CheckboxTest, SetBoxColor_UpdatesColor) {
    checkbox->SetBoxColor(Colors::GOLD);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(CheckboxTest, SetCheckmarkColor_UpdatesColor) {
    checkbox->SetCheckmarkColor(Colors::GREEN);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(CheckboxTest, SetLabelColor_UpdatesColor) {
    checkbox->SetLabelColor(Colors::WHITE);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(CheckboxTest, SetFocusColor_UpdatesColor) {
    checkbox->SetFocusColor(Colors::ORANGE);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(CheckboxTest, SetFocused_UpdatesFocusState) {
    checkbox->SetFocused(true);
    EXPECT_TRUE(checkbox->IsFocused());
    
    checkbox->SetFocused(false);
    EXPECT_FALSE(checkbox->IsFocused());
}

TEST_F(CheckboxTest, IsVisible_DefaultsToTrue) {
    EXPECT_TRUE(checkbox->IsVisible());
}

TEST_F(CheckboxTest, SetVisible_UpdatesVisibility) {
    checkbox->SetVisible(false);
    EXPECT_FALSE(checkbox->IsVisible());
    
    checkbox->SetVisible(true);
    EXPECT_TRUE(checkbox->IsVisible());
}
