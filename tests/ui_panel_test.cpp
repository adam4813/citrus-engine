#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Panel Tests
// ============================================================================

class PanelTest : public ::testing::Test {
protected:
    void SetUp() override {
        panel = std::make_unique<Panel>(100, 100, 300, 200);
    }

    void TearDown() override {
        panel.reset();
    }

    std::unique_ptr<Panel> panel;
};

TEST_F(PanelTest, Constructor_SetsInitialBounds) {
    EXPECT_EQ(panel->GetRelativeBounds().x, 100);
    EXPECT_EQ(panel->GetRelativeBounds().y, 100);
    EXPECT_EQ(panel->GetWidth(), 300);
    EXPECT_EQ(panel->GetHeight(), 200);
}

TEST_F(PanelTest, Constructor_SetsDefaultColors) {
    // Panel should have default colors set
    EXPECT_EQ(panel->GetBackgroundColor().r, Colors::DARK_GRAY.r);
    EXPECT_EQ(panel->GetBackgroundColor().g, Colors::DARK_GRAY.g);
    EXPECT_EQ(panel->GetBackgroundColor().b, Colors::DARK_GRAY.b);
}

TEST_F(PanelTest, SetBackgroundColor_UpdatesColor) {
    panel->SetBackgroundColor(Colors::GOLD);
    
    EXPECT_EQ(panel->GetBackgroundColor().r, Colors::GOLD.r);
    EXPECT_EQ(panel->GetBackgroundColor().g, Colors::GOLD.g);
    EXPECT_EQ(panel->GetBackgroundColor().b, Colors::GOLD.b);
}

TEST_F(PanelTest, SetBorderWidth_UpdatesWidth) {
    panel->SetBorderWidth(5.0f);
    EXPECT_EQ(panel->GetBorderWidth(), 5.0f);
}

TEST_F(PanelTest, SetBorderWidth_NegativeClampedToZero) {
    panel->SetBorderWidth(-5.0f);
    EXPECT_EQ(panel->GetBorderWidth(), 0.0f);
}

TEST_F(PanelTest, SetOpacity_ClampsToRange) {
    panel->SetOpacity(1.5f);
    EXPECT_EQ(panel->GetOpacity(), 1.0f);
    
    panel->SetOpacity(-0.5f);
    EXPECT_EQ(panel->GetOpacity(), 0.0f);
    
    panel->SetOpacity(0.5f);
    EXPECT_EQ(panel->GetOpacity(), 0.5f);
}

TEST_F(PanelTest, SetPadding_UpdatesPadding) {
    panel->SetPadding(15.0f);
    EXPECT_EQ(panel->GetPadding(), 15.0f);
}

TEST_F(PanelTest, SetPadding_NegativeClampedToZero) {
    panel->SetPadding(-10.0f);
    EXPECT_EQ(panel->GetPadding(), 0.0f);
}

TEST_F(PanelTest, SetClipChildren_UpdatesClipping) {
    panel->SetClipChildren(false);
    EXPECT_FALSE(panel->GetClipChildren());
    
    panel->SetClipChildren(true);
    EXPECT_TRUE(panel->GetClipChildren());
}

TEST_F(PanelTest, AddChild_ChildBecomesPartOfPanel) {
    auto child_panel = std::make_unique<Panel>(10, 10, 50, 50);
    Panel* child_ptr = child_panel.get();
    
    panel->AddChild(std::move(child_panel));
    
    EXPECT_EQ(panel->GetChildren().size(), 1);
    EXPECT_EQ(child_ptr->GetParent(), panel.get());
}

TEST_F(PanelTest, RemoveChild_RemovesChildFromPanel) {
    auto child_panel = std::make_unique<Panel>(10, 10, 50, 50);
    Panel* child_ptr = child_panel.get();
    
    panel->AddChild(std::move(child_panel));
    EXPECT_EQ(panel->GetChildren().size(), 1);
    
    panel->RemoveChild(child_ptr);
    EXPECT_EQ(panel->GetChildren().size(), 0);
}

TEST_F(PanelTest, IsVisible_DefaultsToTrue) {
    EXPECT_TRUE(panel->IsVisible());
}

TEST_F(PanelTest, SetVisible_UpdatesVisibility) {
    panel->SetVisible(false);
    EXPECT_FALSE(panel->IsVisible());
    
    panel->SetVisible(true);
    EXPECT_TRUE(panel->IsVisible());
}
