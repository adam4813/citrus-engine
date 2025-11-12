# UI Mouse Events Guide

This guide demonstrates how to use the citrus-engine UI mouse event system for interactive UI components.

## Quick Start

### 1. Basic Button with Click Handler

```cpp
import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;

class SimpleButton : public UIElement {
public:
    SimpleButton(float x, float y, float width, float height, const std::string& label)
        : UIElement(x, y, width, height)
        , label_(label) {}
    
    // Handle mouse clicks
    bool OnClick(const MouseEvent& event) override {
        if (!Contains(event.x, event.y)) {
            return false;  // Not clicked on this button
        }
        
        if (event.left_pressed) {
            // Execute callback
            if (on_click_) on_click_();
            return true;  // Event consumed
        }
        
        return false;
    }
    
    // Visual feedback on hover
    bool OnHover(const MouseEvent& event) override {
        // Update hover state (already set by ProcessMouseEvent)
        // Could trigger tooltip, color change, etc.
        return false;  // Don't consume hover events
    }
    
    // Render the button
    void Render() const override {
        Color bg_color = IsHovered() 
            ? Color{0.3f, 0.5f, 0.8f, 1.0f}  // Lighter when hovered
            : Color{0.2f, 0.4f, 0.7f, 1.0f}; // Normal color
        
        Rectangle bounds = GetAbsoluteBounds();
        BatchRenderer::SubmitQuad(bounds, bg_color);
        BatchRenderer::SubmitText(label_, bounds.x + 10, bounds.y + 10, 20, Color{1, 1, 1, 1});
    }
    
    void SetClickCallback(std::function<void()> callback) {
        on_click_ = callback;
    }

private:
    std::string label_;
    std::function<void()> on_click_;
};

// Usage
auto button = std::make_unique<SimpleButton>(100, 100, 150, 40, "Click Me");
button->SetClickCallback([]() {
    std::cout << "Button clicked!" << std::endl;
});

// Dispatch mouse event
MouseEvent event{150, 120, false, false, true, false};  // Left click at (150, 120)
bool handled = button->ProcessMouseEvent(event);
```

### 2. UI Hierarchy with Event Propagation

```cpp
// Create a panel with buttons
auto panel = std::make_unique<UIElement>(50, 50, 400, 300);

// Add buttons as children
auto button1 = std::make_unique<SimpleButton>(10, 10, 100, 40, "Button 1");
button1->SetClickCallback([]() { std::cout << "Button 1" << std::endl; });
panel->AddChild(std::move(button1));

auto button2 = std::make_unique<SimpleButton>(120, 10, 100, 40, "Button 2");
button2->SetClickCallback([]() { std::cout << "Button 2" << std::endl; });
panel->AddChild(std::move(button2));

// Dispatch event - automatically propagates to children
MouseEvent event{80, 80, false, false, true, false};  // Click at button1 position
bool handled = panel->ProcessMouseEvent(event);
// Output: "Button 1" (button1 consumed the event)
```

### 3. Modal Dialog Blocking Lower Layers

```cpp
class ModalDialog : public UIElement {
public:
    ModalDialog(float x, float y, float w, float h)
        : UIElement(x, y, w, h) {}
    
    // Block ALL events when visible (modal behavior)
    bool ProcessMouseEvent(const MouseEvent& event) override {
        if (!IsVisible()) {
            return false;
        }
        
        // Let children handle first
        if (UIElement::ProcessMouseEvent(event)) {
            return true;
        }
        
        // Block ALL events, even if not over dialog
        return true;  // Always consume when visible
    }
    
    void Render() const override {
        // Render dark overlay
        BatchRenderer::SubmitQuad(
            Rectangle{0, 0, 1920, 1080},  // Full screen
            Color{0, 0, 0, 0.7f}  // Dark transparent
        );
        
        // Render dialog
        UIElement::Render();
    }
};

// Usage
auto modal = std::make_unique<ModalDialog>(400, 300, 500, 400);
modal->SetVisible(true);

// Add OK/Cancel buttons
auto ok_button = std::make_unique<SimpleButton>(150, 320, 100, 40, "OK");
ok_button->SetClickCallback([&modal]() {
    modal->SetVisible(false);
});
modal->AddChild(std::move(ok_button));

// Modal blocks all events when visible
MouseEvent event{100, 100};  // Click outside dialog
bool handled = modal->ProcessMouseEvent(event);
// Returns true - event blocked by modal
```

