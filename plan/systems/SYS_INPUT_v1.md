# SYS_INPUT_v1

> **System-Level Design Document for Cross-Platform Input Management**

## Executive Summary

This document defines a comprehensive input system for the modern C++20 game engine, designed to provide unified input
handling across Windows, Linux, and WebAssembly platforms while integrating seamlessly with the established threading
model and ECS architecture. The system emphasizes declarative action mapping, event-driven processing, and support for
multiple input devices (keyboard, mouse, controllers) through a single, type-safe API. The design accounts for
platform-specific constraints such as WebAssembly's limited input capabilities while maintaining identical high-level
behavior across all targets.

## Scope and Objectives

### In Scope

- [ ] Cross-platform input abstraction for keyboard, mouse, and controllers
- [ ] Declarative action mapping system using JSON configuration files
- [ ] Event-driven input processing with frame-coherent state updates
- [ ] ECS integration through input components and specialized input systems
- [ ] Integration with existing threading model (main thread events, render thread processing)
- [ ] WebAssembly-specific input handling with browser API limitations
- [ ] Hot-reload support for input configuration during development
- [ ] Input recording and playback for testing and debugging
- [ ] Accessibility features including key remapping and input assistance
- [ ] Performance profiling hooks for input latency measurement
- [ ] Platform abstraction integration for configuration file I/O and timing utilities

### Out of Scope

- [ ] Advanced haptic feedback beyond basic controller rumble
- [ ] Voice input or gesture recognition systems
- [ ] Eye tracking or biometric input devices
- [ ] Network input synchronization (future networking extension)
- [ ] Custom input device drivers or low-level hardware access
- [ ] Platform-specific input features not available across all targets
- [ ] Low-level file system operations for configuration (handled by Platform Abstraction system)

### Primary Objectives

1. **Cross-Platform Parity**: Identical input behavior between native and WebAssembly builds
2. **Low Latency**: Input-to-action latency under 8ms for responsive gameplay
3. **ECS Integration**: Seamless integration with component-based entity input handling
4. **Threading Safety**: Safe input processing within the established threading architecture
5. **Declarative Configuration**: JSON-based action mapping with hot-reload capabilities

### Secondary Objectives

- Comprehensive accessibility support for diverse player needs
- Input analytics and debugging tools for development workflow
- Future extensibility for additional input devices and platforms
- Memory-efficient operation suitable for WebAssembly heap constraints
- Integration with existing GLFW input handling without breaking changes

## Architecture/Design

### High-Level Overview

```
Input System Architecture Integration:

┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                            │
│  Game Input Handlers │ UI Input Processing │ Debug Commands    │
├─────────────────────────────────────────────────────────────────┤
│                    ECS Input Systems                            │
│  PlayerInputSystem  │ UIInputSystem │ DebugInputSystem │ Menu   │
├─────────────────────────────────────────────────────────────────┤
│                   Action Mapping Layer                          │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Action      │ │ Input       │ │ Context     │               │
│  │ Registry    │ │ Bindings    │ │ Management  │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
├─────────────────────────────────────────────────────────────────┤
│                   Input Event Processing                        │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │
│  │ Event       │ │ State       │ │ Frame       │               │
│  │ Buffering   │ │ Tracking    │ │ Coherency   │               │
│  └─────────────┘ └─────────────┘ └─────────────┘               │
├─────────────────────────────────────────────────────────────────┤
│              Platform Input Abstraction                         │
│  Native Platform    │         WebAssembly Platform             │
│                     │                                           │
│  ┌─────────────┐    │  ┌─────────────┐ ┌─────────────┐         │
│  │ GLFW        │    │  │ Browser     │ │ Pointer     │         │
│  │ Integration │    │  │ Events      │ │ Lock API    │         │
│  │             │    │  │             │ │             │         │
│  │ Controller  │    │  │ Gamepad API │ │ Keyboard    │         │
│  │ Support     │    │  │ (Limited)   │ │ Events      │         │
│  └─────────────┘    │  └─────────────┘ └─────────────┘         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Threading Model Integration                         │
│                                                                 │
│  Main Thread            │         Render Thread                 │
│  (GLFW Events)          │         (Input Processing)           │
│                         │                                       │
│  ┌─────────────┐        │  ┌─────────────┐                     │
│  │ glfwWaitEvts│        │  │ Input       │                     │
│  │             │        │  │ System      │                     │
│  │ Raw Input   │────────┼─▶│ Update()    │                     │
│  │ Capture     │        │  │             │                     │
│  │             │        │  │ Action      │                     │
│  │ Event Queue │        │  │ Processing  │                     │
│  └─────────────┘        │  └─────────────┘                     │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Platform Input Abstraction

- **Purpose**: Unified input device access hiding platform implementation differences
- **Responsibilities**: Raw input capture, device enumeration, event normalization, capability detection
- **Key Classes/Interfaces**: `InputDevice`, `PlatformInputProvider`, `InputEvent`, `DeviceCapabilities`
- **Data Flow**: Platform events → Normalization → Event queue → Action mapping

#### Component 2: Action Mapping System

- **Purpose**: Declarative input configuration with runtime binding modification
- **Responsibilities**: Action registration, binding resolution, context switching, configuration hot-reload
- **Key Classes/Interfaces**: `ActionRegistry`, `InputBinding`, `ActionContext`, `ConfigurationManager`
- **Data Flow**: Configuration files → Action registration → Runtime binding → Action triggers

#### Component 3: Event Processing Engine

- **Purpose**: Frame-coherent input state management and event buffering
- **Responsibilities**: Event buffering, state tracking, frame synchronization, input prediction
- **Key Classes/Interfaces**: `InputEventProcessor`, `InputState`, `FrameInputData`, `EventBuffer`
- **Data Flow**: Raw events → Buffering → Frame processing → State updates → Action dispatch

#### Component 4: ECS Integration Layer

- **Purpose**: Component-based input handling for entity-driven input processing
- **Responsibilities**: Input component management, system integration, entity input routing
- **Key Classes/Interfaces**: `InputComponent`, `PlayerInputSystem`, `InputActionComponent`
- **Data Flow**: Action triggers → Component queries → Entity processing → Component updates

### Platform Abstraction Implementation

#### Unified Input Device Interface

```cpp
// Abstract interface for all input devices
class InputDevice {
public:
    enum class Type : uint8_t {
        Keyboard,
        Mouse,
        Controller,
        Touch,      // Future WebAssembly support
        Unknown
    };
    
    enum class ConnectionState : uint8_t {
        Connected,
        Disconnected,
        Error
    };
    
