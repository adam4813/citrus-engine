cmake_minimum_required(VERSION 3.28)

# Examples project configuration
project(
    citrus-examples
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine Examples - Feature Demonstrations"
    LANGUAGES CXX)

# Find the Citrus Engine via vcpkg (uses overlay port from ../ports)
find_package(game-engine CONFIG REQUIRED)

# Examples application
add_executable(
    citrus-examples
    src/main.cpp
    src/example_scene.cpp
    src/scene_registry.cpp
    src/scene_switcher.cpp
    src/triangle_2d_scene.cpp
    src/cube_3d_scene.cpp
    src/scene_management_scene.cpp
    src/ui_image_scene.cpp)

target_include_directories(citrus-examples
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to the engine (provides all dependencies transitively)
target_link_libraries(citrus-examples PRIVATE game-engine::engine-core)

# Setup asset handling
setup_asset_preload(citrus-examples "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Platform-specific configuration
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")

  # Set memory limits
  set(INITIAL_MEMORY_BYTES 67108864) # 64MB initial memory
  set(MAXIMUM_MEMORY_BYTES 134217728) # 128MB maximum memory

    target_link_options(
        citrus-examples
        PRIVATE
        -sWASM=1
        -sFORCE_FILESYSTEM=1
        -sINITIAL_MEMORY=${INITIAL_MEMORY_BYTES}
        -sMAXIMUM_MEMORY=${MAXIMUM_MEMORY_BYTES}
        -sALLOW_MEMORY_GROWTH=1
        # Debug vs Release specific flags
        "$<$<CONFIG:Debug>:-sASSERTIONS=1;-sGL_DEBUG=1>"
        "$<$<NOT:$<CONFIG:Debug>>:-O3;--closure 1>")

    message(STATUS "Emscripten configuration applied")
    message(
        STATUS
        "Memory: Initial ${INITIAL_MEMORY_BYTES} bytes, Maximum ${MAXIMUM_MEMORY_BYTES} bytes"
    )
endif()

# Install target
install(TARGETS citrus-examples RUNTIME DESTINATION bin)

if(NOT EMSCRIPTEN)
    install(
        DIRECTORY assets/
        DESTINATION bin/assets
        PATTERN "*")
endif()
