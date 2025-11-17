module;

#include <cstdint>
#include <functional>
#include <vector>

export module engine.input;

export namespace engine::input {
    // Cross-platform key code enum
    enum class KeyCode : uint16_t {
        UNKNOWN = 0,
        W, A, S, D,
        UP, DOWN, LEFT, RIGHT,
        ESCAPE, SPACE, ENTER,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        // Extend as needed
    };

    // Key event type
    enum class KeyEventType : uint8_t {
        DOWN,
        UP,
        REPEAT
    };

    // Key event struct
    struct KeyEvent {
        KeyCode key;
        KeyEventType type;
        bool just_pressed; // True if this is the first frame the key is down
    };

    // Key state struct for polling
    struct KeyState {
        bool held = false;
        bool just_pressed = false;
        bool just_released = false;
    };

    // Handler signature
    using KeyEventHandler = std::function<void(const KeyEvent &)>;

    // Input system API
    namespace Input {
        // Initialize input system (returns false on failure)
        [[nodiscard]] bool Initialize();

        void Shutdown() noexcept;

        // Poll input events (call once per frame, main thread)
        void PollEvents();

        // Polling API
        [[nodiscard]] bool IsKeyPressed(KeyCode key);

        [[nodiscard]] bool IsKeyJustPressed(KeyCode key);

        [[nodiscard]] bool IsKeyJustReleased(KeyCode key);

        [[nodiscard]] KeyState GetKeyState(KeyCode key);

        // Event registration
        // Per-key handler
        void RegisterKeyHandler(KeyCode key, KeyEventHandler handler);

        void UnregisterKeyHandler(KeyCode key, const KeyEventHandler &handler);

        // Global handler
        void RegisterGlobalKeyHandler(KeyEventHandler handler);

        void UnregisterGlobalKeyHandler(const KeyEventHandler &handler);
    }
}
