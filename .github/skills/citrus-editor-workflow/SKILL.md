---
name: citrus-editor-workflow
description: Manage the Citrus Engine editor project and its dependency on the engine. Use this skill when modifying engine code, building the editor, or managing version synchronization between engine and editor.
---

# Editor Project Workflow

This skill handles the Citrus Engine editor project, which is a separate CMake project that depends on the engine.

## Project Structure

The editor is a **separate CMake project** located in `editor/`:

- **Separate CMakeLists.txt** — Independent build configuration
- **Separate vcpkg.json** — Manifest for editor dependencies
- **Depends on engine** — Installed via vcpkg overlay port from `ports/citrus-engine/`

## Building the Editor

### Configure (first time)

```bash
cd editor
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
```

### Build

```bash
cmake --build --preset cli-native-debug
```

### Output

The editor executable is located at:
```
editor/build/cli-native/Debug/citrus-scene-editor.exe  # Windows
editor/build/cli-native/Debug/citrus-scene-editor      # Linux/macOS
```

## Engine Updates Workflow

### When You Modify Engine Code

If you modify engine source files (`src/engine/**`), the editor must pick up those changes:

1. **Bump version** in `ports/citrus-engine/vcpkg.json`

   Before:
   ```json
   { "version": "0.0.7" }
   ```

   After:
   ```json
   { "version": "0.0.8" }
   ```

2. **Reconfigure editor project**

   ```bash
   cd editor
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
   ```

   This triggers vcpkg to rebuild and reinstall the engine package with the new version.

3. **Rebuild editor**

   ```bash
   cmake --build --preset cli-native-debug
   ```

### Why Version Bumping Is Required

The editor project uses vcpkg manifest mode with an overlay port pointing to `../ports/citrus-engine`. The overlay port has a `version` field that vcpkg uses as a cache key. If the version doesn't change, vcpkg assumes the package hasn't changed and skips rebuilding it.

By bumping the version, you tell vcpkg: "This package has changed, rebuild it."

## Verification Checklist

After modifying engine code:

1. **Bump version** in `ports/citrus-engine/vcpkg.json`
2. **Reconfigure editor**: `cd editor && cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows`
3. **Build editor**: `cmake --build --preset cli-native-debug`
4. **Verify no errors** — Editor executable created at `editor/build/cli-native/Debug/citrus-scene-editor.exe`
5. **Test editor** — If applicable, verify the editor runs and reflects engine changes

## Common Issues

| Issue | Solution |
|-------|----------|
| Editor fails to build after engine changes | Ensure version was bumped in `ports/citrus-engine/vcpkg.json` |
| "citrus-engine package not found" | Reconfigure editor project to trigger vcpkg to install engine |
| Old engine code still used in editor | Check version number was incremented; reconfigure editor |
| vcpkg cache issues | Delete `editor/build/` and reconfigure |

## Development Workflow

**Typical workflow when developing both engine and editor:**

1. Modify engine code
2. Bump `ports/citrus-engine/vcpkg.json` version
3. Reconfigure editor (triggers engine rebuild)
4. Build editor
5. Test editor
6. Commit both engine changes and version bump

**Do NOT commit engine changes without bumping version**, as consumers (editor, examples) will not pick up the changes.

## IDE vs CLI Building

- **IDE (CLion, Visual Studio)**: Works without additional setup; IDE handles vcvars and environment
- **CLI (Command line)**: Must set up Visual Studio Developer environment on Windows (vcvars64.bat)
