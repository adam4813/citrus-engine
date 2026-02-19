cmake_minimum_required(VERSION 3.30)

# ============================================================================
# Project Configuration
# ============================================================================
# TODO: Replace "my-game" with your project name (must match vcpkg.json "name")
project(
    my-game
    VERSION 0.1.0
    DESCRIPTION "A game built with Citrus Engine"
    LANGUAGES CXX)

# ============================================================================
# Engine Dependencies
# ============================================================================
find_package(citrus-engine CONFIG REQUIRED)

# ============================================================================
# Game Executable
# ============================================================================
# Add your source files here
add_executable(
    ${PROJECT_NAME}
    src/main.cpp)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link to the engine (provides all dependencies transitively)
target_link_libraries(${PROJECT_NAME} PRIVATE citrus-engine::engine-core)

# Setup asset handling (required for both native and WASM)
setup_asset_preload(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# ============================================================================
# Platform-Specific Configuration
# ============================================================================
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Set memory limits (adjust as needed for your game)
    set(INITIAL_MEMORY_BYTES 67108864)  # 64MB initial memory
    set(MAXIMUM_MEMORY_BYTES 134217728) # 128MB maximum memory

    target_link_options(
        ${PROJECT_NAME}
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

# ============================================================================
# Install Rules
# ============================================================================
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

if(NOT EMSCRIPTEN)
    install(
        DIRECTORY assets/
        DESTINATION bin/assets
        PATTERN "*")
endif()
