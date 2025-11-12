cmake_minimum_required(VERSION 3.28)

# Add sanity check test
add_engine_test(sanity_check_test
    sanity_check_test.cpp
)

# Add scripting system test
add_engine_test(scripting_test
    scripting_test.cpp
)

# Add renderer test
add_engine_test(renderer_test
    renderer_test.cpp
)

# Add UI element test
add_engine_test(ui_element_test
    ui_element_test.cpp
)

# Add UI color test
add_engine_test(ui_color_test
    ui_color_test.cpp
)

