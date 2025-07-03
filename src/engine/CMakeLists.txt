# Engine Core Library Configuration

# Engine library configuration - set up as C++20 module
add_library(engine-core)

# C++20 module interface files
target_sources(engine-core
    PUBLIC
    FILE_SET CXX_MODULES FILES
    assets/asset_manager.cppm
    assets/assets.cppm
    platform/platform.cppm
    components/components.cppm
    ecs/flecs_ecs.cppm
    os/os.cppm
    rendering/types.cppm
    rendering/texture.cppm
    rendering/shader.cppm
    rendering/mesh.cppm
    rendering/material.cppm
    rendering/renderer.cppm
    rendering/rendering.cppm
    scene/scene.cppm
    engine.cppm  # Main module last - depends on others
)

# Engine implementation source files
target_sources(engine-core PRIVATE
    # Assets module implementation
    assets/asset_manager.cpp

    # OS module implementation
    os/os.cpp

    # Platform module implementation
    platform/platform.cpp
    platform/file_system.cpp
    platform/timing.cpp
    platform/memory.cpp

    # Rendering module implementation
    rendering/renderer.cpp
    rendering/shader.cpp
    rendering/texture.cpp
    rendering/mesh.cpp
    rendering/material.cpp

    # Scene module implementation
    scene/scene_manager.cpp
)

# Engine include directories
target_include_directories(engine-core
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${STB_INCLUDE_DIRS}
)

# Engine dependencies
target_link_libraries(engine-core
    PUBLIC
    $<IF:$<TARGET_EXISTS:flecs::flecs>,flecs::flecs,flecs::flecs_static>
    glfw
    #glm::glm-header-only
    glm-module # Use the C++20 module version of GLM created in Dependencies.cmake
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    imgui::imgui
)

target_compile_features(engine-core PUBLIC cxx_std_20)
target_compile_definitions(engine-core PUBLIC
    PLATFORM_WASM=0
    PLATFORM_NATIVE=1
    NOMINMAX
)

if (PLATFORM_NATIVE)
    # Native builds need glad for OpenGL function loading
    target_link_libraries(engine-core PUBLIC glad::glad)

    # Platform-specific libraries
    if (UNIX)
        # Linux: Additional system libraries for X11 and threading
        target_link_libraries(engine-core PRIVATE X11 pthread dl)
    endif ()
endif ()

message(STATUS "Engine core library configured")
