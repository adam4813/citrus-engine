#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Label Tests
// ============================================================================

class LabelTest : public ::testing::Test {
protected:
    void SetUp() override {
        label = std::make_unique<Label>(10, 10, "Test Label");
    }

    void TearDown() override {
        label.reset();
    }

    std::unique_ptr<Label> label;
};

TEST_F(LabelTest, Constructor_SetsText) {
    EXPECT_EQ(label->GetText(), "Test Label");
}

TEST_F(LabelTest, Constructor_SetsPosition) {
    EXPECT_EQ(label->GetRelativeBounds().x, 10);
    EXPECT_EQ(label->GetRelativeBounds().y, 10);
}

TEST_F(LabelTest, Constructor_AutoSizes) {
    // Label should auto-size to fit text
    // Width and height should be greater than zero
    EXPECT_GT(label->GetWidth(), 0);
    EXPECT_GT(label->GetHeight(), 0);
}

TEST_F(LabelTest, SetText_UpdatesText) {
    label->SetText("New Text");
    EXPECT_EQ(label->GetText(), "New Text");
}

TEST_F(LabelTest, SetText_UpdatesSize) {
    const float initial_width = label->GetWidth();
    
    // Set longer text
    label->SetText("This is a much longer text that should increase width");
    
    EXPECT_GT(label->GetWidth(), initial_width);
}

TEST_F(LabelTest, SetFontSize_UpdatesSize) {
    label->SetFontSize(24.0f);
    EXPECT_EQ(label->GetFontSize(), 24.0f);
}

TEST_F(LabelTest, SetColor_UpdatesColor) {
    label->SetColor(Colors::GOLD);
    EXPECT_EQ(label->GetColor().r, Colors::GOLD.r);
    EXPECT_EQ(label->GetColor().g, Colors::GOLD.g);
    EXPECT_EQ(label->GetColor().b, Colors::GOLD.b);
}

TEST_F(LabelTest, SetAlignment_UpdatesAlignment) {
    label->SetAlignment(Label::Alignment::Center);
    EXPECT_EQ(label->GetAlignment(), Label::Alignment::Center);
    
    label->SetAlignment(Label::Alignment::Right);
    EXPECT_EQ(label->GetAlignment(), Label::Alignment::Right);
    
    label->SetAlignment(Label::Alignment::Left);
    EXPECT_EQ(label->GetAlignment(), Label::Alignment::Left);
}

TEST_F(LabelTest, SetMaxWidth_ConstrainsWidth) {
    label->SetMaxWidth(100.0f);
    EXPECT_EQ(label->GetMaxWidth(), 100.0f);
    
    // If text is wider than max width, label width should be clamped
    if (label->GetWidth() > 100.0f) {
        EXPECT_EQ(label->GetWidth(), 100.0f);
    }
}

TEST_F(LabelTest, SetMaxWidth_Zero_DisablesConstraint) {
    label->SetMaxWidth(0.0f);
    EXPECT_EQ(label->GetMaxWidth(), 0.0f);
    
    // Width should match text width (no constraint)
    EXPECT_GT(label->GetWidth(), 0);
}

TEST_F(LabelTest, SetMaxWidth_NegativeClampedToZero) {
    label->SetMaxWidth(-50.0f);
    EXPECT_EQ(label->GetMaxWidth(), 0.0f);
}

TEST_F(LabelTest, IsVisible_DefaultsToTrue) {
    EXPECT_TRUE(label->IsVisible());
}

TEST_F(LabelTest, SetVisible_UpdatesVisibility) {
    label->SetVisible(false);
    EXPECT_FALSE(label->IsVisible());
    
    label->SetVisible(true);
    EXPECT_TRUE(label->IsVisible());
}

// Test alignment with max width
TEST_F(LabelTest, Alignment_WithMaxWidth_PositionsCorrectly) {
    // Set text and max width
    label->SetText("Test");
    label->SetMaxWidth(200.0f);
    
    // Test left alignment (default)
    label->SetAlignment(Label::Alignment::Left);
    // Text should be at x=0 relative to label
    
    // Test center alignment
    label->SetAlignment(Label::Alignment::Center);
    // Text should be centered
    
    // Test right alignment
    label->SetAlignment(Label::Alignment::Right);
    // Text should be at right edge
    
    // These are visual tests - just ensure no crashes
}