    virtual ~InputDevice() = default;
    
    [[nodiscard]] virtual auto GetType() const -> Type = 0;
    [[nodiscard]] virtual auto GetDeviceId() const -> DeviceId = 0;
    [[nodiscard]] virtual auto GetConnectionState() const -> ConnectionState = 0;
    [[nodiscard]] virtual auto GetCapabilities() const -> const DeviceCapabilities& = 0;
    
    // Device-specific input polling
    virtual void UpdateState() = 0;
    virtual void Reset() = 0;
    
    // Event generation for the input system
    [[nodiscard]] virtual auto GetPendingEvents() -> std::vector<InputEvent> = 0;
    virtual void ClearEvents() = 0;
};

// Device capability description
struct DeviceCapabilities {
    struct ButtonCapability {
        uint32_t button_count = 0;
        bool supports_analog = false;
        bool supports_pressure = false;
    };
    
    struct AxisCapability {
        uint32_t axis_count = 0;
        float min_value = -1.0f;
        float max_value = 1.0f;
        bool supports_deadzone = true;
    };
    
    ButtonCapability buttons;
    AxisCapability axes;
    bool supports_rumble = false;
    bool supports_motion = false;
    std::string device_name;
    std::string manufacturer;
};
```

#### GLFW Integration for Native Platforms

```cpp
// GLFW-based input implementation for native platforms
class GLFWInputProvider final : public PlatformInputProvider {
public:
    explicit GLFWInputProvider(GLFWwindow* window) : window_(window) {
        SetupCallbacks();
        EnumerateDevices();
    }
    
    [[nodiscard]] auto GetDevices() const -> std::vector<std::unique_ptr<InputDevice>> override {
        std::vector<std::unique_ptr<InputDevice>> devices;
        
        // Always have keyboard and mouse
        devices.push_back(std::make_unique<GLFWKeyboard>(window_));
        devices.push_back(std::make_unique<GLFWMouse>(window_));
        
        // Add connected controllers
        for (const auto& controller : connected_controllers_) {
            devices.push_back(std::make_unique<GLFWController>(controller.joystick_id));
        }
        
        return devices;
    }
    
    void PollEvents() override {
        // GLFW events are already handled by main thread via glfwWaitEvents()
        // This method processes buffered events from callbacks
        ProcessBufferedEvents();
    }
    
private:
    GLFWwindow* window_;
    std::vector<ControllerInfo> connected_controllers_;
    std::mutex event_buffer_mutex_;
    std::vector<InputEvent> buffered_events_;
    
    void SetupCallbacks() {
        // Set GLFW callbacks that buffer events for later processing
        glfwSetWindowUserPointer(window_, this);
        
        glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            auto* provider = static_cast<GLFWInputProvider*>(glfwGetWindowUserPointer(window));
            provider->OnKeyEvent(key, scancode, action, mods);
        });
        
        glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int mods) {
            auto* provider = static_cast<GLFWInputProvider*>(glfwGetWindowUserPointer(window));
            provider->OnMouseButtonEvent(button, action, mods);
        });
        
        glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos) {
            auto* provider = static_cast<GLFWInputProvider*>(glfwGetWindowUserPointer(window));
            provider->OnMouseMoveEvent(xpos, ypos);
        });
        
        glfwSetJoystickCallback([](int joystick, int event) {
            // Handle controller connect/disconnect
            if (event == GLFW_CONNECTED) {
                // Add controller
            } else if (event == GLFW_DISCONNECTED) {
                // Remove controller
            }
        });
    }
    
    void OnKeyEvent(int key, int scancode, int action, int mods) {
        std::unique_lock lock(event_buffer_mutex_);
        
        InputEvent event;
        event.type = InputEvent::Type::KeyboardButton;
        event.device_id = DeviceId::Keyboard;
        event.timestamp = std::chrono::high_resolution_clock::now();
        event.keyboard_button.key_code = static_cast<KeyCode>(key);
        event.keyboard_button.scan_code = scancode;
        event.keyboard_button.action = static_cast<ButtonAction>(action);
        event.keyboard_button.modifiers = static_cast<KeyModifiers>(mods);
        
        buffered_events_.push_back(event);
    }
    
    void OnMouseButtonEvent(int button, int action, int mods) {
        std::unique_lock lock(event_buffer_mutex_);
        
        InputEvent event;
        event.type = InputEvent::Type::MouseButton;
        event.device_id = DeviceId::Mouse;
        event.timestamp = std::chrono::high_resolution_clock::now();
        event.mouse_button.button = static_cast<MouseButton>(button);
        event.mouse_button.action = static_cast<ButtonAction>(action);
        event.mouse_button.modifiers = static_cast<KeyModifiers>(mods);
        
        buffered_events_.push_back(event);
    }
    
    void OnMouseMoveEvent(double xpos, double ypos) {
        std::unique_lock lock(event_buffer_mutex_);
        
        InputEvent event;
        event.type = InputEvent::Type::MouseMove;
        event.device_id = DeviceId::Mouse;
        event.timestamp = std::chrono::high_resolution_clock::now();
        event.mouse_move.x = static_cast<float>(xpos);
        event.mouse_move.y = static_cast<float>(ypos);
        
        buffered_events_.push_back(event);
    }
    
    void ProcessBufferedEvents() {
        std::unique_lock lock(event_buffer_mutex_);
        
        // Process all buffered events and clear the buffer
        for (const auto& event : buffered_events_) {
            input_system_->ProcessEvent(event);
        }
        
        buffered_events_.clear();
    }
};
```

#### WebAssembly Browser Integration

```cpp
// WebAssembly-specific input implementation
class WebAssemblyInputProvider final : public PlatformInputProvider {
public:
    WebAssemblyInputProvider() {
        SetupEventListeners();
        RequestPointerLock();
    }
    
    [[nodiscard]] auto GetDevices() const -> std::vector<std::unique_ptr<InputDevice>> override {
        std::vector<std::unique_ptr<InputDevice>> devices;
        
        // Basic keyboard and mouse support
        devices.push_back(std::make_unique<WebKeyboard>());
        devices.push_back(std::make_unique<WebMouse>());
        
        // Limited gamepad support through Gamepad API
        if (SupportsGamepadAPI()) {
            auto gamepads = GetConnectedGamepads();
            for (const auto& gamepad : gamepads) {
                devices.push_back(std::make_unique<WebGamepad>(gamepad.index));
            }
        }
        
        return devices;
    }
    