### 4. MouseEventManager for Complex UI

```cpp
MouseEventManager manager;

// Register UI regions with priorities

// Modal dialog (highest priority - blocks everything)
auto modal_handle = manager.RegisterRegion(
    Rectangle{400, 300, 500, 400},
    [](const MouseEvent& event) {
        // Handle modal events
        return true;  // Block all events
    },
    1000  // Very high priority
);

// Context menu (high priority)
auto context_menu_handle = manager.RegisterRegion(
    Rectangle{200, 200, 150, 200},
    [](const MouseEvent& event) {
        if (event.left_pressed) {
            // Handle menu item click
            return true;
        }
        return false;
    },
    500  // High priority
);

// Button (normal priority)
auto button_handle = manager.RegisterRegion(
    Rectangle{100, 100, 150, 40},
    [](const MouseEvent& event) {
        if (event.left_pressed) {
            ExecuteAction();
            return true;
        }
        return false;
    },
    50  // Normal priority
);

// Background (lowest priority)
auto background_handle = manager.RegisterRegion(
    Rectangle{0, 0, 1920, 1080},
    [](const MouseEvent& event) {
        // Handle background clicks (deselect, etc.)
        return true;
    },
    0  // Lowest priority
);

// Dispatch event
MouseEvent event{450, 350, false, false, true, false};
bool handled = manager.DispatchEvent(event);
// Modal handles it first (highest priority)

// Hide modal dynamically
manager.SetRegionEnabled(modal_handle, false);

// Update button position (for animations)
manager.UpdateRegionBounds(button_handle, Rectangle{100, 150, 150, 40});

// Remove context menu
manager.UnregisterRegion(context_menu_handle);
```

### 5. Scroll Event Handling

```cpp
class ScrollablePanel : public UIElement {
public:
    ScrollablePanel(float x, float y, float w, float h)
        : UIElement(x, y, w, h)
        , scroll_offset_(0.0f) {}
    
    bool OnScroll(const MouseEvent& event) override {
        if (!Contains(event.x, event.y)) {
            return false;
        }
        
        // Update scroll offset
        scroll_offset_ += event.scroll_delta * 20.0f;  // Scale scroll speed
        scroll_offset_ = std::max(0.0f, std::min(scroll_offset_, max_scroll_));
        
        // Reposition children based on scroll
        UpdateLayout();
        
        return true;  // Consume scroll event
    }
    
    void UpdateLayout() {
        float y = -scroll_offset_;
        for (auto& child : GetChildren()) {
            child->SetRelativePosition(0, y);
            y += child->GetHeight() + 10;  // 10px spacing
        }
    }

private:
    float scroll_offset_;
    float max_scroll_{1000.0f};
};
```

### 6. Drag-and-Drop

```cpp
class DraggableElement : public UIElement {
public:
    DraggableElement(float x, float y, float w, float h)
        : UIElement(x, y, w, h)
        , is_dragging_(false)
        , drag_start_x_(0)
        , drag_start_y_(0) {}
    
    bool OnClick(const MouseEvent& event) override {
        if (!Contains(event.x, event.y)) {
            return false;
        }
        
        if (event.left_pressed) {
            // Start drag
            is_dragging_ = true;
            drag_start_x_ = event.x - GetAbsoluteBounds().x;
            drag_start_y_ = event.y - GetAbsoluteBounds().y;
            return true;
        }
        
        if (event.left_released) {
            // End drag
            is_dragging_ = false;
            return true;
        }
        
        return false;
    }
    
    bool OnDrag(const MouseEvent& event) override {
        if (!is_dragging_) {
            return false;
        }
        
        // Update position while dragging
        float new_x = event.x - drag_start_x_;
        float new_y = event.y - drag_start_y_;
        SetRelativePosition(new_x, new_y);
        
        return true;  // Consume drag event
    }

private:
    bool is_dragging_;
    float drag_start_x_, drag_start_y_;
};
```

