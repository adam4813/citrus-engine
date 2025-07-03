# Testing configuration with GoogleTest

# Only set up testing if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)

    if(BUILD_TESTING)
        message(STATUS "Testing enabled - GoogleTest will be configured")

        # Future: Add test subdirectory when tests exist
        # add_subdirectory(tests)

        # Helper function to add engine tests
        function(add_engine_test test_name)
            add_executable(${test_name} ${ARGN})
            target_link_libraries(${test_name}
                PRIVATE
                    GTest::gtest
                    GTest::gtest_main
                    engine-core
            )
            add_test(NAME ${test_name} COMMAND ${test_name})
        endfunction()

    else()
        message(STATUS "Testing disabled")
    endif()
else()
    message(STATUS "Not main project - testing configuration skipped")
endif()
