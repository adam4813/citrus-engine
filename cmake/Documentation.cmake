# Documentation build targets for Citrus Engine
# Integrates Doxygen and MkDocs into CMake build system

# Find required tools
find_package(Doxygen)
find_program(MKDOCS_EXECUTABLE mkdocs)
find_program(PYTHON_EXECUTABLE python3)

# Option to build documentation
option(BUILD_DOCS "Build documentation with Doxygen and MkDocs" OFF)

if(BUILD_DOCS)
    if(NOT DOXYGEN_FOUND)
        message(WARNING "Doxygen not found. Documentation will not be built. Install with: apt-get install doxygen (or brew install doxygen)")
        return()
    endif()

    if(NOT MKDOCS_EXECUTABLE)
        message(WARNING "MkDocs not found. Install with: pip install -r docs/requirements.txt")
        return()
    endif()

    # Doxygen configuration
    set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src/engine)
    set(DOXYGEN_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/docs/_doxygen)
    set(DOXYGEN_CONFIG_FILE ${PROJECT_SOURCE_DIR}/Doxyfile)

    # MkDocs configuration
    set(MKDOCS_CONFIG_FILE ${PROJECT_SOURCE_DIR}/mkdocs.yml)
    set(MKDOCS_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/site)

    # Custom target: Build Doxygen documentation
    add_custom_target(docs-doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    # Custom target: Build MkDocs site
    add_custom_target(docs-mkdocs
        COMMAND ${MKDOCS_EXECUTABLE} build
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Building documentation site with MkDocs"
        DEPENDS docs-doxygen
        VERBATIM
    )

    # Custom target: Build all documentation
    add_custom_target(docs
        COMMENT "Building complete documentation (Doxygen + MkDocs)"
        DEPENDS docs-mkdocs
    )

    # Custom target: Serve documentation locally
    if(MKDOCS_EXECUTABLE)
        add_custom_target(docs-serve
            COMMAND ${MKDOCS_EXECUTABLE} serve
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Serving documentation locally at http://127.0.0.1:8000"
            USES_TERMINAL
            VERBATIM
        )
    endif()

    # Custom target: Clean documentation build artifacts
    add_custom_target(docs-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${MKDOCS_OUTPUT_DIR}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Cleaning documentation build artifacts"
        VERBATIM
    )

    message(STATUS "Documentation targets enabled:")
    message(STATUS "  - docs         : Build complete documentation")
    message(STATUS "  - docs-doxygen : Build Doxygen API docs only")
    message(STATUS "  - docs-mkdocs  : Build MkDocs site only")
    message(STATUS "  - docs-serve   : Serve documentation locally")
    message(STATUS "  - docs-clean   : Clean documentation artifacts")
    message(STATUS "")
    message(STATUS "Usage: cmake --build build --target docs")
    message(STATUS "   or: make docs (if using Unix Makefiles)")
endif()
