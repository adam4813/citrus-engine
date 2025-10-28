# Citrus Engine ImGui UI Development Guide

**AI Agent Reference: Building immediate mode user interfaces with ImGui**

**Purpose**: Technical reference for AI agents implementing UI with ImGui  
**Audience**: AI assistants only (not user documentation)  
**Status**: Active

---

## Table of Contents

1. [Philosophy & Core Principles](#philosophy--core-principles)
2. [ImGui Basics](#imgui-basics)
3. [Window Management](#window-management)
4. [Layout Patterns](#layout-patterns)
5. [Input Handling](#input-handling)
6. [Styling and Theming](#styling-and-theming)
7. [Custom Widgets](#custom-widgets)
8. [Performance Considerations](#performance-considerations)
9. [Best Practices](#best-practices)
10. [Quick Reference](#quick-reference)

---

## Philosophy & Core Principles

### Immediate Mode vs Retained Mode

ImGui uses an **immediate mode** approach, fundamentally different from retained mode UI systems:

```cpp
// IMMEDIATE MODE (ImGui) - Code runs every frame
void RenderUI() {
    if (ImGui::Begin("Game Menu")) {
        if (ImGui::Button("Start Game")) {
            StartGame();  // Action executed immediately
        }
        ImGui::Text("Score: %d", score);
    }
    ImGui::End();
}

// Called every frame
void Update() {
    RenderUI();  // UI code executes every frame
}
```

**Key Principles**:
- **No state management**: UI code runs every frame, no need to track widget state
- **Direct data binding**: UI reads/writes application data directly
- **Simple control flow**: Use normal if statements and loops
- **Automatic layout**: Widgets automatically position themselves

---

### Benefits of Immediate Mode

1. **Simplicity**: No complex event wiring or callbacks
2. **Flexibility**: Easy to conditionally show/hide UI elements
3. **Debugging**: Set breakpoints in UI code like any other code
4. **Tight coupling**: UI naturally reflects current application state

```cpp
// Easy conditional UI
void RenderUI() {
    if (gameState == GameState::Playing) {
        ImGui::Text("Time: %.1f", gameTimer);
        if (ImGui::Button("Pause")) {
            Pause();
        }
    } else if (gameState == GameState::Paused) {
        if (ImGui::Button("Resume")) {
            Resume();
        }
    }
}
```

---

## ImGui Basics

### Initialization

```cpp
// In engine initialization
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();

// Configure flags
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

// Setup platform bindings
ImGui_ImplGlfw_InitForOpenGL(window, true);
ImGui_ImplOpenGL3_Init("#version 300 es");

// Setup style
ImGui::StyleColorsDark();
```

### Frame Structure

```cpp
void RenderFrame() {
    // Start new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render your UI
    RenderGameUI();

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
```

### Basic Widgets

```cpp
// Text
ImGui::Text("Hello, World!");
ImGui::Text("Score: %d", score);

// Buttons
if (ImGui::Button("Click Me")) {
    // Button was clicked
}

// Checkboxes
bool enabled = true;
ImGui::Checkbox("Enable Feature", &enabled);

// Sliders
float volume = 0.5f;
ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);

// Input fields
char buffer[256] = "";
ImGui::InputText("Name", buffer, sizeof(buffer));

// Combo boxes
const char* items[] = { "Option 1", "Option 2", "Option 3" };
int selected = 0;
ImGui::Combo("Choose", &selected, items, IM_ARRAYSIZE(items));
```

---

## Window Management

### Creating Windows

```cpp
void RenderUI() {
    // Simple window
    ImGui::Begin("My Window");
    ImGui::Text("Content here");
    ImGui::End();

    // Window with options
    ImGui::Begin("Settings", nullptr, 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize);
    // Content
    ImGui::End();

    // Check if window is open
    bool show_window = true;
    if (ImGui::Begin("Closeable Window", &show_window)) {
        ImGui::Text("This window can be closed");
    }
    ImGui::End();
}
```

### Window Flags

```cpp
ImGuiWindowFlags flags = 0;
flags |= ImGuiWindowFlags_NoTitleBar;      // No title bar
flags |= ImGuiWindowFlags_NoResize;        // Cannot resize
flags |= ImGuiWindowFlags_NoMove;          // Cannot move
flags |= ImGuiWindowFlags_NoCollapse;      // Cannot collapse
flags |= ImGuiWindowFlags_NoScrollbar;     // No scrollbar
flags |= ImGuiWindowFlags_AlwaysAutoResize; // Auto-resize to content

ImGui::Begin("Window", nullptr, flags);
```

### Child Windows

```cpp
// Create scrollable region
ImGui::BeginChild("ScrollRegion", ImVec2(0, 300), true);
for (int i = 0; i < 100; i++) {
    ImGui::Text("Item %d", i);
}
ImGui::EndChild();
```

---

## Layout Patterns

### Horizontal Layout

```cpp
// Same line layout
ImGui::Text("Label:");
ImGui::SameLine();
ImGui::Button("Button");

// Multiple elements on same line
ImGui::Button("One");
ImGui::SameLine();
ImGui::Button("Two");
ImGui::SameLine();
ImGui::Button("Three");
```

### Columns

```cpp
// Multi-column layout
ImGui::Columns(3, "mycolumns");
ImGui::Separator();

// Column 1
ImGui::Text("Column 1");
ImGui::NextColumn();

// Column 2
ImGui::Text("Column 2");
ImGui::NextColumn();

// Column 3
ImGui::Text("Column 3");
ImGui::NextColumn();

ImGui::Columns(1); // End columns
```

### Tables

```cpp
// Modern table API (ImGui 1.80+)
if (ImGui::BeginTable("table1", 3)) {
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Value");
    ImGui::TableSetupColumn("Actions");
    ImGui::TableHeadersRow();

    for (int row = 0; row < 10; row++) {
        ImGui::TableNextRow();
        
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Item %d", row);
        
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", row * 10);
        
        ImGui::TableSetColumnIndex(2);
        if (ImGui::Button("Delete")) {
            // Handle delete
        }
    }
    ImGui::EndTable();
}
```

### Groups and Spacing

```cpp
// Group elements together
ImGui::BeginGroup();
ImGui::Text("Grouped");
ImGui::Button("Button 1");
ImGui::Button("Button 2");
ImGui::EndGroup();

// Add spacing
ImGui::Spacing();
ImGui::Separator();
ImGui::NewLine();

// Custom spacing
ImGui::Dummy(ImVec2(0, 20)); // 20 pixels vertical space
```

---

## Input Handling

### Mouse Input

```cpp
// Mouse position
ImVec2 mouse_pos = ImGui::GetMousePos();

// Mouse buttons
if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    // Left mouse button clicked
}

if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    // Right mouse button held
}

// Hover detection
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("This is a tooltip");
}
```

### Keyboard Input

```cpp
// Check for keyboard input
if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
    // Space key pressed
}

if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
    // Ctrl key held
}

// Text input focus
if (ImGui::IsItemFocused()) {
    // Input field has focus
}
```

### Drag and Drop

```cpp
// Drag source
if (ImGui::BeginDragDropSource()) {
    ImGui::SetDragDropPayload("ITEM_TYPE", &item_data, sizeof(item_data));
    ImGui::Text("Dragging item");
    ImGui::EndDragDropSource();
}

// Drop target
if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ITEM_TYPE")) {
        // Handle drop
        auto* data = (ItemType*)payload->Data;
    }
    ImGui::EndDragDropTarget();
}
```

---

## Styling and Theming

### Colors

```cpp
// Push color for next widget
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
ImGui::Button("Red Button");
ImGui::PopStyleColor();

// Multiple colors
ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
ImGui::Button("Green Button");
ImGui::PopStyleColor(2);
```

### Style Variables

```cpp
// Push style variable
ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
ImGui::Button("Rounded Button");
ImGui::PopStyleVar();

// Multiple variables
ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
// Widgets here
ImGui::PopStyleVar(2);
```

### Custom Theme

```cpp
void SetupCustomTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    
    // Rounding
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    
    // Spacing
    style.ItemSpacing = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 4);
}
```

---

## Custom Widgets

### Creating Custom Widgets

```cpp
// Simple custom widget
bool CustomToggle(const char* label, bool* value) {
    bool changed = false;
    
    ImGui::PushID(label);
    
    // Draw custom UI
    if (ImGui::Button(*value ? "ON" : "OFF")) {
        *value = !*value;
        changed = true;
    }
    ImGui::SameLine();
    ImGui::Text("%s", label);
    
    ImGui::PopID();
    
    return changed;
}

// Usage
bool enabled = true;
if (CustomToggle("Feature", &enabled)) {
    // Value changed
}
```

### Custom Drawing

```cpp
// Get draw list for custom rendering
ImDrawList* draw_list = ImGui::GetWindowDrawList();

// Draw primitives
ImVec2 pos = ImGui::GetCursorScreenPos();
draw_list->AddRect(pos, ImVec2(pos.x + 100, pos.y + 50), 
    IM_COL32(255, 255, 0, 255));

draw_list->AddCircleFilled(ImVec2(pos.x + 50, pos.y + 25), 
    20, IM_COL32(255, 0, 0, 255));

draw_list->AddLine(pos, ImVec2(pos.x + 100, pos.y + 50), 
    IM_COL32(0, 255, 0, 255), 2.0f);

// Reserve space for custom drawing
ImGui::Dummy(ImVec2(100, 50));
```

---

## Performance Considerations

### Optimization Tips

1. **Minimize window creation**: Reuse windows instead of creating new ones
2. **Use IDs properly**: Use `ImGui::PushID()` / `ImGui::PopID()` for dynamic content
3. **Avoid unnecessary rendering**: Use window flags to disable unused features
4. **Cache calculations**: Don't recalculate static values every frame

```cpp
// Good: Cache static strings
static const char* items[] = { "Item 1", "Item 2", "Item 3" };

// Good: Use IDs for dynamic lists
for (int i = 0; i < items.size(); i++) {
    ImGui::PushID(i);
    if (ImGui::Button("Delete")) {
        // Delete item i
    }
    ImGui::PopID();
}
```

### Conditional UI

```cpp
// Only render when needed
if (show_debug_window) {
    ImGui::Begin("Debug");
    // Debug UI here
    ImGui::End();
}

// Use window collapse state
if (ImGui::CollapsingHeader("Advanced Settings")) {
    // Only execute when expanded
    RenderAdvancedSettings();
}
```

---

## Best Practices

### Code Organization

```cpp
// Organize UI code into functions
void RenderMainMenu() {
    if (ImGui::Begin("Main Menu")) {
        if (ImGui::Button("Start Game")) {
            StartGame();
        }
        if (ImGui::Button("Settings")) {
            show_settings = true;
        }
        if (ImGui::Button("Quit")) {
            Quit();
        }
    }
    ImGui::End();
}

void RenderSettings() {
    if (show_settings) {
        if (ImGui::Begin("Settings", &show_settings)) {
            ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
            ImGui::Checkbox("Fullscreen", &fullscreen);
        }
        ImGui::End();
    }
}

void RenderUI() {
    RenderMainMenu();
    RenderSettings();
}
```

### State Management

```cpp
// Keep UI state in application classes
class Game {
public:
    void RenderUI() {
        if (ImGui::Begin("Game UI")) {
            ImGui::Text("Score: %d", score_);
            ImGui::Text("Lives: %d", lives_);
            
            if (ImGui::Button("Pause")) {
                paused_ = !paused_;
            }
        }
        ImGui::End();
    }
    
private:
    int score_ = 0;
    int lives_ = 3;
    bool paused_ = false;
};
```

### Error Handling

```cpp
// Always check Begin() return value for collapsed windows
if (ImGui::Begin("Window")) {
    // Only render contents if window is visible
    RenderWindowContents();
}
ImGui::End(); // Always call End() even if Begin() returned false
```

---

## Quick Reference

### Common Widgets

```cpp
// Text
ImGui::Text("Simple text");
ImGui::TextColored(ImVec4(1,0,0,1), "Colored text");
ImGui::BulletText("Bullet point");

// Buttons
ImGui::Button("Button");
ImGui::SmallButton("Small");
ImGui::ArrowButton("arrow", ImGuiDir_Left);
ImGui::RadioButton("Radio", &value, 1);

// Input
ImGui::InputText("Text", buffer, size);
ImGui::InputInt("Integer", &value);
ImGui::InputFloat("Float", &value);

// Sliders
ImGui::SliderInt("Int", &value, 0, 100);
ImGui::SliderFloat("Float", &value, 0.0f, 1.0f);

// Selection
ImGui::Checkbox("Check", &checked);
ImGui::Combo("Combo", &selected, items, count);
ImGui::ListBox("List", &selected, items, count);

// Colors
ImGui::ColorEdit3("Color", color);
ImGui::ColorPicker3("Picker", color);
```

### Layout Helpers

```cpp
ImGui::Spacing();           // Add spacing
ImGui::Separator();         // Horizontal line
ImGui::NewLine();          // Line break
ImGui::SameLine();         // Next widget on same line
ImGui::Indent();           // Increase indent
ImGui::Unindent();         // Decrease indent
```

### Window Positioning

```cpp
// Set window position
ImGui::SetNextWindowPos(ImVec2(100, 100));

// Set window size
ImGui::SetNextWindowSize(ImVec2(400, 300));

// Set window focus
ImGui::SetNextWindowFocus();

// Center window
ImVec2 center = ImGui::GetMainViewport()->GetCenter();
ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
```

---

## Summary: ImGui Best Practices

1. **Keep it simple** - Let ImGui handle layout and state
2. **Direct data binding** - Read/write application data directly
3. **Use IDs** - `PushID()`/`PopID()` for dynamic content
4. **Organize code** - Break UI into logical functions
5. **Conditional rendering** - Only show what's needed
6. **Consistent styling** - Use themes and style variables
7. **Performance** - Cache static data, minimize calculations
8. **Always call End()** - Match every Begin() with End()

---

**End of ImGui UI Development Guide**

*Reference for AI agents implementing UI with ImGui in citrus-engine*
