# Engine Core Library Configuration

# Engine library configuration - set up as C++20 module
add_library(engine-core STATIC)

set_compiler_options(engine-core)

# Ensure debug information is available for library consumers
set_target_properties(engine-core PROPERTIES
    # Generate debug info for all configurations
    DEBUG_POSTFIX "d"
    # Ensure symbols are exported
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    # Keep debug info in separate files for MSVC
    COMPILE_PDB_NAME "engine-core"
    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Platform-specific debug info settings
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if (MSVC)
        # Only generate PDB files for debug builds
        target_compile_options(engine-core PRIVATE /Zi)
        target_link_options(engine-core PRIVATE /DEBUG /OPT:REF /OPT:ICF)

        # For static libraries, we need to handle PDB files differently
        set(ENGINE_PDB_FILE "${CMAKE_CURRENT_BINARY_DIR}/engine-core.pdb")

        # Install PDB files alongside the library (only if it exists)
        install(FILES "${ENGINE_PDB_FILE}"
            DESTINATION lib
            OPTIONAL)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Ensure debug symbols are preserved for debug builds
        target_compile_options(engine-core PRIVATE -g)
        target_link_options(engine-core PRIVATE -g)
    endif ()
endif ()

# C++20 module interface files
target_sources(engine-core
    PUBLIC
    FILE_SET CXX_MODULES FILES
    ai/blackboard.cppm
    ai/behavior_tree.cppm
    ai/ai.cppm
    assets/asset_manager.cppm
    assets/assets.cppm
    assets/tileset.cppm
    animation/animation_clip.cppm
    animation/animation_state.cppm
    animation/animator.cppm
    animation/animation_system.cppm
    animation/animation_serializer.cppm
    animation/animation.cppm
    audio/audio_clip.cppm
    audio/audio_source.cppm
    audio/audio_listener.cppm
    audio/audio_system.cppm
    audio/audio.cppm
    platform/platform.cppm
    components/components.cppm
    data/data_asset.cppm
    data/data_table.cppm
    data/data_asset_registry.cppm
    data/data_serializer.cppm
    data/data.cppm
    ecs/component_registry.cppm
    ecs/flecs_ecs.cppm
    input/input.cppm
    os/os.cppm
    graph/types.cppm
    graph/node_graph.cppm
    graph/node_type_registry.cppm
    graph/graph_evaluator.cppm
    graph/graph_serializer.cppm
    graph/graph.cppm
    rendering/types.cppm
    rendering/texture.cppm
    rendering/shader.cppm
    rendering/mesh.cppm
    rendering/material.cppm
    rendering/framebuffer.cppm
    rendering/renderer.cppm
    rendering/tilemap_renderer.cppm
    rendering/rendering.cppm
    scene/scene_assets.cppm
    scene/scene_serializer.cppm
    scene/prefab.cppm
    scene/scene.cppm
    scripting/scripting_backend.cppm
    scripting/scripting_system.cppm
    ui/ui.cppm
    ui/types.cppm
    ui/text_renderer.cppm
    ui/mouse_event.cppm
    ui/mouse_event_manager.cppm
    ui/ui_element.cppm
    ui/batch_renderer.cppm
    ui/batch_renderer/batch_types.cppm
    ui/batch_renderer/batch_renderer.cppm
    ui/elements/text.cppm
    ui/elements/panel.cppm
    ui/elements/button.cppm
    ui/elements/label.cppm
    ui/elements/slider.cppm
    ui/elements/checkbox.cppm
    ui/elements/divider.cppm
    ui/elements/progress_bar.cppm
    ui/elements/tab_container.cppm
    ui/elements/confirmation_dialog.cppm
    ui/elements/image_element.cppm
    ui/elements/container.cppm
    ui/elements/elements.cppm
    ui/components/layout.cppm
    ui/components/constraints.cppm
    ui/components/scroll.cppm
    ui/components/tooltip.cppm
    ui/components/components.cppm
    ui/builder.cppm
    engine.cppm  # Main module last - depends on others
)

# Add the GLM module interface. The default GLM package does not correctly export the C++20 module interface,
if (EXISTS "${SHARED_MODULES_DIR}/glm.cppm")
    target_sources(engine-core
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${SHARED_MODULES_DIR}
        FILES ${SHARED_MODULES_DIR}/glm.cppm
    )
    target_compile_definitions(engine-core PUBLIC GLM_ENABLE_EXPERIMENTAL GLM_GTC_INLINE_NAMESPACE GLM_EXT_INLINE_NAMESPACE GLM_GTX_INLINE_NAMESPACE)
endif ()

# Engine implementation source files
target_sources(engine-core PRIVATE
    # AI module implementation
    ai/behavior_tree.cpp

    # Animation module implementation
    animation/animation_clip.cpp
    animation/animation_state.cpp
    animation/animation_system.cpp
    animation/animation_serializer.cpp

    # Assets module implementation
    assets/asset_manager.cpp
    assets/tileset.cpp

    # Data module implementation
    data/data_serializer.cpp

    ecs/component_registry.cpp
    ecs/ecs_world.cpp

    # Graph module implementation
    graph/types.cpp
    graph/node_graph.cpp
    graph/node_type_registry.cpp
    graph/graph_evaluator.cpp
    graph/graph_serializer.cpp

    input/input.cpp

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
    rendering/framebuffer.cpp
    rendering/tilemap_renderer.cpp

    # Scene module implementation
    scene/scene_assets.cpp
    scene/scene_manager.cpp
    scene/scene_serializer.cpp
    scene/prefab.cpp

    # Scripting module implementation
    scripting/lua_backend.cpp
    scripting/angelscript_backend.cpp
    scripting/scripting_factory.cpp

    ui/text_renderer.cpp
    ui/batch_renderer/batch_renderer.cpp

    engine.cpp
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
    glm::glm-header-only
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    imgui::imgui
    PRIVATE
    Angelscript::angelscript
    lua
)

if (EMSCRIPTEN)
    target_link_options(engine-core
        PUBLIC
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sUSE_GLFW=3
        -sSTACK_SIZE=1mb
        -EXPORTED_RUNTIME_METHODS=cwrap
        -sALLOW_MEMORY_GROWTH=1
    )
endif ()

target_compile_features(engine-core PUBLIC cxx_std_20)
target_compile_definitions(engine-core PUBLIC NOMINMAX)

if (NOT EMSCRIPTEN)
    # Native builds need glad for OpenGL function loading
    target_link_libraries(engine-core PUBLIC glad::glad)

    # Platform-specific libraries
    if (UNIX)
        # Linux: Additional system libraries for X11 and threading
        target_link_libraries(engine-core PRIVATE X11 pthread dl)
    endif ()
endif ()

message(STATUS "Engine core library configured")
