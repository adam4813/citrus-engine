set(VCPKG_ENV_PASSTHROUGH_UNTRACKED EMSCRIPTEN_ROOT EMSDK PATH)
# Pass EMCC_CFLAGS through TRACKED so changing it invalidates the binary cache and
# forces deps (notably flecs) to rebuild with the new flags. Without this, the cache
# can hand back stale objects compiled without -pthread.
set(VCPKG_ENV_PASSTHROUGH EMCC_CFLAGS)

if (NOT DEFINED ENV{EMSCRIPTEN_ROOT})
    find_path(EMSCRIPTEN_ROOT "emcc")
else ()
    set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
endif ()

if (NOT EMSCRIPTEN_ROOT)
    if (NOT DEFINED ENV{EMSDK})
        message(FATAL_ERROR "The emcc compiler not found in PATH")
    endif ()
    set(EMSCRIPTEN_ROOT "$ENV{EMSDK}/upstream/emscripten")
endif ()

if (NOT EXISTS "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
    message(FATAL_ERROR "Emscripten.cmake toolchain file not found")
endif ()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")

# Build every vcpkg dependency with -pthread so the resulting wasm objects expose
# the atomics + bulk-memory features that emscripten's pthread runtime requires.
# Without this, mixing engine code (compiled with -pthread) and a vcpkg-built flecs
# (compiled without) yields a wasm-ld "--shared-memory is disallowed by X because it
# was not compiled with atomics or bulk-memory features" link error.
set(VCPKG_C_FLAGS "-pthread")
set(VCPKG_CXX_FLAGS "-pthread")
set(VCPKG_LINKER_FLAGS "-pthread")
