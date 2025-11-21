export module engine.ui:mouse_event;

import engine.input;

export namespace engine::ui {
    /**
     * @brief Mouse event structure for UI interaction
     * 
     * Contains mouse position, button states, and scroll information.
     * Designed for cross-platform compatibility (native, web).
     * 
     * Button states:
     * - left_down/right_down: Button is currently held
     * - left_pressed/right_pressed: Button was just pressed this frame
     * - left_released/right_released: Button was just released this frame
     * 
     * Usage pattern:
     * ```cpp
     * bool OnClick(const MouseEvent& event) {
     *     if (event.left_pressed && Contains(event.x, event.y)) {
     *         HandleClick();
     *         return true;  // Event consumed
     *     }
     *     return false;  // Event not handled
     * }
     * ```
     */
    struct MouseEvent {
        float x{0.0f};              ///< Mouse X position (screen space)
        float y{0.0f};              ///< Mouse Y position (screen space)
        
        bool left_down{false};      ///< Left button is currently held
        bool right_down{false};     ///< Right button is currently held
        bool middle_down{false};    ///< Middle button is currently held
        
        bool left_pressed{false};   ///< Left button was just pressed this frame
        bool right_pressed{false};  ///< Right button was just pressed this frame
        bool middle_pressed{false}; ///< Middle button was just pressed this frame
        
        bool left_released{false};  ///< Left button was just released this frame
        bool right_released{false}; ///< Right button was just released this frame
        bool middle_released{false};///< Middle button was just released this frame
        
        float scroll_delta{0.0f};   ///< Vertical scroll delta (positive = scroll up)
        
        /**
         * @brief Default constructor - creates event at origin with no buttons pressed
         */
        MouseEvent() = default;
        
        /**
         * @brief Construct mouse event with position and button states
         * 
         * @param x Mouse X coordinate
         * @param y Mouse Y coordinate
         * @param left_down Left button held state
         * @param right_down Right button held state
         * @param left_pressed Left button just pressed
         * @param right_pressed Right button just pressed
         * @param scroll Scroll wheel delta
         */
        MouseEvent(float x, float y, 
                  bool left_down = false, bool right_down = false,
                  bool left_pressed = false, bool right_pressed = false,
                  float scroll = 0.0f)
            : x(x), y(y)
            , left_down(left_down), right_down(right_down)
            , left_pressed(left_pressed), right_pressed(right_pressed)
            , scroll_delta(scroll) {}
        
        /**
         * @brief Construct mouse event from input system's MouseState
         * 
         * Convenience function to convert engine input state to UI event format.
         * 
         * @param state MouseState from engine::input::Input::GetMouseState()
         */
        explicit MouseEvent(const engine::input::MouseState& state)
            : x(state.x), y(state.y)
            , left_down(state.left_down), right_down(state.right_down), middle_down(state.middle_down)
            , left_pressed(state.left_pressed), right_pressed(state.right_pressed), middle_pressed(state.middle_pressed)
            , left_released(state.left_released), right_released(state.right_released), middle_released(state.middle_released)
            , scroll_delta(state.scroll_delta) {}
    };

    /**
     * @brief Mouse event handler interface for UI elements
     * 
     * Implement this interface to receive mouse events.
     * Return true from event handlers to consume the event and stop propagation.
     */
    class IMouseInteractive {
    public:
        virtual ~IMouseInteractive() = default;
        
        /**
         * @brief Called when mouse enters or moves within element bounds
         * @param event Current mouse state
         * @return true if event was handled (stops propagation)
         */
        virtual bool OnHover(const MouseEvent& event) { return false; }
        
        /**
         * @brief Called when mouse button is clicked within element bounds
         * @param event Current mouse state
         * @return true if event was handled (stops propagation)
         */
        virtual bool OnClick(const MouseEvent& event) { return false; }
        
        /**
         * @brief Called when mouse is dragged (button held and moving)
         * @param event Current mouse state
         * @return true if event was handled (stops propagation)
         */
        virtual bool OnDrag(const MouseEvent& event) { return false; }
        
        /**
         * @brief Called when scroll wheel is used within element bounds
         * @param event Current mouse state (includes scroll_delta)
         * @return true if event was handled (stops propagation)
         */
        virtual bool OnScroll(const MouseEvent& event) { return false; }
    };
}
