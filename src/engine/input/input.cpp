module;

#include <GLFW/glfw3.h>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <ranges>

module engine.input;

namespace engine::input {
    namespace {
        std::mutex event_mutex;
        std::unordered_map<KeyCode, std::vector<KeyEventHandler> > key_handlers;
        std::vector<KeyEventHandler> global_handlers;
        std::unordered_map<KeyCode, KeyState> key_states;
        
        // Mouse state tracking
        MouseState mouse_state;
        MouseState prev_mouse_state;
        
        const std::unordered_map<int, KeyCode> GLFW_TO_KEYCODE = {
            {GLFW_KEY_W, KeyCode::W}, {GLFW_KEY_A, KeyCode::A}, {GLFW_KEY_S, KeyCode::S}, {GLFW_KEY_D, KeyCode::D},
            {GLFW_KEY_UP, KeyCode::UP}, {GLFW_KEY_DOWN, KeyCode::DOWN}, {GLFW_KEY_LEFT, KeyCode::LEFT},
            {GLFW_KEY_RIGHT, KeyCode::RIGHT},
            {GLFW_KEY_ESCAPE, KeyCode::ESCAPE}, {GLFW_KEY_SPACE, KeyCode::SPACE}, {GLFW_KEY_ENTER, KeyCode::ENTER}
        };
        std::atomic initialized{false};

        void ScrollCallback([[maybe_unused]] GLFWwindow *window, double x_offset, double y_offset) {
            const std::lock_guard lock(event_mutex);
            mouse_state.scroll_delta_x += static_cast<float>(x_offset);
            mouse_state.scroll_delta_y += static_cast<float>(y_offset);
        }

        void KeyCallback([[maybe_unused]] GLFWwindow *window, const int key, [[maybe_unused]] int key_code,
                         const int action, int) {
            const auto it = GLFW_TO_KEYCODE.find(key);
            if (it == GLFW_TO_KEYCODE.end()) { return; }
            const KeyCode code = it->second;
            const KeyEventType type = (action == GLFW_PRESS)
                                          ? KeyEventType::DOWN
                                          : (action == GLFW_RELEASE)
                                                ? KeyEventType::UP
                                                : KeyEventType::REPEAT;
            const std::lock_guard lock(event_mutex);
            auto &[held, just_pressed, just_released] = key_states[code];
            if (type == KeyEventType::DOWN) {
                just_pressed = !held;
                held = true;
            } else if (type == KeyEventType::UP) {
                just_released = held;
                held = false;
            } else if (type == KeyEventType::REPEAT) {
                // No state change for repeat
            }
            const KeyEvent event{code, type, action == GLFW_PRESS};
            for (auto &handler: key_handlers[code]) { handler(event); }
            for (auto &handler: global_handlers) { handler(event); }
        }
    }

    bool Input::Initialize() {
        if (initialized) { return true; }
        if (!glfwInit()) { return false; }
        GLFWwindow *window = glfwGetCurrentContext();
        if (!window) { return false; }
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetScrollCallback(window, ScrollCallback);
        initialized = true;
        return true;
    }

    void Input::Shutdown() noexcept {
        initialized = false;
        key_handlers.clear();
        global_handlers.clear();
        key_states.clear();
    }

    void Input::PollEvents() {
        if (!initialized) { return; }
        
        GLFWwindow *window = glfwGetCurrentContext();
        if (!window) { return; }
        
        // Reset scroll delta before polling. The scroll callback accumulates new values
        // during glfwPollEvents() via +=, so resetting first ensures we only capture
        // scroll events from the current frame.
        {
            std::lock_guard lock(event_mutex);
            mouse_state.scroll_delta_x = 0.0f;
            mouse_state.scroll_delta_y = 0.0f;
        }
        
        glfwPollEvents();
        
        std::lock_guard lock(event_mutex);
        
        // Clear just_pressed/just_released for keys
        for (auto &state: key_states | std::views::values) {
            state.just_pressed = false;
            state.just_released = false;
        }
        
        // Update mouse state
        prev_mouse_state = mouse_state;
        
        // Get mouse position
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        mouse_state.x = static_cast<float>(mouse_x);
        mouse_state.y = static_cast<float>(mouse_y);
        
        // Get mouse button states
        bool left_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool right_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool middle_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        
        // Detect press/release events
        mouse_state.left_down = left_down;
        mouse_state.right_down = right_down;
        mouse_state.middle_down = middle_down;
        mouse_state.left_pressed = left_down && !prev_mouse_state.left_down;
        mouse_state.right_pressed = right_down && !prev_mouse_state.right_down;
        mouse_state.middle_pressed = middle_down && !prev_mouse_state.middle_down;
        mouse_state.left_released = !left_down && prev_mouse_state.left_down;
        mouse_state.right_released = !right_down && prev_mouse_state.right_down;
        mouse_state.middle_released = !middle_down && prev_mouse_state.middle_down;
    }

