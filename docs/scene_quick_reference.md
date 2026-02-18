# Scene Management Quick Reference

Quick reference for common scene management operations in Citrus Engine.

## 📦 Module Import

```cpp
import engine.scene;
using namespace engine::scene;
```

## 🎬 Scene Manager Basics

```cpp
// Get global scene manager
auto& mgr = GetSceneManager();

// Create scene
SceneId id = mgr.CreateScene("Level1");
Scene& scene = mgr.GetScene(id);

// Activate scene
mgr.SetActiveScene(id);

// Multi-scene
mgr.ActivateAdditionalScene(ui_scene_id);

// Save/Load
mgr.SaveScene(id, "scenes/level1.scene.json");
SceneId loaded = mgr.LoadSceneFromFile("scenes/level1.scene.json");

// Update (call every frame)
mgr.Update(delta_time);
mgr.Render();
```

## 🌳 Scene Entity Management

```cpp
Scene& scene = mgr.GetScene(scene_id);

// Create entities
auto entity = scene.CreateEntity("Player");
auto child = scene.CreateEntity("Weapon", parent);

// Hierarchy
scene.SetParent(child, parent);
scene.RemoveParent(child);
auto parent = scene.GetParent(entity);
auto children = scene.GetChildren(parent);
auto all = scene.GetDescendants(scene.GetSceneRoot());

// Find
auto entity = scene.FindEntityByName("Boss");
auto entities = scene.GetAllEntities();

// Destroy
scene.DestroyEntity(entity);
```

## 🎯 Spatial Queries

```cpp
// Scene-scoped
auto at_point = scene.QueryPoint(glm::vec3(0, 0, 0));
auto in_sphere = scene.QuerySphere(center, radius, layer_mask);

// Global (all active scenes)
auto all = mgr.QueryPoint(point);
auto nearby = mgr.QuerySphere(center, radius);
```

## ⚙️ Scene Settings

```cpp
scene.SetName("Level 1");
scene.SetBackgroundColor(glm::vec4(0.2, 0.3, 0.4, 1.0));
scene.SetAmbientLight(glm::vec4(0.1, 0.1, 0.1, 1.0));
scene.SetPhysicsBackend("jolt"); // "jolt", "bullet3", "none"
scene.SetAuthor("Developer");
scene.SetDescription("First level");
```

## 🔄 Lifecycle Callbacks

```cpp
scene.SetInitializeCallback([]() {
    std::cout << "Scene started!" << std::endl;
});

scene.SetShutdownCallback([]() {
    std::cout << "Scene ended!" << std::endl;
});

scene.SetUpdateCallback([](float dt) {
    // Custom update logic
});

scene.SetRenderCallback([]() {
    // Custom render logic
});
```

## 📦 Scene Assets

```cpp
auto& assets = scene.GetAssets();

// Add assets
auto shader = std::make_shared<ShaderAssetInfo>(
    "basic", "basic.vert", "basic.frag"
);
assets.Add(shader);

auto mesh = std::make_shared<MeshAssetInfo>("cube", "cube");
assets.Add(mesh);

// Find assets
auto shader = assets.FindTyped<ShaderAssetInfo>("basic");
auto all_shaders = assets.GetAllOfType<ShaderAssetInfo>();

// Load/unload
scene.LoadAssets();
scene.UnloadAssets();
```

## 🧩 Prefabs

```cpp
// Create prefab from entity
auto prefab = PrefabUtility::SaveAsPrefab(
    entity, ecs_world, "prefabs/enemy.prefab.json"
);

// Load prefab
auto prefab = PrefabUtility::LoadPrefab(
    "prefabs/enemy.prefab.json", ecs_world
);

// Instantiate
auto instance = PrefabUtility::InstantiatePrefab(
    "prefabs/enemy.prefab.json",
    &scene,
    ecs_world,
    parent  // optional
);

// Apply changes back to prefab
PrefabUtility::ApplyToSource(instance, ecs_world);
```

## 🔀 Scene Transitions

```cpp
// Instant transition
mgr.SetActiveScene(next_scene_id);

// Timed transition (TODO: fade effects)
mgr.TransitionToScene(next_scene_id, 1.0f);

// Check transition state
bool transitioning = mgr.IsTransitioning();
float progress = mgr.GetTransitionProgress();
```

## 📊 Scene States

```
Created → Loaded → Active → Inactive → Unloaded → Destroyed
```

```cpp
// State queries
bool active = scene.IsActive();
bool loaded = scene.IsLoaded();

// State transitions
scene.SetActive(true);
scene.SetLoaded(true);
mgr.LoadScene(scene_id);    // Load assets
mgr.UnloadScene(scene_id);  // Unload assets
mgr.DestroyScene(scene_id); // Destroy scene
```

## 📈 Statistics

```cpp
size_t total_scenes = mgr.GetSceneCount();
size_t active_scenes = mgr.GetActiveSceneCount();
size_t total_entities = mgr.GetTotalEntityCount();
```

## 💾 Serialization

```cpp
// Save scene
bool ok = SceneSerializer::Save(scene, ecs_world, path);

// Load scene
SceneId id = SceneSerializer::Load(path, mgr, ecs_world);

// Snapshot (for editor play mode)
std::string snapshot = SceneSerializer::SnapshotEntities(scene, ecs_world);
bool ok = SceneSerializer::RestoreEntities(snapshot, scene, ecs_world);
```

## 🗂️ File Formats

### Scene File Structure
```json
{
  "version": 1,
  "name": "Level1",
  "settings": { ... },
  "assets": [ ... ],
  "flecs_data": "[...]",
  "active_camera": "..."
}
```

### Prefab File Structure
```json
{
  "version": 1,
  "name": "EnemyPrefab",
  "entities": [ ... ]
}
```

## 🎨 Common Patterns

### Basic Scene Setup
```cpp
SceneId id = mgr.CreateScene("Level1");
Scene& scene = mgr.GetScene(id);
auto player = scene.CreateEntity("Player");
player.set<Transform>({.position = {0, 1, 0}});
mgr.SetActiveScene(id);
```

### Scene with Hierarchy
```cpp
auto tank = scene.CreateEntity("Tank");
auto turret = scene.CreateEntity("Turret", tank);
auto barrel = scene.CreateEntity("Barrel", turret);
```

### Multi-Scene (Game + UI)
```cpp
mgr.SetActiveScene(game_scene);
mgr.ActivateAdditionalScene(ui_scene);
```

### Enemy Spawning with Prefabs
```cpp
for (int i = 0; i < 10; i++) {
    auto enemy = PrefabUtility::InstantiatePrefab(
        "prefabs/enemy.prefab.json", &scene, ecs_world, {}
    );
    enemy.set<Transform>({.position = spawn_positions[i]});
}
```

### Scene Transition
```cpp
mgr.SetActiveScene(next_level_id);
// Old scene's Shutdown() called
// New scene's Initialize() called
```

---

See **SCENE_MANAGEMENT_GUIDE.md** for detailed documentation.
