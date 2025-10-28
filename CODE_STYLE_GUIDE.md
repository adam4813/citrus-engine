# Citrus Engine - C++20 Code Style Guide

> **Consistent coding standards for the Citrus Engine project**

## Overview

This document defines the coding standards and style guidelines for Citrus Engine. Consistency in code style
improves readability, maintainability, and collaboration.

## Naming Conventions

### Functions and Methods

- **Style**: PascalCase
- **Examples**:
  ```cpp
  void InitializeRenderer();
  bool LoadTexture(const std::string& path);
  Mat4 GetViewMatrix() const;
  ```

### Variables and Parameters

- **Style**: snake_case
- **Examples**:
  ```cpp
  int entity_count;
  float delta_time;
  const std::string& file_path;
  ```

### Classes and Structs

- **Style**: PascalCase
- **Examples**:
  ```cpp
  class EntityManager;
  struct Transform;
  class TextureManager;
  ```

### Enums and Enum Values

- **Style**: PascalCase for enum class, PascalCase for values
- **Examples**:
  ```cpp
  enum class ProjectionType {
      Perspective,
      Orthographic
  };
  
  enum class TextureFormat {
      RGB8,
      RGBA8,
      RGBA16F
  };
  ```

### Constants

- **Style**: PascalCase for const variables, UPPER_CASE for macros
- **Examples**:
  ```cpp
  constexpr float DefaultFov = 60.0f;
  const Color WhiteColor{1.0f, 1.0f, 1.0f, 1.0f};
  
  #define ENGINE_VERSION_MAJOR 1
  ```

### Member Variables

- **Style**: snake_case with trailing underscore for private members
- **Examples**:
  ```cpp
  class Camera {
  public:
      Vec3 position{0.0f};  // Public members: no underscore
      
  private:
      Vec3 position_{0.0f}; // Private members: trailing underscore
      bool view_dirty_ = true;
  };
  ```

### Namespaces

- **Style**: snake_case
- **Examples**:
  ```cpp
  namespace engine::platform {}
  namespace engine::rendering {}
  ```

### Template Parameters

- **Style**: PascalCase
- **Examples**:
  ```cpp
  template<typename ComponentType>
  template<Component T>
  template<size_t BufferSize>
  ```

## File Naming

### Header Files (Module Interfaces)

- **Style**: snake_case with .cppm extension
- **Examples**: `platform.cppm`, `rendering.cppm`, `entity_manager.cppm`

### Source Files

- **Style**: snake_case with .cpp extension
- **Examples**: `platform.cpp`, `entity_manager.cpp`

### Directory Structure

- **Style**: snake_case
- **Examples**: `src/engine/`, `cmake/`, `plan/modules/`

## Code Formatting

### Indentation

- **4 spaces** (no tabs)
- Consistent indentation for all nested blocks

### Braces

- **K&R style** for all situations (opening brace on same line)

```cpp
class MyClass {
public:
    void MyFunction() {
        if (condition) {
            // Do something
        } else {
            // Do something else
        }
        
        for (int i = 0; i < count; ++i) {
            // Loop body
        }
    }
};
```

### Line Length

- **Maximum 120 characters** per line
- Break long lines at logical points (parameters, operators)

### Spacing

- Space after keywords: `if (`, `for (`, `while (`
- Space around operators: `a + b`, `x == y`
- No space before semicolons: `func();`
- Space after commas: `func(a, b, c);`

## C++20 Specific Guidelines

### Modules

- Use `export module` for module interfaces
- Use `import` statements instead of `#include` where possible
- Group imports logically (standard library, third-party, engine modules)

```cpp
module;

#include <vector>  // Legacy includes in global module fragment
#include <memory>

export module engine.platform;

import std.filesystem;  // C++20 module imports when available
// ... rest of module
```

### Concepts

- Use PascalCase for concept names
- Place concepts before template declarations

```cpp
template<typename T>
concept Component = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template<Component T>
void RegisterComponent();
```

### Auto and Type Deduction

- Use `auto` for complex type names and lambda expressions
- Avoid `auto` when type clarity is important

```cpp
auto texture_manager = GetTextureManager();  // Good: complex type
auto count = 5;                              // Bad: unclear type
int count = 5;                               // Good: clear intent
```

## Documentation

### Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Document public interfaces with brief descriptions

```cpp
/// Get the current frame rate in frames per second
/// @return Current FPS as a double value
double GetCurrentFps() const;

/**
 * Create a new texture from image file
 * @param path File path to the image
 * @param format Desired texture format
 * @return Texture ID or INVALID_TEXTURE on failure
 */
TextureId CreateTexture(const fs::Path& path, TextureFormat format);
```

### Module Documentation

- Each module interface should have a header comment explaining its purpose
- Document major design decisions and constraints

## Error Handling

### Return Values

- Use meaningful return types (not just bool)
- Prefer exceptions for exceptional cases
- Use `std::optional` for operations that may fail

```cpp
std::optional<TextureId> LoadTexture(const fs::Path& path);
void InitializeRenderer();  // Throws on critical failure
bool IsValid(TextureId id) const;  // Simple validation
```

### Assertions

- Use assertions for development-time checks
- Use graceful error handling for runtime errors

```cpp
assert(entity.IsValid());  // Development check
if (!file.IsOpen()) {      // Runtime error handling
    return std::nullopt;
}
```

