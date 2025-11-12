#include <gtest/gtest.h>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;

// Test fixture for UIElement and mouse events
class UIMouseEventTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// ============================================================================
// MouseEvent Structure Tests
// ============================================================================

TEST_F(UIMouseEventTest, MouseEventDefaultConstructor) {
    MouseEvent event;
    
    EXPECT_FLOAT_EQ(event.x, 0.0f);
    EXPECT_FLOAT_EQ(event.y, 0.0f);
    EXPECT_FALSE(event.left_down);
    EXPECT_FALSE(event.right_down);
    EXPECT_FALSE(event.left_pressed);
    EXPECT_FALSE(event.right_pressed);
    EXPECT_FLOAT_EQ(event.scroll_delta, 0.0f);
}

TEST_F(UIMouseEventTest, MouseEventParameterizedConstructor) {
    MouseEvent event{100.0f, 200.0f, true, false, false, true, 5.0f};
    
    EXPECT_FLOAT_EQ(event.x, 100.0f);
    EXPECT_FLOAT_EQ(event.y, 200.0f);
    EXPECT_TRUE(event.left_down);
    EXPECT_FALSE(event.right_down);
    EXPECT_FALSE(event.left_pressed);
    EXPECT_TRUE(event.right_pressed);
    EXPECT_FLOAT_EQ(event.scroll_delta, 5.0f);
}

// ============================================================================
// UIElement Hit Testing Tests
// ============================================================================

class TestUIElement : public UIElement {
public:
    TestUIElement(float x, float y, float w, float h) 
        : UIElement(x, y, w, h) {}
    
    void Render() const override {}
    
    // Expose event handlers for testing
    using UIElement::OnHover;
    using UIElement::OnClick;
    using UIElement::OnDrag;
    using UIElement::OnScroll;
};

TEST_F(UIMouseEventTest, UIElementContainsPoint) {
    TestUIElement element{100.0f, 100.0f, 200.0f, 100.0f};
    
    // Inside bounds
    EXPECT_TRUE(element.Contains(150.0f, 150.0f));
    EXPECT_TRUE(element.Contains(100.0f, 100.0f));  // Top-left corner
    EXPECT_TRUE(element.Contains(300.0f, 200.0f));  // Bottom-right corner
    
    // Outside bounds
    EXPECT_FALSE(element.Contains(50.0f, 150.0f));   // Left
    EXPECT_FALSE(element.Contains(350.0f, 150.0f));  // Right
    EXPECT_FALSE(element.Contains(150.0f, 50.0f));   // Above
    EXPECT_FALSE(element.Contains(150.0f, 250.0f));  // Below
}

TEST_F(UIMouseEventTest, UIElementAbsoluteBoundsWithParent) {
    TestUIElement parent{100.0f, 100.0f, 400.0f, 300.0f};
    auto child = std::make_unique<TestUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    TestUIElement* child_ptr = child.get();
    
    parent.AddChild(std::move(child));
    
    Rectangle child_bounds = child_ptr->GetAbsoluteBounds();
    EXPECT_FLOAT_EQ(child_bounds.x, 150.0f);  // 100 + 50
    EXPECT_FLOAT_EQ(child_bounds.y, 150.0f);  // 100 + 50
    EXPECT_FLOAT_EQ(child_bounds.width, 100.0f);
    EXPECT_FLOAT_EQ(child_bounds.height, 80.0f);
}

TEST_F(UIMouseEventTest, UIElementAbsoluteBoundsNestedHierarchy) {
    TestUIElement root{0.0f, 0.0f, 800.0f, 600.0f};
    auto panel = std::make_unique<TestUIElement>(100.0f, 100.0f, 400.0f, 300.0f);
    TestUIElement* panel_ptr = panel.get();
    
    root.AddChild(std::move(panel));
    
    auto button = std::make_unique<TestUIElement>(50.0f, 50.0f, 100.0f, 40.0f);
    TestUIElement* button_ptr = button.get();
    
    panel_ptr->AddChild(std::move(button));
    
    Rectangle button_bounds = button_ptr->GetAbsoluteBounds();
    EXPECT_FLOAT_EQ(button_bounds.x, 150.0f);  // 0 + 100 + 50
    EXPECT_FLOAT_EQ(button_bounds.y, 150.0f);  // 0 + 100 + 50
}