    void PollEvents() override {
        // Web platform is event-driven, but we need to poll gamepad state
        if (SupportsGamepadAPI()) {
            UpdateGamepadStates();
        }
        
        ProcessQueuedEvents();
    }
    
private:
    std::vector<InputEvent> queued_events_;
    std::mutex event_queue_mutex_;
    bool pointer_locked_ = false;
    
    void SetupEventListeners() {
        // Use Emscripten's event handling for browser integration
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true, 
                                       [](int event_type, const EmscriptenKeyboardEvent* event, void* user_data) -> EM_BOOL {
            auto* provider = static_cast<WebAssemblyInputProvider*>(user_data);
            return provider->OnKeyDown(event);
        });
        
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, this, true,
                                     [](int event_type, const EmscriptenKeyboardEvent* event, void* user_data) -> EM_BOOL {
            auto* provider = static_cast<WebAssemblyInputProvider*>(user_data);
            return provider->OnKeyUp(event);
        });
        
        emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_CANVAS, this, true,
                                         [](int event_type, const EmscriptenMouseEvent* event, void* user_data) -> EM_BOOL {
            auto* provider = static_cast<WebAssemblyInputProvider*>(user_data);
            return provider->OnMouseDown(event);
        });
        
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_CANVAS, this, true,
                                       [](int event_type, const EmscriptenMouseEvent* event, void* user_data) -> EM_BOOL {
            auto* provider = static_cast<WebAssemblyInputProvider*>(user_data);
            return provider->OnMouseUp(event);
        });
        
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_CANVAS, this, true,
                                         [](int event_type, const EmscriptenMouseEvent* event, void* user_data) -> EM_BOOL {
            auto* provider = static_cast<WebAssemblyInputProvider*>(user_data);
            return provider->OnMouseMove(event);
        });
    }
    
    auto OnKeyDown(const EmscriptenKeyboardEvent* event) -> EM_BOOL {
        std::unique_lock lock(event_queue_mutex_);
        
        InputEvent input_event;
        input_event.type = InputEvent::Type::KeyboardButton;
        input_event.device_id = DeviceId::Keyboard;
        input_event.timestamp = std::chrono::high_resolution_clock::now();
        input_event.keyboard_button.key_code = TranslateWebKeyCode(event->code);
        input_event.keyboard_button.action = ButtonAction::Press;
        input_event.keyboard_button.modifiers = TranslateWebModifiers(event);
        
        queued_events_.push_back(input_event);
        
        // Prevent default browser behavior for game keys
        return ShouldPreventDefault(input_event.keyboard_button.key_code) ? EM_TRUE : EM_FALSE;
    }
    
    auto OnMouseDown(const EmscriptenMouseEvent* event) -> EM_BOOL {
        std::unique_lock lock(event_queue_mutex_);
        
        InputEvent input_event;
        input_event.type = InputEvent::Type::MouseButton;
        input_event.device_id = DeviceId::Mouse;
        input_event.timestamp = std::chrono::high_resolution_clock::now();
        input_event.mouse_button.button = TranslateWebMouseButton(event->button);
        input_event.mouse_button.action = ButtonAction::Press;
        
        queued_events_.push_back(input_event);
        
        return EM_TRUE; // Always prevent default for mouse events
    }
    
    auto OnMouseMove(const EmscriptenMouseEvent* event) -> EM_BOOL {
        std::unique_lock lock(event_queue_mutex_);
        
        InputEvent input_event;
        input_event.type = InputEvent::Type::MouseMove;
        input_event.device_id = DeviceId::Mouse;
        input_event.timestamp = std::chrono::high_resolution_clock::now();
        
        if (pointer_locked_) {
            // Use movement delta when pointer is locked
            input_event.mouse_move.x = static_cast<float>(event->movementX);
            input_event.mouse_move.y = static_cast<float>(event->movementY);
        } else {
            // Use absolute position when not locked
            input_event.mouse_move.x = static_cast<float>(event->targetX);
            input_event.mouse_move.y = static_cast<float>(event->targetY);
        }
        
        queued_events_.push_back(input_event);
        
        return EM_TRUE;
    }
    
    void RequestPointerLock() {
        // Request pointer lock for better mouse control
        emscripten_request_pointerlock(EMSCRIPTEN_EVENT_TARGET_CANVAS, true);
    }
    
    [[nodiscard]] auto SupportsGamepadAPI() const -> bool {
        // Check if browser supports Gamepad API
        return EM_ASM_INT({
            return typeof navigator.getGamepads === 'function';
        }) != 0;
    }
    
    void UpdateGamepadStates() {
        // Poll gamepad states using Emscripten's gamepad support
        // This is necessary because gamepad events are not reliable in browsers
    }
    
    [[nodiscard]] auto TranslateWebKeyCode(const char* web_code) const -> KeyCode {
        // Translate web KeyboardEvent.code to engine KeyCode enum
        static const std::unordered_map<std::string, KeyCode> translation_map = {
            {"KeyW", KeyCode::W},
            {"KeyA", KeyCode::A},
            {"KeyS", KeyCode::S},
            {"KeyD", KeyCode::D},
            {"Space", KeyCode::Space},
            {"Enter", KeyCode::Enter},
            {"Escape", KeyCode::Escape},
            // ... complete mapping
        };
        
        auto it = translation_map.find(web_code);
        return it != translation_map.end() ? it->second : KeyCode::Unknown;
    }
    
    [[nodiscard]] auto ShouldPreventDefault(KeyCode key) const -> bool {
        // Prevent default browser behavior for game-relevant keys
        switch (key) {
            case KeyCode::Space:    // Prevent page scroll
            case KeyCode::Tab:      // Prevent focus change
            case KeyCode::F11:      // Prevent fullscreen toggle
            case KeyCode::F5:       // Prevent page refresh
                return true;
            default:
                return false;
        }
    }
};
```

### Action Mapping System

#### Declarative Action Configuration

```cpp
// JSON-based action mapping configuration
struct ActionMapping {
    std::string name;
    std::string display_name;
    ActionType type;
    std::vector<InputBinding> bindings;
    bool enabled = true;
    
    // Action processing settings
    float deadzone = 0.1f;
    float sensitivity = 1.0f;
    bool invert_axis = false;
    
    // Context information
    std::string context;
    uint32_t priority = 0;
};

struct InputBinding {
    DeviceType device_type;
    uint32_t input_id;        // Key code, button index, axis index
    InputModifiers modifiers = InputModifiers::None;
    
    // Advanced binding settings
    float scale = 1.0f;
    float offset = 0.0f;
    bool requires_exact_modifiers = false;
};