## Performance Guidelines

### Memory Management

- Prefer stack allocation over heap allocation
- Use smart pointers for ownership management
- Consider custom allocators for high-frequency allocations

### Function Design

- Pass large objects by const reference
- Return small objects by value
- Use move semantics for expensive-to-copy objects

```cpp
void ProcessEntity(const Transform& transform);     // Large object by const ref
Vec3 GetPosition() const;                           // Small object by value
std::unique_ptr<Texture> CreateTexture(TextureCreateInfo&& info);  // Move semantics
```

## Example: Well-Formatted Code

```cpp
export module engine.rendering;

import engine.platform;
import engine.ecs;

export namespace engine::rendering {

    class TextureManager {
    public:
        TextureManager();
        ~TextureManager();
        
        /// Create a new texture from the provided creation info
        TextureId CreateTexture(const TextureCreateInfo& info);
        
        /// Load texture from file path
        std::optional<TextureId> LoadTexture(const fs::Path& path);
        
        /// Get texture dimensions
        Vec2 GetTextureDimensions(TextureId id) const;
        
        /// Check if texture ID is valid
        bool IsValidTexture(TextureId id) const;
        
    private:
        std::unordered_map<TextureId, TextureData> textures_;
        TextureId next_id_ = 1;
        
        bool ValidateTextureData(const TextureCreateInfo& info) const;
    };

} // namespace engine::rendering
```

# C++20 Game Engine Code Style Guide

> **Consistent coding standards for the Modern C++20 Game Engine project**

## Overview

This document defines the coding standards and style guidelines for the game engine project. Consistency in code style
improves readability, maintainability, and collaboration.

## C++20 Module Structure

### Module Interface Files (.cppm)

Module interface files must follow this structure:

#### With Standard Library Includes (Global Module Fragment Required)

```cpp
module;

#include <memory>
#include <vector>
#include <string>

export module engine.rendering:types;

export namespace engine::rendering {
    // Module content here
}
```

#### Without Standard Library Includes (No Global Module Fragment)

```cpp
export module engine.rendering:simple;

export namespace engine::rendering {
    // Module content here - only using imported modules
}
```

#### Module Partition Import Pattern

```cpp
export module engine.rendering;

// Import and re-export all sub-modules
export import :types;
export import :components;
export import :texture;

// Import dependencies
import engine.ecs;
import engine.platform;
```

### Module Implementation Files (.cpp)

Implementation files must follow this structure and import pattern:

#### Implementation File Import Rules

```cpp
module;

#include <memory>  // Standard library includes in global fragment
#include <vector>

module engine.rendering;  // Declare which module this implements

// REQUIRED: Import your own interface
import :texture;

// REQUIRED: Import dependencies your interface uses  
import :types;
import engine.platform;

// OPTIONAL: Import additional modules only the implementation needs
import :internal_utils;

namespace engine::rendering {
    // Implementation code here
}
```

#### Critical Import Pattern

**Rule**: Implementation files (.cpp) do NOT automatically inherit imports from their interface files (.cppm). Each file
must explicitly import what it uses.

```cpp
// texture.cppm (interface)
export module engine.rendering:texture;
import :types;           // Interface imports these
import engine.platform;

// texture.cpp (implementation) - MUST repeat necessary imports
module engine.rendering;
import :texture;         // Import own interface
import :types;           // Must repeat - needed for TextureId, etc.
import engine.platform;  // Must repeat - needed for fs::Path
```

#### Why This Pattern is Required

1. **Module isolation**: Each file has its own import scope
2. **Explicit dependencies**: Makes dependencies clear and maintainable
3. **Compilation efficiency**: Only imports what each file actually uses
4. **Error prevention**: Compiler will catch missing imports immediately

### Module Guidelines

1. **Global Module Fragment Rule**: Only include the `module;` section if you have `#include` statements for standard
   library headers
2. **Include Order**: Place all `#include` statements in the global module fragment (before `export module` or `module`)
3. **Import Order**: Place `import` statements after the module declaration
4. **Implementation Import Pattern**: Always import your own interface first, then dependencies, then
   implementation-only modules
5. **Namespace Consistency**: Always use the `engine::` namespace hierarchy
6. **Sub-module Naming**: Use colon notation for module partitions (e.g., `:types`, `:mesh`)
7. **MSVC Module Constants**: Use `constexpr` without explicit `inline` for constants in module interfaces to avoid 4GB
   object file issues

### MSVC-Specific Module Considerations

#### Constant Declarations in Modules

```cpp
// ✅ GOOD: Use constexpr without explicit inline in modules
export namespace engine::rendering {
    constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};
    constexpr TextureId INVALID_TEXTURE = 0;
}

// ❌ BAD: Explicit inline can cause 4GB object file errors in MSVC
export namespace engine::rendering {
    inline constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};
    inline constexpr TextureId INVALID_TEXTURE = 0;
}
```

**Rationale**: MSVC's C++20 module implementation has a bug where explicit `inline constexpr` generates excessive debug
symbols and template instantiations, leading to the C1605 "object file size cannot exceed 4 GB" error. Using `constexpr`
alone (which is implicitly inline) avoids this issue while maintaining the same semantics.

This style guide ensures consistency across the codebase while leveraging modern C++20 features effectively.
