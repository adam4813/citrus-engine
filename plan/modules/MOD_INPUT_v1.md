# MOD_INPUT_v1

> **Cross-Platform Input System with Action Mapping - Foundation Module**

## Executive Summary

The `engine.input` module provides a comprehensive cross-platform input system with action mapping, device abstraction,
and event-driven architecture enabling seamless user interaction across desktop and WebAssembly platforms. This module
implements keyboard, mouse, and gamepad support with configurable key bindings, input buffering, and real-time input
processing to deliver responsive controls for the Colony Game Engine's complex simulation interface requiring precise
camera control, entity selection, and UI interaction.

## Scope and Objectives

### In Scope

- [ ] Cross-platform input device abstraction (keyboard, mouse, gamepad)
- [ ] Action mapping system with configurable key bindings
- [ ] Input event buffering and real-time processing
- [ ] Mouse cursor management and custom cursor support
- [ ] Input context switching for different game states
- [ ] Accessibility features and input customization

### Out of Scope

- [ ] Touch input for mobile platforms
- [ ] Advanced haptic feedback systems
- [ ] Voice input and speech recognition
- [ ] Eye tracking and alternative input methods

### Primary Objectives

1. **Low Latency**: Input response time under 2ms for critical actions
2. **Cross-Platform Consistency**: Identical input behavior on Windows/Linux/WebAssembly
3. **Flexibility**: Support custom key bindings and action remapping

### Secondary Objectives

- Zero input lag for mouse movement and camera control
- Support for 4+ simultaneous gamepad controllers
- Hot-swappable input configurations during runtime

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: No direct entity management; provides input data for other systems
- **Entity Queries**: N/A - Input system operates as a service layer for other systems
- **Component Dependencies**: Provides input events for systems handling player interaction

#### Component Design

```cpp
// Input-related components for ECS integration (used by other systems)
struct InputReceiver {
    std::bitset<256> subscribed_actions; // Which input actions this entity responds to
    InputPriority priority{InputPriority::Normal};
    bool is_active{true};
};

struct CameraController {
    float mouse_sensitivity{1.0f};
    float scroll_sensitivity{1.0f};
    Vec2 rotation_limits{-90.0f, 90.0f}; // Min/max pitch
    bool invert_y{false};
};

struct PlayerInput {
    Vec2 movement_input{0.0f, 0.0f};
    Vec2 camera_delta{0.0f, 0.0f};
    bool is_selecting{false};
    bool is_context_menu{false};
    std::optional<Vec2> mouse_position;
};

// Component traits for input
template<>
struct ComponentTraits<InputReceiver> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;
};

template<>
struct ComponentTraits<PlayerInput> {
    static constexpr bool is_trivially_copyable = true;
    static constexpr size_t max_instances = 4; // Multiple players
};
```

#### System Integration

- **System Dependencies**: Runs first in update cycle to provide input data for all systems
- **Component Access Patterns**: Write-only to PlayerInput components; reads input device state
- **Inter-System Communication**: Broadcasts input events to subscribed systems

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Input batch - executes on main thread with minimal processing time
- **Thread Safety**: Input event queue is thread-safe for cross-thread event posting
- **Data Dependencies**: No dependencies on other systems; provides input for all systems

#### Parallel Execution Strategy

```cpp
// Thread-safe input processing with minimal main thread impact
class InputSystem : public ISystem {
public:
    // Main thread input collection and event generation
    void ProcessInputEvents(const ComponentManager& components,
                           std::span<EntityId> entities,
                           const ThreadContext& context) override;
    
    // Background input device polling (separate thread)
    void PollInputDevices(const ThreadContext& context);
    
    // Thread-safe event submission from platform callbacks
    void SubmitInputEvent(const InputEvent& event);

private:
    struct InputEvent {
        InputEventType type;
        InputDevice device;
        union {
            KeyboardEvent keyboard;
            MouseEvent mouse;
            GamepadEvent gamepad;
        } data;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    // Lock-free queue for input events
    moodycamel::ConcurrentQueue<InputEvent> event_queue_;
    std::atomic<bool> input_polling_active_{false};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Input events stored in contiguous queue for efficient processing
- **Memory Ordering**: Event queue uses acquire-release semantics for thread safety
- **Lock-Free Sections**: Input event queuing and action mapping are lock-free

### Public APIs

#### Primary Interface: `InputSystemInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <functional>

