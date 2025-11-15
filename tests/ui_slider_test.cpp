#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Slider Tests
// ============================================================================

class SliderTest : public ::testing::Test {
protected:
    void SetUp() override {
        slider = std::make_unique<Slider>(10, 10, 200, 30, 0.0f, 100.0f, 50.0f);
    }

    void TearDown() override {
        slider.reset();
    }

    std::unique_ptr<Slider> slider;
};

TEST_F(SliderTest, Constructor_SetsInitialValue) {
    EXPECT_EQ(slider->GetValue(), 50.0f);
}

TEST_F(SliderTest, Constructor_SetsMinMax) {
    EXPECT_EQ(slider->GetMinValue(), 0.0f);
    EXPECT_EQ(slider->GetMaxValue(), 100.0f);
}

TEST_F(SliderTest, Constructor_ClampsToInitialValue) {
    // Create slider with initial value outside range
    auto slider2 = std::make_unique<Slider>(10, 10, 200, 30, 0.0f, 100.0f, 150.0f);
    EXPECT_EQ(slider2->GetValue(), 100.0f);  // Clamped to max
    
    auto slider3 = std::make_unique<Slider>(10, 10, 200, 30, 0.0f, 100.0f, -10.0f);
    EXPECT_EQ(slider3->GetValue(), 0.0f);  // Clamped to min
}

TEST_F(SliderTest, SetValue_UpdatesValue) {
    slider->SetValue(75.0f);
    EXPECT_EQ(slider->GetValue(), 75.0f);
}

TEST_F(SliderTest, SetValue_ClampsToRange) {
    slider->SetValue(150.0f);
    EXPECT_EQ(slider->GetValue(), 100.0f);  // Clamped to max
    
    slider->SetValue(-10.0f);
    EXPECT_EQ(slider->GetValue(), 0.0f);  // Clamped to min
}

TEST_F(SliderTest, SetValue_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    slider->SetValueChangedCallback([&](float value) {
        callback_triggered = true;
    });
    
    slider->SetValue(75.0f);
    
    EXPECT_FALSE(callback_triggered);  // Programmatic change doesn't trigger callback
}

TEST_F(SliderTest, SetMinValue_UpdatesMin) {
    slider->SetMinValue(10.0f);
    EXPECT_EQ(slider->GetMinValue(), 10.0f);
}

TEST_F(SliderTest, SetMinValue_ClampsCurrentValue) {
    slider->SetValue(25.0f);
    slider->SetMinValue(50.0f);
    
    EXPECT_EQ(slider->GetValue(), 50.0f);  // Value clamped to new min
}

TEST_F(SliderTest, SetMaxValue_UpdatesMax) {
    slider->SetMaxValue(200.0f);
    EXPECT_EQ(slider->GetMaxValue(), 200.0f);
}

TEST_F(SliderTest, SetMaxValue_ClampsCurrentValue) {
    slider->SetValue(75.0f);
    slider->SetMaxValue(50.0f);
    
    EXPECT_EQ(slider->GetValue(), 50.0f);  // Value clamped to new max
}

TEST_F(SliderTest, SetLabel_UpdatesLabel) {
    slider->SetLabel("Volume");
    EXPECT_EQ(slider->GetLabel(), "Volume");
}

TEST_F(SliderTest, SetShowValue_UpdatesShowValue) {
    slider->SetShowValue(true);
    EXPECT_TRUE(slider->GetShowValue());
    
    slider->SetShowValue(false);
    EXPECT_FALSE(slider->GetShowValue());
}

TEST_F(SliderTest, OnClick_LeftPressed_TriggersCallback) {
    bool callback_triggered = false;
    float callback_value = 0.0f;
    
    slider->SetValueChangedCallback([&](float value) {
        callback_triggered = true;
        callback_value = value;
    });
    
    // Click at middle of slider (should be ~50% of range)
    MouseEvent event{110, 25, false, false, true, true, false, false, false, false, false, false, 0.0f};
    slider->OnClick(event);
    
    EXPECT_TRUE(callback_triggered);
    // Value should be somewhere in the middle of range (allowing for rounding)
    EXPECT_GT(callback_value, 40.0f);
    EXPECT_LT(callback_value, 60.0f);
}

TEST_F(SliderTest, OnClick_OutsideBounds_DoesNotTriggerCallback) {
    bool callback_triggered = false;
    
    slider->SetValueChangedCallback([&](float value) {
        callback_triggered = true;
    });
    
    // Click outside slider bounds
    MouseEvent event{500, 500, false, false, true, true, false, false, false, false, false, false, 0.0f};
    slider->OnClick(event);
    
    EXPECT_FALSE(callback_triggered);
}

TEST_F(SliderTest, OnDrag_UpdatesValue) {
    bool callback_triggered = false;
    float callback_value = 0.0f;
    
    slider->SetValueChangedCallback([&](float value) {
        callback_triggered = true;
        callback_value = value;
    });
    
    // Start dragging
    MouseEvent click_event{10, 25, false, false, true, true, false, false, false, false, false, false, 0.0f};
    slider->OnClick(click_event);
    
    callback_triggered = false;  // Reset flag
    
    // Drag to right side (should move value towards max)
    MouseEvent drag_event{200, 25, true, false, false, false, false, false, true, false, false, false, 0.0f};
    slider->OnDrag(drag_event);
    
    EXPECT_TRUE(callback_triggered);
    // Value should be near max
    EXPECT_GT(callback_value, 90.0f);
}

TEST_F(SliderTest, SetTrackColor_UpdatesColor) {
    slider->SetTrackColor(Colors::GRAY);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(SliderTest, SetFillColor_UpdatesColor) {
    slider->SetFillColor(Colors::BLUE);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}

TEST_F(SliderTest, SetThumbColor_UpdatesColor) {
    slider->SetThumbColor(Colors::WHITE);
    // Note: Can't directly test color retrieval as it's private
    // This test ensures the method doesn't crash
}
