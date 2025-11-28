# UI Component System

The UI component system provides composable behaviors for UI elements using the **Component Pattern**.

## Overview

Components add optional behaviors to any `UIElement`:

| Component | Purpose |
|-----------|---------|
| `LayoutComponent` | Automatic child positioning (vertical, horizontal, grid) |
| `ConstraintComponent` | Position/size relative to parent edges |
| `ScrollComponent` | Scrollable content with scrollbars |

**Note:** While all UIElements can have components, some components are more useful on certain elements:
- `LayoutComponent` and `ScrollComponent` are typically used with `Container` elements that have children
- `ConstraintComponent` is useful on any element to anchor it within its parent (e.g., a close button in a modal)

## Quick Start

### Using ContainerBuilder (Recommended)

```cpp
using namespace engine::ui;
using namespace engine::ui::components;

auto menu = ContainerBuilder()
    .Size(300, 400)
    .Padding(10)
    .Layout<VerticalLayout>(8.0f, Alignment::Center)
    .Scrollable(ScrollDirection::Vertical)
    .Background(UITheme::Background::PANEL)
    .ClipChildren()
    .Build();

menu->AddChild(std::make_unique<Button>(0, 0, 100, 30, "Button 1"));
menu->AddChild(std::make_unique<Button>(0, 0, 100, 30, "Button 2"));
menu->Update();  // Apply layout
```

### Direct Component API

Components can be added to any `UIElement`, not just containers:

```cpp
using namespace engine::ui::elements;
using namespace engine::ui::components;

// Add constraints to a button (anchor to top-right of parent)
auto button = std::make_unique<Button>(0, 0, 30, 20, "X");
button->AddComponent<ConstraintComponent>(Anchor::TopRight(5.0f));
panel->AddChild(std::move(button));

// Container example with layout and scroll
auto container = std::make_unique<Container>(0, 0, 300, 400);
container->AddComponent<LayoutComponent>(
    std::make_unique<VerticalLayout>(8.0f, Alignment::Center)
);
container->AddComponent<ScrollComponent>(ScrollDirection::Vertical);

// Query components
auto* layout = container->GetComponent<LayoutComponent>();
bool has_scroll = container->HasComponent<ScrollComponent>();

// Remove components
container->RemoveComponent<ScrollComponent>();

// Update components recursively (applies constraints to all children)
container->UpdateComponentsRecursive();
```

### Component Invalidation

When element state changes, call `InvalidateComponents()` to notify components:

```cpp
element->InvalidateComponents();  // Marks LayoutComponent dirty, etc.
element->Update();                // Components recalculate on next update
```

---

## Layout Strategies

Layout strategies control how children are positioned within a container.

### VerticalLayout

Stack children top to bottom:

```cpp
container->AddComponent<LayoutComponent>(
    std::make_unique<VerticalLayout>(
        8.0f,              // Gap between children
        Alignment::Center  // Horizontal alignment
    )
);
```

### HorizontalLayout

Arrange children left to right:

```cpp
container->AddComponent<LayoutComponent>(
    std::make_unique<HorizontalLayout>(10.0f, Alignment::Center)
);
```

### GridLayout

Arrange in rows and columns:

```cpp
container->AddComponent<LayoutComponent>(
    std::make_unique<GridLayout>(
        3,      // Number of columns
        8.0f,   // Horizontal gap
        8.0f    // Vertical gap
    )
);
```

### StackLayout

Overlay all children at the same position (useful for layered UI like background + content + overlay):

```cpp
container->AddComponent<LayoutComponent>(
    std::make_unique<StackLayout>(
        Alignment::Center,  // Horizontal alignment
        Alignment::Center   // Vertical alignment
    )
);

// Example: Badge in top-right corner
container->AddComponent<LayoutComponent>(
    std::make_unique<StackLayout>(Alignment::End, Alignment::Start)
);
```

### JustifyLayout

Distribute children evenly (like CSS flexbox justify-content: space-between):

```cpp
container->AddComponent<LayoutComponent>(
    std::make_unique<JustifyLayout>(
        JustifyDirection::Horizontal,  // Or Vertical
        Alignment::Center              // Cross-axis alignment
    )
);
```

**Note:** Gap is calculated automatically to distribute children evenly.

### Alignment Options

```cpp
enum class Alignment {
    Start,   // Left/top
    Center,  // Center
    End,     // Right/bottom
    Stretch  // Fill available space
};
```

### Padding Behavior

When a Panel/Container has padding, layouts handle it per-axis:

- **Primary axis**: Padding insets start position and reduces available space
- **Cross axis**: 
  - `Start`/`End`/`Stretch`: Respect padding
  - `Center`: Uses full dimension (ignores padding for true centering)

---

## Constraints

Constraints position and size elements relative to their parent.

### Anchor Presets

```cpp
Anchor::TopLeft(margin)           // Top-left corner
Anchor::TopRight(margin)          // Top-right corner
Anchor::BottomLeft(margin)        // Bottom-left corner
Anchor::BottomRight(margin)       // Bottom-right corner
Anchor::StretchHorizontal(l, r)   // Stretch between left/right
Anchor::StretchVertical(t, b)     // Stretch between top/bottom
Anchor::Fill(margin)              // Fill with uniform margin
```

### Manual Anchor Configuration

```cpp
Anchor anchor;
anchor.SetLeft(10.0f);    // 10px from left
anchor.SetRight(10.0f);   // 10px from right (causes stretch)
anchor.SetTop(20.0f);     // 20px from top
```

### Size Constraints

