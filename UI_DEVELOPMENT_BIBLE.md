# Citrus Engine UI Development Bible

**AI Agent Reference: Building declarative, reactive, event-driven user interfaces**

**Purpose**: Technical reference for AI agents implementing UI components for citrus-engine  
**Audience**: AI assistants only (not user documentation)  
**Status**: Active

**CRITICAL**: The citrus-engine production UI system is the **custom vertex-based batch renderer**. ImGui is included **ONLY** for temporary debugging and testing purposes and is NOT the primary UI system. All production UI must use the batch renderer.

---

## Table of Contents

1. [Philosophy & Core Principles](#philosophy--core-principles)
2. [Gang of Four Design Patterns](#gang-of-four-design-patterns)
3. [Component Recognition & Reusability](#component-recognition--reusability)
4. [Event System Architecture](#event-system-architecture)
5. [Ownership & Memory Management](#ownership--memory-management)
6. [Layout & Positioning](#layout--positioning)
7. [Creating New Components](#creating-new-components)
8. [Testing & Validation](#testing--validation)
9. [Performance Considerations](#performance-considerations)
10. [Quick Reference](#quick-reference)

---

## Philosophy & Core Principles

### Declarative Over Imperative

UI structure is **declared once** in constructors, not rebuilt every frame.

```cpp
// CORRECT: Declarative - build once, render many
Constructor() {
    for (const auto& item : items) {
        auto button = std::make_unique<Button>(x, y, width, height, item.label);
        button->SetClickCallback([this, action = item.action]() {
            ExecuteAction(action);
        });
        panel_->AddChild(std::move(button));
    }
}

void Render() const {
    // UI components submit vertices to batch renderer
    using namespace engine::ui::batch_renderer;
    
    BatchRenderer::BeginFrame();
    panel_->Render();  // Simple tree traversal, submits vertices
    BatchRenderer::EndFrame();  // Flushes all batches to GPU
}
```

**Benefits**: Easier to reason about, test, and optimize. State changes are explicit, not implicit. Batch renderer minimizes GPU draw calls.

---

### Reactive Updates

Visual state changes **only** in response to events, never continuously.

```cpp
// CORRECT: Update on events only
void OnSelectionChanged(int new_index) {
    buttons_[old_index_]->SetColor({0.5f, 0.5f, 0.5f, 1.0f});  // GRAY
    buttons_[new_index]->SetColor({1.0f, 0.84f, 0.0f, 1.0f});   // GOLD
    old_index_ = new_index;
}

void Render() const {
    // Button properties already set - just submit vertices
    using namespace engine::ui::batch_renderer;
    
    for (const auto& button : buttons_) {
        BatchRenderer::SubmitQuad(
            button->GetRect(),
            button->GetColor()
        );
    }
}
```

**Benefits**: 60x performance improvement over per-frame updates. UI updates when it needs to, not "just in case." Batch renderer efficiently handles the actual vertex submissions.

---

### Event-Driven Communication

Components notify observers via callbacks, never return magic values.

```cpp
// CORRECT: Observer pattern with callbacks
Constructor() {
    button->SetClickCallback([this, action]() {
        if (callback_) callback_(action);
    });
}

void SetActionCallback(std::function<void(Action)> callback) {
    callback_ = callback;
}

// Game code responds directly:
menu.SetActionCallback([this](Action action) {
    switch (action) {
        case Action::Build: ShowBuildMenu(); break;
        case Action::Settings: ShowSettings(); break;
    }
});
```

**Benefits**: Loose coupling. Menu doesn't know about game logic. Game doesn't know about menu internals.

---

### Composition Over Inheritance

Build complex UIs from small, reusable components using the Composite pattern.

```cpp
// CORRECT: Compose complex UI from primitives
class SettingsMenu {
    std::unique_ptr<Panel> root_panel_;
    std::unique_ptr<Panel> audio_section_;
    std::unique_ptr<Panel> video_section_;
    
    Slider* master_volume_;
    Slider* music_volume_;
    Checkbox* fullscreen_;
    Button* apply_button_;
};

Constructor() {
    // Build tree structure
    root_panel_ = std::make_unique<Panel>(...);
    
    audio_section_ = std::make_unique<Panel>(...);
    auto slider = std::make_unique<Slider>(...);
    master_volume_ = slider.get();
    audio_section_->AddChild(std::move(slider));
    
    root_panel_->AddChild(std::move(audio_section_));
}
```

**Benefits**: Small, focused components are easier to test, debug, and reuse.

---

## Citrus Engine Batch Renderer Architecture (Production UI System)

### Vertex-Based Rendering

**This is the production UI system.** The citrus-engine UI is built on a high-performance vertex-based batch renderer that minimizes GPU draw calls:

```cpp
using namespace engine::ui::batch_renderer;

// Initialize once at startup
BatchRenderer::Initialize();

// Each frame
BatchRenderer::BeginFrame();

// Submit UI primitives (converted to vertices internally)
BatchRenderer::SubmitQuad(
    Rectangle{x, y, width, height},  // Screen-space rectangle
    Color{r, g, b, a},                // Fill color
    std::nullopt,                     // UV coords (optional)
    texture_id                        // Texture ID (0 = solid color)
);

BatchRenderer::SubmitLine(x0, y0, x1, y1, thickness, color);
BatchRenderer::SubmitText(text, x, y, font_size, color);

// Flush all batches to GPU
BatchRenderer::EndFrame();

// Cleanup at shutdown
BatchRenderer::Shutdown();
```

### Key Features

- **Batching**: Automatically batches draw calls to minimize GPU submissions
- **Texture Management**: Supports up to 8 simultaneous texture units
- **Scissor Clipping**: Hierarchical scissor rectangles for panels and scroll regions
- **Vertex Pooling**: Pre-allocated vertex/index buffers to avoid allocations

### Scissor Clipping for Panels

```cpp
// Render a panel with content clipped to bounds
void Panel::Render() const {
    using namespace engine::ui::batch_renderer;
    
    // Push scissor for this panel
    BatchRenderer::PushScissor(ScissorRect{x, y, width, height});
    
    // Render panel background
    BatchRenderer::SubmitQuad(GetRect(), background_color_);
    
    // Render children (clipped to panel bounds)
    for (const auto& child : children_) {
        child->Render();
    }
    
    // Restore previous scissor
    BatchRenderer::PopScissor();
}
```

### Component Rendering Pattern

UI components should **submit vertices** via BatchRenderer, not manage their own vertex buffers:

```cpp
class Button : public UIElement {
public:
    void Render() const override {
        using namespace engine::ui::batch_renderer;
        
        // Submit button background quad
        BatchRenderer::SubmitQuad(GetRect(), GetBackgroundColor());
        
        // Submit button text
        if (!label_.empty()) {
            BatchRenderer::SubmitText(
                label_,
                GetTextPosition(),
                font_size_,
                GetTextColor()
            );
        }
        
        // Render children
        UIElement::Render();
    }
    
private:
    std::string label_;
    float font_size_;
    // Colors updated reactively, not every frame
};
```

---

## Gang of Four Design Patterns

### 1. Composite Pattern ⭐⭐⭐

**Purpose**: Treat individual UI elements and compositions uniformly.

**When to Use**: Always. Every UI element inherits from `UIElement`.

**Implementation**:
```cpp
class UIElement {
public:
    void AddChild(std::unique_ptr<UIElement> child);
    void RemoveChild(UIElement* child);
    virtual void Render() const;
    
protected:
    std::vector<std::unique_ptr<UIElement>> children_;
};

// Panel, Button, Slider all extend UIElement
// Can be nested arbitrarily deep
Panel -> Panel -> Button
      -> Slider
      -> Checkbox
```

**Benefits**:
- Uniform interface for all UI elements
- Tree traversal for rendering, input, layout
- Easy to add new component types

---

### 2. Observer Pattern ⭐⭐⭐

**Purpose**: Decouple UI events from business logic.

**When to Use**: Any time a component needs to notify external code.

**Implementation**:
```cpp
// Component provides callback registration
class Button : public UIElement {
public:
    using ClickCallback = std::function<void()>;
    void SetClickCallback(ClickCallback callback) { click_callback_ = callback; }
    
    bool OnClick(const MouseEvent& event) override {
        if (click_callback_) click_callback_();
        return true;
    }
    
private:
    ClickCallback click_callback_;
};

// Subscriber registers interest
button->SetClickCallback([this]() {
    ExecuteAction();
});
```

**Event Types to Support**:
- **Click**: `std::function<void()>`
- **Value Changed**: `std::function<void(T value)>`
- **Selection Changed**: `std::function<void(int index)>`
- **Toggle**: `std::function<void(bool checked)>`
- **Focus**: `std::function<void(bool focused)>`
- **Hover**: `std::function<void(bool hovered)>`
- **Validation**: `std::function<bool(T value)>` (return false to reject)

---

### 3. Template Method Pattern ⭐⭐

**Purpose**: Define skeleton of rendering algorithm, let subclasses customize steps.

**When to Use**: Base rendering logic shared across components.

**Implementation**:
```cpp
class UIElement {
public:
    // Template method
    void Render() const {
        if (!IsVisible()) return;
        
        RenderBackground();  // Virtual
        RenderContent();     // Virtual
        RenderBorder();      // Virtual
        
        for (const auto& child : children_) {
            child->Render();  // Recurse
        }
        
        RenderForeground();  // Virtual (tooltips, etc)
    }
    
protected:
    virtual void RenderBackground() const {}
    virtual void RenderContent() const {}
    virtual void RenderBorder() const {}
    virtual void RenderForeground() const {}
};

// Subclass overrides only what it needs
class Button : public UIElement {
protected:
    void RenderBackground() const override {
        using namespace engine::ui::batch_renderer;
        BatchRenderer::SubmitQuad(bounds_, background_color_);
    }
    
    void RenderContent() const override {
        using namespace engine::ui::batch_renderer;
        auto text_pos = bounds_.center();
        BatchRenderer::SubmitText(label_, text_pos.x, text_pos.y, font_size_, text_color_);
    }
    
    void RenderBorder() const override {
        using namespace engine::ui::batch_renderer;
        // Submit 4 lines for border
        float x = bounds_.x, y = bounds_.y, w = bounds_.width, h = bounds_.height;
        BatchRenderer::SubmitLine(x, y, x+w, y, 1.0f, border_color_);       // Top
        BatchRenderer::SubmitLine(x+w, y, x+w, y+h, 1.0f, border_color_);   // Right
        BatchRenderer::SubmitLine(x+w, y+h, x, y+h, 1.0f, border_color_);   // Bottom
        BatchRenderer::SubmitLine(x, y+h, x, y, 1.0f, border_color_);       // Left
    }
};
```

---

### 4. Strategy Pattern ⭐

**Purpose**: Encapsulate interchangeable algorithms.

**When to Use**: Multiple ways to do the same thing (layout, validation, formatting).

**Implementation**:
```cpp
// Layout strategies
class ILayoutStrategy {
public:
    virtual void Layout(std::vector<UIElement*>& children, Rectangle bounds) = 0;
};

class VerticalLayout : public ILayoutStrategy {
    void Layout(std::vector<UIElement*>& children, Rectangle bounds) override {
        float y = bounds.y;
        for (auto* child : children) {
            child->SetPosition(bounds.x, y);
            y += child->GetHeight() + spacing_;
        }
    }
};

class HorizontalLayout : public ILayoutStrategy {
    void Layout(std::vector<UIElement*>& children, Rectangle bounds) override {
        float x = bounds.x;
        for (auto* child : children) {
            child->SetPosition(x, bounds.y);
            x += child->GetWidth() + spacing_;
        }
    }
};

// Panel uses strategy
class Panel {
    void SetLayoutStrategy(std::unique_ptr<ILayoutStrategy> strategy) {
        layout_strategy_ = std::move(strategy);
    }
    
    void UpdateLayout() {
        if (layout_strategy_) {
            layout_strategy_->Layout(children_, GetBounds());
        }
    }
};
```

**Common Strategies**:
- Layout: Vertical, Horizontal, Grid, Flow, Absolute
- Validation: Range, Pattern, Custom
- Formatting: Currency, Percentage, Date/Time
- Animation: Linear, EaseIn, EaseOut, Bounce

### 5. Facade Pattern ⭐⭐

**Purpose**: Simplify complex subsystem interfaces.

**When to Use**: High-level menus that hide UI element complexity.

**Implementation**:
```cpp
// Facade hides complex tree structure
class AudioSettingsMenu {
public:
    AudioSettingsMenu() {
        // Complex construction
        root_ = std::make_unique<Panel>(...);
        auto slider1 = std::make_unique<Slider>(...);
        auto slider2 = std::make_unique<Slider>(...);
        // ... 50 lines of component creation
    }
    
    // Simple interface for game code
    void Show() { root_->Show(); }
    void Hide() { root_->Hide(); }
    void Render() const { root_->Render(); }
    
    // High-level API
    void SetMasterVolume(float volume) { master_volume_->SetValue(volume); }
    float GetMasterVolume() const { return master_volume_->GetValue(); }
    
private:
    std::unique_ptr<Panel> root_;
    Slider* master_volume_;  // Raw pointer for access
};
```

---

### 6. Command Pattern ⭐

**Purpose**: Encapsulate requests as objects (for undo/redo, macro recording).

**When to Use**: Actions that need to be undoable or recorded.

**Implementation**:
```cpp
class ICommand {
public:
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

class DeleteFacilityCommand : public ICommand {
public:
    DeleteFacilityCommand(FacilityID id) : id_(id) {}
    
    void Execute() override {
        facility_data_ = game_->GetFacility(id_);
        game_->DeleteFacility(id_);
    }
    
    void Undo() override {
        game_->RestoreFacility(facility_data_);
    }
    
    std::string GetDescription() const override {
        return "Delete " + facility_data_.name;
    }
    
private:
    FacilityID id_;
    FacilityData facility_data_;
};

// Button executes command
button->SetClickCallback([this]() {
    auto cmd = std::make_unique<DeleteFacilityCommand>(selected_id_);
    command_history_.Execute(std::move(cmd));
});
```

---

## Component Recognition & Reusability

### When to Extract a Reusable Component

Ask yourself these questions:

1. **Does it follow a known pattern?**
   - Card, Badge, Toggle, Tab, Accordion, Tooltip, Modal, Dropdown, etc.
   - Extract component

2. **Is it used 3+ times?**
   - Same structure appears in multiple places
   - Extract component

3. **Does it have clear boundaries?**
   - Self-contained visual and behavioral unit
   - Extract component

4. **Does it manage its own state?**
   - Selected, expanded, hovered, dragging, etc.
   - Extract component

5. **Could it be useful in other projects?**
   - Generic enough to reuse
   - Extract component

---

### Common UI Patterns to Recognize

#### Card
**Visual**: Rectangular container with shadow/border, padding, optional header/footer.  
**Use Cases**: Item display, info panels, product listings.

```cpp
class Card : public Panel {
public:
    Card(const std::string& title, const std::string& content);
    void SetHeader(std::unique_ptr<UIElement> header);
    void SetFooter(std::unique_ptr<UIElement> footer);
    void SetElevation(int level);  // Shadow depth
};
```

---

#### Badge
**Visual**: Small colored indicator (number or icon).  
**Use Cases**: Notification counts, status indicators.

```cpp
class Badge : public UIElement {
public:
    Badge(int count, Color color = RED);
    void SetCount(int count);
    void SetStyle(BadgeStyle style);  // Dot, Number, Icon
};
```

---

#### Toggle
**Visual**: Switch with on/off states.  
**Use Cases**: Settings, feature flags.

```cpp
class Toggle : public UIElement {
public:
    using ToggleCallback = std::function<void(bool)>;
    Toggle(const std::string& label);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void SetToggleCallback(ToggleCallback callback);
};
```

---

#### Tab Bar
**Visual**: Horizontal buttons for switching views.  
**Use Cases**: Multi-section menus.

```cpp
class TabBar : public Panel {
public:
    using TabCallback = std::function<void(int index)>;
    void AddTab(const std::string& label);
    void SetActiveTab(int index);
    void SetTabCallback(TabCallback callback);
};
```

---

#### Grid
**Visual**: Scrollable grid of items.  
**Use Cases**: Inventories, build menus, galleries.

```cpp
class GridPanel : public Panel {
public:
    GridPanel(int columns, int rows);
    void AddItem(std::unique_ptr<UIElement> item);
    void SetItemClickCallback(std::function<void(int index)> callback);
    void SetScrollPosition(float position);
};
```

---

#### Modal
**Visual**: Overlay that blocks interaction with lower layers.  
**Use Cases**: Dialogs, confirmations, alerts.

```cpp
class Modal : public Panel {
public:
    Modal(std::unique_ptr<UIElement> content);
    void Show();
    void Hide();
    bool IsVisible() const;
    
    // CRITICAL: Block events to lower layers
    bool ProcessMouseEvent(const MouseEvent& event) override {
        if (!IsVisible()) return false;
        
        // Consume ALL events when visible
        Panel::ProcessMouseEvent(event);
        return true;  // Always block propagation
    }
};
```

**Key Lesson**: Modals must explicitly block event propagation to prevent lower layers from receiving input.

---

#### Tooltip
**Visual**: Small popup near cursor with hint text.  
**Use Cases**: Contextual help.

```cpp
class Tooltip : public Panel {
public:
    Tooltip(const std::string& text);
    void ShowAt(float x, float y);
    void Hide();
    void SetDelay(float seconds);  // Hover delay
};
```

---

#### Dropdown
**Visual**: Button that expands to show options.  
**Use Cases**: Selection from list.

```cpp
class Dropdown : public Panel {
public:
    using SelectionCallback = std::function<void(int index, const std::string& value)>;
    void AddOption(const std::string& label);
    void SetSelection(int index);
    void SetSelectionCallback(SelectionCallback callback);
};
```

---

#### Progress Bar
**Visual**: Horizontal bar showing completion percentage.  
**Use Cases**: Loading, construction progress.

```cpp
class ProgressBar : public UIElement {
public:
    ProgressBar(float width, float height);
    void SetProgress(float percentage);  // 0.0 - 1.0
    void SetLabel(const std::string& label);
    void SetColor(Color color);
};
```

---

#### Accordion
**Visual**: Collapsible sections with headers.  
**Use Cases**: Settings groups, FAQs.

```cpp
class Accordion : public Panel {
public:
    void AddSection(const std::string& title, std::unique_ptr<Panel> content);
    void ExpandSection(int index);
    void CollapseSection(int index);
    void SetAllowMultiple(bool allow);  // Multiple sections expanded?
};
```

---

## Event System Architecture

### Event Types

#### 1. Mouse Events

**Implementation Status**: ✅ Fully Implemented

The citrus-engine UI mouse event system provides cross-platform mouse interaction with bubble-down propagation.

```cpp
struct MouseEvent {
    float x, y;                   // Screen coordinates
    
    bool left_down;               // Left button currently held
    bool right_down;              // Right button currently held
    bool middle_down;             // Middle button currently held
    
    bool left_pressed;            // Left button just pressed this frame
    bool right_pressed;           // Right button just pressed this frame
    bool middle_pressed;          // Middle button just pressed this frame
    
    bool left_released;           // Left button just released this frame
    bool right_released;          // Right button just released this frame
    bool middle_released;         // Middle button just released this frame
    
    float scroll_delta;           // Vertical scroll (positive = up)
};

class UIElement : public IMouseInteractive {
public:
    // Override to handle events (return true to consume)
    virtual bool OnHover(const MouseEvent& event) { return false; }
    virtual bool OnClick(const MouseEvent& event) { return false; }
    virtual bool OnDrag(const MouseEvent& event) { return false; }
    virtual bool OnScroll(const MouseEvent& event) { return false; }
    
    // Propagate to children (bubble-down, reverse order)
    bool ProcessMouseEvent(const MouseEvent& event);
    
    // Hit testing
    bool Contains(float x, float y) const;
};
```

**Event Propagation**: Bubble-down (parent → children → parent). Children are checked in reverse order (last added = top-most). First handler to return `true` consumes the event and stops propagation.

**Usage Example**:
```cpp
class MyButton : public UIElement {
public:
    MyButton(float x, float y, float w, float h) 
        : UIElement(x, y, w, h) {}
    
    bool OnClick(const MouseEvent& event) override {
        if (!Contains(event.x, event.y)) {
            return false;  // Not our business
        }
        
        if (event.left_pressed) {
            // Handle left click
            if (callback_) callback_();
            return true;  // Event consumed
        }
        
        return false;  // Let others handle
    }
    
    void SetCallback(std::function<void()> cb) {
        callback_ = cb;
    }
    
private:
    std::function<void()> callback_;
};
```

**MouseEventManager** (Priority-Based Region Handling):

For advanced use cases, use `MouseEventManager` to register regions with priorities:

```cpp
MouseEventManager manager;

// Register modal dialog (high priority blocks lower layers)
auto modal_handle = manager.RegisterRegion(
    Rectangle{100, 100, 400, 300},
    [this](const MouseEvent& event) {
        HandleModalEvent(event);
        return true;  // Block all events when visible
    },
    100  // High priority
);

// Register button (normal priority)
auto button_handle = manager.RegisterRegion(
    Rectangle{150, 150, 100, 40},
    [this](const MouseEvent& event) {
        if (event.left_pressed) {
            HandleButtonClick();
            return true;
        }
        return false;
    },
    50  // Normal priority
);

// Dispatch event
MouseEvent event{200, 175, false, false, true, false};
bool handled = manager.DispatchEvent(event);

// Update region bounds dynamically (for animations)
manager.UpdateRegionBounds(button_handle, Rectangle{150, 200, 100, 40});

// Disable region temporarily
manager.SetRegionEnabled(modal_handle, false);

// Unregister when done
manager.UnregisterRegion(button_handle);
```

**Key Features**:
- **Priority-based dispatch**: Higher priority handlers called first
- **Hit-test regions**: Only handlers in bounds receive events
- **Dynamic updates**: Update bounds, enable/disable regions at runtime
- **User data tagging**: Bulk unregister by user data pointer
- **Event consumption**: First handler returning `true` stops propagation
- **Stable handles**: Handles remain valid across all operations (registration, dispatch, updates)

---

#### 2. Keyboard Events
```cpp
struct KeyboardEvent {
    int key;                      // Engine key code (see engine::input)
    bool is_pressed;              // Just pressed this frame
    bool is_released;             // Just released this frame
    bool is_down;                 // Currently held
    bool ctrl, shift, alt;        // Modifiers
};

class UIElement {
public:
    virtual bool OnKeyPress(const KeyboardEvent& event) { return false; }
    virtual bool OnKeyRelease(const KeyboardEvent& event) { return false; }
    
    bool ProcessKeyboardEvent(const KeyboardEvent& event);
};
```

**Focus Management**: Only focused element receives keyboard input.

---

#### 3. Focus Events
```cpp
class UIElement {
public:
    virtual void OnFocus() {}     // Called when element gains focus
    virtual void OnBlur() {}      // Called when element loses focus
    
    void SetFocused(bool focused) {
        if (focused == is_focused_) return;
        is_focused_ = focused;
        focused ? OnFocus() : OnBlur();
    }
};
```

---

#### 4. Lifecycle Events
```cpp
class UIElement {
public:
    virtual void OnAdded() {}     // Called when added to parent
    virtual void OnRemoved() {}   // Called when removed from parent
    virtual void OnShow() {}      // Called when made visible
    virtual void OnHide() {}      // Called when hidden
};
```

---

### Event Source Integration

#### Window Events
```cpp
class UIWindow {
public:
    void OnResize(int new_width, int new_height) {
        // Notify all root UI elements
        for (auto& element : root_elements_) {
            element->UpdateLayout();
        }
    }
    
    void OnMinimize() {
        // Pause animations
    }
    
    void OnMaximize() {
        // Resume animations
    }
};
```

---

#### Game Events
```cpp
class Game {
public:
    void OnFacilityBuilt(FacilityID id) {
        // Update build menu
        build_menu_->NotifyFacilityBuilt(id);
        
        // Update research tree
        research_tree_->UpdateAvailableResearch();
        
        // Show notification
        notification_center_->ShowNotification("Facility built!");
    }
};
```

---

#### Timer Events
```cpp
class UIElement {
protected:
    void ScheduleCallback(float delay_seconds, std::function<void()> callback) {
        timer_manager_->Schedule(delay_seconds, callback);
    }
};

// Usage: Auto-hide tooltip after 5 seconds
tooltip_->ScheduleCallback(5.0f, [this]() {
    tooltip_->Hide();
});
```

---

#### Animation Events
```cpp
class Panel {
public:
    void Show(bool animate = true) {
        if (animate) {
            StartShowAnimation();
            OnAnimationComplete([this]() {
                OnShow();  // Lifecycle event
            });
        } else {
            is_visible_ = true;
            OnShow();
        }
    }
};
```

---

### Callback Wiring Best Practices

#### 1. Wire Callbacks in Constructor
```cpp
// CORRECT: Callbacks set up immediately
Constructor() {
    auto button = std::make_unique<Button>(...);
    button->SetClickCallback([this]() {
        HandleButtonClick();
    });
    panel_->AddChild(std::move(button));
}
```

#### 2. Store Raw Pointers for Access
```cpp
// CORRECT: Ownership in parent, access via raw pointer
Constructor() {
    auto slider = std::make_unique<Slider>(...);
    master_volume_slider_ = slider.get();  // Store raw pointer
    panel_->AddChild(std::move(slider));   // Transfer ownership
}

void SetMasterVolume(float volume) {
    master_volume_slider_->SetValue(volume);  // Access via raw pointer
}
```

#### 3. Capture by Value for Immutable Data
```cpp
// CORRECT: Capture by value for primitives/copies
for (const auto& item : items) {
    auto button = std::make_unique<Button>(...);
    button->SetClickCallback([this, action = item.action]() {
        ExecuteAction(action);  // 'action' is a copy
    });
    panel_->AddChild(std::move(button));
}
```

#### 4. Use std::weak_ptr for Long-Lived Callbacks
```cpp
// CORRECT: Avoid dangling pointers
auto self = shared_from_this();
timer_->Schedule(5.0f, [self_weak = std::weak_ptr(self)]() {
    if (auto self = self_weak.lock()) {
        self->DoSomething();
    }
});
```

#### 5. Complete Callback Wiring
```cpp
// CORRECT: Components wired up immediately
Constructor() {
    action_bar_ = std::make_unique<ActionBar>(...);
    build_menu_ = std::make_unique<BuildMenu>(...);
    
    // Wire up: ActionBar "Build" button shows BuildMenu
    action_bar_->SetActionCallback([this](ActionBar::Action action) {
        if (action == ActionBar::Action::Build) {
            build_menu_->Show();
        }
    });
}
```

**Key Lesson**: Integration requires connecting callbacks between components. Create and wire immediately.

---

## Ownership & Memory Management

### The Golden Rule

> **One owner, many observers.**  
> **`std::unique_ptr` for ownership, raw pointers for access.**

---

### Ownership Patterns

#### Pattern 1: Parent Owns Children
```cpp
class Panel {
private:
    std::vector<std::unique_ptr<UIElement>> children_;  // OWNS
    
public:
    void AddChild(std::unique_ptr<UIElement> child) {
        child->SetParent(this);
        children_.push_back(std::move(child));  // Transfer ownership
    }
};
```

---

#### Pattern 2: Store Raw Pointers for Access
```cpp
class Menu {
private:
    std::unique_ptr<Panel> root_panel_;      // OWNS
    Button* ok_button_;                      // ACCESSES (non-owning)
    
public:
    Constructor() {
        root_panel_ = std::make_unique<Panel>(...);
        
        auto button = std::make_unique<Button>(...);
        ok_button_ = button.get();           // Store raw pointer
        root_panel_->AddChild(std::move(button));  // Transfer ownership
    }
    
    void EnableOkButton() {
        ok_button_->SetEnabled(true);        // Access via raw pointer
    }
};
```

**Rationale**: Parent owns the `unique_ptr`, children never outlive parent. Raw pointer is safe for access.

---

#### Pattern 3: Shared Ownership (Rare)
```cpp
// Use std::shared_ptr only when multiple owners needed
class ResourceCache {
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    
public:
    std::shared_ptr<Texture> GetTexture(const std::string& name) {
        return textures_[name];  // Multiple UI elements can share
    }
};
```

**When to use `shared_ptr`:**
- Resources (textures, fonts) shared across many components
- Async operations that outlive the caller
- **Not for parent-child relationships!**

---

### Common Ownership Mistakes & Solutions

#### Issue 1: Moving Between Containers
**Problem**: Moving `unique_ptr` between containers invalidates iterators.

```cpp
// Solution: Swap in place instead of moving
void Reorder() {
    std::swap(children_[0], children_[1]);
}
```

---

#### Issue 2: Dangling Raw Pointers
**Problem**: Raw pointer outlives the owned object.

```cpp
// Solution: Return unique_ptr or store in member
std::unique_ptr<Button> CreateButton() {
    return std::make_unique<Button>(...);  // Caller takes ownership
}
```

#### Issue 3: Circular References
**Problem**: Parent and child both use `shared_ptr` to each other.

```cpp
// Solution: Parent owns child, child stores weak_ptr to parent
class Child {
    std::weak_ptr<Parent> parent_;  // Breaks cycle
};
```

**Key Lesson**: Mixing `unique_ptr` moves between containers breaks ownership. Use swap/indices instead.

---

### RAII for UI Resources

```cpp
class Panel {
public:
    Panel() {
        // Acquire resources in constructor
        texture_ = LoadTexture("panel_bg.png");
    }
    
    ~Panel() {
        // Release in destructor (RAII)
        UnloadTexture(texture_);
    }
    
    // Delete copy (prevent double-free)
    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;
    
    // Allow move (transfer ownership)
    Panel(Panel&& other) noexcept
        : texture_(other.texture_) {
        other.texture_ = {};  // Nullify moved-from object
    }
    
private:
    Texture2D texture_;
};
```

---

## Layout & Positioning

### Coordinate Systems

#### Relative Coordinates
Each element stores position relative to parent.

```cpp
// Child at (10, 20) relative to parent
auto child = std::make_unique<Button>(10, 20, 100, 50);
parent->AddChild(std::move(child));
```

#### Absolute Coordinates
Computed on-demand by walking up the tree.

```cpp
Rectangle UIElement::GetAbsoluteBounds() const {
    Rectangle bounds = GetRelativeBounds();
    if (parent_) {
        Rectangle parent_bounds = parent_->GetAbsoluteBounds();
        bounds.x += parent_bounds.x;
        bounds.y += parent_bounds.y;
    }
    return bounds;
}
```

---

### Layout Invalidation

Only recalculate layout when necessary:
- Window resize
- Content change (add/remove children)
- Explicit `UpdateLayout()` call

```cpp
class Panel {
public:
    void UpdateLayout() {
        if (!layout_dirty_) return;
        
        // Recalculate child positions
        float y = padding_;
        for (auto& child : children_) {
            child->SetRelativePosition(padding_, y);
            y += child->GetHeight() + spacing_;
        }
        
        layout_dirty_ = false;
    }
    
    void AddChild(std::unique_ptr<UIElement> child) {
        children_.push_back(std::move(child));
        layout_dirty_ = true;  // Mark for recalculation
    }
    
    void Render() const {
        const_cast<Panel*>(this)->UpdateLayout();  // Lazy update
        // ...
    }
};
```

---

### Dynamic Positioning Requirements

**Challenge**: Setting position once isn't enough for dynamic UIs (window resize, animations).

**Solution**: Call `Update()` or `UpdateLayout()` each frame or on resize events.

```cpp
// CORRECT: Position updated on events
void OnWindowResize() {
    UpdateLayout();
}

void UpdateLayout() {
    int screen_width = GetScreenWidth();
    panel_->SetRelativePosition(screen_width / 2 - panel_->GetWidth() / 2, 100);
}

void Update(float delta_time) {
    UpdateLayout();  // Or call only when needed
    panel_->Update(delta_time);
}
```

**Key Lesson**: Dynamic UI positioning requires `Update()` calls, not one-time setup.

---

### Window Resize Pattern

**Problem**: UI elements positioned relative to screen edges don't reposition when window resizes.

**Solution**: Track screen dimensions and reposition on change.

```cpp
class UIWindowManager {
private:
    int last_screen_width_;
    int last_screen_height_;
    bool is_info_window_;  // Flag for windows that need recentering
    
public:
    UIWindowManager() 
        : last_screen_width_(0)
        , last_screen_height_(0)
        , is_info_window_(false) {}
    
    void Update(float delta_time) {
        const int current_width = GetScreenWidth();
        const int current_height = GetScreenHeight();
        
        // Detect resize
        if (current_width != last_screen_width_ || 
            current_height != last_screen_height_) {
            
            last_screen_width_ = current_width;
            last_screen_height_ = current_height;
            
            // Reposition windows that need it
            if (is_info_window_ && !windows_.empty()) {
                for (const auto& window : windows_) {
                    RepositionWindow(window.get());
                }
            }
        }
        
        // Update all windows
        for (const auto& window : windows_) {
            window->Update(delta_time);
        }
    }
    
    void RepositionWindow(UIWindow* window) {
        // Recenter at bottom with margin
        const int x = (last_screen_width_ - window->GetWidth()) / 2;
        const int y = last_screen_height_ - window->GetHeight() - BOTTOM_MARGIN;
        window->SetPosition(x, y);
    }
};
```

**When to Use**:
- Modals/dialogs centered on screen
- HUD elements anchored to edges (minimap, health bar)
- Info windows positioned relative to screen dimensions
- Action bars at bottom of screen

**Benefits**:
- Windows stay positioned correctly on resize
- Only repositions when needed (not every frame)
- Works with fullscreen toggles and window dragging

**Key Lesson**: For screen-relative positioning, track screen dimensions and reposition on change.

---

### Common Layout Patterns

#### Centering
```cpp
void CenterInParent(UIElement* element) {
    Rectangle parent_bounds = element->GetParent()->GetRelativeBounds();
    float x = (parent_bounds.width - element->GetWidth()) / 2;
    float y = (parent_bounds.height - element->GetHeight()) / 2;
    element->SetRelativePosition(x, y);
}
```

#### Anchoring
```cpp
void AnchorToBottom(UIElement* element) {
    Rectangle parent_bounds = element->GetParent()->GetRelativeBounds();
    float y = parent_bounds.height - element->GetHeight() - padding_;
    element->SetRelativePosition(element->GetRelativeX(), y);
}
```

#### Stacking (Vertical)
```cpp
void StackVertically(std::vector<UIElement*>& elements, float spacing) {
    float y = 0;
    for (auto* element : elements) {
        element->SetRelativePosition(0, y);
        y += element->GetHeight() + spacing;
    }
}
```

---

## Creating New Components

### Step-by-Step Implementation

#### 1. Extend UIElement
```cpp
class MyComponent : public UIElement {
public:
    MyComponent(float x, float y, float width, float height)
        : UIElement(x, y, width, height) {}
};
```

#### 2. Define Events
```cpp
class MyComponent : public UIElement {
public:
    using EventCallback = std::function<void(EventData)>;
    void SetEventCallback(EventCallback callback) { callback_ = callback; }
    
private:
    EventCallback callback_;
};
```

#### 3. Implement Rendering
```cpp
void Render() const override {
    using namespace engine::ui::batch_renderer;
    
    // Submit background quad
    BatchRenderer::SubmitQuad(GetAbsoluteBounds(), background_color_);
    
    // Submit content (text, icons, etc)
    // ...
    
    // Render children (if composite)
    for (const auto& child : children_) {
        child->Render();
    }
}
```

#### 4. Handle Input
```cpp
bool OnClick(const MouseEvent& event) override {
    if (!Contains(event.x, event.y)) return false;
    
    // Update state
    is_selected_ = !is_selected_;
    
    // Notify observers
    if (callback_) callback_(is_selected_);
    
    return true;  // Event consumed
}
```

#### 5. Support Keyboard Navigation
```cpp
void OnFocus() override {
    // Visual feedback
    border_color_ = GOLD;
}

void OnBlur() override {
    border_color_ = GRAY;
}

bool OnKeyPress(const KeyboardEvent& event) override {
    if (event.key == KEY_ENTER || event.key == KEY_SPACE) {
        // Trigger action
        if (callback_) callback_(GetData());
        return true;
    }
    return false;
}
```

#### 6. Accessibility Support
```cpp
void Render() const override {
    using namespace engine::ui::batch_renderer;
    
    // Respect accessibility settings
    const auto& settings = AccessibilitySettings::Get();
    
    float font_scale = settings.GetFontScale();
    int font_size = base_font_size_ * font_scale;
    
    Color text_color = settings.IsHighContrast() 
        ? Color{1.0f, 1.0f, 1.0f, 1.0f}  // WHITE
        : text_color_;
    
    BatchRenderer::SubmitText(label_, position.x, position.y, font_size, text_color);
}
```

#### 7. Animation Support (Optional)
```cpp
void Update(float delta_time) {
    if (is_animating_) {
        animation_time_ += delta_time;
        float progress = std::min(animation_time_ / duration_, 1.0f);
        
        // Apply easing
        float eased = EaseOutCubic(progress);
        current_value_ = Lerp(start_value_, end_value_, eased);
        
        if (progress >= 1.0f) {
            is_animating_ = false;
        }
    }
}
```

#### 8. Testing
```cpp
// Unit test component in isolation
TEST(MyComponentTest, ClickTriggersCallback) {
    bool callback_fired = false;
    MyComponent component(0, 0, 100, 50);
    component.SetEventCallback([&](EventData data) {
        callback_fired = true;
    });
    
    MouseEvent click_event{50, 25, true, false, true, false, 0};
    component.OnClick(click_event);
    
    EXPECT_TRUE(callback_fired);
}
```

---

## Testing & Validation

### Unit Testing Components

```cpp
// Test component in isolation
TEST(ButtonTest, ClickTriggersCallback) {
    bool clicked = false;
    Button button(0, 0, 100, 50, "Test");
    button.SetClickCallback([&]() { clicked = true; });
    
    MouseEvent event{50, 25, false, false, true, false, 0};
    button.OnClick(event);
    
    EXPECT_TRUE(clicked);
}

TEST(ButtonTest, ClickOutsideBoundsDoesNotTrigger) {
    bool clicked = false;
    Button button(0, 0, 100, 50, "Test");
    button.SetClickCallback([&]() { clicked = true; });
    
    MouseEvent event{200, 200, false, false, true, false, 0};
    button.OnClick(event);
    
    EXPECT_FALSE(clicked);
}

TEST(SliderTest, DragUpdatesValue) {
    Slider slider(0, 0, 200, 50, 0.0f, 1.0f);
    
    MouseEvent drag{100, 25, true, false, false, false, 0};
    slider.OnClick(drag);
    
    EXPECT_FLOAT_EQ(slider.GetValue(), 0.5f);
}
```

---

### Integration Testing

```cpp
// Test component interactions
TEST(MenuTest, ButtonClickShowsPanel) {
    TestMenu menu;
    Panel* panel = menu.GetPanel();
    EXPECT_FALSE(panel->IsVisible());
    
    Button* button = menu.GetButton();
    MouseEvent click{50, 25, false, false, true, false, 0};
    button->OnClick(click);
    
    EXPECT_TRUE(panel->IsVisible());
}
```

---

### Visual Regression Testing

```cpp
// Capture screenshot, compare to baseline
TEST(VisualTest, MainMenuAppearance) {
    MainMenu menu;
    menu.Render();
    
    Image screenshot = CaptureScreen();
    Image baseline = LoadImage("baseline_main_menu.png");
    
    EXPECT_TRUE(ImagesEqual(screenshot, baseline));
}
```

---

### Accessibility Testing

```cpp
TEST(AccessibilityTest, AllButtonsKeyboardNavigable) {
    Menu menu;
    std::vector<Button*> buttons = menu.GetAllButtons();
    
    for (auto* button : buttons) {
        button->SetFocused(true);
        KeyboardEvent enter_key{KEY_ENTER, true, false, true};
        
        bool handled = button->OnKeyPress(enter_key);
        EXPECT_TRUE(handled) << "Button '" << button->GetLabel() << "' not keyboard accessible";
    }
}

TEST(AccessibilityTest, ContrastRatioSufficient) {
    Button button(0, 0, 100, 50, "Test");
    Color bg = button.GetBackgroundColor();
    Color fg = button.GetTextColor();
    
    float contrast = CalculateContrastRatio(bg, fg);
    EXPECT_GE(contrast, 4.5f) << "WCAG AA requires 4.5:1 contrast";
}
```

---

## Performance Considerations

### Batch Rendering

```cpp
// ✅ GOOD: BatchRenderer automatically batches similar draw calls
void Panel::Render() const {
    using namespace engine::ui::batch_renderer;
    
    // BatchRenderer automatically batches these submissions
    for (const auto& child : children_) {
        child->Render();  // Each child submits vertices
    }
    // All vertices are automatically batched and submitted efficiently
}

// Note: The BatchRenderer handles batching internally - you don't need to
// manually collect and batch primitives. Just submit them and let the
// renderer optimize the GPU submissions.
```

---

### Culling Off-Screen Elements

```cpp
void Render() const override {
    Rectangle screen_bounds{0, 0, GetScreenWidth(), GetScreenHeight()};
    Rectangle my_bounds = GetAbsoluteBounds();
    
    // Skip rendering if completely off-screen
    if (!CheckCollisionRecs(screen_bounds, my_bounds)) {
        return;
    }
    
    // Render normally
    // ...
}
```

---

### Lazy Updates

```cpp
void Panel::Render() const {
    // Only update layout when dirty
    if (layout_dirty_) {
        const_cast<Panel*>(this)->UpdateLayout();
    }
    
    // Render
    // ...
}
```

---

### Object Pooling

```cpp
// Reuse notification objects instead of allocating new ones
class NotificationCenter {
private:
    std::vector<std::unique_ptr<Notification>> pool_;
    std::vector<Notification*> active_;
    
public:
    void ShowNotification(const std::string& text) {
        Notification* notif = nullptr;
        
        // Try to reuse from pool
        if (!pool_.empty()) {
            notif = pool_.back().get();
            pool_.pop_back();
        } else {
            pool_.push_back(std::make_unique<Notification>());
            notif = pool_.back().get();
        }
        
        notif->SetText(text);
        notif->Show();
        active_.push_back(notif);
    }
    
    void Update(float delta_time) {
        // Return finished notifications to pool
        active_.erase(
            std::remove_if(active_.begin(), active_.end(),
                [this](Notification* n) {
                    if (n->IsFinished()) {
                        pool_.push_back(std::unique_ptr<Notification>(n));
                        return true;
                    }
                    return false;
                }),
            active_.end()
        );
    }
};
```

---

## Summary: The UI Commandments

1. **Declare once, render many** - Build UI in constructors, not every frame
2. **React to events** - Update state only when things change
3. **Observe, don't poll** - Use callbacks, not return values
4. **Compose components** - Build complex UIs from simple parts
5. **Own clearly** - `unique_ptr` for ownership, raw pointers for access
6. **Layout lazily** - Recalculate only on resize or content change
7. **Block modals** - Overlays must consume ALL events when visible
8. **Wire immediately** - Connect callbacks right after component creation
9. **Test in isolation** - Components should work without full game
10. **Respect accessibility** - High contrast, font scaling, keyboard nav

---

## Quick Reference

### Creating a Button
```cpp
auto button = std::make_unique<Button>(x, y, width, height, "Label");
button->SetClickCallback([this]() { HandleClick(); });
panel_->AddChild(std::move(button));
```

### Creating a Modal
```cpp
class MyModal : public Panel {
public:
    bool ProcessMouseEvent(const MouseEvent& event) override {
        if (!IsVisible()) return false;
        Panel::ProcessMouseEvent(event);
        return true;  // Block all events when visible
    }
};
```

### Centering an Element
```cpp
void UpdateLayout() {
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    float x = (screen_width - width_) / 2;
    float y = (screen_height - height_) / 2;
    panel_->SetRelativePosition(x, y);
}
```

### Keyboard Navigation
```cpp
void OnKeyPress(const KeyboardEvent& event) {
    if (event.key == KEY_TAB) {
        FocusNextElement();
    } else if (event.key == KEY_ENTER) {
        ActivateFocusedElement();
    }
}
```

### Complete Component Example
```cpp
class CustomButton : public UIElement {
public:
    using ClickCallback = std::function<void()>;
    
    CustomButton(float x, float y, float width, float height, const std::string& label)
        : UIElement(x, y, width, height)
        , label_(label)
        , callback_(nullptr) {}
    
    void Render() const override {
        using namespace engine::ui::batch_renderer;
        
        Rectangle bounds = GetAbsoluteBounds();
        Color bg_color = is_focused_ 
            ? Color{1.0f, 0.84f, 0.0f, 1.0f}   // GOLD
            : Color{0.5f, 0.5f, 0.5f, 1.0f};   // GRAY
        
        BatchRenderer::SubmitQuad(bounds, bg_color);
        BatchRenderer::SubmitText(
            label_.c_str(), 
            bounds.x + 10, 
            bounds.y + 10, 
            20, 
            Color{1.0f, 1.0f, 1.0f, 1.0f}  // WHITE
        );
    }
    
    bool OnClick(const MouseEvent& event) override {
        if (!Contains(event.x, event.y)) return false;
        if (callback_) callback_();
        return true;
    }
    
    void SetClickCallback(ClickCallback callback) { callback_ = callback; }
    
private:
    std::string label_;
    ClickCallback callback_;
};
```

---

## Key Lessons from Production

1. **Panel child management** requires careful ownership - use `unique_ptr` for storage, raw pointers for access
2. **Modal overlays** must explicitly block events to lower layers via `return true` in `ProcessMouseEvent()`
3. **Dynamic UI positioning** requires `Update()` calls for window resize and animations
4. **Window resize handling** requires tracking screen dimensions and repositioning on change - screen-relative elements won't update automatically
5. **Integration** isn't complete until callbacks are wired - connect components immediately after creation
6. **Vertex-based rendering** - Components submit vertices via BatchRenderer, not manage their own buffers
7. **Automatic batching** - BatchRenderer handles optimization; just submit primitives in render order

---

## Architecture Summary

**Citrus Engine UI System**:
- **Production System**: Custom vertex-based batch renderer (declarative, reactive, event-driven)
- **Vertex-based rendering** via `BatchRenderer` for high performance
- **Automatic batching** minimizes GPU draw calls
- **Scissor clipping** for panels and scroll regions
- **ImGui**: ONLY for temporary debugging/testing - NOT production UI

**Key APIs**:
- `BatchRenderer::SubmitQuad()` - Submit filled rectangles
- `BatchRenderer::SubmitLine()` - Submit lines with thickness
- `BatchRenderer::SubmitText()` - Submit text rendering
- `BatchRenderer::PushScissor()` / `PopScissor()` - Hierarchical clipping
- `BatchRenderer::BeginFrame()` / `EndFrame()` - Frame lifecycle

**Component Pattern**:
```cpp
class MyComponent : public UIElement {
    void Render() const override {
        using namespace engine::ui::batch_renderer;
        BatchRenderer::SubmitQuad(GetBounds(), background_color_);
        // Render children
        UIElement::Render();
    }
};
```

---

**End of Citrus Engine UI Development Bible**

*Reference for AI agents implementing declarative, reactive, event-driven UI components using vertex-based batch rendering*