namespace engine::input {

template<typename T>
concept InputHandler = requires(T t, const InputEvent& event) {
    { t(event) } -> std::convertible_to<bool>;
};

class InputSystemInterface {
public:
    [[nodiscard]] auto Initialize(const InputConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Action mapping and binding
    void MapAction(std::string_view action_name, const InputBinding& binding);
    void UnmapAction(std::string_view action_name);
    [[nodiscard]] auto GetActionBinding(std::string_view action_name) const -> std::optional<InputBinding>;
    
    // Input state queries
    [[nodiscard]] auto IsActionPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto IsActionJustPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto IsActionJustReleased(std::string_view action_name) const -> bool;
    [[nodiscard]] auto GetActionValue(std::string_view action_name) const -> float;
    
    // Mouse and cursor management
    [[nodiscard]] auto GetMousePosition() const -> Vec2;
    [[nodiscard]] auto GetMouseDelta() const -> Vec2;
    void SetCursorMode(CursorMode mode);
    void SetCustomCursor(AssetId cursor_texture);
    
    // Input context management
    void PushInputContext(std::string_view context_name);
    void PopInputContext();
    void SetActiveInputContext(std::string_view context_name);
    
    // Event system
    template<InputHandler H>
    [[nodiscard]] auto RegisterInputHandler(std::string_view action_name, H&& handler) -> CallbackId;
    void UnregisterInputHandler(CallbackId id);
    
    // Device management
    [[nodiscard]] auto GetConnectedGamepads() const -> std::vector<GamepadId>;
    [[nodiscard]] auto IsGamepadConnected(GamepadId id) const -> bool;
    
    // Configuration and persistence
    void SaveInputBindings(std::string_view file_path) const;
    void LoadInputBindings(std::string_view file_path);
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<InputSystemImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::input
```

#### Scripting Interface Requirements

```cpp
// Input scripting interface for dynamic input handling
class InputScriptInterface {
public:
    // Type-safe input queries from scripts
    [[nodiscard]] auto IsActionPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto IsActionJustPressed(std::string_view action_name) const -> bool;
    [[nodiscard]] auto GetActionValue(std::string_view action_name) const -> float;
    
    // Mouse and cursor queries
    [[nodiscard]] auto GetMousePosition() const -> std::pair<float, float>;
    [[nodiscard]] auto GetMouseDelta() const -> std::pair<float, float>;
    
    // Input binding management
    void BindAction(std::string_view action_name, std::string_view key_name);
    void UnbindAction(std::string_view action_name);
    
    // Input context control
    void SetInputContext(std::string_view context_name);
    [[nodiscard]] auto GetActiveInputContext() const -> std::string;
    
    // Event registration
    [[nodiscard]] auto OnActionPressed(std::string_view action_name, 
                                      std::string_view callback_function) -> bool;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<InputSystemInterface> input_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **Action Mapping**: Support configurable key bindings with save/load functionality
- [ ] **Multi-Device**: Simultaneously handle keyboard, mouse, and multiple gamepads
- [ ] **Cross-Platform**: Consistent input behavior on Windows, Linux, and WebAssembly

### Performance Requirements

- [ ] **Latency**: Input response time under 2ms for critical actions
- [ ] **Throughput**: Process 1000+ input events per frame without frame drops
- [ ] **Memory**: Input system memory usage under 16MB including event buffers
- [ ] **CPU**: Input processing under 1% CPU usage on target hardware

### Quality Requirements

- [ ] **Reliability**: Zero input events lost during normal operation
- [ ] **Maintainability**: All input code covered by automated tests
- [ ] **Testability**: Mock input devices for deterministic testing
- [ ] **Documentation**: Complete input binding reference and customization guide

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(InputTest, LatencyRequirement) {
    auto input_system = engine::input::InputSystem{};
    input_system.Initialize(InputConfig{});
    
    // Simulate key press and measure response time
    auto start = std::chrono::high_resolution_clock::now();
    SimulateKeyPress(KeyCode::Space);
    
    // Process input events
    input_system.Update(0.016f);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();
    
    EXPECT_LT(latency, 2000); // Under 2ms (2000 microseconds)
    EXPECT_TRUE(input_system.IsActionJustPressed("jump"));
}

TEST(InputTest, ActionMapping) {
    auto input_system = engine::input::InputSystem{};
    input_system.Initialize(InputConfig{});
    
    // Test action mapping
    InputBinding jump_binding{KeyCode::Space, InputModifier::None};
    input_system.MapAction("jump", jump_binding);
    
    auto retrieved_binding = input_system.GetActionBinding("jump");
    ASSERT_TRUE(retrieved_binding.has_value());
    EXPECT_EQ(retrieved_binding->key_code, KeyCode::Space);
    
    // Test action remapping
    InputBinding new_jump_binding{KeyCode::W, InputModifier::None};
    input_system.MapAction("jump", new_jump_binding);
    
    SimulateKeyPress(KeyCode::W);
    input_system.Update(0.016f);
    EXPECT_TRUE(input_system.IsActionPressed("jump"));
    
    SimulateKeyPress(KeyCode::Space);
    input_system.Update(0.016f);
    EXPECT_FALSE(input_system.IsActionPressed("jump"));
}

TEST(InputTest, InputContextSwitching) {
    auto input_system = engine::input::InputSystem{};
    input_system.Initialize(InputConfig{});
    
    // Set up contexts with different bindings
    input_system.SetActiveInputContext("game");
    input_system.MapAction("select", InputBinding{MouseButton::Left, InputModifier::None});
    
    input_system.SetActiveInputContext("menu");
    input_system.MapAction("select", InputBinding{KeyCode::Enter, InputModifier::None});
    
    // Test context switching
    input_system.SetActiveInputContext("game");
    SimulateMouseClick(MouseButton::Left);
    input_system.Update(0.016f);
    EXPECT_TRUE(input_system.IsActionPressed("select"));
    
    input_system.SetActiveInputContext("menu");
    SimulateMouseClick(MouseButton::Left);
    input_system.Update(0.016f);
    EXPECT_FALSE(input_system.IsActionPressed("select"));
    
    SimulateKeyPress(KeyCode::Enter);
    input_system.Update(0.016f);
    EXPECT_TRUE(input_system.IsActionPressed("select"));
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Input System (Estimated: 4 days)

- [ ] **Task 1.1**: Implement basic input device abstraction and event system
- [ ] **Task 1.2**: Add keyboard and mouse input processing
- [ ] **Task 1.3**: Create action mapping system with key binding support
- [ ] **Deliverable**: Basic keyboard and mouse input working

#### Phase 2: Advanced Features (Estimated: 3 days)

- [ ] **Task 2.1**: Add gamepad support with multiple controller handling
- [ ] **Task 2.2**: Implement input context switching system
- [ ] **Task 2.3**: Add mouse cursor management and custom cursors
- [ ] **Deliverable**: Full input feature set operational

#### Phase 3: Configuration System (Estimated: 2 days)

- [ ] **Task 3.1**: Implement input binding save/load functionality
- [ ] **Task 3.2**: Add runtime input configuration and remapping
- [ ] **Task 3.3**: Create default input presets and validation
- [ ] **Deliverable**: Complete input customization system

#### Phase 4: Platform Integration (Estimated: 2 days)

- [ ] **Task 4.1**: Optimize WebAssembly input handling
- [ ] **Task 4.2**: Add platform-specific input optimizations
- [ ] **Task 4.3**: Implement accessibility features and input assist
- [ ] **Deliverable**: Production-ready cross-platform input

### File Structure

```
src/engine/input/
├── input.cppm                  // Primary module interface
├── input_system.cpp           // Core input management
├── input_device_manager.cpp   // Device detection and management
├── action_mapper.cpp          // Key binding and action mapping
├── input_context_manager.cpp  // Context switching and state
├── input_event_processor.cpp  // Event processing and distribution
├── devices/
│   ├── keyboard_device.cpp    // Keyboard input handling
│   ├── mouse_device.cpp       // Mouse input and cursor management
│   └── gamepad_device.cpp     // Gamepad input and rumble
├── platforms/
│   ├── windows_input.cpp      // Windows-specific input handling
│   ├── linux_input.cpp        // Linux-specific input handling
│   └── wasm_input.cpp          // WebAssembly input handling
└── tests/
    ├── input_system_tests.cpp
    ├── action_mapping_tests.cpp
    └── input_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::input`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with platform-specific device implementations
- **Build Integration**: Links with platform-specific input libraries

### Testing Strategy

- **Unit Tests**: Mock input devices for deterministic input simulation
- **Integration Tests**: Real input device testing with automated input injection
- **Performance Tests**: Latency measurement and event throughput testing
- **Platform Tests**: Cross-platform input consistency validation

## Risk Assessment

### Technical Risks

| Risk                           | Probability | Impact | Mitigation                                                |
|--------------------------------|-------------|--------|-----------------------------------------------------------|
| **Platform Input Differences** | High        | Medium | Extensive cross-platform testing, input abstraction layer |
| **Input Lag Issues**           | Medium      | High   | Real-time input processing, performance monitoring        |
| **Device Compatibility**       | Medium      | Medium | Device detection, fallback input methods                  |
| **WebAssembly Limitations**    | High        | Low    | Feature detection, graceful degradation                   |

### Integration Risks

- **ECS Performance**: Risk that input processing impacts frame rate
    - *Mitigation*: Efficient event processing, input batching
- **Platform Differences**: Risk of inconsistent input behavior across platforms
    - *Mitigation*: Comprehensive input abstraction, platform-specific testing

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (window management, timing)
    - engine.ecs (component access for input receivers)
    - engine.assets (cursor textures, input configuration files)

- **Optional Systems**:
    - engine.profiling (input latency monitoring)

### External Dependencies

- **Standard Library Features**: C++20 atomic operations, bitset, functional
- **Platform APIs**: Windows Input API, Linux evdev, Web Gamepad API

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.assets
- **vcpkg Packages**: No additional dependencies for core functionality
- **Platform-Specific**: Different input libraries per platform
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.assets

### Asset Pipeline Dependencies

- **Configuration Files**: Input binding configuration in JSON format
- **Cursor Assets**: Custom cursor textures and animations
- **Resource Loading**: Input configuration loaded through asset system
