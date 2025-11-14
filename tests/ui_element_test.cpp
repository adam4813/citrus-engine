#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;

// Test UIElement implementation for testing purposes
class TestUIElement : public UIElement {
public:
    TestUIElement(float x, float y, float width, float height)
        : UIElement(x, y, width, height) {}

    void Render() const override {
        // No-op for testing
    }
};

class UIElementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test elements
        parent = std::make_unique<TestUIElement>(100, 100, 200, 200);
        child1 = std::make_unique<TestUIElement>(10, 10, 50, 50);
        child2 = std::make_unique<TestUIElement>(70, 70, 50, 50);
    }

    void TearDown() override {
        parent.reset();
        child1.reset();
        child2.reset();
    }

    std::unique_ptr<TestUIElement> parent;
    std::unique_ptr<TestUIElement> child1;
    std::unique_ptr<TestUIElement> child2;
};

// === Tree Structure Tests ===

TEST_F(UIElementTest, AddChild_SetsParentPointer) {
    TestUIElement* child_ptr = child1.get();
    
    parent->AddChild(std::move(child1));
    
    EXPECT_EQ(child_ptr->GetParent(), parent.get());
}

TEST_F(UIElementTest, AddChild_AddsToChildrenVector) {
    parent->AddChild(std::move(child1));
    
    EXPECT_EQ(parent->GetChildren().size(), 1);
}

TEST_F(UIElementTest, AddChild_MultipleChildren) {
    TestUIElement* child1_ptr = child1.get();
    TestUIElement* child2_ptr = child2.get();
    
    parent->AddChild(std::move(child1));
    parent->AddChild(std::move(child2));
    
    EXPECT_EQ(parent->GetChildren().size(), 2);
    EXPECT_EQ(child1_ptr->GetParent(), parent.get());
    EXPECT_EQ(child2_ptr->GetParent(), parent.get());
}

TEST_F(UIElementTest, RemoveChild_RemovesFromChildren) {
    TestUIElement* child_ptr = child1.get();
    parent->AddChild(std::move(child1));
    
    EXPECT_EQ(parent->GetChildren().size(), 1);
    
    parent->RemoveChild(child_ptr);
    
    EXPECT_EQ(parent->GetChildren().size(), 0);
}

TEST_F(UIElementTest, RemoveChild_NonExistentChildDoesNotCrash) {
    auto orphan = std::make_unique<TestUIElement>(0, 0, 10, 10);
    TestUIElement* orphan_ptr = orphan.get();
    
    // Should not crash when removing child that doesn't exist
    EXPECT_NO_THROW(parent->RemoveChild(orphan_ptr));
}

// === Bounds Calculation Tests ===

TEST_F(UIElementTest, GetRelativeBounds_ReturnsCorrectBounds) {
    auto element = std::make_unique<TestUIElement>(10, 20, 100, 50);
    
    Rectangle bounds = element->GetRelativeBounds();
    
    EXPECT_FLOAT_EQ(bounds.x, 10.0f);
    EXPECT_FLOAT_EQ(bounds.y, 20.0f);
    EXPECT_FLOAT_EQ(bounds.width, 100.0f);
    EXPECT_FLOAT_EQ(bounds.height, 50.0f);
}

TEST_F(UIElementTest, GetAbsoluteBounds_NoParent_ReturnsSameAsRelative) {
    auto element = std::make_unique<TestUIElement>(50, 75, 100, 50);
    
    Rectangle abs_bounds = element->GetAbsoluteBounds();
    Rectangle rel_bounds = element->GetRelativeBounds();
    
    EXPECT_FLOAT_EQ(abs_bounds.x, rel_bounds.x);
    EXPECT_FLOAT_EQ(abs_bounds.y, rel_bounds.y);
    EXPECT_FLOAT_EQ(abs_bounds.width, rel_bounds.width);
    EXPECT_FLOAT_EQ(abs_bounds.height, rel_bounds.height);
}

TEST_F(UIElementTest, GetAbsoluteBounds_WithParent_AddsParentPosition) {
    // Parent at (100, 100), child at relative (10, 10)
    TestUIElement* child_ptr = child1.get();
    parent->AddChild(std::move(child1));
    
    Rectangle abs_bounds = child_ptr->GetAbsoluteBounds();
    
    // Absolute position should be parent + relative
    EXPECT_FLOAT_EQ(abs_bounds.x, 110.0f);  // 100 + 10
    EXPECT_FLOAT_EQ(abs_bounds.y, 110.0f);  // 100 + 10
    EXPECT_FLOAT_EQ(abs_bounds.width, 50.0f);
    EXPECT_FLOAT_EQ(abs_bounds.height, 50.0f);
}