// ============================================================================
// Event Propagation Tests (Bubble-Down)
// ============================================================================

class CountingUIElement : public UIElement {
public:
    CountingUIElement(float x, float y, float w, float h) 
        : UIElement(x, y, w, h) {}
    
    void Render() const override {}
    
    bool OnHover(const MouseEvent& event) override {
        hover_count_++;
        return consume_hover_;
    }
    
    bool OnClick(const MouseEvent& event) override {
        click_count_++;
        return consume_click_;
    }
    
    mutable int hover_count_{0};
    mutable int click_count_{0};
    bool consume_hover_{false};
    bool consume_click_{false};
};

TEST_F(UIMouseEventTest, EventPropagationBubbleDown) {
    auto parent = std::make_unique<CountingUIElement>(0.0f, 0.0f, 400.0f, 300.0f);
    auto child = std::make_unique<CountingUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    
    CountingUIElement* parent_ptr = parent.get();
    CountingUIElement* child_ptr = child.get();
    
    // Child consumes event
    child_ptr->consume_click_ = true;
    
    parent_ptr->AddChild(std::move(child));
    
    // Event at child position
    MouseEvent event{60.0f, 60.0f, false, false, true, false, 0.0f};
    
    bool handled = parent_ptr->ProcessMouseEvent(event);
    
    // Child should handle first (bubble-down)
    EXPECT_EQ(child_ptr->click_count_, 1);
    EXPECT_EQ(parent_ptr->click_count_, 0);  // Parent not called because child consumed
    EXPECT_TRUE(handled);
}

TEST_F(UIMouseEventTest, EventPropagationNotConsumed) {
    auto parent = std::make_unique<CountingUIElement>(0.0f, 0.0f, 400.0f, 300.0f);
    auto child = std::make_unique<CountingUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    
    CountingUIElement* parent_ptr = parent.get();
    CountingUIElement* child_ptr = child.get();
    
    // Child won't consume event
    child_ptr->consume_click_ = false;
    
    parent_ptr->AddChild(std::move(child));
    
    // Event at parent position (not on child)
    MouseEvent event{200.0f, 150.0f, false, false, true, false, 0.0f};
    
    bool handled = parent_ptr->ProcessMouseEvent(event);
    
    // Only parent should handle
    EXPECT_EQ(child_ptr->click_count_, 0);  // Child not in bounds
    EXPECT_EQ(parent_ptr->click_count_, 1);
}

TEST_F(UIMouseEventTest, EventPropagationStopsWhenConsumed) {
    auto parent = std::make_unique<CountingUIElement>(0.0f, 0.0f, 400.0f, 300.0f);
    auto child = std::make_unique<CountingUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    
    CountingUIElement* parent_ptr = parent.get();
    CountingUIElement* child_ptr = child.get();
    
    // Child consumes event
    child_ptr->consume_click_ = true;
    
    parent_ptr->AddChild(std::move(child));
    
    // Event at child position
    MouseEvent event{60.0f, 60.0f, false, false, true, false, 0.0f};
    
    bool handled = parent_ptr->ProcessMouseEvent(event);
    
    // Child consumes, parent never called
    EXPECT_EQ(child_ptr->click_count_, 1);
    EXPECT_EQ(parent_ptr->click_count_, 0);
    EXPECT_TRUE(handled);
}