## Event Propagation Rules

### Bubble-Down Algorithm

1. Parent receives event first via `ProcessMouseEvent()`
2. Parent delegates to children in **reverse order** (last added = top-most)
3. If any child returns `true`, propagation stops
4. If no child consumed event, parent tries its own handlers
5. Parent handlers called in order: OnClick → OnScroll → OnDrag → OnHover

### Return Values

- **Return `true`**: Event consumed, stop propagation
- **Return `false`**: Event not handled, continue to next handler

### Common Patterns

**Button**: Return `true` only on left_pressed to consume click
```cpp
bool OnClick(const MouseEvent& event) override {
    return event.left_pressed && Contains(event.x, event.y);
}
```

**Tooltip**: Return `false` to allow events to pass through
```cpp
bool OnHover(const MouseEvent& event) override {
    ShowTooltip();
    return false;  // Don't consume
}
```

**Modal**: Return `true` always to block all events
```cpp
bool ProcessMouseEvent(const MouseEvent& event) override {
    UIElement::ProcessMouseEvent(event);  // Let children handle
    return IsVisible();  // Block everything when visible
}
```

## Best Practices

1. **Always check Contains()** before handling events
2. **Return true only when event is consumed** (prevents unexpected behavior)
3. **Use OnHover for visual feedback** (don't consume events)
4. **Use OnClick for actions** (consume events)
5. **Test hit-testing carefully** with nested hierarchies
6. **Consider z-order** when using overlapping elements (last added = top)
7. **Use MouseEventManager** for modal dialogs and priority-based dispatch
8. **Update region bounds** dynamically for animated UI elements

## Integration with Input System

```cpp
// Pseudo-code for game loop integration
void GameLoop() {
    // Poll input from platform
    engine::input::PollEvents();
    
    // Create MouseEvent from platform input
    float mouse_x = GetMouseX();
    float mouse_y = GetMouseY();
    bool left_down = IsMouseButtonDown(MOUSE_LEFT);
    bool left_pressed = IsMouseButtonPressed(MOUSE_LEFT);
    float scroll = GetMouseWheelDelta();
    
    MouseEvent event{
        mouse_x, mouse_y,
        left_down, false,  // left_down, right_down
        left_pressed, false,  // left_pressed, right_pressed
        scroll
    };
    
    // Dispatch to UI
    root_ui_element->ProcessMouseEvent(event);
    
    // Or use MouseEventManager
    ui_event_manager.DispatchEvent(event);
    
    // Render
    BatchRenderer::BeginFrame();
    root_ui_element->Render();
    BatchRenderer::EndFrame();
}
```

## Troubleshooting

**Events not reaching child elements?**
- Check that parent is calling `UIElement::ProcessMouseEvent()` to propagate to children
- Verify parent isn't consuming all events (returning `true` unconditionally)

**Multiple elements responding to same click?**
- Ensure handlers return `true` to consume events
- Check z-order (last added child is on top)

**Hit-testing not working with nested elements?**
- Use `GetAbsoluteBounds()` for screen-space coordinates
- Verify parent-child hierarchy is set up correctly

**Modal not blocking events?**
- Ensure modal's `ProcessMouseEvent` returns `true` when visible
- Check modal has highest priority in MouseEventManager

## See Also

- [UI_DEVELOPMENT_BIBLE.md](../UI_DEVELOPMENT_BIBLE.md) - Comprehensive UI architecture guide
- [issue #38](https://github.com/adam4813/citrus-engine/issues/38) - UI component implementation roadmap
- [towerforge reference](https://github.com/adam4813/towerforge) - Original implementation inspiration
