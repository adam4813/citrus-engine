# Emscripten-specific configuration for WebAssembly builds

if (NOT EMSCRIPTEN)
    message(FATAL_ERROR "Emscripten.cmake should only be included for WASM builds")
endif ()

# Emscripten build settings
set(CMAKE_EXECUTABLE_SUFFIX ".html")

add_link_options(
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sUSE_GLFW=3
    -sALLOW_MEMORY_GROWTH=1
    -sWASM=1
    -sFORCE_FILESYSTEM=1
    -sINITIAL_MEMORY=67108864  # 64MB initial
    -sMAXIMUM_MEMORY=134217728 # 128MB maximum
    -sEXPORTED_RUNTIME_METHODS=cwrap
    -sSTACK_SIZE=1mb
    # Debug vs Release specific flags
    "$<$<CONFIG:Debug>:-sASSERTIONS=1;-sGL_DEBUG=1>"
    "$<$<NOT:$<CONFIG:Debug>>:-O3;--closure 1>"
)

# WASM-specific helper target for local serving
function(add_wasm_serve_target target_name)
    add_custom_target(serve-${target_name}
        COMMAND ${CMAKE_COMMAND} -E echo "Serving on http://localhost:8080/"
        COMMAND python -m http.server 8080
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${target_name}
        USES_TERMINAL
        COMMENT "Start a local HTTP server for WASM output"
    )
endfunction()

message(STATUS "Emscripten configuration applied")
message(STATUS "Memory: Initial ${INITIAL_MEMORY}MB, Maximum ${MAXIMUM_MEMORY}MB")
