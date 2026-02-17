cmake_minimum_required(VERSION 3.30)

# Project configuration
project(citrus-engine
    VERSION 0.0.1
    DESCRIPTION "Citrus Engine - Modern C++20 Game Engine"
    LANGUAGES CXX
)

# C++20 requirements
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
