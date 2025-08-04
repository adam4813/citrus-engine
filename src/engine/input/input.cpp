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
        const std::unordered_map<int, KeyCode> GLFW_TO_KEYCODE = {
            {GLFW_KEY_W, KeyCode::W}, {GLFW_KEY_A, KeyCode::A}, {GLFW_KEY_S, KeyCode::S}, {GLFW_KEY_D, KeyCode::D},
            {GLFW_KEY_UP, KeyCode::UP}, {GLFW_KEY_DOWN, KeyCode::DOWN}, {GLFW_KEY_LEFT, KeyCode::LEFT},
            {GLFW_KEY_RIGHT, KeyCode::RIGHT},
            {GLFW_KEY_ESCAPE, KeyCode::ESCAPE}, {GLFW_KEY_SPACE, KeyCode::SPACE}, {GLFW_KEY_ENTER, KeyCode::ENTER}
        };
        std::atomic initialized{false};

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
        glfwPollEvents();
        std::lock_guard lock(event_mutex);
        for (auto &state: key_states | std::views::values) {
            state.just_pressed = false;
            state.just_released = false;
        }
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

