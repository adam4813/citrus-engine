# AI Agent Instructions for Citrus Engine

**Purpose**: Internal process guidelines for AI agents working on citrus-engine  
**Audience**: AI assistants only (not user documentation)  
**Status**: Active

---

## ⚠️ MANDATORY FIRST STEPS - READ THIS BEFORE ANY ACTION ⚠️

### Step 1: Install System Dependencies

**Linux (Native Build)**:
```bash
sudo apt-get update && sudo apt-get install -y build-essential cmake pkg-config \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev clang-18
```

**Critical**: These libraries are required for GLFW3, OpenGL support, and C++20 modules.

**Linux (Web/Emscripten Build)**:
```bash
# Install Emscripten SDK
cd /opt  # or any location
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Sets up PATH and environment variables
```

**Windows/macOS**: Install appropriate compilers (MSVC 2022+ or Clang-18+).

### Step 2: Bootstrap vcpkg

```bash
# Clone vcpkg if not present
git clone https://github.com/microsoft/vcpkg.git

# Bootstrap
./vcpkg/bootstrap-vcpkg.sh      # Linux/macOS
vcpkg\bootstrap-vcpkg.bat       # Windows

# Set environment
export VCPKG_ROOT=/path/to/vcpkg    # Linux/macOS
set VCPKG_ROOT=D:\path\to\vcpkg     # Windows
```

### Step 3: Configure CMake

**Native Build**:
```bash
# Linux (use Clang-18)
export CC=clang-18
export CXX=clang++-18
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux

# Windows
cmake --preset cli-native
```

**Web Build (Emscripten)**:
```bash
# Ensure emsdk is activated
source /opt/emsdk/emsdk_env.sh  # Adjust path as needed

# Configure (Emscripten toolchain from vcpkg)
cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
```

**Note**: citrus-engine uses `cli-native` presets to avoid conflicts with IDE builds.

**Platform-specific triplets**:
- `x64-windows` - Windows (default)
- `x64-linux` - Linux native
- `x64-osx` - macOS native
- `wasm32-emscripten` - Web/Emscripten

**These steps are NOT optional. Complete them before proceeding with any task.**

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

- `UI_DEVELOPMENT_BIBLE.md` - ImGui-based UI patterns and immediate mode GUI architecture
- `TESTING.md` - Test structure, priorities, best practices
- Future: Additional API guides for engine subsystems

These define **how to use the engine**. Read the relevant guide before working in that domain.

### Source Structure

- `src/` - Implementation files
- `docs/` - User-facing documentation only

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

**Status**: This file is the only internal process documentation permitted. Follow these directives strictly.
