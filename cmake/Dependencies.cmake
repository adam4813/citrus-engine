# Find and configure all project dependencies

# Core runtime dependencies (always required to build the library)
find_package(Angelscript CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(flecs CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_path(MINIAUDIO_INCLUDE_DIR "miniaudio.h" REQUIRED)
if(NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio::miniaudio INTERFACE IMPORTED)
    target_include_directories(miniaudio::miniaudio INTERFACE "${MINIAUDIO_INCLUDE_DIR}")
endif()
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(unofficial-lua CONFIG REQUIRED)

# Native-only dependency
if (NOT EMSCRIPTEN)
    find_package(glad CONFIG REQUIRED)
endif ()

# Create a shared modules directory in your project
set(SHARED_MODULES_DIR "${CMAKE_SOURCE_DIR}/build/shared-modules")

# Copy modules to shared directory, since each preset creates its own copy, intellisense fails to resolve them correctly
if (NOT EXISTS "${SHARED_MODULES_DIR}/glm.cppm")
    find_package(glm CONFIG REQUIRED)
    find_file(GLM_MODULE_FILE "glm.cppm"
        PATHS ${glm_DIR}/../../include/glm
        NO_DEFAULT_PATH)

    if (GLM_MODULE_FILE)
        file(COPY ${GLM_MODULE_FILE} DESTINATION ${SHARED_MODULES_DIR})
    endif ()
endif ()

# Log found packages
message(STATUS "Found dependencies:")
message(STATUS "  Flecs: ${flecs_VERSION}")
message(STATUS "  GLFW: ${glfw3_VERSION}")
if (NOT EMSCRIPTEN)
    message(STATUS "  GLAD: ${glad_VERSION}")
endif ()
message(STATUS "  GLM: ${glm_VERSION}")
message(STATUS "  spdlog: ${spdlog_VERSION}")
message(STATUS "  nlohmann_json: ${nlohmann_json_VERSION}")
message(STATUS "  STB: Found")
message(STATUS "  AngelScript: Found")
message(STATUS "  Lua: Found")
message(STATUS "  JoltPhysics: Found")
message(STATUS "  Bullet3: Found")
message(STATUS "  miniaudio: Found")