// Example JSON configuration
/*
{
    "action_contexts": {
        "gameplay": {
            "priority": 100,
            "actions": [
                {
                    "name": "move_forward",
                    "display_name": "Move Forward",
                    "type": "button",
                    "bindings": [
                        {
                            "device": "keyboard",
                            "input": "KeyW",
                            "modifiers": []
                        },
                        {
                            "device": "controller",
                            "input": "left_stick_y",
                            "scale": -1.0
                        }
                    ]
                },
                {
                    "name": "look_horizontal",
                    "display_name": "Look Left/Right",
                    "type": "axis",
                    "deadzone": 0.05,
                    "sensitivity": 2.0,
                    "bindings": [
                        {
                            "device": "mouse",
                            "input": "axis_x"
                        },
                        {
                            "device": "controller",
                            "input": "right_stick_x"
                        }
                    ]
                }
            ]
        },
        "menu": {
            "priority": 200,
            "actions": [
                {
                    "name": "navigate_up",
                    "display_name": "Navigate Up",
                    "type": "button",
                    "bindings": [
                        {
                            "device": "keyboard",
                            "input": "KeyW"
                        },
                        {
                            "device": "keyboard",
                            "input": "ArrowUp"
                        }
                    ]
                }
            ]
        }
    }
}
*/

// Action registry for runtime management
class ActionRegistry {
public:
    void LoadConfiguration(const std::filesystem::path& config_path) {
        auto config_data = LoadJsonFile(config_path);
        ParseActionConfiguration(config_data);
        BuildActionMaps();
    }
    
    void RegisterAction(const ActionMapping& mapping) {
        std::unique_lock lock(registry_mutex_);
        
        actions_[mapping.name] = mapping;
        BuildActionMaps();
    }
    
    void SetActiveContext(const std::string& context_name) {
        std::unique_lock lock(registry_mutex_);
        
        active_context_ = context_name;
        RebuildActiveBindings();
    }
    
    [[nodiscard]] auto GetActionValue(const std::string& action_name) const -> std::optional<ActionValue> {
        std::shared_lock lock(registry_mutex_);
        
        auto it = current_action_values_.find(action_name);
        return it != current_action_values_.end() ? 
               std::make_optional(it->second) : std::nullopt;
    }
    
    [[nodiscard]] auto GetActionState(const std::string& action_name) const -> ActionState {
        std::shared_lock lock(registry_mutex_);
        
        auto it = current_action_states_.find(action_name);
        return it != current_action_states_.end() ? it->second : ActionState::Inactive;
    }
    
    // Real-time binding modification for accessibility
    void AddBinding(const std::string& action_name, const InputBinding& binding) {
        std::unique_lock lock(registry_mutex_);
        
        if (auto it = actions_.find(action_name); it != actions_.end()) {
            it->second.bindings.push_back(binding);
            RebuildActiveBindings();
        }
    }
    
    void RemoveBinding(const std::string& action_name, const InputBinding& binding) {
        std::unique_lock lock(registry_mutex_);
        
        if (auto it = actions_.find(action_name); it != actions_.end()) {
            auto& bindings = it->second.bindings;
            bindings.erase(
                std::remove_if(bindings.begin(), bindings.end(),
                              [&binding](const InputBinding& b) { return b == binding; }),
                bindings.end());
            RebuildActiveBindings();
        }
    }
    
    // Hot-reload support
    void EnableHotReload(bool enable) {
        hot_reload_enabled_ = enable;
        if (enable && !config_watcher_) {
            config_watcher_ = std::make_unique<ConfigurationWatcher>(*this);
        }
    }
    
    void OnConfigurationChanged(const std::filesystem::path& config_path) {
        if (hot_reload_enabled_) {
            LoadConfiguration(config_path);
        }
    }
    
private:
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, ActionMapping> actions_;
    std::string active_context_;
    
    // Runtime state
    std::unordered_map<std::string, ActionValue> current_action_values_;
    std::unordered_map<std::string, ActionState> current_action_states_;
    
    // Optimized lookup structures
    std::unordered_map<InputBinding, std::vector<std::string>, InputBindingHash> binding_to_actions_;
    std::unordered_map<std::string, std::vector<InputBinding>> action_to_bindings_;
    
    bool hot_reload_enabled_ = false;
    std::unique_ptr<ConfigurationWatcher> config_watcher_;
    
    void ParseActionConfiguration(const nlohmann::json& config);
    void BuildActionMaps();
    void RebuildActiveBindings();
};
```

#### Input Event Processing

```cpp
// Frame-coherent input processing
class InputEventProcessor {
public:
    void ProcessEvent(const InputEvent& event) {
        std::unique_lock lock(event_buffer_mutex_);
        
        // Add to current frame's event buffer
        current_frame_events_.push_back(event);
        
        // Update immediate state for queries that need latest input
        UpdateImmediateState(event);
    }
    
    void BeginFrame() {
        std::unique_lock lock(event_buffer_mutex_);
        
        // Swap event buffers for frame-coherent processing
        previous_frame_events_ = std::move(current_frame_events_);
        current_frame_events_.clear();
        
        // Reset frame-specific state
        frame_action_triggers_.clear();
        frame_action_values_.clear();
    }
    
    void ProcessFrameEvents(ActionRegistry& action_registry) {
        // Process all events from the previous frame
        for (const auto& event : previous_frame_events_) {
            ProcessEventForActions(event, action_registry);
        }
        
        // Update action states based on current input state
        UpdateActionStates(action_registry);
    }
    
    [[nodiscard]] auto GetFrameActionTriggers() const -> const std::vector<ActionTrigger>& {
        return frame_action_triggers_;
    }
    
    [[nodiscard]] auto GetActionValue(const std::string& action_name) const -> float {
        auto it = frame_action_values_.find(action_name);
        return it != frame_action_values_.end() ? it->second : 0.0f;
    }
    
    [[nodiscard]] auto WasActionTriggered(const std::string& action_name) const -> bool {
        return std::ranges::any_of(frame_action_triggers_, 
                                 [&action_name](const ActionTrigger& trigger) {
                                     return trigger.action_name == action_name && 
                                            trigger.state == ActionState::Triggered;
                                 });
    }
    
    [[nodiscard]] auto WasActionReleased(const std::string& action_name) const -> bool {
        return std::ranges::any_of(frame_action_triggers_,
                                 [&action_name](const ActionTrigger& trigger) {
                                     return trigger.action_name == action_name && 
                                            trigger.state == ActionState::Released;
                                 });
    }
    
private:
    std::mutex event_buffer_mutex_;
    std::vector<InputEvent> current_frame_events_;
    std::vector<InputEvent> previous_frame_events_;
    
