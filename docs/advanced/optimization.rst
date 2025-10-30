Performance Optimization
========================

Tips and techniques for optimizing your Citrus Engine game.

General Principles
------------------

1. **Measure First**: Use profiling tools before optimizing
2. **Batch Operations**: Group similar operations together
3. **Cache-Friendly**: Keep data contiguous in memory
4. **Minimize Allocations**: Reuse objects when possible

ECS Optimization
----------------

Component Design
~~~~~~~~~~~~~~~~

Keep components small and data-oriented:

.. code-block:: cpp

   // GOOD: Small, focused component
   struct Transform {
       glm::vec3 position;
       glm::quat rotation;
       glm::vec3 scale;
   };
   
   // BAD: Large component with behavior
   struct GameObject {
       Transform transform;
       std::vector<Component*> components;
       void Update() { /* ... */ }
   };

Query Optimization
~~~~~~~~~~~~~~~~~~

Cache query results when possible:

.. code-block:: cpp

   // Cache query for reuse
   auto moving_entities = scene.Query<Transform, Velocity>();
   
   // Reuse in multiple systems
   for (auto [entity, transform, velocity] : moving_entities) {
       // Process
   }

Entity Pooling
~~~~~~~~~~~~~~

Reuse entities instead of create/destroy:

.. code-block:: cpp

   class EntityPool {
       std::vector<Entity> inactive_;
       
       Entity Acquire() {
           if (!inactive_.empty()) {
               auto entity = inactive_.back();
               inactive_.pop_back();
               entity.Enable();
               return entity;
           }
           return scene.CreateEntity();
       }
       
       void Release(Entity entity) {
           entity.Disable();
           inactive_.push_back(entity);
       }
   };

Rendering Optimization
----------------------

Batch Rendering
~~~~~~~~~~~~~~~

Minimize draw calls by batching:

.. code-block:: cpp

   // Use batch renderer for sprites
   auto batch = engine::BatchRenderer::Create();
   
   // Add all sprites to batch
   scene.Query<Sprite>([&](auto entity, auto& sprite) {
       batch->AddSprite(sprite);
   });
   
   // Single draw call
   batch->Render();

Texture Atlases
~~~~~~~~~~~~~~~

Combine textures to reduce state changes:

.. code-block:: cpp

   // Pack textures into atlas
   auto atlas = TextureAtlas::Create();
   atlas->AddTexture("player", "player.png");
   atlas->AddTexture("enemy", "enemy.png");
   atlas->Pack();
   
   // All sprites use same texture
   sprite1.texture = atlas->GetTexture();
   sprite1.uv = atlas->GetUV("player");

Frustum Culling
~~~~~~~~~~~~~~~

Only render visible objects:

.. code-block:: cpp

   auto camera_bounds = camera.GetBounds();
   
   scene.Query<Transform, Sprite>([&](auto e, auto& t, auto& s) {
       if (camera_bounds.Contains(t.position)) {
           renderer->DrawSprite(s, t);
       }
   });

LOD (Level of Detail)
~~~~~~~~~~~~~~~~~~~~~

Use simpler models when far away:

.. code-block:: cpp

   auto distance = glm::distance(camera.position, object.position);
   
   if (distance < 10.0f) {
       renderer->DrawHighDetail(object);
   } else if (distance < 50.0f) {
       renderer->DrawMediumDetail(object);
   } else {
       renderer->DrawLowDetail(object);
   }

Memory Optimization
-------------------

Object Pooling
~~~~~~~~~~~~~~

Reuse objects to avoid allocations:

.. code-block:: cpp

   template<typename T>
   class ObjectPool {
       std::vector<std::unique_ptr<T>> pool_;
       std::vector<T*> available_;
       
       T* Acquire() {
           if (available_.empty()) {
               pool_.push_back(std::make_unique<T>());
               return pool_.back().get();
           }
           auto obj = available_.back();
           available_.pop_back();
           return obj;
       }
       
       void Release(T* obj) {
           available_.push_back(obj);
       }
   };

Smart Pointer Usage
~~~~~~~~~~~~~~~~~~~

Use appropriate ownership semantics:

