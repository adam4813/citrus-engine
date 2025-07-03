# Platform detection and configuration
if(EMSCRIPTEN)
    set(PLATFORM_WASM TRUE)
    set(PLATFORM_NATIVE FALSE)
else()
    set(PLATFORM_WASM FALSE)
    set(PLATFORM_NATIVE TRUE)
endif()

message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "WASM Build: ${PLATFORM_WASM}")
message(STATUS "Native Build: ${PLATFORM_NATIVE}")
