# Emscripten-specific configuration for WebAssembly builds

if (NOT PLATFORM_WASM)
    message(FATAL_ERROR "Emscripten.cmake should only be included for WASM builds")
endif ()

# Emscripten build settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")

# Emscripten link options
# Check if assets directory exists and has files before preloading
file(GLOB_RECURSE ASSET_FILES "${CMAKE_SOURCE_DIR}/assets/*")
if (ASSET_FILES)
    set(PRELOAD_ASSETS_FLAG "--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets")
    list(LENGTH ASSET_FILES ASSET_FILE_COUNT)
    message(STATUS "WASM: Preloading ${ASSET_FILE_COUNT} asset files")
else ()
    set(PRELOAD_ASSETS_FLAG "")
    message(WARNING "WASM: No assets found in ${CMAKE_SOURCE_DIR}/assets - skipping asset preloading")
endif ()

add_link_options(
    -ogame_wasm.html
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sFULL_ES3=1
    -sUSE_GLFW=3
    -sALLOW_MEMORY_GROWTH=1
    -sWASM=1
    -sFORCE_FILESYSTEM=1
    -sINITIAL_MEMORY=67108864  # 64MB initial
    -sMAXIMUM_MEMORY=134217728 # 128MB maximum
    -sEXPORTED_RUNTIME_METHODS=cwrap
    -sSTACK_SIZE=1mb
    --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
)

# Debug vs Release specific settings
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_link_options(-s ASSERTIONS=1 -s GL_DEBUG=1)
    message(STATUS "WASM Debug build: Assertions and GL debugging enabled")
else ()
    add_link_options(-O3 --closure 1)
    message(STATUS "WASM Release build: Optimizations and closure compiler enabled")
endif ()

# WASM-specific helper target for local serving
function(add_wasm_serve_target target_name)
    add_custom_target(serve-wasm
        COMMAND ${CMAKE_COMMAND} -E echo "Serving on http://localhost:8080/"
        COMMAND python -m http.server 8080
        WORKING_DIRECTORY $<TARGET_FILE_DIR:game-engine-demo>
        USES_TERMINAL
        COMMENT "Start a local HTTP server for WASM output"
    )
endfunction()

message(STATUS "Emscripten configuration applied")
message(STATUS "Memory: Initial ${INITIAL_MEMORY}MB, Maximum ${MAXIMUM_MEMORY}MB")
