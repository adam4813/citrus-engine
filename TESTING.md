# Testing Guide for Citrus Engine

This document provides information about the test suite for citrus-engine.

## Test Overview

The project uses Google Test (gtest) for unit and integration testing.

## Test Structure

```
tests/
├── integration/          # Integration tests
│   ├── test_tower_grid_integration.cpp
│   ├── test_facility_manager_integration.cpp
│   ├── test_ecs_world_integration.cpp
│   ├── test_save_load_integration.cpp
│   ├── test_achievement_manager_integration.cpp
│   └── test_lua_mod_manager_integration.cpp
├── e2e/                  # End-to-end tests
│   ├── test_game_initialization_e2e.cpp
│   ├── test_facility_placement_workflow_e2e.cpp
│   └── test_save_load_workflow_e2e.cpp
└── unit/                 # Unit tests
    ├── test_user_preferences_unit.cpp
    ├── test_command_history_unit.cpp
    └── test_accessibility_settings_unit.cpp
```

## Prerequisites

Before running tests, ensure you have:

1. CMake 3.28 or newer
2. A C++20-capable compiler (GCC 10+, Clang 10+, or MSVC 2022+)
3. vcpkg installed and bootstrapped
4. On Linux: X11 development libraries and OpenGL libraries

```bash
# On Ubuntu/Debian
sudo apt-get install -y build-essential cmake pkg-config \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev
```

## Building Tests

### Initial Setup

1. Clone and bootstrap vcpkg (if not already done):
```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh  # or .bat on Windows
```

2. Configure the project with the test CMake preset:

**For CLI/Agent builds** (recommended to avoid IDE conflicts):
```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset cli-native-test
```

**For IDE builds**:
```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset native-test
```

3. Build all tests:

**For CLI/Agent builds**:
```bash
cmake --build --preset cli-native-test-debug
```

On Windows (cmd.exe):
```cmd
cmake --build --preset cli-native-test-debug
```

**For IDE builds**:
```bash
cmake --build --preset native-test-debug
```

**Note**: The `cli-native-test` preset builds to `build/cli-native-test/` to avoid CMake cache conflicts with IDE builds in `build/native-test/`.

### Building Specific Tests

You can build individual test targets:

```bash
cmake --build --preset native-test-debug --target specific_test_name
```

## Running Tests

### Run All Tests

**For CLI/Agent builds**:
```bash
ctest --preset cli-native-test-debug
```

Or run from the test directory:
```bash
cd build/cli-native-test
ctest -C Debug --output-on-failure
```

Or run tests in parallel:
```bash
cd build/cli-native-test
ctest -C Debug -j$(nproc) --output-on-failure
```

**For IDE builds**:
```bash
cd build/native-test
ctest -C Debug --output-on-failure
```

Or use the test preset:
```bash
ctest --preset native-test-debug
```

### Run Individual Tests

Each test executable can be run directly:

**For CLI/Agent builds**:
```bash
# From the project root
./build-cli/cli-test/bin/Debug/test_tower_grid_integration
./build-cli/cli-test/bin/Debug/test_facility_manager_integration
./build-cli/cli-test/bin/Debug/test_user_preferences_unit
```

**For IDE builds**:
**For CLI/Agent builds**:
# From the project root
# Run only integration tests
cd build-cli/cli-test/tests
ctest -C Debug -R ".*_integration" --output-on-failure

# Run only E2E tests
cd build-cli/cli-test/tests
ctest -C Debug -R ".*_e2e" --output-on-failure

# Run only unit tests
cd build-cli/cli-test/tests
ctest -C Debug -R ".*_unit" --output-on-failure
```

**For IDE builds**:
```bash
# Run only integration tests
./build/test/bin/Debug/test_tower_grid_integration
./build/test/bin/Debug/test_facility_manager_integration
```
# Run only E2E tests
**For CLI/Agent builds**:
```bash
# Run only tests matching a pattern
./build-cli/cli-test/bin/Debug/test_tower_grid_integration --gtest_filter="*FloorExpansion*"

# Run all tests except those matching a pattern
./build-cli/cli-test/bin/Debug/test_tower_grid_integration --gtest_filter="-*Removal*"

# List all tests without running them
./build-cli/cli-test/bin/Debug/test_tower_grid_integration --gtest_list_tests
```

**For IDE builds**:

Run only integration tests:
cd build/test/tests
# Run only unit tests

Run only E2E tests:
```bash
cd build/test/tests
ctest -C Debug -R ".*_e2e" --output-on-failure
```

Run only unit tests:
```bash
cd build/test/tests
ctest -C Debug -R ".*_unit" --output-on-failure
```

### Using GTest Filters

You can run specific test cases within a test executable:

**For CLI/Agent builds**:
```bash
# Run only tests matching a pattern
./build/cli-native-test/bin/Debug/test_name --gtest_filter="*TestCase*"

# Run all tests except those matching a pattern
./build/cli-native-test/bin/Debug/test_name --gtest_filter="-*Skip*"

# List all tests without running them
./build/cli-native-test/bin/Debug/test_name --gtest_list_tests
```

**For IDE builds**:
```bash
# Run only tests matching a pattern
./build/native-test/bin/Debug/test_name --gtest_filter="*TestCase*"

# Run all tests except those matching a pattern
./build/native-test/bin/Debug/test_name --gtest_filter="-*Skip*"

# List all tests without running them
./build/native-test/bin/Debug/test_name --gtest_list_tests
```

## Test Coverage

Tests should cover:

- Core engine functionality
- System interactions
- Edge cases and error handling
- Performance-critical code paths

## Writing New Tests

### Test Structure

Follow this structure for new tests:

```cpp
#include <gtest/gtest.h>
#include "engine/your_component.hpp"

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
    }

    void TearDown() override {
        // Clean up after tests
    }

    // Test fixtures
};

TEST_F(YourComponentTest, DescriptiveTestName) {
    // Arrange
    // Act
    // Assert
    EXPECT_EQ(actual, expected);
}
```

### Best Practices

1. **One assertion concept per test**: Each test should verify one specific behavior
2. **Clear test names**: Use descriptive names that explain what is being tested
3. **Arrange-Act-Assert**: Structure tests with clear setup, execution, and verification
4. **Clean up resources**: Always clean up temporary files or state
5. **Independent tests**: Tests should not depend on each other
6. **Meaningful assertions**: Use appropriate EXPECT_* macros (EXPECT_EQ, EXPECT_TRUE, etc.)

## Continuous Integration

Tests are automatically run in CI on:
- Push to main or develop branches
- Pull request creation or updates

CI configuration is in `.github/workflows/`.

## Troubleshooting

### Tests fail to build

1. Ensure vcpkg is properly bootstrapped
2. Verify all dependencies are installed
3. Check that you're using C++20 compatible compiler
4. Use the test preset: `cmake --preset cli-native-test` or `cmake --preset native-test`
5. Try cleaning and rebuilding: `cmake --build --preset cli-native-test-debug --clean-first`

### Tests fail to run

1. Check that all required system libraries are installed (especially on Linux)
2. Verify file permissions on test executables
3. Ensure OpenGL/GLFW dependencies are available

### Performance issues

1. Run tests in parallel: `ctest -j$(nproc)`
2. Run only necessary tests during development
3. Use test filters to focus on relevant tests

## Contact

For questions or issues with the test suite, please create an issue on the GitHub repository.