    // Frame-specific action processing results
    std::vector<ActionTrigger> frame_action_triggers_;
    std::unordered_map<std::string, float> frame_action_values_;
    
    // Current input device states
    std::unordered_map<DeviceId, InputDeviceState> device_states_;
    
    void UpdateImmediateState(const InputEvent& event) {
        // Update device state for immediate queries
        auto& device_state = device_states_[event.device_id];
        
        switch (event.type) {
            case InputEvent::Type::KeyboardButton:
                device_state.keyboard.keys[event.keyboard_button.key_code] = 
                    (event.keyboard_button.action != ButtonAction::Release);
                break;
                
            case InputEvent::Type::MouseButton:
                device_state.mouse.buttons[event.mouse_button.button] = 
                    (event.mouse_button.action != ButtonAction::Release);
                break;
                
            case InputEvent::Type::MouseMove:
                device_state.mouse.position.x = event.mouse_move.x;
                device_state.mouse.position.y = event.mouse_move.y;
                break;
                
            case InputEvent::Type::ControllerButton:
                device_state.controller.buttons[event.controller_button.button] = 
                    (event.controller_button.action != ButtonAction::Release);
                break;
                
            case InputEvent::Type::ControllerAxis:
                device_state.controller.axes[event.controller_axis.axis] = 
                    event.controller_axis.value;
                break;
        }
    }
    
    void ProcessEventForActions(const InputEvent& event, ActionRegistry& action_registry) {
        // Find all actions bound to this input
        auto bindings = action_registry.FindBindingsForInput(event);
        
        for (const auto& [action_name, binding] : bindings) {
            ProcessActionBinding(action_name, binding, event, action_registry);
        }
    }
    
    void ProcessActionBinding(const std::string& action_name, 
                            const InputBinding& binding, 
                            const InputEvent& event,
                            ActionRegistry& action_registry) {
        
        const auto& action = action_registry.GetAction(action_name);
        
        // Calculate raw input value based on event type
        float raw_value = CalculateRawInputValue(event, binding);
        
        // Apply action processing (deadzone, sensitivity, etc.)
        float processed_value = ApplyActionProcessing(raw_value, action);
        
        // Update action value
        frame_action_values_[action_name] = processed_value;
        
        // Determine if this constitutes an action trigger
        ActionState new_state = DetermineActionState(processed_value, action);
        ActionState previous_state = action_registry.GetActionState(action_name);
        
        if (new_state != previous_state) {
            ActionTrigger trigger{
                .action_name = action_name,
                .state = new_state,
                .value = processed_value,
                .timestamp = event.timestamp
            };
            
            frame_action_triggers_.push_back(trigger);
        }
    }
    
    [[nodiscard]] auto CalculateRawInputValue(const InputEvent& event, 
                                            const InputBinding& binding) const -> float {
        switch (event.type) {
            case InputEvent::Type::KeyboardButton:
                return (event.keyboard_button.action != ButtonAction::Release) ? 1.0f : 0.0f;
                
            case InputEvent::Type::MouseButton:
                return (event.mouse_button.action != ButtonAction::Release) ? 1.0f : 0.0f;
                
            case InputEvent::Type::MouseMove:
                // For mouse movement, return delta or absolute position based on binding
                return binding.input_id == 0 ? event.mouse_move.x : event.mouse_move.y;
                
            case InputEvent::Type::ControllerButton:
                return (event.controller_button.action != ButtonAction::Release) ? 1.0f : 0.0f;
                
            case InputEvent::Type::ControllerAxis:
                return event.controller_axis.value;
                
            default:
                return 0.0f;
        }
    }
    
    [[nodiscard]] auto ApplyActionProcessing(float raw_value, 
                                           const ActionMapping& action) const -> float {
        float processed = raw_value;
        
        // Apply deadzone
        if (std::abs(processed) < action.deadzone) {
            processed = 0.0f;
        } else {
            // Scale to account for deadzone
            float sign = processed < 0.0f ? -1.0f : 1.0f;
            processed = sign * ((std::abs(processed) - action.deadzone) / (1.0f - action.deadzone));
        }
        
        // Apply sensitivity
        processed *= action.sensitivity;
        
        // Apply inversion
        if (action.invert_axis) {
            processed = -processed;
        }
        
        // Clamp to valid range
        return std::clamp(processed, -1.0f, 1.0f);
    }
};
```

### ECS Integration

#### Input Components for Entity-Based Input

```cpp
// Component for entities that can receive input
struct InputComponent {
    std::string action_context = "default";
    bool enabled = true;
    float input_priority = 0.0f;
    
    // Action mappings specific to this entity
    std::unordered_map<std::string, std::string> action_mappings;
    
    // Input state for this entity
    std::unordered_map<std::string, float> action_values;
    std::unordered_set<std::string> triggered_actions;
    std::unordered_set<std::string> released_actions;
};

// Component traits for ECS integration
template<>
struct ComponentTraits<InputComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;  // Fewer entities need input
    static constexpr size_t alignment = alignof(std::string);
};

// Component for player-controlled entities
struct PlayerInputComponent {
    PlayerId player_id = 0;
    bool accepts_input = true;
    
    // Player-specific input settings
    float mouse_sensitivity = 1.0f;
    bool invert_mouse_y = false;
    std::string control_scheme = "default";
};

// Component for UI elements that can receive input
struct UIInputComponent {
    bool focusable = true;
    bool has_focus = false;
    uint32_t tab_order = 0;
    
    // UI-specific input handling
    std::function<void(const InputEvent&)> input_handler;
    std::unordered_set<std::string> handled_actions;
};
```

#### ECS Input Systems

```cpp
// Primary input system for processing entity input
class InputSystem final : public System {
public:
    explicit InputSystem(InputEventProcessor& event_processor, ActionRegistry& action_registry)
        : event_processor_(event_processor)
        , action_registry_(action_registry) {}
    
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {},
            .write_components = {
                GetComponentTypeId<InputComponent>(),
                GetComponentTypeId<PlayerInputComponent>(),
                GetComponentTypeId<UIInputComponent>()
            },
            .execution_phase = ExecutionPhase::PreUpdate,
            .thread_safety = ThreadSafety::MainThreadOnly  // Input must be on render thread
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Begin frame processing
        event_processor_.BeginFrame();
        event_processor_.ProcessFrameEvents(action_registry_);
        
        // Update all entities with input components
        UpdateInputComponents(world);
        UpdatePlayerInputComponents(world);
        UpdateUIInputComponents(world);
    }
    
