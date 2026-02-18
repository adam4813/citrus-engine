# Scene Management Guide

Learn how to organize your game into scenes using the Citrus Engine Scene Management system.

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Core Concepts](#core-concepts)
- [Creating Scenes](#creating-scenes)
- [Managing Entities](#managing-entities)
- [Scene Hierarchies](#scene-hierarchies)
- [Scene Lifecycle](#scene-lifecycle)
- [Multi-Scene Workflows](#multi-scene-workflows)
- [Saving and Loading](#saving-and-loading)
- [Prefabs](#prefabs)
- [Scene Assets](#scene-assets)
- [Best Practices](#best-practices)

---

## Overview

The Scene Management system provides a way to organize your game entities into logical groups (scenes). Each scene can represent a level, menu, game state, or any other logical grouping you need.

**Key Features:**
- 🌳 **Hierarchical organization** - Parent-child entity relationships
- 💾 **Scene serialization** - Save/load complete scenes to JSON
- 🧩 **Prefab system** - Reusable entity templates with inheritance
- 🎨 **Multi-scene support** - Run multiple scenes simultaneously (e.g., game + UI)
- ⚡ **ECS-native** - Built on Flecs, seamlessly integrates with your ECS code
- 🔄 **Lifecycle callbacks** - Initialize, Update, Render, Shutdown hooks

---

## Quick Start

```cpp
#include <engine/scene/scene.cppm>
#include <engine/scene/scene_manager.cpp>

using namespace engine::scene;

// Get the scene manager (singleton)
auto& mgr = GetSceneManager();

// Create a new scene
SceneId scene_id = mgr.CreateScene("Level1");
Scene& scene = mgr.GetScene(scene_id);

// Configure scene settings
scene.SetBackgroundColor({0.2f, 0.3f, 0.4f, 1.0f});
scene.SetAmbientLight({0.1f, 0.1f, 0.1f, 1.0f});

// Create entities
auto player = scene.CreateEntity("Player");
player.set<Transform>({.position = {0.0f, 0.0f, 0.0f}});
player.set<Renderable>({/* ... */});

// Activate the scene
mgr.SetActiveScene(scene_id);

// In your game loop
mgr.Update(delta_time);
mgr.Render();
```

---

## Core Concepts

### Scene Root Pattern

Every scene has a **root entity**. All entities in the scene are descendants of this root via Flecs' `ChildOf` relationship.

```cpp
Scene& scene = mgr.GetScene(scene_id);
auto root = scene.GetSceneRoot();  // Returns the root entity

// All scene entities are descendants
auto player = scene.CreateEntity("Player");
// player has ChildOf relationship to scene root
```

### Scenes Organize, Don't Own

**Important:** Scenes don't "own" entities. Entities belong to the ECS world. Scenes provide logical grouping through parent-child relationships.

This means:
- ✅ You can create entities directly in the ECS world without a scene
- ✅ Entities from different scenes can interact
- ✅ ECS systems run globally across all scenes
- ⚠️ Destroying a scene destroys all its entities

### Multi-Scene Architecture

You can have **one active scene** and **multiple additional scenes**:

```cpp
// Primary scene (game world)
mgr.SetActiveScene(game_scene_id);

// Overlay scenes (UI, debug tools)
mgr.ActivateAdditionalScene(ui_scene_id);
mgr.ActivateAdditionalScene(debug_scene_id);

// All scenes receive Update() and Render()
```

**Use cases:**
- **Game + HUD** - Keep UI separate from game world
- **Game + Pause Menu** - Overlay pause menu without unloading game
- **Game + Debug Console** - Persistent debug overlay

---

## Creating Scenes

### Method 1: Programmatically

```cpp
auto& mgr = GetSceneManager();

// Create an empty scene
SceneId id = mgr.CreateScene("MyScene");
Scene& scene = mgr.GetScene(id);

// Configure settings
scene.SetBackgroundColor({0.5f, 0.7f, 1.0f, 1.0f});  // Sky blue
scene.SetAmbientLight({0.2f, 0.2f, 0.2f, 1.0f});     // Dark gray
scene.SetPhysicsBackend("jolt");                      // Physics engine

// Set lifecycle callbacks
scene.SetInitializeCallback([]() {
    CE_LOG_INFO("Scene initialized!");
});

scene.SetUpdateCallback([](float delta_time) {
    // Per-frame logic
});

scene.SetRenderCallback([]() {
    // Custom rendering
});

scene.SetShutdownCallback([]() {
    CE_LOG_INFO("Scene shutting down!");
});
```

### Method 2: Load from File

```cpp
// Load a scene from JSON
SceneId id = mgr.LoadSceneFromFile("assets/scenes/level1.scene.json");
Scene& scene = mgr.GetScene(id);

// Activate it
mgr.SetActiveScene(id);
```

### Scene Metadata

```cpp
scene.SetName("Forest Level");
scene.SetDescription("The player explores a dark forest");
scene.SetAuthor("Your Name");
```

---

## Managing Entities

### Creating Entities

```cpp
Scene& scene = mgr.GetScene(scene_id);

// Simple entity
auto entity = scene.CreateEntity("Player");

// With components
auto enemy = scene.CreateEntity("Enemy");
enemy.set<Transform>({.position = {10.0f, 0.0f, 0.0f}});
enemy.set<Health>({.current = 100, .max = 100});
enemy.set<Renderable>({/* ... */});

// With parent (creates hierarchy)
auto weapon = scene.CreateEntity("Weapon", enemy);
weapon.set<Transform>({.position = {0.0f, 1.5f, 0.5f}});  // Relative to enemy
```

### Querying Entities

```cpp
// Get all entities in the scene
auto all_entities = scene.GetAllEntities();

// Get entities by name
auto player = scene.GetEntityByName("Player");

// Spatial queries
auto at_point = scene.QueryPoint(glm::vec3(5.0f, 0.0f, 5.0f));
auto in_sphere = scene.QuerySphere(
    glm::vec3(0.0f, 0.0f, 0.0f),  // Center
    10.0f,                         // Radius
    LayerMask::Default             // Layer filter
);
```

### Destroying Entities

```cpp
// Destroy a single entity
scene.DestroyEntity(entity);

// Destroy all entities in the scene
scene.Clear();
```

---

## Scene Hierarchies

Hierarchies use Flecs' native `ChildOf` relationship. Child transforms are relative to their parent.

### Creating Hierarchies

```cpp
// Create parent
auto tank = scene.CreateEntity("Tank");
tank.set<Transform>({.position = {10.0f, 0.0f, 0.0f}});

// Create child (Method 1: pass parent to CreateEntity)
auto turret = scene.CreateEntity("Turret", tank);
turret.set<Transform>({.position = {0.0f, 2.0f, 0.0f}});  // Relative to tank

// Create child (Method 2: SetParent)
auto barrel = scene.CreateEntity("Barrel");
barrel.set<Transform>({.position = {0.0f, 0.5f, 1.0f}});
scene.SetParent(barrel, turret);

// Hierarchy:
// Tank (world position: 10, 0, 0)
//  └─ Turret (world position: 10, 2, 0)
//      └─ Barrel (world position: 10, 2.5, 1)
```

### Navigating Hierarchies

```cpp
// Get immediate children
auto children = scene.GetChildren(tank);
for (auto child : children) {
    CE_LOG_INFO("Child: {}", child.name().c_str());
}

// Get all descendants (recursive)
auto descendants = scene.GetDescendants(tank);
// Returns: [turret, barrel]

// Get parent
auto parent = scene.GetParent(barrel);
// Returns: turret
```

### Transform Spaces

```cpp
// Local transform (relative to parent)
auto local = turret.get<Transform>();
// local->position = {0, 2, 0}

// World transform (absolute position)
auto world = turret.get<WorldTransform>();
// world->position = {10, 2, 0}

// Transform system automatically updates WorldTransform
// when Transform or parent changes
```

---

## Scene Lifecycle

Scenes follow this lifecycle:

```
Created → Loaded → Active → Inactive → Unloaded → Destroyed
```

### State Transitions

```cpp
// 1. Create
SceneId id = mgr.CreateScene("Level1");
// State: Created

// 2. Load assets
mgr.LoadScene(id);
// State: Loaded
// → Shaders compiled, textures loaded

// 3. Activate
mgr.SetActiveScene(id);
// State: Active
// → Initialize() callback invoked
// → Receives Update() and Render() calls

// 4. Deactivate
mgr.DeactivateScene(id);
// State: Inactive
// → Shutdown() callback invoked
// → No longer receives updates

// 5. Unload
mgr.UnloadScene(id);
// State: Unloaded
// → GPU resources freed

// 6. Destroy
mgr.DestroyScene(id);
// State: Destroyed
// → All entities deleted
// → Scene removed from manager
```

### Lifecycle Callbacks

```cpp
scene.SetInitializeCallback([]() {
    // Called once when scene becomes active
    // Setup initial state, spawn entities, etc.
    CE_LOG_INFO("Scene starting");
});

scene.SetUpdateCallback([](float delta_time) {
    // Called every frame while scene is active
    // Game logic, AI, physics, etc.
});

scene.SetRenderCallback([]() {
    // Called every frame while scene is active
    // Custom rendering (most rendering is automatic)
});

scene.SetShutdownCallback([]() {
    // Called once when scene is deactivated
    // Cleanup, save state, etc.
    CE_LOG_INFO("Scene ending");
});
```

---

## Multi-Scene Workflows

### Example: Game + HUD

Keep your UI separate from the game world:

```cpp
// Create game world scene
SceneId game_id = mgr.CreateScene("GameWorld");
Scene& game = mgr.GetScene(game_id);
// ... populate with 3D entities

// Create UI scene
SceneId ui_id = mgr.CreateScene("HUD");
Scene& ui = mgr.GetScene(ui_id);
// ... populate with UI elements

// Activate both
mgr.SetActiveScene(game_id);           // Primary scene
mgr.ActivateAdditionalScene(ui_id);    // Overlay

// Both receive updates
mgr.Update(delta_time);  // Updates game AND ui
mgr.Render();            // Renders game, then ui
```

### Example: Pause Menu

Overlay a pause menu without unloading the game:

```cpp
// Game is running
mgr.SetActiveScene(game_id);

// Player presses pause
SceneId pause_id = mgr.CreateScene("PauseMenu");
// ... create pause menu UI
mgr.ActivateAdditionalScene(pause_id);

// Game keeps rendering (frozen), pause menu renders on top

// Player unpauses
mgr.DeactivateScene(pause_id);
mgr.DestroyScene(pause_id);
```

### Example: Level Transitions

Smoothly transition between levels:

```cpp
// Fade out current level
StartFadeOut();

// Wait for fade to complete (async or in update loop)
WaitForFade();

// Switch scenes
mgr.DeactivateScene(current_level_id);
mgr.UnloadScene(current_level_id);
mgr.DestroyScene(current_level_id);

SceneId next_level_id = mgr.LoadSceneFromFile("scenes/level2.scene.json");
mgr.SetActiveScene(next_level_id);

// Fade in new level
StartFadeIn();
```

---

## Saving and Loading

### Save Scene to JSON

```cpp
Scene& scene = mgr.GetScene(scene_id);

// Save the entire scene
bool success = mgr.SaveScene(scene_id, "assets/scenes/my_level.scene.json");

if (!success) {
    CE_LOG_ERROR("Failed to save scene");
}
```

### Load Scene from JSON

```cpp
// Load scene from file
SceneId id = mgr.LoadSceneFromFile("assets/scenes/my_level.scene.json");

if (id == InvalidSceneId) {
    CE_LOG_ERROR("Failed to load scene");
    return;
}

// Activate it
mgr.SetActiveScene(id);
```

### What Gets Saved?

- ✅ Scene metadata (name, description, author)
- ✅ Scene settings (background color, ambient light, physics backend)
- ✅ Asset definitions (shaders, meshes, textures)
- ✅ All entities and their components
- ✅ Entity hierarchies (parent-child relationships)
- ✅ Active camera reference
- ✅ World singletons (physics configuration)

### Scene JSON Format

```json
{
  "version": 1,
  "name": "Forest Level",
  "description": "Dark forest exploration",
  "author": "Your Name",
  "settings": {
    "background_color": [0.2, 0.3, 0.4, 1.0],
    "ambient_light": [0.1, 0.1, 0.1, 1.0],
    "physics_backend": "jolt"
  },
  "assets": [
    {
      "type": "shader",
      "name": "basic_shader",
      "vertex_path": "shaders/basic.vert",
      "fragment_path": "shaders/basic.frag"
    },
    {
      "type": "mesh",
      "name": "ground",
      "mesh_type": "quad",
      "params": [100.0, 100.0, 1.0]
    }
  ],
  "flecs_data": "[{\"path\": \"::Scene_Root::Player\", ...}]",
  "active_camera": "::Scene_Root::MainCamera"
}
```

### Snapshot/Restore (Editor Play Mode)

Use snapshots to save and restore scene state without file I/O:

```cpp
#include <engine/scene/scene_serializer.cppm>

// Capture current state
std::string snapshot = SceneSerializer::SnapshotEntities(scene, ecs_world);

// ... modify scene (e.g., play mode in editor)

// Restore original state
SceneSerializer::RestoreEntities(snapshot, scene, ecs_world);
```

---

## Prefabs

Prefabs are reusable entity templates using Flecs' native prefab system with `is_a()` inheritance.

### Creating Prefabs

```cpp
#include <engine/scene/prefab.cppm>

// Create a template entity
auto enemy_template = ecs_world.CreateEntity("EnemyTemplate");
enemy_template.set<Transform>({.position = {0, 0, 0}});
enemy_template.set<Health>({.current = 100, .max = 100});
enemy_template.set<AI>({.type = AIType::Aggressive});

// Save as prefab
auto prefab = PrefabUtility::SaveAsPrefab(
    enemy_template, 
    ecs_world, 
    "assets/prefabs/enemy.prefab.json"
);

// Template entity is now converted to an instance!
```

### Instantiating Prefabs

```cpp
// Load and instantiate
auto enemy1 = PrefabUtility::InstantiatePrefab(
    "assets/prefabs/enemy.prefab.json",
    &scene,
    ecs_world,
    {}  // No parent
);

// Override components
enemy1.set<Transform>({.position = {10, 0, 0}});

// Instance inherits other components from prefab
auto health = enemy1.get<Health>();  // Still 100/100
```

### Multiple Instances

```cpp
// Spawn 10 enemies
for (int i = 0; i < 10; i++) {
    auto enemy = PrefabUtility::InstantiatePrefab(
        "assets/prefabs/enemy.prefab.json",
        &scene,
        ecs_world,
        {}
    );
    
    // Position each uniquely
    enemy.set<Transform>({.position = {i * 5.0f, 0, 0}});
}
```

### Prefab Inheritance

Instances use Flecs' `is_a()` relationship for component inheritance:

```cpp
// Instance inherits from prefab
auto enemy = PrefabUtility::InstantiatePrefab(...);

// Component queries automatically resolve inheritance
auto health = enemy.get<Health>();  
// Returns prefab's Health if not overridden, or instance's Health if overridden

// Override a component
enemy.set<Health>({.current = 50, .max = 100});  
// Now this instance has its own Health component
```

### Apply Changes to Prefab Source

```cpp
// Modify an instance
auto enemy = PrefabUtility::InstantiatePrefab(...);
enemy.set<Health>({.current = 150, .max = 150});  // Buff enemies!

// Apply changes back to prefab
PrefabUtility::ApplyToSource(enemy, ecs_world);

// All future instances will have 150 health
```

### Prefab JSON Format

```json
{
  "version": 1,
  "name": "Enemy",
  "flecs_data": "[{\"path\": \"Enemy\", \"data\": \"{...}\"}]"
}
```

---

## Scene Assets

Scenes can define and load their own assets (shaders, meshes, textures).

### Defining Assets in Code

```cpp
#include <engine/scene/scene_assets.cppm>

Scene& scene = mgr.GetScene(scene_id);

// Add shader
ShaderAsset shader;
shader.name = "basic_shader";
shader.vertex_path = "shaders/basic.vert";
shader.fragment_path = "shaders/basic.frag";
scene.AddAsset(shader);

// Add mesh
MeshAsset mesh;
mesh.name = "player_mesh";
mesh.mesh_type = MeshType::FromFile;
mesh.params = {"models/player.obj"};
scene.AddAsset(mesh);

// Add texture
TextureAsset texture;
texture.name = "player_texture";
texture.texture_type = TextureType::FromFile;
texture.params = {"textures/player.png"};
scene.AddAsset(texture);
```

### Defining Assets in JSON

```json
{
  "assets": [
    {
      "type": "shader",
      "name": "basic",
      "vertex_path": "shaders/basic.vert",
      "fragment_path": "shaders/basic.frag"
    },
    {
      "type": "mesh",
      "name": "sphere",
      "mesh_type": "sphere",
      "params": [1.0, 32, 32]
    },
    {
      "type": "texture",
      "name": "wood",
      "texture_type": "from_file",
      "params": ["textures/wood.png"]
    }
  ]
}
```

### Using Scene Assets

Assets are automatically loaded when the scene is loaded:

```cpp
// Load scene (loads all assets)
SceneId id = mgr.LoadSceneFromFile("scenes/level1.scene.json");

// Assets are now available in the asset manager
auto shader = AssetManager::Get().GetShader("basic_shader");
auto mesh = AssetManager::Get().GetMesh("player_mesh");
auto texture = AssetManager::Get().GetTexture("player_texture");
```

### Asset Lifecycle

- **Scene Load** → Assets compiled/loaded
- **Scene Unload** → Assets unloaded (GPU resources freed)
- **Scene Destroy** → Assets removed from manager

---

## Best Practices

### ✅ Do

1. **Use scenes for logical grouping** - Levels, menus, game states
2. **Keep hierarchies shallow** - Deep hierarchies are expensive to update
3. **Use multi-scene for UI** - Separate game world from UI overlays
4. **Use prefabs for repeated entities** - Memory efficient, easier to maintain
5. **Save scenes often** - Version control your scene files
6. **Use lifecycle callbacks** - Clean initialization and shutdown
7. **Name entities clearly** - Makes debugging and serialization easier

### ❌ Don't

1. **Don't create scenes for temporary entities** - Use ECS directly for bullets, particles
2. **Don't nest scenes** - Scenes are top-level, not nestable
3. **Don't store gameplay logic in scenes** - Use ECS systems instead
4. **Don't assume scene isolation** - Entities from different scenes can interact
5. **Don't forget to unload** - Free GPU resources when done with scenes

### Performance Tips

1. **Spatial queries** - Use `QuerySphere()` instead of iterating all entities
2. **Transform updates** - Only modified transforms are recomputed (dirty flag)
3. **Batch entity creation** - Create all entities at once, then set components
4. **Prefab instances** - Share component data across many instances
5. **Scene-scoped queries** - Filter ECS queries by scene membership

### Organization Patterns

**Simple Game:**
```
- MainMenu (scene)
- Level1 (scene)
- Level2 (scene)
- GameOver (scene)
```

**Complex Game:**
```
- MainMenu (scene)
- InGameWorld (scene)
- InGameUI (scene, overlay)
- PauseMenu (scene, conditional overlay)
- Settings (scene, conditional overlay)
```

**Editor:**
```
- EditorScene (scene)
- EditorUI (scene, overlay)
- GamePreview (scene, conditional)
```

---

## Complete Example

Putting it all together:

```cpp
#include <engine/scene/scene.cppm>
#include <engine/scene/scene_manager.cpp>
#include <engine/scene/prefab.cppm>

using namespace engine::scene;

void SetupGame() {
    auto& mgr = GetSceneManager();
    
    // Create main game scene
    SceneId game_id = mgr.CreateScene("ForestLevel");
    Scene& game = mgr.GetScene(game_id);
    
    // Configure scene
    game.SetBackgroundColor({0.5f, 0.7f, 1.0f, 1.0f});  // Sky blue
    game.SetAmbientLight({0.2f, 0.2f, 0.2f, 1.0f});
    
    // Add assets
    ShaderAsset shader;
    shader.name = "basic";
    shader.vertex_path = "shaders/basic.vert";
    shader.fragment_path = "shaders/basic.frag";
    game.AddAsset(shader);
    
    // Create player
    auto player = game.CreateEntity("Player");
    player.set<Transform>({.position = {0, 0, 0}});
    player.set<Renderable>({/* ... */});
    player.set<PlayerController>({/* ... */});
    
    // Create enemies from prefab
    for (int i = 0; i < 5; i++) {
        auto enemy = PrefabUtility::InstantiatePrefab(
            "prefabs/enemy.prefab.json",
            &game,
            ecs_world,
            {}
        );
        enemy.set<Transform>({.position = {i * 10.0f, 0, 20.0f}});
    }
    
    // Create camera
    auto camera = game.CreateEntity("MainCamera");
    camera.set<Transform>({.position = {0, 5, -10}});
    camera.set<Camera>({/* ... */});
    
    // Create UI overlay
    SceneId ui_id = mgr.CreateScene("HUD");
    Scene& ui = mgr.GetScene(ui_id);
    
    auto health_bar = ui.CreateEntity("HealthBar");
    health_bar.set<UIElement>({/* ... */});
    
    // Activate both scenes
    mgr.SetActiveScene(game_id);
    mgr.ActivateAdditionalScene(ui_id);
    
    // Save for later
    mgr.SaveScene(game_id, "scenes/forest_level.scene.json");
}

void GameLoop(float delta_time) {
    auto& mgr = GetSceneManager();
    
    // Update all active scenes
    mgr.Update(delta_time);
    
    // Render all active scenes
    mgr.Render();
}

void LoadLevel(const std::string& path) {
    auto& mgr = GetSceneManager();
    
    // Unload current level
    if (current_level_id != InvalidSceneId) {
        mgr.DeactivateScene(current_level_id);
        mgr.UnloadScene(current_level_id);
        mgr.DestroyScene(current_level_id);
    }
    
    // Load new level
    current_level_id = mgr.LoadSceneFromFile(path);
    mgr.SetActiveScene(current_level_id);
}
```

---

## Next Steps

- Read the [Architecture Overview](architecture.md) to understand the ECS architecture
- Check [Asset System](asset-system.md) for loading shaders, textures, and resources
- Explore [Physics API](physics.md) to add physics simulation to your scenes
- See [Audio API](audio.md) for adding sound effects and music
- Browse the [API Reference](api/index.md) for detailed class documentation

---

*For detailed API reference, see the generated [Doxygen documentation](api/index.md).*
