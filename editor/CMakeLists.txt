cmake_minimum_required(VERSION 3.28)

# Scene Editor project configuration
project(
    citrus-scene-editor
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine 2D Scene Editor"
    LANGUAGES CXX)

# Find the Citrus Engine via vcpkg (uses overlay port from ../ports)
find_package(citrus-engine CONFIG REQUIRED)

# Scene Editor application
add_executable(
    citrus-scene-editor
    src/main.cpp
    src/animation_editor_panel.cpp
    src/asset_browser_panel.cpp
    src/code_editor_panel.cpp
    src/command.cpp
    src/commands/clipboard_commands.cpp
    src/data_table_editor_panel.cpp
    src/editor_scene.cpp
    src/graph_editor_panel.cpp
    src/hierarchy_panel.cpp
    src/properties_panel.cpp
    src/shader_editor_panel.cpp
    src/texture_editor_panel.cpp
    src/tileset_editor_panel.cpp
    src/viewport_panel.cpp)

target_include_directories(citrus-scene-editor
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to the engine (provides all dependencies transitively)
target_link_libraries(citrus-scene-editor PRIVATE citrus-engine::engine-core)

# Setup asset handling
setup_asset_preload(citrus-scene-editor "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Platform-specific configuration
if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")

  # Set memory limits
  set(INITIAL_MEMORY_BYTES 67108864) # 64MB initial memory
  set(MAXIMUM_MEMORY_BYTES 134217728) # 128MB maximum memory

    target_link_options(
        citrus-scene-editor
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
        "Memory: Initial ${INITIAL_MEMORY_BYTES} bytes, Maximum ${MAXIMUM_MEMORY_BYTES} bytes")
endif()

# Install target
install(TARGETS citrus-scene-editor RUNTIME DESTINATION bin)

if(NOT EMSCRIPTEN)
    install(
        DIRECTORY assets/
        DESTINATION bin/assets
        PATTERN "*")
endif()
