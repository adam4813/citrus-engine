cmake_minimum_required(VERSION 4.0)

# Honor CMAKE_MSVC_RUNTIME_LIBRARY (CMP0091). Required for static-runtime triplets
# like x64-windows-static so the project's own targets link against /MT[d] to match
# the statically-built vcpkg dependencies. Must be set before project().
cmake_policy(SET CMP0091 NEW)
if (VCPKG_TARGET_TRIPLET MATCHES "-static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

# ============================================================================
# Project Configuration (substituted by the editor's BuildStager)
# ============================================================================
project(
    @PROJECT_NAME@
    VERSION @PROJECT_VERSION@
    DESCRIPTION "@PROJECT_DESCRIPTION@"
    LANGUAGES CXX)

# ============================================================================
# Engine Dependencies
# ============================================================================
find_package(citrus-engine CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

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
target_link_libraries(${PROJECT_NAME} PRIVATE citrus-engine::engine-core nlohmann_json::nlohmann_json)

# Setup asset handling (required for both native and WASM). For WASM this also
# embeds the asset tree into the .data file, so prefer the packaged dir if the
# editor has supplied one - that's the post-processed copy that excludes .gitkeep
# and any cooked outputs.
if (NOT DEFINED CITRUS_PACKAGED_ASSETS_DIR)
    set(CITRUS_PACKAGED_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
endif ()
if (NOT DEFINED CITRUS_PACKAGED_PROJECT_JSON)
    set(CITRUS_PACKAGED_PROJECT_JSON "${CMAKE_CURRENT_SOURCE_DIR}/project.json")
endif ()
setup_asset_preload(${PROJECT_NAME} "${CITRUS_PACKAGED_ASSETS_DIR}")

# ============================================================================
# Platform-Specific Configuration
# ============================================================================
if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Embed project.json into the emscripten virtual filesystem alongside the
    # assets. The runtime searches exe_dir/project.json first; on wasm, exe_dir
    # is `/` and this makes the file resolvable there.
    target_link_options(${PROJECT_NAME} PRIVATE
        --embed-file "${CITRUS_PACKAGED_PROJECT_JSON}@/project.json")

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
        -sAUDIO_WORKLET
        -sWASM_WORKERS
        -sUSE_PTHREADS
        # Debug vs Release specific flags
        "$<$<CONFIG:Debug>:-sASSERTIONS=1;-sGL_DEBUG=1>"
        "$<$<NOT:$<CONFIG:Debug>>:-O3;--closure 1>")

    message(STATUS "Emscripten configuration applied")
    message(
        STATUS
        "Memory: Initial ${INITIAL_MEMORY_BYTES} bytes, Maximum ${MAXIMUM_MEMORY_BYTES} bytes")
endif ()

# ============================================================================
# Install Rules
# ============================================================================
# The editor's build pipeline stages project.json + assets into a packaging dir and
# passes their locations via CITRUS_PACKAGED_* cache vars (defaulted above to the
# in-tree project for IDE / CLI builds invoked outside the editor).
install(FILES "${CITRUS_PACKAGED_PROJECT_JSON}" DESTINATION bin)

if (NOT EMSCRIPTEN)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(
        DIRECTORY "${CITRUS_PACKAGED_ASSETS_DIR}/"
        DESTINATION bin/assets
        PATTERN ".gitkeep" EXCLUDE
        PATTERN ".gitignore" EXCLUDE)
else ()
    # Emscripten emits sibling files (.js, .wasm, .data) alongside the .html that
    # install(TARGETS) doesn't track. Install them explicitly, and rename the .html to
    # index.html so `python server.py` serves it from the root URL. Assets + project.json
    # are embedded in the .data file via --preload-file above, so they aren't copied here.
    install(FILES
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.js"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.wasm"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.data"
            DESTINATION bin)
    install(FILES "$<TARGET_FILE:${PROJECT_NAME}>" DESTINATION bin RENAME index.html)

    # Ship a tiny COOP/COEP-enabled Python server next to the build output so users can
    # run it with `python server.py` (those headers are required for SharedArrayBuffer /
    # pthreads to work in browsers).
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/server.py" DESTINATION bin)
endif ()
