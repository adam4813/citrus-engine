# Citrus Engine UI Development Bible

**AI Agent Reference: Building declarative, reactive, event-driven user interfaces**

**Purpose**: Technical reference for AI agents implementing UI components for citrus-engine  
**Audience**: AI assistants only (not user documentation)  
**Status**: Active

**Note**: ImGui is included as a dependency for debugging/development tools only. The production UI system is a custom retained-mode architecture built on the engine's sprite rendering system.

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
        auto sprite = engine::ui::Sprite{};
        sprite.texture = item.texture_id;
        sprite.position = glm::vec2{x, y};
        sprite.size = glm::vec2{width, height};
        sprite.color = item.normal_color;
        sprite.layer = item.layer;
        
        ui_elements_.push_back(sprite);
        callbacks_[sprite_index] = [this, action = item.action]() {
            ExecuteAction(action);
        };
    }
}

void Render() {
    auto& ui_renderer = engine::ui::UIRenderer::GetInstance();
    for (const auto& sprite : ui_elements_) {
        ui_renderer.AddSprite(sprite);
    }
    ui_renderer.Render();
}
```

**Benefits**: Easier to reason about, test, and optimize. State changes are explicit, not implicit.

---

### Reactive Updates

Visual state changes **only** in response to events, never continuously.

```cpp
// CORRECT: Update on events only
void OnSelectionChanged(int new_index) {
    ui_elements_[old_index_].color = GRAY_COLOR;
    ui_elements_[new_index].color = GOLD_COLOR;
    old_index_ = new_index;
}

void Render() {
    // Sprites already have updated properties
    auto& ui_renderer = engine::ui::UIRenderer::GetInstance();
    for (const auto& sprite : ui_elements_) {
        ui_renderer.AddSprite(sprite);
    }
}
```

**Benefits**: 60x performance improvement over per-frame updates. UI updates when it needs to, not "just in case."

---

### Event-Driven Communication

Components notify observers via callbacks, never return magic values.

```cpp
// CORRECT: Observer pattern with callbacks
Constructor() {
    for (size_t i = 0; i < button_sprites_.size(); ++i) {
        button_callbacks_[i] = [this, action = actions_[i]]() {
            if (on_action_callback_) {
                on_action_callback_(action);
            }
        };
    }
}

