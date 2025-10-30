Entity-Component-System
=======================

Citrus Engine uses the Entity-Component-System (ECS) architecture powered by
the `flecs <https://github.com/SanderMertens/flecs>`_ library.

What is ECS?
------------

ECS is a design pattern that separates data from behavior:

* **Entities**: Unique identifiers for game objects
* **Components**: Pure data (position, health, sprite, etc.)
* **Systems**: Logic that operates on entities with specific components

Benefits
~~~~~~~~

* **Performance**: Cache-friendly data layout
* **Flexibility**: Compose entities from components dynamically
* **Maintainability**: Separation of concerns
* **Scalability**: Easy to add new features

Basic Usage
-----------

Creating Entities
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.scene;
   
   engine::Scene scene;
   
   // Create an entity
   auto entity = scene.CreateEntity("Player");
   
   // Create multiple entities
   for (int i = 0; i < 100; i++) {
       auto enemy = scene.CreateEntity("Enemy");
   }

Adding Components
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.components;
   
   // Add transform component
   entity.AddComponent<engine::Transform>({
       .position = {100.0f, 50.0f, 0.0f},
       .rotation = {0.0f, 0.0f, 0.0f, 1.0f},
       .scale = {1.0f, 1.0f, 1.0f}
   });
   
   // Add sprite component
   entity.AddComponent<engine::Sprite>({
       .texture_id = player_texture,
       .size = {64.0f, 64.0f},
       .color = {1.0f, 1.0f, 1.0f, 1.0f}
   });

Accessing Components
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get a component
   if (auto* transform = entity.GetComponent<engine::Transform>()) {
       transform->position.x += 10.0f;
   }
   
   // Check if entity has component
   if (entity.HasComponent<engine::Sprite>()) {
       // Do something
   }

Removing Components
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   entity.RemoveComponent<engine::Sprite>();

Systems
-------

Systems contain the game logic that processes entities with specific components.

Built-in Systems
~~~~~~~~~~~~~~~~

Citrus Engine includes several built-in systems:

* **RenderSystem**: Renders sprites and meshes
* **PhysicsSystem**: Handles collision and movement
* **AnimationSystem**: Updates sprite animations

Creating Custom Systems
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.scene;
   
   class MovementSystem {
   public:
       void Update(engine::Scene& scene, float delta_time) {
           // Query all entities with Transform and Velocity
           scene.Query<engine::Transform, engine::Velocity>(
               [delta_time](auto entity, auto& transform, auto& velocity) {
                   // Update position based on velocity
                   transform.position.x += velocity.x * delta_time;
                   transform.position.y += velocity.y * delta_time;
               }
           );
       }
   };

Querying Entities
~~~~~~~~~~~~~~~~~

The scene provides powerful query capabilities:

.. code-block:: cpp

   // Query with multiple components
   scene.Query<Transform, Health>([](auto entity, auto& t, auto& h) {
       if (h.value <= 0) {
           // Entity is dead
       }
   });
   
   // Query with filters
   scene.Query<Transform>([](auto entity, auto& transform) {
       if (transform.position.y < 0) {
           // Entity fell off the map
       }
   });

Common Patterns
---------------

Parent-Child Relationships
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto parent = scene.CreateEntity("Parent");
   auto child = scene.CreateEntity("Child");
   
   // Make child a child of parent
   child.SetParent(parent);
   
   // Child transforms are relative to parent
   child.GetComponent<Transform>()->position = {10.0f, 0.0f, 0.0f};

Entity Pooling
~~~~~~~~~~~~~~

Reuse entities for better performance:

.. code-block:: cpp

   class BulletPool {
       std::vector<Entity> inactive_bullets_;
       
       Entity SpawnBullet() {
           if (!inactive_bullets_.empty()) {
               auto bullet = inactive_bullets_.back();
               inactive_bullets_.pop_back();
               bullet.Enable();
               return bullet;
           }
           return scene.CreateEntity("Bullet");
       }
       
       void ReturnBullet(Entity bullet) {
           bullet.Disable();
           inactive_bullets_.push_back(bullet);
       }
   };

Component Tags
~~~~~~~~~~~~~~

Use empty components as tags:

.. code-block:: cpp

   struct Player {};
   struct Enemy {};
   struct Collectible {};
   
   entity.AddComponent<Player>();  // Tag as player
   
   // Query only players
   scene.Query<Player, Transform>([](auto entity, auto& transform) {
       // Player-specific logic
   });

Best Practices
--------------

1. **Keep components as data**: No logic in components
2. **Systems are stateless**: Systems should not store state
3. **Use queries efficiently**: Cache query results when possible
4. **Avoid deep hierarchies**: Keep parent-child relationships shallow
5. **Component size**: Keep components small and focused

Performance Tips
----------------

* **Batch operations**: Group similar operations together
* **Minimize queries**: Don't query every frame if not needed
* **Component layout**: Group frequently-accessed components
* **Entity recycling**: Reuse entities instead of create/destroy

See Also
--------

* :doc:`../api/components` - Complete component reference
* :doc:`rendering` - How rendering integrates with ECS
* `flecs documentation <https://www.flecs.dev/flecs/>`_ - Full ECS capabilities
