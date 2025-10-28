# Copilot Instructions for Modern C++ Game Engine

## 🚨 CRITICAL: Read AGENTS.md First 🚨

**Before doing ANY work, read `AGENTS.md` completely.** It contains:
- Mandatory environment setup steps (Linux dependencies, vcpkg, CMake)
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
- Configure: `cmake --preset cli-native`
- Build: `cmake --build --preset cli-native-debug`
- Tests: `cmake --preset cli-test` then `ctest --preset cli-test-debug`

**Presets**: Use `cli-*` presets (NOT `native` or `test` - those are for IDEs)

**Tech Stack**: C++20, CMake 3.20+, vcpkg, flecs (ECS), Raylib

**Rule of Thumb**: If you're unsure about anything, check AGENTS.md or the relevant pattern guide.
