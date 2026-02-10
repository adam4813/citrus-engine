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

# Add UI Layout mixin test
add_engine_test(ui_layout_test
    ui_layout_test.cpp
)

# Add UI Anchoring mixin test
add_engine_test(ui_anchoring_test
    ui_anchoring_test.cpp
)

# Add UI Scroll mixin test
add_engine_test(ui_scroll_test
    ui_scroll_test.cpp
)

# Add UI Divider element test
add_engine_test(ui_divider_test
    ui_divider_test.cpp
)

# Add UI ProgressBar element test
add_engine_test(ui_progress_bar_test
    ui_progress_bar_test.cpp
)

# Add UI TabContainer element test
add_engine_test(ui_tab_container_test
    ui_tab_container_test.cpp
)

# Add UI Tooltip component test
add_engine_test(ui_tooltip_test
    ui_tooltip_test.cpp
)

# Add Scene Serialization test
add_engine_test(scene_serialization_test
    scene_serialization_test.cpp
)

# Add scene assets test
add_engine_test(scene_assets_test
    scene_assets_test.cpp
)

# Add graph framework test
add_engine_test(graph_framework_test
    graph_framework_test.cpp
)

# Add data module test
add_engine_test(data_module_test
    data_module_test.cpp
)
