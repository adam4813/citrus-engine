# Design Principles

Architectural and design guidelines for Citrus Engine. These encode opinions and tradeoffs — read them before
implementing a new system. For syntax and naming rules, see [Code Style](code-style.md).

---

## Correctness > Simplicity > Performance

Make it work first. Keep it simple. Optimize only when profiling proves a bottleneck.

```cpp
// First: make it correct
std::vector<Entity> GetVisibleEntities(const Frustum& frustum) {
    std::vector<Entity> visible;
    for (const auto& entity : entities_) {
        if (frustum.Contains(entity.GetBounds())) visible.push_back(entity);
    }
    return visible;
}

// Later, if profiling shows this is the bottleneck:
visible.reserve(entities_.size() / 4);  // Pre-allocate — still correct, now faster
```

---

## Composition Over Inheritance

Build functionality by composing member objects. Use inheritance only for polymorphism (abstract interfaces).

```cpp
// ✅ CORRECT: Composition — GameWorld "has-a" scene, registry, systems
class GameWorld {
    SceneManager scene_manager_;
    EntityRegistry entity_registry_;
    SystemManager system_manager_;
};

// ✅ CORRECT: Inheritance for polymorphism (Strategy pattern)
class System {
public:
    virtual ~System() = default;
    virtual void Update(float delta_time) = 0;
};
class PhysicsSystem : public System { /* ... */ };

// ❌ WRONG: Inheritance for code reuse — use composition or ECS instead
class PlayerEntity : public Entity { /* adds player methods */ };
```

**Keep hierarchies shallow.** Max 2–3 levels. Deep hierarchies are hard to trace and fragile to change.

---

## Gang of Four Patterns

Use patterns when they solve a real problem. Don't apply them speculatively.

| Pattern       | When to use                                                                             |
|---------------|-----------------------------------------------------------------------------------------|
| **Observer**  | Event callbacks, signal systems, reactive state (e.g. input events, entity lifecycle)   |
| **Strategy**  | Interchangeable algorithms behind a stable interface (rendering backends, AI behaviors) |
| **Factory**   | Object creation with complex initialization or runtime type selection                   |
| **Command**   | Undo/redo, action history, input recording and replay                                   |
| **Singleton** | Engine core and resource managers — use sparingly, prefer dependency injection          |

When a Singleton would work but the component is testable, prefer passing it explicitly instead.

---

## DRY — Don't Repeat Yourself (Without Over-Engineering)

Extract shared logic when duplication is the actual problem. Don't extract preemptively.

**Extract when:**

- The same logic appears 3+ times in unrelated locations
- A change to the logic would require updating multiple places manually
- The extracted abstraction has a clear name and single responsibility

**Don't extract when:**

- Removing duplication requires 3+ files to understand one feature
- The "shared" code is coincidentally similar but serves different purposes
- The abstraction would be more complex than the duplication it eliminates

```cpp
// ✅ GOOD: Extracted because the logic is identical and has a clear name
float NormalizeAngle(float angle) {
    return std::fmod(angle + 360.0f, 360.0f);
}

// ❌ OVER-ENGINEERED: Abstraction that obscures what's happening
template<typename T, typename Normalizer>
T NormalizeValue(T value, Normalizer fn) { return fn(value); }
```

---

## Options Objects Over Long Parameter Lists

When a function takes more than **3 parameters**, group related parameters into a named struct.

```cpp
// ❌ HARD TO READ: What are these floats?
TextureId CreateTexture(int width, int height, TextureFormat format,
                        bool generate_mipmaps, int mip_levels, bool srgb);

// ✅ CLEAR: Named fields, self-documenting at the call site
struct TextureCreateInfo {
    int width{};
    int height{};
    TextureFormat format{TextureFormat::RGBA8};
    bool generate_mipmaps{false};
    int mip_levels{1};
    bool srgb{false};
};
TextureId CreateTexture(const TextureCreateInfo& info);

// Call site is readable:
auto id = CreateTexture({.width = 512, .height = 512, .format = TextureFormat::RGBA8});
```

**Exception**: Functions where parameter identity is obvious from domain convention don't need wrapping:

```cpp
Color(float r, float g, float b, float a);  // Convention-clear: r, g, b, a
Vec3(float x, float y, float z);            // Convention-clear: x, y, z
lerp(float a, float b, float t);            // Convention-clear: from, to, factor
```

