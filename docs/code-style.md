# Code Style Reference

Quick-reference conventions for everyday coding in Citrus Engine. For design patterns and architecture decisions, see [Design Principles](design-principles.md). For C++20 module mechanics, see [C++20 Modules](cpp20-modules.md).

---

## Naming Conventions

| Symbol | Style | Examples |
|--------|-------|---------|
| Functions / Methods | PascalCase | `InitializeRenderer()`, `LoadTexture()`, `GetViewMatrix()` |
| Variables / Parameters | snake_case | `entity_count`, `delta_time`, `file_path` |
| Classes / Structs | PascalCase | `EntityManager`, `Transform`, `TextureManager` |
| Enum class values | PascalCase | `ProjectionType::Perspective`, `TextureFormat::RGBA8` |
| Constants (`constexpr`, namespace scope) | `UPPER_CASE` | `DEFAULT_FOV`, `INVALID_TEXTURE` |
| Constant parameters / local `const` | `snake_case` | `const float delta_time` |
| Macros | UPPER_CASE | `ENGINE_VERSION_MAJOR` |
| Public member variables | snake_case | `Vec3 position{};` |
| Private member variables | snake_case + trailing `_` | `Vec3 position_{}; bool view_dirty_ = true;` |
| Namespaces | snake_case | `engine::platform`, `engine::rendering` |
| Template parameters | PascalCase | `ComponentType`, `T`, `BufferSize` |
| Files | snake_case | `entity_manager.cppm`, `platform.cpp` |
| Directories | snake_case | `src/engine/`, `cmake/` |

Use descriptive names. Abbreviate only when universally known (`fps`, `ui`, `ecs`, `id`).

---

## Formatting

- **Indentation**: Tabs (width 4) for C++ files. YAML and CMake files use 2 spaces (see `.editorconfig`).
- **Braces**: Opening brace on same line for functions, classes, and control flow. `else` and `catch` go on their own line (break before `else`/`catch`).
- **Line length**: 120 characters max; break at logical points (parameters, operators)
- **Binary operators**: When breaking a long expression, operators go at the start of the new line (e.g., `&& condition` not `condition &&`)
- **Function parameters**: When arguments don't fit on one line, each goes on its own line (never partially packed)
- **Spacing**: space after keywords (`if (`, `for (`), space around operators, no space before semicolons, space after commas

```cpp
class MyClass {
public:
	void MyFunction() {
		if (condition) {
			// Do something
		}
		else {  // else on its own line
			// Do something else
		}

		for (int i = 0; i < count; ++i) {
			// Loop body
		}

		try {
			DoSomething();
		}
		catch (const std::exception& e) {  // catch on its own line
			Handle(e);
		}
	}
};
```

`.clang-format` encodes these rules — run it before committing.

---

## Comments

Comment the **why**, not the what. Self-documenting code is preferred; comments fill in intent and algorithm rationale.

```cpp
// ✅ GOOD: Explains non-obvious algorithm
// Uses exponential backoff to reduce lock contention under high load
void RetryWithBackoff(int attempt) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100 << attempt));
}

// ✅ GOOD: Public API doc comment
/// Get the current frame rate in frames per second.
/// @return Current FPS as a double value
double GetCurrentFps() const;

// ❌ BAD: Redundant — code is self-explanatory
int GetEntityCount() {
    return entities_.size();  // Get the count of entities
}

// ❌ BAD: Explains "what", not "why"
x = x + 1;  // Increment x
```

Use `///` for public API documentation (Doxygen-compatible). Use `//` for inline implementation notes.

---

## Type Safety

### Prefer `enum class` over plain `enum`

```cpp
// ✅ CORRECT: Scoped, explicit underlying type
enum class ComponentType : uint8_t { Transform = 0, Sprite = 1, Physics = 2 };

// ❌ WRONG: Pollutes namespace, implicit conversions
enum ComponentType { TRANSFORM, SPRITE };
```

### Use named types to prevent accidental swaps

```cpp
using EntityID = uint32_t;
using TextureID = uint32_t;

void Process(EntityID id);   // Clear: this is an entity
void Bind(TextureID tex_id); // Clear: this is a texture
// Passing wrong ID type is now a compile error
```

### Const correctness

Mark everything `const` that doesn't need to mutate. It documents intent and catches bugs.

```cpp
const Transform& GetTransform() const;  // Doesn't modify *this
void Process(const Component& comp);    // Doesn't modify comp
Vec3 GetPosition() const;               // Returns by value (immutable snapshot)
```

---

## Auto and Type Deduction

Use `auto` for complex types and lambdas. Avoid it when the type communicates important intent.

```cpp
// ✅ GOOD: Auto for complex/iterator types
auto texture_manager = GetTextureManager();
auto it = components.find(id);
auto filter = [](const Entity& e) { return e.IsActive(); };

// ❌ BAD: Auto hides intent
auto count = 5;      // int? size_t? Use int count = 5;
auto* ptr = GetPtr(); // Owning? Non-owning? Use Entity* ptr = ...;
```

---

## Containers and Algorithms

Prefer standard library algorithms and C++20 ranges over raw loops when the declarative form is clearer.

```cpp
// ✅ GOOD: Ranges express intent directly
auto active_ids = entities
    | std::views::filter([](const auto& e) { return e.IsActive(); })
    | std::views::transform([](const auto& e) { return e.GetID(); });

// ✅ GOOD: ranges::find_if
auto it = std::ranges::find_if(components,
    [](const auto& c) { return c.GetType() == ComponentType::Transform; });
```

### Structured bindings

Use them to unpack pairs, tuples, and multi-value returns.

```cpp
auto [width, height] = GetScreenDimensions();
auto& [transform, physics] = GetEntityComponents();
```

---

## Error Handling

```cpp
std::optional<TextureId> LoadTexture(const fs::Path& path);  // May fail — caller checks
void InitializeRenderer();  // Throws on unrecoverable failure
bool IsValid(TextureId id) const;  // Simple validation query
```

Use assertions for development-time invariants:

```cpp
assert(entity.IsValid());  // Caught in debug builds
if (!file.IsOpen()) {      // Graceful runtime handling
    return std::nullopt;
}
```