TEST_F(UIMouseEventTest, EventPropagationReverseOrder) {
    auto parent = std::make_unique<CountingUIElement>(0.0f, 0.0f, 400.0f, 300.0f);
    auto child1 = std::make_unique<CountingUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    auto child2 = std::make_unique<CountingUIElement>(50.0f, 50.0f, 100.0f, 80.0f);
    
    CountingUIElement* parent_ptr = parent.get();
    CountingUIElement* child1_ptr = child1.get();
    CountingUIElement* child2_ptr = child2.get();
    
    // Child2 consumes event
    child2_ptr->consume_click_ = true;
    
    parent_ptr->AddChild(std::move(child1));
    parent_ptr->AddChild(std::move(child2));
    
    // Event at overlapping position
    MouseEvent event{60.0f, 60.0f, false, false, true, false, 0.0f};
    
    bool handled = parent_ptr->ProcessMouseEvent(event);
    
    // Child2 (added last) should handle first (reverse order = top to bottom)
    EXPECT_EQ(child2_ptr->click_count_, 1);
    EXPECT_EQ(child1_ptr->click_count_, 0);  // Not called
    EXPECT_TRUE(handled);
}

// ============================================================================
// MouseEventManager Tests
// ============================================================================

TEST_F(UIMouseEventTest, MouseEventManagerRegistration) {
    MouseEventManager manager;
    
    EXPECT_EQ(manager.GetRegionCount(), 0);
    
    Rectangle region1{100.0f, 100.0f, 200.0f, 100.0f};
    auto handle = manager.RegisterRegion(region1, [](const MouseEvent&) { return false; });
    
    EXPECT_EQ(manager.GetRegionCount(), 1);
    EXPECT_NE(handle, MouseEventManager::INVALID_HANDLE);
}

TEST_F(UIMouseEventTest, MouseEventManagerPriorityOrdering) {
    MouseEventManager manager;
    
    std::vector<int> call_order;
    
    // Register low priority first
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&call_order](const MouseEvent&) {
            call_order.push_back(1);
            return false;
        },
        10  // Low priority
    );
    
    // Register high priority second
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&call_order](const MouseEvent&) {
            call_order.push_back(2);
            return false;
        },
        100  // High priority
    );
    
    // Register medium priority third
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&call_order](const MouseEvent&) {
            call_order.push_back(3);
            return false;
        },
        50  // Medium priority
    );
    
    MouseEvent event{50.0f, 50.0f};
    manager.DispatchEvent(event);
    
    // Should be called in priority order: high (100) -> medium (50) -> low (10)
    ASSERT_EQ(call_order.size(), 3);
    EXPECT_EQ(call_order[0], 2);  // High priority
    EXPECT_EQ(call_order[1], 3);  // Medium priority
    EXPECT_EQ(call_order[2], 1);  // Low priority
}

TEST_F(UIMouseEventTest, MouseEventManagerEventConsumption) {
    MouseEventManager manager;
    
    int call_count = 0;
    
    // First handler consumes event
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&call_count](const MouseEvent&) {
            call_count++;
            return true;  // Consume event
        },
        100
    );
    
    // Second handler should not be called
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&call_count](const MouseEvent&) {
            call_count++;
            return false;
        },
        50
    );
    
    MouseEvent event{50.0f, 50.0f};
    bool handled = manager.DispatchEvent(event);
    
    EXPECT_TRUE(handled);
    EXPECT_EQ(call_count, 1);  // Only first handler called
}

TEST_F(UIMouseEventTest, MouseEventManagerHitTesting) {
    MouseEventManager manager;
    
    bool handler1_called = false;
    bool handler2_called = false;
    
    // Region 1: Top-left quadrant
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&handler1_called](const MouseEvent&) {
            handler1_called = true;
            return false;
        }
    );
    
    // Region 2: Bottom-right quadrant
    manager.RegisterRegion(
        Rectangle{200.0f, 200.0f, 100.0f, 100.0f},
        [&handler2_called](const MouseEvent&) {
            handler2_called = true;
            return false;
        }
    );
    
    // Event in region 1
    MouseEvent event1{50.0f, 50.0f};
    manager.DispatchEvent(event1);
    
    EXPECT_TRUE(handler1_called);
    EXPECT_FALSE(handler2_called);
    
    handler1_called = false;
    handler2_called = false;
    
    // Event in region 2
    MouseEvent event2{250.0f, 250.0f};
    manager.DispatchEvent(event2);
    
    EXPECT_FALSE(handler1_called);
    EXPECT_TRUE(handler2_called);
}

