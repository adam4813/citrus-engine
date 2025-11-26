cmake_minimum_required(VERSION 3.28)

# Add sanity check test
add_engine_test(sanity_check_test
    sanity_check_test.cpp
)

# Add scripting system test
add_engine_test(scripting_test
    scripting_test.cpp
)

# Add input mouse test
add_engine_test(input_mouse_test
    input_mouse_test.cpp
)

# Add renderer test
add_engine_test(renderer_test
    renderer_test.cpp
)

# Add UI mouse event test
add_engine_test(ui_mouse_event_test
    ui_mouse_event_test.cpp
)

# Add UI element test
add_engine_test(ui_element_test
    ui_element_test.cpp
)

# Add UI color test
add_engine_test(ui_color_test
    ui_color_test.cpp
)

# Add UI text test
add_engine_test(ui_text_test
    ui_text_test.cpp
)

# Add UI Panel component test
add_engine_test(ui_panel_test
    ui_panel_test.cpp
)

# Add UI Button component test
add_engine_test(ui_button_test
    ui_button_test.cpp
)

# Add UI Label component test
add_engine_test(ui_label_test
    ui_label_test.cpp
)

# Add UI Slider component test
add_engine_test(ui_slider_test
    ui_slider_test.cpp
)

# Add UI Checkbox component test
add_engine_test(ui_checkbox_test
    ui_checkbox_test.cpp
)

# Add UI ConfirmationDialog component test
add_engine_test(ui_confirmation_dialog_test
    ui_confirmation_dialog_test.cpp
)

# Add UI Theme test
add_engine_test(ui_theme_test
    ui_theme_test.cpp
)

# Add physics system test
add_engine_test(physics_test
    physics_test.cpp
)
