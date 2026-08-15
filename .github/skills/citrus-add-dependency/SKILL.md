---
name: citrus-add-dependency
description: Add a new dependency to Citrus Engine. Use this skill when adding external libraries or frameworks as dependencies to the engine project.
allowed-tools: shell
---

# Adding Dependencies to Citrus Engine

This skill guides the process of adding new dependencies to Citrus Engine.

## Critical Requirements

When adding a new dependency to the engine, you **MUST update THREE files**:

1. `vcpkg.json` (root) — Main dependency list for the engine
2. `ports/citrus-engine/vcpkg.json` — Overlay port dependency list
3. `cmake/citrus-engine-config.cmake.in` — CMake config with `find_dependency()` calls

Forgetting any of these causes build failures for downstream consumers (e.g., examples).

## Step 1: Update Root vcpkg.json

Add the new dependency to the root `vcpkg.json`:

```json
{
  "name": "citrus-engine",
  "version": "0.0.x",
  "dependencies": [
    "new-library",
    // Add here
    "flecs",
    "glfw3",
    "opengl",
    "glm",
    "imgui"
  ]
}
```

## Step 2: Update Overlay Port vcpkg.json

Add the **same dependency** to `ports/citrus-engine/vcpkg.json`:

```json
{
  "name": "citrus-engine",
  "version": "0.0.x",
  "dependencies": [
    "new-library",
    // Add here (must match root vcpkg.json)
    "flecs",
    "glfw3",
    "opengl",
    "glm",
    "imgui"
  ]
}
```

## Step 3: Update CMake Config

Add a `find_dependency()` call to `cmake/citrus-engine-config.cmake.in`:

```cmake
# cmake/citrus-engine-config.cmake.in
find_dependency(new-library CONFIG)  # Add this
find_dependency(flecs CONFIG)
find_dependency(glfw3 CONFIG)
find_dependency(OpenGL CONFIG)
find_dependency(glm CONFIG)
find_dependency(imgui CONFIG)
```

## Why All Three Files Are Required

- **Root vcpkg.json**: Tells vcpkg what to install when building the engine standalone
- **Overlay port vcpkg.json**: Tells vcpkg what to install when building via the overlay port (used by examples and
  editor)
- **CMake config**: Tells CMake how to find the dependencies when consumers use the engine

If any of these is missing, downstream consumers will fail to build.

## Verification Checklist

After updating all three files:

1. **Reconfigure CMake**: `cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=<triplet>`
2. **Build engine**: `cmake --build --preset cli-native-debug`
3. **Run tests**: `ctest --preset cli-native-test-debug`
4. **Build examples** (if applicable):
   `cd examples && cmake --preset cli-native && cmake --build --preset cli-native-debug`
5. **Build editor** (if applicable): `cd editor && cmake --preset cli-native && cmake --build --preset cli-native-debug`

All builds and tests must succeed.

## Common Dependency Issues

| Issue                             | Solution                                                                                |
|-----------------------------------|-----------------------------------------------------------------------------------------|
| "Could not find X package"        | Verify `find_dependency(X CONFIG)` is in cmake/citrus-engine-config.cmake.in            |
| vcpkg fails to install dependency | Check vcpkg.json for correct package name and version; verify on vcpkg registry         |
| Examples/editor fail to build     | Ensure dependency is added to ports/citrus-engine/vcpkg.json (not just root vcpkg.json) |
| CMake config errors               | Verify `find_dependency()` call is in cmake/citrus-engine-config.cmake.in               |