    bool Input::IsKeyPressed(const KeyCode key) {
        std::lock_guard lock(event_mutex);
        const auto it = key_states.find(key);
        return it != key_states.end() && it->second.held;
    }

    bool Input::IsKeyJustPressed(const KeyCode key) {
        std::lock_guard lock(event_mutex);
        const auto it = key_states.find(key);
        return it != key_states.end() && it->second.just_pressed;
    }

    bool Input::IsKeyJustReleased(const KeyCode key) {
        std::lock_guard lock(event_mutex);
        const auto it = key_states.find(key);
        return it != key_states.end() && it->second.just_released;
    }

    KeyState Input::GetKeyState(const KeyCode key) {
        std::lock_guard lock(event_mutex);
        const auto it = key_states.find(key);
        return it != key_states.end() ? it->second : KeyState{};
    }

    MouseState Input::GetMouseState() {
        std::lock_guard lock(event_mutex);
        return mouse_state;
    }

    float Input::GetMouseX() {
        std::lock_guard lock(event_mutex);
        return mouse_state.x;
    }

    float Input::GetMouseY() {
        std::lock_guard lock(event_mutex);
        return mouse_state.y;
    }

    bool Input::IsMouseButtonDown(const MouseButton button) {
        std::lock_guard lock(event_mutex);
        switch (button) {
            case MouseButton::LEFT: return mouse_state.left_down;
            case MouseButton::RIGHT: return mouse_state.right_down;
            case MouseButton::MIDDLE: return mouse_state.middle_down;
            default: return false;
        }
    }

    bool Input::IsMouseButtonPressed(const MouseButton button) {
        std::lock_guard lock(event_mutex);
        switch (button) {
            case MouseButton::LEFT: return mouse_state.left_pressed;
            case MouseButton::RIGHT: return mouse_state.right_pressed;
            case MouseButton::MIDDLE: return mouse_state.middle_pressed;
            default: return false;
        }
    }

    bool Input::IsMouseButtonReleased(const MouseButton button) {
        std::lock_guard lock(event_mutex);
        switch (button) {
            case MouseButton::LEFT: return mouse_state.left_released;
            case MouseButton::RIGHT: return mouse_state.right_released;
            case MouseButton::MIDDLE: return mouse_state.middle_released;
            default: return false;
        }
    }

    void Input::RegisterKeyHandler(const KeyCode key, KeyEventHandler handler) {
        std::lock_guard lock(event_mutex);
        key_handlers[key].push_back(std::move(handler));
    }

    void Input::UnregisterKeyHandler(const KeyCode key, const KeyEventHandler &handler) {
        std::lock_guard lock(event_mutex);
        auto &vec = key_handlers[key];
        std::erase_if(vec, [&](const KeyEventHandler &h) {
            return h.target_type() == handler.target_type();
        });
    }

    void Input::RegisterGlobalKeyHandler(KeyEventHandler handler) {
        std::lock_guard lock(event_mutex);
        global_handlers.push_back(std::move(handler));
    }

    void Input::UnregisterGlobalKeyHandler(const KeyEventHandler &handler) {
        std::lock_guard lock(event_mutex);
        std::erase_if(global_handlers,
                      [&](const KeyEventHandler &h) {
                          return h.target_type() == handler.target_type();
                      });
    }
} // namespace engine::input

