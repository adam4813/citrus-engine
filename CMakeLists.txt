cmake_minimum_required(VERSION 4.0)

# Honor CMAKE_MSVC_RUNTIME_LIBRARY (CMP0091) — must precede project()
cmake_policy(SET CMP0091 NEW)

# Project configuration
project(citrus-engine
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine - Modern C++20 Game Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(Dependencies)

if (NOT EMSCRIPTEN)
    include(StaticAnalysis)
endif ()

# Add subdirectories
add_subdirectory(src/engine)

# Testing setup
include(Testing)

# Install and packaging
include(Install)

# Documentation
include(Documentation)
