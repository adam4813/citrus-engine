# AI Agent Instructions for Citrus Engine

**Purpose**: Internal process guidelines for AI agents working on citrus-engine  
**Audience**: AI assistants only (not user documentation)  
**Status**: Active

---

## ⚠️ MANDATORY FIRST STEPS - READ THIS BEFORE ANY ACTION ⚠️

**CRITICAL**: Follow these steps IN ORDER before attempting ANY build or code work. Skipping steps will cause build
failures.

### Complete Setup Checklist

**For Windows Native Builds:**

1. ✅ Open a Visual Studio Developer Command Prompt or PowerShell (required for MSVC toolchain)
2. ✅ Clone and bootstrap vcpkg
3. ✅ Set VCPKG_ROOT environment variable
4. ✅ Run CMake configure with x64-windows triplet
5. ✅ Build

**For Linux Native Builds:**

1. ✅ Install system dependencies (X11, OpenGL, Clang-18, ninja-build)
2. ✅ Clone and bootstrap vcpkg
3. ✅ Set VCPKG_ROOT environment variable
4. ✅ Set CC=clang-18 and CXX=clang++-18
5. ✅ Run CMake configure with x64-linux triplet
6. ✅ Build

**For Web/Emscripten Builds:**

1. ✅ Install Emscripten SDK (emsdk)
2. ✅ Activate emsdk environment
3. ✅ Clone and bootstrap vcpkg
4. ✅ Set VCPKG_ROOT environment variable
5. ✅ Run CMake configure with wasm32-emscripten triplet
6. ✅ Build

---

### Editor Project

The editor is a **separate CMake project** located in the `editor/` directory. It has its own CMakeLists.txt and vcpkg.json.

**To build/run the editor:**
1. Change directory to `editor/`
2. Configure: `cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows`
3. Build: `cmake --build --preset cli-native-debug`
4. The editor executable will be at `editor/build/cli-native/Debug/citrus-scene-editor.exe`

**After modifying engine code:**
When you modify engine source files (`src/engine/**`), you must:
1. **Bump the version** in `ports/citrus-engine/vcpkg.json` (e.g., `0.0.7` → `0.0.8`)
2. **Reconfigure** the editor project to pick up the new engine:
   ```bash
   cd editor && cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
   ```

The engine is installed via vcpkg manifest mode using an overlay port (`../ports/citrus-engine`). Bumping the version and reconfiguring triggers vcpkg to rebuild and reinstall the engine package.

**Note**: On Windows, building from CLI requires a Visual Studio Developer environment. Building from CLion or Visual Studio IDE works without additional setup.

### Step 1: Install System Dependencies

**Windows (Native Build)**:

- Windows: Install Visual Studio 2022 with C++ tools OR Clang-18+

#### Windows: Visual Studio Compiler Environment

**When using MSVC (Visual Studio compiler)**, you must initialize the Visual Studio toolchain environment before building. Wrap build commands with `vcvars64.bat`:

```powershell
# PowerShell - wrap the entire command
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake --build --preset cli-native-debug 2>&1'

# For tests
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" && ctest --preset cli-native-test-debug --output-on-failure 2>&1'
```

**How to detect MSVC**: Check the CMake output during configure - it will show the compiler path containing `MSVC` or `cl.exe`, e.g.:
```
Compiler found: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/.../bin/Hostx64/x64/cl.exe
```

**Why**: The `vcvars64.bat` script initializes environment variables that tell the compiler where to find the Windows SDK and standard library headers.

**Alternative paths** (adjust for your VS version/edition):
- VS 2022 Professional: `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat`
- VS 2019: `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat`

**Linux (Native Build)**:

```bash
# Install ALL dependencies in one command
sudo apt-get update && sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  clang-18 \
  libx11-dev \
  libxrandr-dev \
  libxinerama-dev \
  libxcursor-dev \
  libxi-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev
```

**What these are for**:

