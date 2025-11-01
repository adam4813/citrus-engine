cmake_minimum_required(VERSION 3.28)

# Add sanity check test
add_engine_test(sanity_check_test
    sanity_check_test.cpp
)

# Add renderer test
add_engine_test(renderer_test
    renderer_test.cpp
)

