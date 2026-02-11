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
        Q, E, R,
        X,
        UP, DOWN, LEFT, RIGHT,
        ESCAPE, SPACE, ENTER,
        LEFT_SHIFT, RIGHT_SHIFT,
        LEFT_CONTROL, RIGHT_CONTROL,
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

    // Mouse button enum
    enum class MouseButton : uint8_t {
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2
    };

    // Mouse state struct
    struct MouseState {
        float x = 0.0f;
        float y = 0.0f;
        bool left_down = false;
        bool right_down = false;
        bool middle_down = false;
        bool left_pressed = false;
        bool right_pressed = false;
        bool middle_pressed = false;
        bool left_released = false;
        bool right_released = false;
        bool middle_released = false;
        float scroll_delta_x = 0.0f;
        float scroll_delta_y = 0.0f;
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

        // Keyboard polling API
        [[nodiscard]] bool IsKeyPressed(KeyCode key);

        [[nodiscard]] bool IsKeyJustPressed(KeyCode key);

        [[nodiscard]] bool IsKeyJustReleased(KeyCode key);

        [[nodiscard]] KeyState GetKeyState(KeyCode key);

        // Mouse polling API
        [[nodiscard]] MouseState GetMouseState();
        
        [[nodiscard]] float GetMouseX();
        
        [[nodiscard]] float GetMouseY();
        
        [[nodiscard]] bool IsMouseButtonDown(MouseButton button);
        
        [[nodiscard]] bool IsMouseButtonPressed(MouseButton button);
        
        [[nodiscard]] bool IsMouseButtonReleased(MouseButton button);

        // Event registration
        // Per-key handler
        void RegisterKeyHandler(KeyCode key, KeyEventHandler handler);

        void UnregisterKeyHandler(KeyCode key, const KeyEventHandler &handler);

        // Global handler
        void RegisterGlobalKeyHandler(KeyEventHandler handler);

        void UnregisterGlobalKeyHandler(const KeyEventHandler &handler);
    }
}