private:
    InputEventProcessor& event_processor_;
    ActionRegistry& action_registry_;
    
    void UpdateInputComponents(World& world) {
        world.ForEachComponent<InputComponent>([this](EntityId entity, InputComponent& input) {
            if (!input.enabled) {
                return;
            }
            
            // Clear previous frame state
            input.triggered_actions.clear();
            input.released_actions.clear();
            
            // Set active context if it's different
            if (action_registry_.GetActiveContext() != input.action_context) {
                action_registry_.SetActiveContext(input.action_context);
            }
            
            // Update action values for this entity
            for (const auto& [local_action, global_action] : input.action_mappings) {
                float value = event_processor_.GetActionValue(global_action);
                input.action_values[local_action] = value;
                
                // Check for triggers and releases
                if (event_processor_.WasActionTriggered(global_action)) {
                    input.triggered_actions.insert(local_action);
                }
                
                if (event_processor_.WasActionReleased(global_action)) {
                    input.released_actions.insert(local_action);
                }
            }
        });
    }
    
    void UpdatePlayerInputComponents(World& world) {
        world.ForEachComponent<PlayerInputComponent>([this](EntityId entity, PlayerInputComponent& player_input) {
            if (!player_input.accepts_input) {
                return;
            }
            
            // Apply player-specific input settings
            ApplyPlayerInputSettings(entity, player_input);
            
            // Handle player-specific actions
            ProcessPlayerActions(entity, player_input);
        });
    }
    
    void UpdateUIInputComponents(World& world) {
        // Handle UI focus management
        UpdateUIFocus(world);
        
        world.ForEachComponent<UIInputComponent>([this](EntityId entity, UIInputComponent& ui_input) {
            if (!ui_input.focusable || !ui_input.has_focus) {
                return;
            }
            
            // Process UI-specific input events
            const auto& triggers = event_processor_.GetFrameActionTriggers();
            for (const auto& trigger : triggers) {
                if (ui_input.handled_actions.contains(trigger.action_name)) {
                    if (ui_input.input_handler) {
                        // Convert action trigger to input event for UI handler
                        InputEvent ui_event = ConvertActionToInputEvent(trigger);
                        ui_input.input_handler(ui_event);
                    }
                }
            }
        });
    }
    
    void ApplyPlayerInputSettings(EntityId entity, const PlayerInputComponent& player_input) {
        // Apply player-specific settings to action registry
        // This could modify sensitivity, inversion, etc. for this player's actions
    }
    
    void ProcessPlayerActions(EntityId entity, const PlayerInputComponent& player_input) {
        // Handle common player actions like movement, camera control, etc.
        // This is where game-specific input logic would be processed
    }
    
    void UpdateUIFocus(World& world) {
        // Implement UI focus management logic
        // Handle tab navigation, mouse focus, etc.
    }
    
    [[nodiscard]] auto ConvertActionToInputEvent(const ActionTrigger& trigger) const -> InputEvent {
        // Convert action trigger back to input event format for UI processing
        InputEvent event;
        // ... implementation details
        return event;
    }
};

// Specialized system for player camera control
class PlayerCameraInputSystem final : public System {
public:
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                GetComponentTypeId<PlayerInputComponent>(),
                GetComponentTypeId<InputComponent>()
            },
            .write_components = {
                GetComponentTypeId<TransformComponent>(),
                GetComponentTypeId<CameraComponent>()
            },
            .execution_phase = ExecutionPhase::Update,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Query for entities with both player input and camera components
        world.ForEachComponent<PlayerInputComponent, InputComponent, CameraComponent, TransformComponent>(
            [this, delta_time](EntityId entity, 
                              const PlayerInputComponent& player_input,
                              const InputComponent& input,
                              CameraComponent& camera,
                              TransformComponent& transform) {
                
                if (!player_input.accepts_input || !input.enabled) {
                    return;
                }
                
                // Process camera input actions
                ProcessCameraMovement(input, transform, delta_time);
                ProcessCameraRotation(input, player_input, camera, delta_time);
            });
    }
    
private:
    void ProcessCameraMovement(const InputComponent& input, 
                             TransformComponent& transform, 
                             float delta_time) {
        // Implementation for camera movement based on input actions
        float forward = GetActionValue(input, "move_forward");
        float right = GetActionValue(input, "move_right");
        
        // Apply movement to transform
        // ... implementation details
    }
    
    void ProcessCameraRotation(const InputComponent& input,
                             const PlayerInputComponent& player_input,
                             CameraComponent& camera,
                             float delta_time) {
        // Implementation for camera rotation based on mouse/controller input
        float look_x = GetActionValue(input, "look_horizontal");
        float look_y = GetActionValue(input, "look_vertical");
        
        // Apply player settings
        look_x *= player_input.mouse_sensitivity;
        look_y *= player_input.mouse_sensitivity * (player_input.invert_mouse_y ? -1.0f : 1.0f);
        
        // Apply rotation to camera
        // ... implementation details
    }
    
    [[nodiscard]] auto GetActionValue(const InputComponent& input, const std::string& action) const -> float {
        auto it = input.action_values.find(action);
        return it != input.action_values.end() ? it->second : 0.0f;
    }
};
```

### Threading Model Integration

#### Integration with Existing Architecture

```cpp
// Integration with the existing Game class and threading model
class InputManager {
public:
    explicit InputManager(GLFWwindow* window) {
        // Initialize platform-specific input provider
        #ifdef __EMSCRIPTEN__
            platform_provider_ = std::make_unique<WebAssemblyInputProvider>();
        #else
            platform_provider_ = std::make_unique<GLFWInputProvider>(window);
        #endif
        
        // Initialize core systems
        event_processor_ = std::make_unique<InputEventProcessor>();
        action_registry_ = std::make_unique<ActionRegistry>();
        
        // Load default input configuration
        LoadInputConfiguration("assets/input_config.json");
    }
    
    // Called from main thread (GLFW event handling)
    void PollEvents() {
        platform_provider_->PollEvents();
    }
    
    // Called from render thread (Game::Update)
    void Update(float delta_time) {
        // Process input events and update action states
        event_processor_->BeginFrame();
        
        // Get events from platform provider
        auto events = platform_provider_->GetEvents();
        for (const auto& event : events) {
            event_processor_->ProcessEvent(event);
        }
        
        // Process events for actions
        event_processor_->ProcessFrameEvents(*action_registry_);
    }
    
    // Access for ECS input systems
    [[nodiscard]] auto GetEventProcessor() -> InputEventProcessor& {
        return *event_processor_;
    }
    
