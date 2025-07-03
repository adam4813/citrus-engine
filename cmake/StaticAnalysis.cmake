# Static analysis tools configuration (native builds only)

if(NOT PLATFORM_NATIVE)
    return()
endif()

# clang-tidy configuration
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if (CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    # Currently commented out - uncomment to enable
    #set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
else()
    message(STATUS "clang-tidy not found")
endif()

# Future: Add other static analysis tools here
# find_program(CPPCHECK_EXE NAMES cppcheck)
# find_program(INCLUDE_WHAT_YOU_USE_EXE NAMES include-what-you-use)

message(STATUS "Static analysis tools configured for native builds")