void SetActionCallback(std::function<void(Action)> callback) {
    on_action_callback_ = callback;
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

Build complex UIs from simple sprite-based components using composition and ECS patterns.

```cpp
// CORRECT: Compose complex UI from sprites and data
class GameMenu {
    std::vector<engine::ui::Sprite> background_sprites_;
    std::vector<engine::ui::Sprite> button_sprites_;
    std::vector<engine::ui::Sprite> icon_sprites_;
    std::unordered_map<size_t, std::function<void()>> button_callbacks_;
    
    // Input state
    glm::vec2 mouse_position_;
    bool mouse_clicked_;
};

Constructor() {
    // Build sprite hierarchy
    background_sprites_.push_back(CreateBackgroundSprite());
    
    for (const auto& button_def : button_definitions_) {
        auto sprite = engine::ui::Sprite{};
        sprite.texture = button_def.texture;
        sprite.position = button_def.position;
        sprite.size = button_def.size;
        sprite.layer = 1; // Above background
        button_sprites_.push_back(sprite);
    }
}
```

**Benefits**: Small, focused components are easier to test, debug, and reuse. Leverages the engine's sprite system and ECS architecture.

---

## Citrus Engine UI Architecture

### Sprite-Based Rendering System

The citrus-engine UI is built on a sprite-based rendering system that leverages the engine's core rendering pipeline:

```cpp
// UI Sprite structure (from engine.ui:uitypes)
struct Sprite {
    rendering::TextureId texture{0};
    glm::vec2 position{0.0F};     // Screen coordinates
    glm::vec2 size{1.0F};
    float rotation{0.0F};
    rendering::Color color{1.0F, 1.0F, 1.0F, 1.0F};
    glm::vec2 texture_offset{0.0F, 0.0F};
    glm::vec2 texture_scale{1.0F, 1.0F};
    int layer{0};                  // Z-order for layering
    glm::vec2 pivot{0.5F};        // 0,0 = bottom-left, 1,1 = top-right
    bool flip_x{false};
    bool flip_y{false};
};
```

### UIRenderer

The `UIRenderer` manages UI sprites and submits them to the main rendering system:

```cpp
// Initialize once
auto& ui_renderer = engine::ui::UIRenderer::GetInstance();
ui_renderer.Initialize();

// Each frame
ui_renderer.ClearSprites();

// Add UI sprites
engine::ui::Sprite button_sprite;
button_sprite.texture = button_texture_id;
button_sprite.position = glm::vec2{100.0f, 200.0f};
button_sprite.size = glm::vec2{150.0f, 50.0f};
button_sprite.layer = 10;
ui_renderer.AddSprite(button_sprite);

// Render all UI sprites
ui_renderer.Render();
```

### Batch Renderer (Advanced)

For high-performance UI rendering with many elements, use the `BatchRenderer`:

```cpp
// One-time initialization
engine::ui::batch_renderer::BatchRenderer::Initialize();

// Each frame
BatchRenderer::BeginFrame();

// Submit quads directly (no sprite objects needed)
BatchRenderer::SubmitQuad(
    position, size, color, 
    texture_id, uv_min, uv_max
);

// Scissor clipping for scrollable regions
BatchRenderer::PushScissor(scissor_rect);
// ... render clipped content ...
BatchRenderer::PopScissor();

BatchRenderer::EndFrame(); // Flushes batches to GPU
```

**Key Features**:
- Batches draw calls to minimize GPU submissions
- Supports up to 8 simultaneous texture units
- Scissor rectangle clipping for panels and scroll regions
- Automatic vertex buffer management

---

## Gang of Four Design Patterns

### 1. Observer Pattern ⭐⭐⭐

**Purpose**: Decouple UI events from business logic.

**When to Use**: Any time a UI component needs to notify external code of user interactions.

**Implementation**:
```cpp
class Button {
public:
    using ClickCallback = std::function<void()>;
    
    void SetClickCallback(ClickCallback callback) { 
        click_callback_ = callback; 
    }
    
    void HandleMouseClick(const glm::vec2& mouse_pos) {
        if (IsPointInside(mouse_pos)) {
            if (click_callback_) {
                click_callback_();
            }
        }
    }
    
private:
    engine::ui::Sprite sprite_;
    ClickCallback click_callback_;
    
    bool IsPointInside(const glm::vec2& point) const {
        // AABB collision detection
        return point.x >= sprite_.position.x && 
               point.x <= sprite_.position.x + sprite_.size.x &&
               point.y >= sprite_.position.y && 
               point.y <= sprite_.position.y + sprite_.size.y;
    }
};

// Usage
Button start_button;
start_button.SetClickCallback([this]() {
    StartGame();
});
```

---

### 2. State Pattern ⭐⭐

**Purpose**: Change sprite appearance based on UI element state.

**When to Use**: Buttons, toggles, or any interactive element with multiple visual states.

**Implementation**:
```cpp
class Button {
public:
    enum class State {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };
    
    void SetState(State new_state) {
        if (state_ == new_state) return;
        
        state_ = new_state;
        UpdateSpriteAppearance();
    }
    
    void Update(const glm::vec2& mouse_pos, bool mouse_down) {
        bool is_hovered = IsPointInside(mouse_pos);
        
        if (state_ == State::Disabled) return;
        
        if (mouse_down && is_hovered) {
            SetState(State::Pressed);
        } else if (is_hovered) {
            SetState(State::Hovered);
        } else {
            SetState(State::Normal);
        }
    }
    
private:
    void UpdateSpriteAppearance() {
        switch (state_) {
            case State::Normal:
                sprite_.color = normal_color_;
                break;
            case State::Hovered:
                sprite_.color = hover_color_;
                break;
            case State::Pressed:
                sprite_.color = pressed_color_;
                break;
            case State::Disabled:
                sprite_.color = disabled_color_;
                break;
        }
    }
    
    engine::ui::Sprite sprite_;
    State state_{State::Normal};
    rendering::Color normal_color_;
    rendering::Color hover_color_;
    rendering::Color pressed_color_;
    rendering::Color disabled_color_;
};
```

---

### 3. Strategy Pattern ⭐

**Purpose**: Encapsulate interchangeable layout algorithms.

**When to Use**: Multiple ways to position/arrange UI elements (horizontal, vertical, grid layouts).

**Implementation**:
```cpp
class ILayoutStrategy {
public:
    virtual ~ILayoutStrategy() = default;
    virtual void Layout(std::vector<engine::ui::Sprite>& sprites, 
                       const glm::vec2& container_pos,
                       const glm::vec2& container_size) = 0;
};

class HorizontalLayout : public ILayoutStrategy {
    void Layout(std::vector<engine::ui::Sprite>& sprites,
               const glm::vec2& container_pos,
               const glm::vec2& container_size) override {
        float x_offset = container_pos.x;
        for (auto& sprite : sprites) {
            sprite.position = glm::vec2{x_offset, container_pos.y};
            x_offset += sprite.size.x + spacing_;
        }
    }
private:
    float spacing_{10.0f};
};

class GridLayout : public ILayoutStrategy {
    void Layout(std::vector<engine::ui::Sprite>& sprites,
               const glm::vec2& container_pos,
               const glm::vec2& container_size) override {
        int col = 0;
        int row = 0;
        for (auto& sprite : sprites) {
            sprite.position = glm::vec2{
                container_pos.x + col * (cell_width_ + spacing_),
                container_pos.y + row * (cell_height_ + spacing_)
            };
            
            if (++col >= columns_) {
                col = 0;
                ++row;
            }
        }
    }
private:
    int columns_{3};
    float cell_width_{100.0f};
    float cell_height_{100.0f};
    float spacing_{5.0f};
};
```

---

### 4. Command Pattern ⭐

**Purpose**: Encapsulate UI actions as objects for undo/redo functionality.

**When to Use**: Actions that need to be undoable or recorded.

**Implementation**:
```cpp
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class ToggleSettingCommand : public ICommand {
public:
    ToggleSettingCommand(bool& setting) : setting_(setting) {}
    
    void Execute() override {
        setting_ = !setting_;
    }
    
    void Undo() override {
        setting_ = !setting_;
    }
    
private:
    bool& setting_;
};

class CommandHistory {
public:
    void ExecuteCommand(std::unique_ptr<ICommand> command) {
        command->Execute();
        undo_stack_.push(std::move(command));
        // Clear redo stack when new command is executed
        while (!redo_stack_.empty()) {
            redo_stack_.pop();
        }
    }
    
    void Undo() {
        if (!undo_stack_.empty()) {
            auto command = std::move(undo_stack_.top());
            undo_stack_.pop();
            command->Undo();
            redo_stack_.push(std::move(command));
        }
    }
    
    void Redo() {
        if (!redo_stack_.empty()) {
            auto command = std::move(redo_stack_.top());
            redo_stack_.pop();
            command->Execute();
            undo_stack_.push(std::move(command));
        }
    }
    
private:
    std::stack<std::unique_ptr<ICommand>> undo_stack_;
    std::stack<std::unique_ptr<ICommand>> redo_stack_;
};
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

#### Button
**Visual**: Rectangular sprite with text/icon, responds to hover and click.  
**Use Cases**: Primary actions, navigation, confirmations.

```cpp
class Button {
public:
    Button(const glm::vec2& pos, const glm::vec2& size, 
           rendering::TextureId texture_id) {
        sprite_.texture = texture_id;
        sprite_.position = pos;
        sprite_.size = size;
        sprite_.layer = 10;
        state_ = State::Normal;
    }
    
    void Update(const glm::vec2& mouse_pos, bool mouse_clicked) {
        bool hovered = IsPointInside(mouse_pos);
        
        if (mouse_clicked && hovered && click_callback_) {
            click_callback_();
        }
        
        SetState(hovered ? State::Hovered : State::Normal);
    }
    
    void Render(engine::ui::UIRenderer& renderer) {
        renderer.AddSprite(sprite_);
    }
    
    void SetClickCallback(std::function<void()> callback) {
        click_callback_ = callback;
    }
    
private:
    engine::ui::Sprite sprite_;
    State state_;
    std::function<void()> click_callback_;
};
```

---

#### Panel/Container
**Visual**: Rectangular background that groups related elements.  
**Use Cases**: Organizing UI sections, modal dialogs, scroll regions.

```cpp
class Panel {
public:
    Panel(const glm::vec2& pos, const glm::vec2& size) {
        background_.texture = 0; // Solid color
        background_.position = pos;
        background_.size = size;
        background_.color = rendering::Color{0.2f, 0.2f, 0.2f, 0.9f};
        background_.layer = 0; // Behind child elements
    }
    
    void AddChild(std::unique_ptr<Button> child) {
        children_.push_back(std::move(child));
    }
    
    void Render(engine::ui::UIRenderer& renderer) {
        renderer.AddSprite(background_);
        for (const auto& child : children_) {
            child->Render(renderer);
        }
    }
    
private:
    engine::ui::Sprite background_;
    std::vector<std::unique_ptr<Button>> children_;
};
```

---

#### Slider
**Visual**: Track with movable handle for value selection.  
**Use Cases**: Volume controls, settings adjustments.

```cpp
class Slider {
public:
    Slider(const glm::vec2& pos, float width, 
           float min_val, float max_val) 
        : position_(pos), width_(width),
          min_value_(min_val), max_value_(max_val) {
        
        // Track sprite
        track_.position = pos;
        track_.size = glm::vec2{width, 4.0f};
        track_.color = rendering::Color{0.5f, 0.5f, 0.5f, 1.0f};
        track_.layer = 5;
        
        // Handle sprite
        handle_.position = pos;
        handle_.size = glm::vec2{16.0f, 16.0f};
        handle_.color = rendering::Color{1.0f, 1.0f, 1.0f, 1.0f};
        handle_.layer = 6;
        
        SetValue((min_val + max_val) / 2.0f);
    }
    
    void SetValue(float value) {
        value_ = std::clamp(value, min_value_, max_value_);
        float t = (value_ - min_value_) / (max_value_ - min_value_);
        handle_.position.x = position_.x + t * width_ - handle_.size.x / 2.0f;
    }
    
    float GetValue() const { return value_; }
    
    void Update(const glm::vec2& mouse_pos, bool mouse_down) {
        if (mouse_down && IsDragging(mouse_pos)) {
            float t = (mouse_pos.x - position_.x) / width_;
            SetValue(min_value_ + t * (max_value_ - min_value_));
            
            if (on_value_changed_) {
                on_value_changed_(value_);
            }
        }
    }
    
    void Render(engine::ui::UIRenderer& renderer) {
        renderer.AddSprite(track_);
        renderer.AddSprite(handle_);
    }
    
private:
    engine::ui::Sprite track_;
    engine::ui::Sprite handle_;
    glm::vec2 position_;
    float width_;
    float value_;
    float min_value_;
    float max_value_;
    std::function<void(float)> on_value_changed_;
    
    bool IsDragging(const glm::vec2& mouse_pos) const {
        // Check if mouse is near the handle or track
        return mouse_pos.x >= position_.x &&
               mouse_pos.x <= position_.x + width_ &&
               mouse_pos.y >= position_.y - 10.0f &&
               mouse_pos.y <= position_.y + 14.0f;
    }
};
```

---

#### Toggle/Checkbox
**Visual**: Switch with on/off states or checkbox sprite.  
**Use Cases**: Boolean settings, feature flags.

```cpp
class Toggle {
public:
    Toggle(const glm::vec2& pos, 
           rendering::TextureId off_tex,
           rendering::TextureId on_tex) {
        sprite_.position = pos;
        sprite_.size = glm::vec2{32.0f, 32.0f};
        sprite_.layer = 10;
        off_texture_ = off_tex;
        on_texture_ = on_tex;
        SetEnabled(false);
    }
    
    void SetEnabled(bool enabled) {
        enabled_ = enabled;
        sprite_.texture = enabled_ ? on_texture_ : off_texture_;
    }
    
    bool IsEnabled() const { return enabled_; }
    
    void Update(const glm::vec2& mouse_pos, bool mouse_clicked) {
        if (mouse_clicked && IsPointInside(mouse_pos)) {
            SetEnabled(!enabled_);
            if (on_toggle_) {
                on_toggle_(enabled_);
            }
        }
    }
    
    void Render(engine::ui::UIRenderer& renderer) {
        renderer.AddSprite(sprite_);
    }
    
    void SetToggleCallback(std::function<void(bool)> callback) {
        on_toggle_ = callback;
    }
    
private:
    engine::ui::Sprite sprite_;
    bool enabled_{false};
    rendering::TextureId off_texture_;
    rendering::TextureId on_texture_;
    std::function<void(bool)> on_toggle_;
    
    bool IsPointInside(const glm::vec2& point) const {
        return point.x >= sprite_.position.x &&
               point.x <= sprite_.position.x + sprite_.size.x &&
               point.y >= sprite_.position.y &&
               point.y <= sprite_.position.y + sprite_.size.y;
    }
};
```

---
    bool IsEnabled() const;
    void SetToggleCallback(ToggleCallback callback);
};
```

---

## Input Handling

### Mouse Input

UI components should handle mouse input by checking if the mouse position is within their bounds:

```cpp
bool IsPointInside(const glm::vec2& point, const engine::ui::Sprite& sprite) {
    return point.x >= sprite.position.x &&
           point.x <= sprite.position.x + sprite.size.x &&
           point.y >= sprite.position.y &&
           point.y <= sprite.position.y + sprite.size.y;
}

// In your UI component's Update method
void Update(const glm::vec2& mouse_pos, bool mouse_clicked) {
    bool hovered = IsPointInside(mouse_pos, sprite_);
    
    if (hovered) {
        // Update visual state for hover
        sprite_.color = hover_color_;
        
        if (mouse_clicked && on_click_) {
            on_click_();
        }
    } else {
        sprite_.color = normal_color_;
    }
}
```

### Keyboard Input

For keyboard navigation and shortcuts, use the engine's input system:

```cpp
#include "engine/input/input.hpp"

void HandleInput() {
    auto& input = engine::input::Input::GetInstance();
    
    if (input.IsKeyPressed(engine::input::Key::Escape)) {
        CloseMenu();
    }
    
    if (input.IsKeyPressed(engine::input::Key::Enter)) {
        if (focused_button_) {
            focused_button_->Click();
        }
    }
    
    // Tab navigation
    if (input.IsKeyPressed(engine::input::Key::Tab)) {
        FocusNextElement();
    }
}
```

---

## Performance Optimization

### Use Batch Rendering for Many Elements

When rendering many UI elements (>100), use `BatchRenderer` instead of individual sprite submissions:

```cpp
// GOOD: Batch renderer for many elements
BatchRenderer::BeginFrame();
for (const auto& item : inventory_items) {  // 1000s of items
    BatchRenderer::SubmitQuad(
        item.position, item.size, item.color,
        item.texture_id, item.uv_min, item.uv_max
    );
}
BatchRenderer::EndFrame();  // Single GPU submission

// AVOID: Individual sprites for many elements
for (const auto& item : inventory_items) {
    ui_renderer.AddSprite(item.sprite);  // Many individual submissions
}
```

### Layer-Based Culling

Only render UI elements that are visible (not occluded by higher layers):

```cpp
// Sort sprites by layer before rendering
std::sort(sprites_.begin(), sprites_.end(),
    [](const auto& a, const auto& b) { return a.layer < b.layer; });

// Only render what's visible
for (const auto& sprite : sprites_) {
    if (IsVisible(sprite)) {
        renderer.AddSprite(sprite);
    }
}
```

### Cache Sprite Data

Don't recreate sprites every frame - update only what changes:

```cpp
// GOOD: Update only changed properties
void OnHoverStateChanged(bool hovered) {
    sprite_.color = hovered ? hover_color_ : normal_color_;
}

// AVOID: Recreating sprite every frame
void Render() {
    auto sprite = engine::ui::Sprite{};  // Don't do this
    sprite.texture = texture_;
    sprite.position = position_;
    // ...
}
```

---

## Best Practices Summary

1. **Sprite-based**: Build UI from `engine::ui::Sprite` objects, not custom widget hierarchies
2. **Retained mode**: Declare UI structure once, update properties reactively
3. **Layering**: Use `layer` property to control render order (higher = on top)
4. **Batch rendering**: Use `BatchRenderer` for high element counts
5. **Event-driven**: Update sprites only when state changes, not every frame
6. **Observer pattern**: Use callbacks for loose coupling between UI and game logic
7. **Input handling**: Check mouse bounds manually, integrate with engine's input system
8. **Performance**: Cache sprites, sort by layer, use scissor clipping for scroll regions
9. **ImGui for tools only**: Production UI uses sprite system, ImGui for debug/dev tools

---

## Key Lessons for Citrus Engine UI

1. **Sprite-based architecture** - UI elements are built from `engine::ui::Sprite` objects submitted to the renderer
2. **Retained mode** - UI structure is declared once and updated reactively, not rebuilt every frame
3. **Batch rendering** - Use `BatchRenderer` for high-performance rendering of many UI elements
4. **Layer management** - Use sprite `layer` property to control render order (Z-order)
5. **Event-driven updates** - Update sprite properties only when state changes, not continuously
6. **Callback patterns** - Use Observer pattern for loose coupling between UI and game logic
7. **ImGui for debugging only** - ImGui is a dependency for development tools, not production UI

---

## Quick Reference

### Creating a Button

```cpp
class Button {
    engine::ui::Sprite sprite_;
    std::function<void()> on_click_;
    State state_{State::Normal};
    
public:
    Button(glm::vec2 pos, glm::vec2 size, rendering::TextureId texture) {
        sprite_.texture = texture;
        sprite_.position = pos;
        sprite_.size = size;
        sprite_.layer = 10;
    }
    
    void SetClickCallback(std::function<void()> callback) {
        on_click_ = callback;
    }
    
    void Update(const glm::vec2& mouse_pos, bool mouse_clicked) {
        if (mouse_clicked && IsPointInside(mouse_pos) && on_click_) {
            on_click_();
        }
    }
    
    void Render(engine::ui::UIRenderer& renderer) {
        renderer.AddSprite(sprite_);
    }
};
```

### Using UIRenderer

```cpp
// Initialize (once)
auto& ui_renderer = engine::ui::UIRenderer::GetInstance();
ui_renderer.Initialize();

// Each frame
ui_renderer.ClearSprites();

// Add sprites
engine::ui::Sprite my_sprite;
my_sprite.texture = texture_id;
my_sprite.position = glm::vec2{100.0f, 200.0f};
my_sprite.size = glm::vec2{50.0f, 50.0f};
my_sprite.layer = 5;
ui_renderer.AddSprite(my_sprite);

// Render all
ui_renderer.Render();
```

### Using BatchRenderer

```cpp
// Initialize (once)
engine::ui::batch_renderer::BatchRenderer::Initialize();

// Each frame
BatchRenderer::BeginFrame();

// Submit quads
BatchRenderer::SubmitQuad(
    position,        // glm::vec2
    size,           // glm::vec2
    color,          // rendering::Color
    texture_id,     // uint32_t
    uv_min,         // glm::vec2
    uv_max          // glm::vec2
);

// Scissor clipping (optional)
BatchRenderer::PushScissor(scissor_rect);
// ... render clipped content ...
BatchRenderer::PopScissor();

// End frame (flushes all batches)
BatchRenderer::EndFrame();

// Shutdown (on exit)
BatchRenderer::Shutdown();
```

---

**End of Citrus Engine UI Development Bible**

*Reference for AI agents implementing retained-mode UI components in citrus-engine*