---

## Smaller Files — One Concept Per File

A file should implement one concept. If you find yourself adding a feature that touches 3+ unrelated concepts in one
file, it's a signal to split. 1 class per file, unless they are minimal (specializations) or for template reasons.

**File size heuristics:**

- **~100–300 lines**: ideal range for a focused module file
- **300–500 lines**: acceptable if the file is genuinely one concept
- **500+ lines**: strong signal to split into module partitions (`:types`, `:mesh`, `:texture`)

```
// ✅ GOOD: focused files
engine.rendering:types      → structs, enums, IDs
engine.rendering:texture    → texture loading and management
engine.rendering:mesh       → mesh/vertex buffer management
engine.rendering:pipeline   → shader and draw call pipeline

// ❌ BAD: one file doing everything
engine.rendering            → all of the above in one 2000-line file
```

---

## Declarative Over Imperative

Prefer expressing *what* you want over *how* to compute it. This reduces bugs, makes intent clear, and is easier to
reason about.

```cpp
// ❌ IMPERATIVE: how
std::vector<EntityID> active_ids;
for (const auto& entity : entities_) {
    if (entity.IsActive()) {
        active_ids.push_back(entity.GetID());
    }
}

// ✅ DECLARATIVE: what
auto active_ids = entities_
    | std::views::filter([](const auto& e) { return e.IsActive(); })
    | std::views::transform([](const auto& e) { return e.GetID(); })
    | std::ranges::to<std::vector>();
```

```cpp
// ❌ IMPERATIVE: magic numbers obscure intent
if (state == 2 && flags & 0x04) { ... }

// ✅ DECLARATIVE: named types express intent
if (state == EntityState::Active && HasFlag(flags, EntityFlag::Visible)) { ... }
```

Use named types, concepts, enums, and well-named functions to make code read like a description of behavior.

---

## Function Size and Single Responsibility

Functions should do one thing. Target 20–30 lines. If you need a comment to explain what a section does, that section
wants to be its own function.

```cpp
// ✅ GOOD: focused, named, easy to test
void UpdateCamera(Camera& camera, float delta_time) {
    if (!camera.IsActive()) return;
    Vec3 movement = camera.GetDirection() * camera.GetMoveSpeed() * delta_time;
    camera.SetPosition(camera.GetPosition() + movement);
}

// ❌ BAD: does camera + lights + physics + rendering setup in one function
void UpdateScene(Camera& camera, std::vector<Light>& lights, float dt) {
    // 40 lines of camera logic
    // 40 lines of light logic
    // 30 lines of physics
    // ...
}
```

---

## RAII — Resource Acquisition Is Initialization

Resources must be acquired in constructors and released in destructors. Never use manual `new`/`delete`.

```cpp
// ✅ CORRECT: RAII — automatic cleanup
class ResourceManager {
    std::unique_ptr<Resource> resource_;
public:
    ResourceManager() : resource_(std::make_unique<Resource>()) {}
    // Destructor is automatic — no leak possible
};

// ❌ WRONG: Manual management — will leak if exception is thrown
ResourceManager() { r = new Resource(); }
~ResourceManager() { delete r; }
```

### Smart Pointer Rules

| Pointer type      | When to use                                     |
|-------------------|-------------------------------------------------|
| `std::unique_ptr` | Default — exclusive ownership                   |
| `std::shared_ptr` | Genuinely shared ownership (use sparingly)      |
| Raw pointer       | Non-owning reference only — never for ownership |

Always use `std::make_unique` / `std::make_shared` — never call `new` directly.

---

## Memory Allocation Strategy

Prefer stack allocation. Use heap only when size is unknown at compile time, lifetime outlasts the scope, or
polymorphism requires it.

```cpp
Transform transform{};          // ✅ Stack — small, known size, local lifetime
auto texture = std::make_unique<Texture>(data);  // ✅ Heap — large, unknown lifetime

auto t = std::make_unique<Transform>();  // ❌ No reason to heap-allocate this
```

### Function Signature Performance Rules

```cpp
void ProcessEntity(const Transform& transform);            // Large objects: const ref
Vec3 GetPosition() const;                                  // Small objects: by value
std::unique_ptr<Texture> CreateTexture(TextureCreateInfo&& info);  // Move expensive objects
```
