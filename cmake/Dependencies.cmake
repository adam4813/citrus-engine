# Find and configure all project dependencies

# Find packages
find_package(flecs CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Hack to convert glm to a C++20 module target
add_library(glm-module)
target_sources(glm-module PUBLIC
    FILE_SET CXX_MODULES
    FILES ${glm_DIR}/../../include/glm/glm.cppm # glm_DIR is where the glmConfig.cmake is located
)
target_link_libraries(glm-module PUBLIC glm::glm)
target_compile_features(glm-module PUBLIC cxx_std_20)
target_compile_definitions(glm-module PUBLIC GLM_ENABLE_EXPERIMENTAL)

# Log found packages
message(STATUS "Found dependencies:")
message(STATUS "  Flecs: ${flecs_VERSION}")
message(STATUS "  GLFW: ${glfw3_VERSION}")
message(STATUS "  GLAD: ${glad_VERSION}")
message(STATUS "  GLM: ${glm_VERSION}")
message(STATUS "  spdlog: ${spdlog_VERSION}")
message(STATUS "  nlohmann_json: ${nlohmann_json_VERSION}")
message(STATUS "  ImGui: Found")
message(STATUS "  STB: Found")
message(STATUS "  GoogleTest: Found")