- `build-essential` - GCC and build tools
- `cmake` - Build system generator (3.28+ required)
- `ninja-build` - Fast build tool (required by CMake presets)
- `pkg-config` - Library detection
- `clang-18` - C++20 compiler with module support (GCC doesn't work)
- `libx11-dev` through `libxi-dev` - X11 libraries for GLFW3
- `libgl1-mesa-dev`, `libglu1-mesa-dev` - OpenGL libraries

**Linux (Web/Emscripten Build)**:

```bash
# Install Emscripten SDK
cd /opt  # or /tmp for temporary install
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # REQUIRED: Sets CC/CXX to emcc/em++

# Verify installation
emcc --version  # Should show version 4.0.x or later
```

**Windows/macOS**:

- Windows: Install Visual Studio 2022 with C++ tools OR Clang-18+
- macOS: Install Xcode Command Line Tools and Clang-18+ (via Homebrew)

---

### Step 2: Clone and Bootstrap vcpkg

**CRITICAL**: vcpkg MUST be cloned in the parent directory of citrus-engine, not inside it.

```bash
# If citrus-engine is in /home/user/citrus-engine
cd /home/user  # Go to parent directory

# Clone vcpkg (if not present)
git clone https://github.com/microsoft/vcpkg.git

# Bootstrap vcpkg
cd vcpkg
./vcpkg/bootstrap-vcpkg.sh      # Linux/macOS
vcpkg\bootstrap-vcpkg.bat       # Windows

# Verify bootstrap
./vcpkg --version  # Should show vcpkg version
```

---

### Step 3: Set Environment Variables

**Linux/macOS Native**:

```bash
# Set these EVERY TIME in your shell session
export VCPKG_ROOT=/home/user/vcpkg  # Adjust path
export CC=clang-18
export CXX=clang++-18

# Verify
echo $VCPKG_ROOT    # Should show path
$CC --version       # Should show clang-18.x
```

**Linux/macOS Web**:

```bash
# Activate Emscripten (sets CC/CXX automatically)
source /opt/emsdk/emsdk_env.sh  # Adjust path

# Set vcpkg root
export VCPKG_ROOT=/home/user/vcpkg  # Adjust path

# Verify
emcc --version      # Should show Emscripten version
echo $VCPKG_ROOT    # Should show path
```

**Windows**:

```cmd
set VCPKG_ROOT=C:\path\to\vcpkg
```

---

### Step 4: Configure CMake

**IMPORTANT**: Only run configure once unless you need to reconfigure.

**Native Build (Linux)**:

```bash
cd citrus-engine
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux
```

**Native Build (Windows)**:

```cmd
cd citrus-engine
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
```

**Web Build (Emscripten)**:

```bash
cd citrus-engine
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
```

This will take 5-15 minutes the first time as vcpkg builds all dependencies.

---

### Step 5: Build

```bash
# Debug build
cmake --build --preset cli-native-debug --parallel 4

# Release build
cmake --build --preset cli-native-release --parallel 4
```

---

### Platform Triplets Reference

- `x64-windows` - Windows native (must specify on command line)
- `x64-linux` - Linux native (Clang-18+)
- `x64-osx` - macOS native
- `wasm32-emscripten` - Web/Emscripten builds

---

### Automated Setup for GitHub Copilot

**GitHub Copilot users**: The repository includes an automated setup workflow at
`.github/workflows/copilot-setup-steps.yml` that runs automatically when Copilot starts. This workflow:

- Installs system dependencies (Linux: X11, OpenGL, build tools)
- Configures Clang-18 compiler
- Sets up vcpkg and caches dependencies
- Prepares the environment for building and testing

**No manual setup required when using GitHub Copilot** - the workflow runs automatically in your Copilot environment.

---

### Using setup-cpp for Automated Setup (GitHub Actions)

For GitHub Actions workflows, use the shared composite action to automate environment setup:

```yaml
# In GitHub Actions workflows
- name: Setup Citrus Engine Environment
  uses: ./.github/actions/setup-environment
  with:
    compiler: clang
    clang-version: 18
```

This composite action is used by both `build.yml` and `build-examples.yml` workflows to ensure consistent setup across
all CI builds.

For other web-based coding environments (like Codespaces), you can also use `aminya/setup-cpp` directly:

```yaml
- name: Setup C++ environment
  uses: aminya/setup-cpp@v1
  with:
    compiler: clang
    clang: 18
    cmake: true
    ninja: true
    vcpkg: true
```

---

**These steps are NOT optional. Complete ALL steps IN ORDER before proceeding with any task.**

---

## Core Directives

### 1. Required Reading Before Any Work

**Read these pattern/API guides immediately when working in their domains**:

- **UI work**: Read `UI_DEVELOPMENT_BIBLE.md` - declarative, reactive, event-driven patterns
- **Testing work**: Read `TESTING.md` - test structure, priorities, and best practices
- **Future**: API documentation describing engine usage patterns (not yet created)

These are **reference documentation** that define correct implementation patterns. Don't improvise - follow the
documented patterns.

### 2. Planning is Internal Only

- **Never create plan documentation files** unless explicitly requested
- Plans, strategies, and implementation approaches are working memory only
- Execute directly without documenting the plan
- Exception: AGENTS.md (this file) is allowed as it improves AI process

### 3. Temporary Files Must Be Cleaned Up

- **Never leave temporary test programs, scripts, or exploration code** in the repository
- If you create temporary files for investigation (e.g., testing a hypothesis, exploring an API):
    - Delete them immediately after use
    - Use `.gitignore`d locations if they must persist briefly
- Clean up at the end of your task before reporting completion
- Examples of temporary files to avoid: test programs, debug scripts, scratch files

### 4. Documentation Policy

**NEVER create summary documents, reports, or reviews** of work completed. These are working memory only.

**Do NOT create** unless explicitly requested:

- HOWTOs, tutorials, guides
- Implementation summaries, work summaries, review summaries
- Design notes or architecture docs
- Supplementary documentation of any kind
- Status reports, completion reports, or any "summary" documents

**Only create NEW documentation if**:

- User explicitly requests it (e.g., "create a document explaining X")
- It documents a human workflow/process users must follow (e.g., build instructions, setup steps)
- It directly improves automated tooling or AI code generation

**When creating NEW documentation files**:

- You MUST provide a justification in your response explaining:
    - Why this needs to be a persistent file vs. chat response
    - What specific future use case requires this reference
    - How it differs from existing documentation
- Updating existing documentation does NOT require justification

**Rule of thumb**: If you're just going to tell the user about it anyway, don't create a file for it.

**CRITICAL**: If the justification for creating documentation is NOT immediately obvious:

1. **DO NOT create the file**
2. **Explain your reasoning to the user first**
3. **Wait for explicit approval**
4. If unclear whether documentation is needed, **update AGENTS.md to clarify the policy** rather than guess

### 5. No Standalone Applications

- Do not create standalone test applications
- Test features by integrating into existing code (e.g., main menu)

---

## Execution Protocol

### Before Any Task

1. Read request carefully
2. Identify files to modify (surgical changes only)
3. Execute changes directly without creating plan documents
4. Build and test to verify changes

### Making Changes

- **Minimal modifications only**: Change as few lines as possible
- Do not delete/remove/modify working code unless absolutely necessary
- Ignore unrelated bugs or broken tests
- Update documentation only if directly related to changes

### Implementation Philosophy

**Priority: Correctness > Simplicity > Performance**

1. **Make it work first** - Focus on correct behavior before optimization
2. **Keep it simple** - Minimal code changes that achieve the goal
3. **Optimize later** - Performance and size optimizations come after correctness is verified

**Avoid:**

- Premature optimization
- Over-engineered solutions
- Complex abstractions without proven need

**Prefer:**

- The simplest solution that works correctly
- Clear, readable code over clever tricks
- Straightforward implementations in first iterations

**Example**: Use unpacked float colors (simple, debuggable) rather than packed uint32 colors (complex, harder to debug)
until profiling proves the memory savings matter.

### Build Verification (REQUIRED)

**Build/test MUST succeed before completing any task.**

**Compiler Requirements**:

- **Linux Native**: Clang-18 or later (required for C++20 modules)
- **Windows Native**: MSVC 2022 or Clang-18+
- **Web (Emscripten)**: Latest Emscripten SDK (3.1.40+)
- **Note**: GCC has incomplete C++20 module support and may not work

**System Dependencies (Linux Native only)**:

```bash
sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libgl1-mesa-dev clang-18
```

**Emscripten Setup (Web builds)**:

```bash
# Install Emscripten SDK (one time)
cd /opt && git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest

# Activate Emscripten environment (each shell session)
source /opt/emsdk/emsdk_env.sh
```

Before completing any work:

1. **Set compiler environment**:

   **Linux Native**:
   ```bash
   export CC=clang-18
   export CXX=clang++-18
   ```

   **Web/Emscripten**:
   ```bash
   source /opt/emsdk/emsdk_env.sh  # Sets CC/CXX to emcc/em++
   ```

2. **Configure** (first time):
   ```bash
   # Linux Native
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux
   
   # Windows Native
   cmake --preset cli-native
   
   # Web/Emscripten
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
   ```

3. **Build**: `cmake --build --preset cli-native-debug`
    - Add `--parallel $(nproc)` for parallel builds on Linux/macOS
    - Add `--parallel %NUMBER_OF_PROCESSORS%` for parallel builds on Windows

4. **Verify**: No errors or warnings
5. **If errors**: Fix and rebuild
6. **Do not mark complete until build succeeds**

**Known Issues**:

- CMake 3.31+ requires `CMAKE_CXX_SCAN_FOR_MODULES=OFF` for C++20 modules
- Some source files may be missing `#include <cstdint>` - add in module preambles
- GCC support is limited due to incomplete C++20 module implementation
- Emscripten builds do not use C++20 modules (uses traditional headers)

**Note**: Use `cli-native` presets (not `native`) to avoid conflicts with IDE builds.
Build directory: `build/cli-native/` (isolated from IDE's `build/native/`)

### Test Verification (REQUIRED)

After successful build, run tests before completing work:

1. **Configure tests (first time)**:
   ```bash
   cmake --preset cli-native-test
   ```

2. **Build tests**:
   ```bash
   cmake --build --preset cli-native-test-debug
   ```
    - Add `--parallel $(nproc)` for parallel builds on Linux/macOS
    - Add `--parallel %NUMBER_OF_PROCESSORS%` for parallel builds on Windows

3. **Run all tests**:
   ```bash
   ctest --preset cli-native-test-debug
   ```

   Or run directly from test directory:
   ```bash
   cd build/cli-native-test
   ctest -C Debug --output-on-failure
   ```

4. **Verify**: All tests pass (100% success rate)
5. **If tests fail**:
    - Only fix failures related to your changes
    - Ignore pre-existing test failures unrelated to your work
    - Document any known issues
6. **Do not mark complete until tests pass**

**Note**: See `TESTING.md` for detailed test documentation and advanced usage.

---

## Project Constraints

### Technical Requirements

- **C++20**: Strict requirement
- **CMake 3.28+**: Required
- **vcpkg**: Use for dependencies
- **ImGui**: UI framework (with GLFW + OpenGL3 bindings)
- **GLFW3**: Windowing and input
- **OpenGL**: Rendering backend
- **flecs**: ECS framework

### Platform Support

- Windows: MSVC or MinGW-w64 GCC 10+
- Linux: GCC 10+ or Clang 10+
- macOS: Xcode 12+ / Clang 10+

### Code Standards

#### Code Formatting

- **Line endings**: Use LF (Unix-style), not CRLF
- **Formatting**: Use `clang-format` - the project includes a `.clang-format` configuration
- **Run formatter**: Before committing, run `clang-format -i <files>` or use IDE integration

#### Modern C++20 Requirements

- **C++20 features**: Use ranges, concepts, modules (when stable), coroutines (when appropriate)
- **RAII**: All resource management (files, memory, locks) via RAII types
- **Smart pointers**:
    - `std::unique_ptr` for exclusive ownership
    - `std::shared_ptr` only when truly shared ownership needed
    - Raw pointers for non-owning references
    - Never use `new`/`delete` directly
- **Containers**: Prefer standard library containers over manual memory management
- **Algorithms**: Use `<algorithm>` and `<ranges>` over raw loops when clearer

#### Modern C++ Patterns

```cpp
// CORRECT: Use ranges instead of raw loops
auto filtered = data 
    | std::views::filter([](const auto& item) { return item.active; })
    | std::views::transform([](const auto& item) { return item.value; });

// CORRECT: Smart pointers for ownership
auto entity = std::make_unique<Entity>();
scene->AddEntity(std::move(entity));  // Transfer ownership

// CORRECT: RAII for resources
class ResourceManager {
    std::unique_ptr<Resource> resource_;  // Automatically cleaned up
};

// CORRECT: Small, focused functions
auto FindEntity(int id) const -> std::optional<Entity> {
    auto it = std::ranges::find_if(entities_, 
        [id](const auto& e) { return e.id == id; });
    return it != entities_.end() ? std::optional{*it} : std::nullopt;
}
```

#### Composition Over Inheritance

- **Prefer composition** for building functionality
- **Use inheritance** only for polymorphism (Gang of Four patterns like Strategy, Observer, Composite)
- Keep inheritance hierarchies shallow (max 2-3 levels)

```cpp
// CORRECT: Composition for building features
class GameWorld {
    SceneManager scene_manager_;        // Has-a
    EntityRegistry entity_registry_;    // Has-a
    SystemManager system_manager_;      // Has-a
};

// CORRECT: Inheritance for polymorphism (Strategy pattern)
class System {
public:
    virtual void Update(float delta_time) = 0;
    virtual void Render() const = 0;
};

class PhysicsSystem : public System { /* ... */ };
class RenderSystem : public System { /* ... */ };

// WRONG: Inheritance for code reuse
class PlayerEntity : public Entity {  // Avoid - use composition/ECS instead
};
```

#### Gang of Four Patterns (Use When Appropriate)

- **Observer**: Event callbacks, reactive updates
- **Strategy**: Interchangeable algorithms (rendering backends, input handlers)
- **Factory**: Entity/component creation in ECS
- **Command**: Action history, undo/redo systems
- **Singleton**: Use sparingly, only for true single instances (engine core, resource managers)

#### Code Organization

- **Small functions**: Max 20-30 lines, single responsibility
- **Comments**: Only for complex logic or non-obvious design decisions
- **Const correctness**: Use `const` everywhere possible
- **Type safety**: Prefer `enum class` over plain `enum` or integers

---

## File Organization

### Pattern/API Documentation

**Internal References** (for AI agents implementing features):
- `UI_DEVELOPMENT_BIBLE.md` - Internal patterns and architecture (being migrated to docs/)
- `TESTING.md` - Test structure, priorities, best practices

**User-Facing Documentation** (in `docs/`):
- Document new features and APIs here
- Update `docs/index.md` to link new pages
- Follow MkDocs format for site generation

When adding new engine features:
1. Add user documentation to `docs/` (how to use the feature)
2. Update `docs/index.md` with links to new pages
3. Update internal guides only for implementation patterns

### Source Structure

- `src/` - Implementation files
- `docs/` - User-facing documentation (MkDocs site)
- Root `.md` files - Internal agent references (AGENTS.md, TESTING.md, etc.)

---

## Documentation Guidelines

### User Documentation (README.md)

- Focus on usage and gameplay
- User manual style
- Not implementation details

### Code Comments

- Only for complex logic explanation or non-obvious design decisions
- Don't comment on "what" changed - that's in version control
- Don't explain obvious code
- Self-documenting code preferred

---

## Process Checklist

When given a task:

- [ ] Understand requirement (no plan doc needed)
- [ ] Identify minimal changes
- [ ] Execute changes directly
- [ ] Build and verify
- [ ] Clean up any temporary files created during investigation
- [ ] No standalone tests/demos created
- [ ] No unnecessary documentation created
- [ ] Changes are surgical and minimal

---

## Error Handling

### Build Errors

- Fix immediately
- Rebuild to verify
- Do not proceed until clean build

### Breaking Changes

- Avoid at all costs
- Opt-in features only
- Maintain backward compatibility

---

## Key Principles

1. **Execute, don't document the plan**
2. **Minimal changes, surgical precision**
3. **Test in production code, not standalone apps**
4. **Build verification is mandatory**
5. **No supplementary documentation**
6. **When in doubt, ask the user**

---

## Adding Dependencies to the Engine

**CRITICAL**: When adding a new dependency to the engine, you MUST update THREE files:

1. **`vcpkg.json`** (root) - Main dependency list for the engine
2. **`ports/citrus-engine/vcpkg.json`** - Overlay port dependency list (must match root vcpkg.json)
3. **`cmake/citrus-engine-config.cmake.in`** - CMake config with `find_dependency()` calls

**Example**: Adding a library called "newlib"

```json
// 1. vcpkg.json (root)
{
  "dependencies": [
    "newlib",
    // Add here
    "flecs"
    // ... other deps
  ]
}

// 2. ports/citrus-engine/vcpkg.json
{
  "dependencies": [
    "newlib",
    // Add here too
    "flecs"
    // ... other deps
  ]
}
```

```cmake
# 3. cmake/citrus-engine-config.cmake.in
find_dependency(newlib CONFIG)  # Add this
find_dependency(flecs CONFIG)
# ... other find_dependency calls
```

**Why this is needed:**

- vcpkg.json: Tells vcpkg what to install when building the engine standalone
- ports/citrus-engine/vcpkg.json: Tells vcpkg what to install when building via the overlay port (used by examples)
- citrus-engine-config.cmake.in: Tells CMake how to find the dependencies when someone uses the engine

**Forgetting any of these will cause build failures** for consumers of the engine (like the examples).

---

**Status**: This file is the only internal process documentation permitted. Follow these directives strictly.
