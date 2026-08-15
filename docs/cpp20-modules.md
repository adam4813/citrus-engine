# C++20 Modules Guide

Citrus Engine uses C++20 modules throughout. This guide covers the patterns you must follow — module mechanics are
non-obvious and getting them wrong produces hard-to-diagnose errors.

---

## Module Interface Files (`.cppm`)

### With Standard Library Includes — Global Module Fragment Required

If you `#include` any standard library headers, you **must** wrap them in a global module fragment:

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

### Without Standard Library Includes — No Global Module Fragment

```cpp
export module engine.rendering:simple;

export namespace engine::rendering {
    // Module content here — only using imported modules
}
```

**Rule**: Only add `module;` if you have `#include` statements. An empty global module fragment is an error.

---

## Module Implementation Files (`.cpp`)

### Critical: Implementations Do NOT Inherit Interface Imports

This is the most common mistake. Each `.cpp` file has its own import scope — it does **not** automatically see what the
corresponding `.cppm` imports.

```cpp
// texture.cppm (interface)
export module engine.rendering:texture;
import :types;           // Interface imports these
import engine.platform;

// texture.cpp (implementation) — MUST repeat all necessary imports
module;

#include <vector>  // Standard library in global fragment

module engine.rendering;  // Declare which module this implements

// REQUIRED: Import your own interface first
import :texture;

// REQUIRED: Re-import dependencies the implementation uses
import :types;           // Must repeat — needed for TextureId, etc.
import engine.platform;  // Must repeat — needed for fs::Path

namespace engine::rendering {
    // Implementation code
}
```

### Import Order Convention

1. Import your own interface (`:mymodule`)
2. Import sibling partitions you depend on
3. Import external engine modules (`engine.platform`, `engine.ecs`, etc.)
4. Optionally import implementation-only helpers (`:internal_utils`)

---

## Module Partitions

Use colon notation for partitions. The primary module re-exports all partitions:

```cpp
// Primary interface re-exports all partitions
export module engine.rendering;

export import :types;
export import :components;
export import :texture;

import engine.ecs;
import engine.platform;
```

---

## Namespace Conventions

Always use the `engine::` namespace hierarchy matching the module name:

```cpp
export module engine.rendering;

export namespace engine::rendering {
    class TextureManager { /* ... */ };
}

export module engine.platform;

export namespace engine::platform {
    class Window { /* ... */ };
}
```

---

## MSVC-Specific: `constexpr` vs `inline constexpr`

!!! warning "4GB Object File Bug"
    MSVC has a bug where `inline constexpr` in module interfaces generates excessive debug symbols, causing the
    C1605 "object file size cannot exceed 4GB" error. Use `constexpr` alone — it is implicitly inline and has
    identical semantics without triggering the bug.

```cpp
// ✅ CORRECT: constexpr without explicit inline
export namespace engine::rendering {
	constexpr Color WHITE{1.0f, 1.0f, 1.0f, 1.0f};
	constexpr TextureId INVALID_TEXTURE = 0;
}

// ❌ BAD: inline constexpr can cause C1605 in MSVC
export namespace engine::rendering {
	inline constexpr Color WHITE{1.0f, 1.0f, 1.0f, 1.0f};
	inline constexpr TextureId INVALID_TEXTURE = 0;
}
```

---

## Module Checklist

When creating a new module:

- [ ] Global module fragment (`module;`) present **only if** `#include` statements exist
- [ ] All `#include` statements are in the global module fragment (before `export module`)
- [ ] All `import` statements are after the module declaration
- [ ] Implementation `.cpp` imports its own interface first
- [ ] Implementation `.cpp` re-imports all dependencies it uses (not relying on transitive imports)
- [ ] Constants use `constexpr` not `inline constexpr`
- [ ] Namespace matches module name hierarchy