```cpp
SizeConstraint::Fixed(200.0f)        // Fixed 200px
SizeConstraint::Percent(0.5f)        // 50% of parent
SizeConstraint::FitContent(100, 300) // Fit content, min 100, max 300

SizeConstraints::Fixed(200.0f, 100.0f)   // Fixed width and height
SizeConstraints::Percent(0.8f, 0.5f)     // Percentage-based
SizeConstraints::Full()                  // 100% of parent
```

### Using ConstraintComponent

```cpp
container->AddComponent<ConstraintComponent>(
    Anchor::Fill(10.0f),
    SizeConstraints::Percent(0.8f, 0.5f)
);
```

---

## Scrolling

Add scrolling behavior to containers with content larger than the viewport.

### Basic Usage

```cpp
auto* scroll = container->AddComponent<ScrollComponent>(ScrollDirection::Vertical);
scroll->SetContentSize(300, 1000);  // Total scrollable area

// Or auto-calculate from children
scroll->CalculateContentSizeFromChildren();
```

### ScrollState API

```cpp
ScrollState& state = scroll->GetState();

// Position control
state.SetScroll(0, 100);
state.ScrollBy(0, 50);
state.ScrollToStart();
state.ScrollToEnd();

// Queries
bool can_scroll = state.CanScrollY();
float max = state.GetMaxScrollY();
float normalized = state.GetScrollYNormalized();  // 0.0-1.0
```

### Scrollbar Styling

```cpp
ScrollbarStyle style;
style.track_color = {0.2f, 0.2f, 0.2f, 0.5f};
style.thumb_color = {0.5f, 0.5f, 0.5f, 0.8f};
style.width = 10.0f;
style.min_thumb_length = 30.0f;
scroll->SetStyle(style);
```

### Scroll Directions

```cpp
enum class ScrollDirection {
    Vertical,    // Up/down only
    Horizontal,  // Left/right only
    Both         // Both directions
};
```

---

## ContainerBuilder Reference

The builder provides a fluent API for container construction:

```cpp
ContainerBuilder()
    // Position and size
    .Position(x, y)
    .Size(width, height)
    .Bounds(x, y, width, height)
    
    // Panel styling
    .Padding(padding)
    .Background(color)
    .Border(width, color)
    .Opacity(opacity)
    .ClipChildren()
    
    // Layout
    .Layout<LayoutType>(args...)
    
    // Constraints
    .Anchor(anchor)
    .SizeConstraints(constraints)
    .Fill(margin)
    .StretchHorizontal(left, right)
    .StretchVertical(top, bottom)
    
    // Scrolling
    .Scrollable(direction)
    .ScrollStyle(style)
    
    // Build
    .Build();
```

---

## Examples

### Responsive Toolbar

```cpp
auto toolbar = ContainerBuilder()
    .Bounds(0, 0, 0, 50)
    .Layout<HorizontalLayout>(8.0f, Alignment::Center)
    .StretchHorizontal(0, 0)
    .Padding(10)
    .Build();
```

### Centered Modal Dialog

```cpp
auto dialog = ContainerBuilder()
    .Size(400, 300)
    .Layout<VerticalLayout>(12.0f, Alignment::Center)
    .Fill(50.0f)
    .Background(UITheme::Background::PANEL)
    .Border(2.0f, UITheme::Border::ACCENT)
    .Build();
```

### Scrollable List

```cpp
auto list = ContainerBuilder()
    .Size(250, 400)
    .Layout<VerticalLayout>(4.0f, Alignment::Stretch)
    .Scrollable(ScrollDirection::Vertical)
    .ClipChildren()
    .Build();

for (const auto& item : items) {
    list->AddChild(CreateListItem(item));
}
list->Update();
```

### Icon Grid

```cpp
auto grid = ContainerBuilder()
    .Size(400, 400)
    .Layout<GridLayout>(4, 8.0f, 8.0f)
    .Scrollable(ScrollDirection::Vertical)
    .Build();

for (const auto& icon : icons) {
    grid->AddChild(std::make_unique<ImageElement>(0, 0, 80, 80, icon.texture));
}
grid->Update();
```

---

## Component Lifecycle

Components participate in the element's update and render cycle:

| Method | When Called |
|--------|-------------|
| `OnAttach(owner)` | When added to element |
| `OnDetach()` | When removed from element |
| `OnUpdate(delta_time)` | Each frame before rendering |
| `OnRender()` | After element renders (for overlays like scrollbars) |
| `OnMouseEvent(event)` | When processing mouse input |

---

## Debug Tools

The engine includes debug utilities for UI development:

### UIDebugVisualizer

Visualizes element bounds and hierarchy:

```cpp
#include "ui_debug_visualizer.h"

UIDebugVisualizer debugger;
debugger.SetEnabled(true);

// Setup click-to-select (once after UI is built)
debugger.SetupClickToSelect(root.get());

// In render loop
BatchRenderer::BeginFrame();
root->Render();
debugger.RenderDebugOverlay(root.get());  // Render after UI
BatchRenderer::EndFrame();

// In ImGui
debugger.RenderImGuiControls();
```

### UIElementInspector

Chrome DevTools-style inspector with inline editing:

```cpp
#include "ui_element_inspector.h"

UIElementInspector inspector;

// In ImGui
ImGui::Begin("Inspector");
if (inspector.Render(selected_element)) {
    // Element was modified
    container->Update();
}
ImGui::End();
```

Features:
- **Box Model**: Visual diagram with inline-editable padding, border, width, height, position
- **Anchor Widget**: Click to toggle anchors, preset buttons for common layouts
- **Component List**: Shows attached LayoutComponent, ConstraintComponent, ScrollComponent

---

## See Also

- [UI Mouse Events Guide](ui_mouse_events_guide.md) - Mouse event handling
- [API Reference](api/index.md) - Auto-generated API documentation
