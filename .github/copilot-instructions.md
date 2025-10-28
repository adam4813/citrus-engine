# Copilot Instructions for Citrus Engine

## 🚨 CRITICAL: Read AGENTS.md First 🚨

**Before doing ANY work, read `AGENTS.md` completely.** It contains:
- Mandatory environment setup steps (vcpkg, CMake)
- Execution protocol and workflow
- Documentation policy (what NOT to create)
- Build verification requirements
- Coding standards
- All project constraints and guidelines

**AGENTS.md is the single source of truth for all project rules.**

## 📚 Pattern/API Documentation - Read Before Working

When working in specific domains, read the relevant pattern guide:
- **UI work** → Read `UI_DEVELOPMENT_BIBLE.md` (declarative, reactive, event-driven patterns)
- **Testing work** → Read `TESTING.md` (test structure, priorities, best practices)
- **Future**: Additional API guides for engine subsystems will be added

**Don't improvise patterns - follow the documented approach.**

## Quick Reference (Full details in AGENTS.md)

**Build Commands**:
- **Native (Windows)**: `cmake --preset cli-native` → `cmake --build --preset cli-native-debug`
- **Native (Linux)**: Use Clang-18: `export CC=clang-18 CXX=clang++-18` → `cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux` → `cmake --build --preset cli-native-debug`
- **Web (Emscripten)**: `source /path/to/emsdk/emsdk_env.sh` → `cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten` → `cmake --build --preset cli-native-debug`
- **Tests**: `cmake --preset cli-native-test` → `ctest --preset cli-native-test-debug`

**Presets**: Use `cli-*` presets (NOT `native` or `test` - those are for IDEs)

**Note**: 
- Presets default to Windows. On Linux/macOS, adjust vcpkg triplet as needed.
- Linux requires Clang-18+ for C++20 modules (GCC has incomplete support)
- Web builds require Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html

**Tech Stack**: C++20, CMake 3.28+, vcpkg, flecs (ECS), ImGui (debug UI), GLFW3 (windowing), OpenGL (rendering)

**Rule of Thumb**: If you're unsure about anything, check AGENTS.md or the relevant pattern guide.