TEST_F(UIElementTest, GetAbsoluteBounds_NestedHierarchy_AccumulatesPositions) {
    // Create 3-level hierarchy
    auto grandparent = std::make_unique<TestUIElement>(100, 100, 300, 300);
    auto parent_elem = std::make_unique<TestUIElement>(50, 50, 200, 200);
    auto child_elem = std::make_unique<TestUIElement>(20, 20, 50, 50);
    
    TestUIElement* child_ptr = child_elem.get();
    parent_elem->AddChild(std::move(child_elem));
    grandparent->AddChild(std::move(parent_elem));
    
    Rectangle abs_bounds = child_ptr->GetAbsoluteBounds();
    
    // Should accumulate: 100 + 50 + 20 = 170
    EXPECT_FLOAT_EQ(abs_bounds.x, 170.0f);
    EXPECT_FLOAT_EQ(abs_bounds.y, 170.0f);
}

TEST_F(UIElementTest, SetRelativePosition_UpdatesPosition) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    
    element->SetRelativePosition(50, 75);
    
    Rectangle bounds = element->GetRelativeBounds();
    EXPECT_FLOAT_EQ(bounds.x, 50.0f);
    EXPECT_FLOAT_EQ(bounds.y, 75.0f);
}

TEST_F(UIElementTest, SetSize_UpdatesSize) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    
    element->SetSize(200, 150);
    
    EXPECT_FLOAT_EQ(element->GetWidth(), 200.0f);
    EXPECT_FLOAT_EQ(element->GetHeight(), 150.0f);
}

// === Hit Testing Tests ===

TEST_F(UIElementTest, Contains_PointInside_ReturnsTrue) {
    auto element = std::make_unique<TestUIElement>(100, 100, 200, 150);
    
    EXPECT_TRUE(element->Contains(150, 125));  // Inside
    EXPECT_TRUE(element->Contains(100, 100));  // Top-left corner
    EXPECT_TRUE(element->Contains(299, 249));  // Near bottom-right corner (inside)
}

TEST_F(UIElementTest, Contains_PointOutside_ReturnsFalse) {
    auto element = std::make_unique<TestUIElement>(100, 100, 200, 150);
    
    EXPECT_FALSE(element->Contains(50, 50));    // Above-left
    EXPECT_FALSE(element->Contains(350, 125));  // Right
    EXPECT_FALSE(element->Contains(150, 300));  // Below
    EXPECT_FALSE(element->Contains(99, 100));   // Just outside left
    EXPECT_FALSE(element->Contains(301, 100));  // Just outside right
    EXPECT_FALSE(element->Contains(300, 125));  // On right edge (half-open interval)
    EXPECT_FALSE(element->Contains(150, 250));  // On bottom edge (half-open interval)
}

TEST_F(UIElementTest, Contains_WithParent_UsesAbsolutePosition) {
    // Parent at (100, 100), child at relative (10, 10), size (50, 50)
    TestUIElement* child_ptr = child1.get();
    parent->AddChild(std::move(child1));
    
    // Child's absolute bounds are (110, 110, 50, 50)
    EXPECT_TRUE(child_ptr->Contains(130, 130));   // Inside (absolute coords)
    EXPECT_FALSE(child_ptr->Contains(30, 30));    // Would be inside if using relative coords
}

// === State Management Tests ===

TEST_F(UIElementTest, SetFocused_UpdatesState) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    
    EXPECT_FALSE(element->IsFocused());
    
    element->SetFocused(true);
    EXPECT_TRUE(element->IsFocused());
    
    element->SetFocused(false);
    EXPECT_FALSE(element->IsFocused());
}

TEST_F(UIElementTest, SetHovered_UpdatesState) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    
    EXPECT_FALSE(element->IsHovered());
    
    element->SetHovered(true);
    EXPECT_TRUE(element->IsHovered());
    
    element->SetHovered(false);
    EXPECT_FALSE(element->IsHovered());
}

TEST_F(UIElementTest, SetVisible_UpdatesState) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    
    EXPECT_TRUE(element->IsVisible());  // Default is visible
    
    element->SetVisible(false);
    EXPECT_FALSE(element->IsVisible());
    
    element->SetVisible(true);
    EXPECT_TRUE(element->IsVisible());
}

// === Event Handler Tests (Stub Behavior) ===

TEST_F(UIElementTest, ProcessMouseEvent_ReturnsNotHandled) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    MouseEvent event{50, 50, true, false, false, false, 0.0f};
    
    // Stub implementation should return false (not handled)
    EXPECT_FALSE(element->ProcessMouseEvent(event));
}

TEST_F(UIElementTest, OnHover_ReturnsNotHandled) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    MouseEvent event{50, 50, false, false, false, false, 0.0f};
    
    EXPECT_FALSE(element->OnHover(event));
}

TEST_F(UIElementTest, OnClick_ReturnsNotHandled) {
    auto element = std::make_unique<TestUIElement>(0, 0, 100, 100);
    MouseEvent event{50, 50, false, false, true, false, 0.0f};
    
    EXPECT_FALSE(element->OnClick(event));
}
