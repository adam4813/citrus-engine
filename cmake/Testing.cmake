# Testing configuration with GoogleTest

# Only set up testing if this is the main project
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if (BUILD_TESTING)
        enable_testing()
        message(STATUS "Testing enabled - GoogleTest will be configured")

        # Find GoogleTest in the testing context
        find_package(GTest CONFIG REQUIRED)
        include(GoogleTest)
        message(STATUS "  GoogleTest: ${GTest_VERSION} (found for testing)")

        # Helper function to add engine tests - MUST be defined before add_subdirectory
        function(add_engine_test test_name)
            add_executable(${test_name} ${ARGN})
            target_link_libraries(${test_name}
                PRIVATE
                GTest::gtest
                GTest::gtest_main
                engine-core
            )
            gtest_discover_tests(${test_name})
        endfunction()

        add_subdirectory(tests)
    else ()
        message(STATUS "Testing disabled")
    endif ()
else ()
    message(STATUS "Not main project - testing configuration skipped")
endif ()