    [[nodiscard]] auto GetActionRegistry() -> ActionRegistry& {
        return *action_registry_;
    }
    
    // Action queries for game logic
    [[nodiscard]] auto IsActionActive(const std::string& action_name) const -> bool {
        return action_registry_->GetActionState(action_name) == ActionState::Active;
    }
    
    [[nodiscard]] auto WasActionTriggered(const std::string& action_name) const -> bool {
        return event_processor_->WasActionTriggered(action_name);
    }
    
    [[nodiscard]] auto GetActionValue(const std::string& action_name) const -> float {
        return event_processor_->GetActionValue(action_name);
    }
    
    // Configuration management
    void LoadInputConfiguration(const std::filesystem::path& config_path) {
        action_registry_->LoadConfiguration(config_path);
    }
    
    void SetActiveContext(const std::string& context_name) {
        action_registry_->SetActiveContext(context_name);
    }
    
    // Hot-reload support
    void EnableHotReload(bool enable) {
        action_registry_->EnableHotReload(enable);
    }
    
private:
    std::unique_ptr<PlatformInputProvider> platform_provider_;
    std::unique_ptr<InputEventProcessor> event_processor_;
    std::unique_ptr<ActionRegistry> action_registry_;
};

// Integration with Game class
class Game {
public:
    // ... existing methods ...
    
    void Update(GLFWwindow* window, double delta_time) {
        // Update input system (called from render thread)
        input_manager_->Update(static_cast<float>(delta_time));
        
        // ECS world update (includes input systems)
        if (engine_world) {
            engine_world->GetSystemScheduler().ExecuteFrame(static_cast<float>(delta_time));
        }
        
        // UI system - can now respond to input actions
        ui_system.Update(window, render_system.GetViewport(), render_system.GetCamera());
    }
    
    // Called from main thread via GLFW callbacks
    void PollInput() {
        input_manager_->PollEvents();
    }
    
private:
    std::unique_ptr<InputManager> input_manager_;
    // ... existing members ...
};
```

## Performance Optimization

### Low-Latency Input Processing

```cpp
// High-performance input processing with minimal latency
class HighPerformanceInputProcessor {
public:
    // Optimized event processing with memory pools
    void ProcessEvents(std::span<const InputEvent> events) {
        // Use frame-local memory pool for temporary allocations
        FrameAllocator allocator(frame_memory_pool_);
        
        // Pre-allocate space for action triggers
        action_triggers_.clear();
        action_triggers_.reserve(events.size() * 2); // Conservative estimate
        
        // Process events in batches for better cache locality
        constexpr size_t batch_size = 64;
        for (size_t i = 0; i < events.size(); i += batch_size) {
            size_t end = std::min(i + batch_size, events.size());
            ProcessEventBatch(events.subspan(i, end - i), allocator);
        }
    }
    
    // Lock-free action state queries for real-time systems
    [[nodiscard]] auto GetActionValue(const std::string& action_name) const -> float {
        // Use atomic reads for lock-free access from multiple threads
        auto it = atomic_action_values_.find(action_name);
        if (it != atomic_action_values_.end()) {
            return it->second.load(std::memory_order_acquire);
        }
        return 0.0f;
    }
    
    // Prediction for latency compensation
    [[nodiscard]] auto PredictActionValue(const std::string& action_name, 
                                        float prediction_time_ms) const -> float {
        auto it = action_histories_.find(action_name);
        if (it == action_histories_.end()) {
            return GetActionValue(action_name);
        }
        
        return PredictUsingHistory(it->second, prediction_time_ms);
    }
    
private:
    // Memory pool for frame-local allocations
    std::unique_ptr<MemoryPool> frame_memory_pool_;
    
    // Lock-free action values for real-time access
    std::unordered_map<std::string, std::atomic<float>> atomic_action_values_;
    
    // Action history for prediction
    struct ActionHistory {
        std::array<float, 16> values;
        std::array<std::chrono::high_resolution_clock::time_point, 16> timestamps;
        size_t current_index = 0;
    };
    std::unordered_map<std::string, ActionHistory> action_histories_;
    
    std::vector<ActionTrigger> action_triggers_;
    
    void ProcessEventBatch(std::span<const InputEvent> batch, FrameAllocator& allocator) {
        // Process events in batch for better cache performance
        // Use SIMD where possible for multiple similar events
    }
    
    [[nodiscard]] auto PredictUsingHistory(const ActionHistory& history, 
                                         float prediction_time_ms) const -> float {
        // Simple linear prediction based on recent history
        // Could be enhanced with more sophisticated prediction algorithms
        if (history.current_index < 2) {
            return history.values[(history.current_index - 1) % 16];
        }
        
        size_t current = (history.current_index - 1) % 16;
        size_t previous = (history.current_index - 2) % 16;
        
        float current_value = history.values[current];
        float previous_value = history.values[previous];
        
        auto time_delta = history.timestamps[current] - history.timestamps[previous];
        float time_delta_ms = std::chrono::duration<float, std::milli>(time_delta).count();
        
        if (time_delta_ms > 0.0f) {
            float rate = (current_value - previous_value) / time_delta_ms;
            return current_value + (rate * prediction_time_ms);
        }
        
        return current_value;
    }
};
```

### Memory-Efficient WebAssembly Support

```cpp
// Optimized input handling for WebAssembly memory constraints
class WebAssemblyOptimizedInput {
public:
    // Compact event representation for memory efficiency
    struct CompactInputEvent {
        uint32_t packed_data;  // Type, device, and basic info packed into 32 bits
        float value;           // Single float for all value types
        uint16_t timestamp_ms; // Relative timestamp in milliseconds
    };
    
    static_assert(sizeof(CompactInputEvent) == 10, "CompactInputEvent should be tightly packed");
    
    // Ring buffer for event storage with minimal memory footprint
    class CompactEventBuffer {
    public:
        static constexpr size_t buffer_size = 512; // Small buffer for memory-constrained environment
        
        void PushEvent(const CompactInputEvent& event) {
            events_[write_index_ % buffer_size] = event;
            write_index_++;
        }
        
        [[nodiscard]] auto GetEvents() const -> std::span<const CompactInputEvent> {
            size_t start = read_index_ % buffer_size;
            size_t count = std::min(write_index_ - read_index_, buffer_size);
            
            if (start + count <= buffer_size) {
                return std::span<const CompactInputEvent>(events_.data() + start, count);
            } else {
                // Handle wrap-around case
                return {}; // Simplified for this example
            }
        }
        