TEST_F(UIMouseEventTest, MouseEventManagerUnregister) {
    MouseEventManager manager;
    
    auto handle = manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [](const MouseEvent&) { return false; }
    );
    
    EXPECT_EQ(manager.GetRegionCount(), 1);
    
    manager.UnregisterRegion(handle);
    
    EXPECT_EQ(manager.GetRegionCount(), 0);
}

TEST_F(UIMouseEventTest, MouseEventManagerUnregisterByUserData) {
    MouseEventManager manager;
    
    int user_data1 = 1;
    int user_data2 = 2;
    
    manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [](const MouseEvent&) { return false; },
        0,
        &user_data1
    );
    
    manager.RegisterRegion(
        Rectangle{100.0f, 100.0f, 100.0f, 100.0f},
        [](const MouseEvent&) { return false; },
        0,
        &user_data1
    );
    
    manager.RegisterRegion(
        Rectangle{200.0f, 200.0f, 100.0f, 100.0f},
        [](const MouseEvent&) { return false; },
        0,
        &user_data2
    );
    
    EXPECT_EQ(manager.GetRegionCount(), 3);
    
    size_t removed = manager.UnregisterByUserData(&user_data1);
    
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(manager.GetRegionCount(), 1);
}

TEST_F(UIMouseEventTest, MouseEventManagerUpdateBounds) {
    MouseEventManager manager;
    
    bool handler_called = false;
    
    auto handle = manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&handler_called](const MouseEvent&) {
            handler_called = true;
            return false;
        }
    );
    
    // Event outside initial bounds
    MouseEvent event{150.0f, 150.0f};
    manager.DispatchEvent(event);
    EXPECT_FALSE(handler_called);
    
    // Update bounds
    manager.UpdateRegionBounds(handle, Rectangle{100.0f, 100.0f, 100.0f, 100.0f});
    
    // Now event should hit
    handler_called = false;
    manager.DispatchEvent(event);
    EXPECT_TRUE(handler_called);
}

TEST_F(UIMouseEventTest, MouseEventManagerEnableDisable) {
    MouseEventManager manager;
    
    bool handler_called = false;
    
    auto handle = manager.RegisterRegion(
        Rectangle{0.0f, 0.0f, 100.0f, 100.0f},
        [&handler_called](const MouseEvent&) {
            handler_called = true;
            return false;
        }
    );
    
    MouseEvent event{50.0f, 50.0f};
    
    // Should work when enabled
    manager.DispatchEvent(event);
    EXPECT_TRUE(handler_called);
    
    // Disable region
    handler_called = false;
    manager.SetRegionEnabled(handle, false);
    manager.DispatchEvent(event);
    EXPECT_FALSE(handler_called);
    
    // Re-enable region
    handler_called = false;
    manager.SetRegionEnabled(handle, true);
    manager.DispatchEvent(event);
    EXPECT_TRUE(handler_called);
}

// ============================================================================
// Integration Test: UIElement + MouseEventManager
// ============================================================================

TEST_F(UIMouseEventTest, IntegrationUIElementAndManager) {
    MouseEventManager manager;
    
    auto element = std::make_unique<CountingUIElement>(100.0f, 100.0f, 200.0f, 100.0f);
    CountingUIElement* element_ptr = element.get();
    
    // Element should consume events
    element_ptr->consume_click_ = true;
    
    // Register element's bounds with manager
    manager.RegisterRegion(
        element_ptr->GetAbsoluteBounds(),
        [element_ptr](const MouseEvent& event) {
            return element_ptr->ProcessMouseEvent(event);
        },
        50  // Normal priority
    );
    
    // Dispatch click event
    MouseEvent event{150.0f, 150.0f, false, false, true, false};
    bool handled = manager.DispatchEvent(event);
    
    EXPECT_TRUE(handled);
    EXPECT_EQ(element_ptr->click_count_, 1);
}