.. code-block:: cpp

   // GOOD: unique_ptr for exclusive ownership
   std::unique_ptr<Texture> texture = LoadTexture("sprite.png");
   
   // GOOD: shared_ptr only when truly shared
   std::shared_ptr<Shader> shader = GetSharedShader();
   
   // GOOD: raw pointer for non-owning reference
   Texture* current_texture = texture.get();

Asset Management
~~~~~~~~~~~~~~~~

Unload unused assets:

.. code-block:: cpp

   // Unload assets when leaving level
   asset_manager->UnloadLevel("level_01");
   
   // Or unload all unused
   asset_manager->UnloadUnused();

CPU Optimization
----------------

Multithreading
~~~~~~~~~~~~~~

Use multiple threads for independent work:

.. code-block:: cpp

   // Process systems in parallel
   std::vector<std::future<void>> futures;
   
   futures.push_back(std::async(std::launch::async, [&]() {
       physics_system.Update(delta_time);
   }));
   
   futures.push_back(std::async(std::launch::async, [&]() {
       animation_system.Update(delta_time);
   }));
   
   // Wait for completion
   for (auto& future : futures) {
       future.wait();
   }

Avoid Branches in Hot Loops
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // BAD: Branch in loop
   for (auto& entity : entities) {
       if (entity.active) {
           entity.Update();
       }
   }
   
   // GOOD: Filter first, then process
   auto active = entities | std::views::filter([](auto& e) {
       return e.active;
   });
   
   for (auto& entity : active) {
       entity.Update();
   }

Cache Locality
~~~~~~~~~~~~~~

Keep frequently accessed data together:

.. code-block:: cpp

   // GOOD: Data-oriented layout
   struct TransformData {
       std::vector<glm::vec3> positions;
       std::vector<glm::quat> rotations;
       std::vector<glm::vec3> scales;
   };
   
   // BAD: Object-oriented layout
   struct Transform {
       glm::vec3 position;
       glm::quat rotation;
       glm::vec3 scale;
       // Other data...
   };
   std::vector<Transform> transforms;

Profiling
---------

Built-in Profiler
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <engine/profiler.h>
   
   void Update() {
       PROFILE_SCOPE("Update");
       
       {
           PROFILE_SCOPE("Physics");
           physics.Update();
       }
       
       {
           PROFILE_SCOPE("Rendering");
           renderer.Render();
       }
   }

External Tools
~~~~~~~~~~~~~~

Recommended profiling tools:

* **Visual Studio Profiler** (Windows)
* **Instruments** (macOS)
* **perf** / **valgrind** (Linux)
* **Tracy Profiler** (Cross-platform)
* **RenderDoc** (Graphics debugging)

Benchmarking
~~~~~~~~~~~~

.. code-block:: cpp

   #include <chrono>
   
   auto start = std::chrono::high_resolution_clock::now();
   
   // Code to measure
   DoWork();
   
   auto end = std::chrono::high_resolution_clock::now();
   auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
       end - start
   );
   
   std::cout << "Duration: " << duration.count() << "µs\n";

Common Bottlenecks
------------------

1. **Too many draw calls**: Use batching
2. **Large textures**: Use texture compression and atlases
3. **Overdraw**: Sort back-to-front, use alpha testing
4. **CPU-GPU sync**: Minimize glReadPixels, use async queries
5. **Memory allocations**: Use object pools
6. **Cache misses**: Improve data locality

Platform-Specific Tips
----------------------

WebAssembly
~~~~~~~~~~~

* Minimize memory usage (heap is limited)
* Use SIMD when available
* Avoid exceptions (slow in WASM)
* Bundle assets to reduce HTTP requests

Mobile
~~~~~~

* Reduce texture sizes
* Use power-of-2 textures
* Minimize fill rate
* Implement suspend/resume correctly

Desktop
~~~~~~~

* Take advantage of more RAM
* Use multithreading extensively
* Enable vsync for smoother frame rates

See Also
--------

* :doc:`platform-specific` - Platform-specific optimizations
* :doc:`../guides/ecs` - ECS best practices
* :doc:`../guides/rendering` - Rendering optimization