        void ConsumeEvents(size_t count) {
            read_index_ += count;
        }
        
    private:
        std::array<CompactInputEvent, buffer_size> events_;
        std::atomic<size_t> write_index_{0};
        std::atomic<size_t> read_index_{0};
    };
    
    // Simplified action mapping for reduced memory usage
    struct CompactActionMapping {
        std::string_view name;  // Use string_view to avoid allocations
        uint32_t input_hash;    // Hash of input binding for fast lookup
        float sensitivity;
        uint8_t flags;          // Packed flags for various settings
    };
    
private:
    CompactEventBuffer event_buffer_;
    std::array<CompactActionMapping, 64> action_mappings_; // Fixed size for predictable memory usage
    size_t active_mappings_count_ = 0;
};
```

## Success Criteria

### Performance Metrics

1. **Input Latency**: End-to-end input latency under 8ms for button presses and mouse movement
2. **Memory Usage**: Input system peak memory usage under 16MB on all platforms
3. **Event Processing**: Process 1000+ input events per frame without frame rate impact
4. **Cross-Platform Parity**: Identical input behavior within 1ms between native and WebAssembly

### Functionality Requirements

1. **Device Support**: Full support for keyboard, mouse, and at least 4 simultaneous controllers
2. **Action Mapping**: Complete JSON-based configuration with hot-reload capabilities
3. **ECS Integration**: Seamless component-based input handling for entities
4. **Context Switching**: Smooth transition between different input contexts (menu, gameplay, etc.)

### Reliability Standards

1. **Thread Safety**: Zero race conditions during 24-hour stress testing with multiple input devices
2. **Event Ordering**: Guaranteed event ordering preservation across platform boundaries
3. **Device Handling**: Graceful handling of device connect/disconnect without system interruption
4. **Configuration Errors**: Robust error handling for malformed input configuration files

### Accessibility Requirements

1. **Key Remapping**: Complete remapping support for all input actions
2. **Multiple Bindings**: Support for multiple input bindings per action for accessibility
3. **Hold/Toggle Options**: Configurable hold vs toggle behavior for all button actions
4. **Sensitivity Control**: Per-action sensitivity and deadzone configuration

## Future Enhancements

### Phase 2: Advanced Input Features

#### Gesture Recognition System

```cpp
// Touch and gesture input for future mobile/tablet support
class GestureRecognitionSystem {
public:
    void RegisterGesture(const std::string& name, const GesturePattern& pattern);
    auto DetectGestures(const TouchInputSequence& sequence) -> std::vector<RecognizedGesture>;
    
private:
    // Machine learning-based gesture recognition
    std::unique_ptr<GestureClassifier> classifier_;
};
```

#### Adaptive Input System

```cpp
// Machine learning-based input adaptation
class AdaptiveInputSystem {
public:
    void LearnPlayerBehavior(PlayerId player_id, const InputSequence& sequence);
    auto PredictPlayerIntent(PlayerId player_id, const CurrentInputState& state) -> PredictedAction;
    void AdjustSensitivity(PlayerId player_id, float performance_metric);
    
private:
    // Player behavior models for personalized input processing
    std::unordered_map<PlayerId, PlayerBehaviorModel> player_models_;
};
```

### Phase 3: Extended Platform Support

#### Mobile Platform Integration

```cpp
// Future mobile platform support
class MobileInputProvider final : public PlatformInputProvider {
public:
    // Touch screen input with gesture recognition
    void HandleTouchEvents(const std::vector<TouchEvent>& events);
    
    // Device orientation and motion sensors
    void HandleMotionInput(const DeviceMotionData& motion);
    
    // Platform-specific features (iOS/Android)
    void SetHapticFeedback(HapticPattern pattern, float intensity);
    
private:
    std::unique_ptr<TouchGestureProcessor> gesture_processor_;
    std::unique_ptr<MotionInputProcessor> motion_processor_;
};
```

#### VR/AR Input Integration

```cpp
// Future VR/AR platform support
class VRInputProvider final : public PlatformInputProvider {
public:
    // Hand tracking and controller input
    void UpdateHandTracking(const HandTrackingData& hand_data);
    void UpdateControllerInput(const VRControllerState& controller_state);
    
    // Eye tracking and head movement
    void UpdateGazeInput(const EyeTrackingData& gaze_data);
    void UpdateHeadTracking(const HeadTrackingData& head_data);
    
private:
    // VR-specific input processing
    std::unique_ptr<HandTrackingProcessor> hand_processor_;
    std::unique_ptr<GazeInputProcessor> gaze_processor_;
};
```

### Phase 4: Advanced Accessibility

#### Assistive Technology Integration

```cpp
// Comprehensive accessibility support
class AccessibilityInputSystem {
public:
    // Screen reader integration
    void SetScreenReaderMode(bool enabled);
    void AnnounceInputChange(const std::string& announcement);
    
    // Alternative input methods
    void EnableSwitchControl(const SwitchControlConfiguration& config);
    void EnableEyeControl(const EyeControlConfiguration& config);
    void EnableVoiceControl(const VoiceControlConfiguration& config);
    
    // Motor accessibility
    void SetDwellTime(std::chrono::milliseconds dwell_time);
    void EnableStickyKeys(bool enabled);
    void SetRepeatRate(float repeat_rate);
    
private:
    std::unique_ptr<ScreenReaderInterface> screen_reader_;
    std::unique_ptr<AlternativeInputProcessor> alternative_input_;
};
```

## Risk Mitigation

### Platform Compatibility Risks

- **Risk**: Input behavior differences between platforms causing gameplay inconsistencies
- **Mitigation**: Comprehensive cross-platform testing with automated input validation
- **Fallback**: Platform-specific input configuration overrides for edge cases

### Performance Risks

- **Risk**: Input processing latency impacting responsive gameplay
- **Mitigation**: Lock-free event processing with pre-allocated memory pools
- **Fallback**: Simplified input processing mode for performance-constrained scenarios

### WebAssembly Limitations

- **Risk**: Browser input API limitations preventing full feature implementation
- **Mitigation**: Feature detection with graceful degradation for unsupported capabilities
- **Fallback**: Alternative input methods for missing browser capabilities

### Configuration Complexity

- **Risk**: Complex action mapping configuration overwhelming developers and players
- **Mitigation**: Visual configuration tools and comprehensive defaults
- **Fallback**: Simple configuration mode with basic input mappings

This input system provides a robust foundation for cross-platform input handling while integrating seamlessly with the
established engine architecture and threading model. The design emphasizes low latency, memory efficiency, and
comprehensive accessibility support to meet diverse player needs.
