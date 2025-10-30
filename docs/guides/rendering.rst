Rendering
=========

Citrus Engine provides a modern OpenGL-based 2D rendering system.

Overview
--------

The rendering system handles:

* Sprite rendering with textures
* Mesh rendering for custom geometry
* Batch rendering for performance
* Shader management
* Material system
* Camera and viewports

Basic Rendering
---------------

Creating a Renderer
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.rendering;
   
   auto renderer = engine::Renderer::Create();
   renderer->Initialize();

Loading Textures
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.assets;
   
   auto texture = engine::Texture::LoadFromFile("assets/player.png");
   if (!texture) {
       // Handle error
   }

Rendering Sprites
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Add sprite component to entity
   entity.AddComponent<engine::Sprite>({
       .texture_id = texture->GetId(),
       .size = {64.0f, 64.0f},
       .color = {1.0f, 1.0f, 1.0f, 1.0f}
   });
   
   // Render in game loop
   renderer->BeginFrame();
   scene.Render(*renderer);
   renderer->EndFrame();

Camera System
-------------

Setting Up a Camera
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Create camera entity
   auto camera = scene.CreateEntity("Camera");
   camera.AddComponent<engine::Camera>({
       .type = engine::CameraType::Orthographic,
       .viewport = {0, 0, 800, 600},
       .near_plane = -1.0f,
       .far_plane = 1.0f
   });
   
   camera.AddComponent<engine::Transform>({
       .position = {0.0f, 0.0f, 0.0f}
   });

Moving the Camera
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Follow player
   auto* camera_transform = camera.GetComponent<engine::Transform>();
   auto* player_transform = player.GetComponent<engine::Transform>();
   
   camera_transform->position = player_transform->position;

Shaders
-------

Using Built-in Shaders
~~~~~~~~~~~~~~~~~~~~~~

Citrus Engine includes default shaders for common use cases:

* **sprite**: Standard sprite rendering
* **mesh**: Basic mesh rendering
* **tilemap**: Optimized tilemap rendering

.. code-block:: cpp

   auto shader = renderer->GetShader("sprite");
   shader->Use();

Custom Shaders
~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.rendering;
   
   // Load custom shader
   auto shader = engine::Shader::LoadFromFiles(
       "assets/shaders/my_shader.vert",
       "assets/shaders/my_shader.frag"
   );
   
   // Use shader
   shader->Use();
   shader->SetUniform("u_color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

Material System
---------------

Creating Materials
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.rendering;
   
   engine::Material material;
   material.shader = shader;
   material.textures[0] = diffuse_texture;
   material.color = {1.0f, 1.0f, 1.0f, 1.0f};
   material.properties["u_metallic"] = 0.5f;
   material.properties["u_roughness"] = 0.8f;

Applying Materials
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   entity.AddComponent<engine::Material>(material);

Batch Rendering
---------------

For rendering many sprites efficiently, use batch rendering:

.. code-block:: cpp

   // Batch renderer groups sprites with same texture
   auto batch_renderer = engine::BatchRenderer::Create();
   
   // Add sprites to batch
   for (auto& entity : entities) {
       batch_renderer->AddSprite(entity.GetComponent<engine::Sprite>());
   }
   
   // Render entire batch in one draw call
   batch_renderer->Render();

Render Layers
-------------

Control render order with layers:

.. code-block:: cpp

   entity.AddComponent<engine::RenderLayer>({
       .layer = 10,  // Higher layers render on top
       .order_in_layer = 0
   });

Common layers:

* ``0-9``: Background
* ``10-19``: World objects
* ``20-29``: Characters
* ``30-39``: Effects
* ``40+``: UI

Viewport and Clipping
---------------------

.. code-block:: cpp

   // Set viewport
   renderer->SetViewport(0, 0, 800, 600);
   
   // Enable scissor test for clipping
   renderer->EnableScissorTest(100, 100, 400, 300);

Performance Optimization
------------------------

Instanced Rendering
~~~~~~~~~~~~~~~~~~~

For many identical objects:

.. code-block:: cpp

   std::vector<glm::mat4> transforms;
   for (auto& entity : entities) {
       transforms.push_back(entity.GetTransformMatrix());
   }
   
   renderer->DrawInstanced(mesh, transforms);

Culling
~~~~~~~

Only render visible entities:

.. code-block:: cpp

   auto camera_bounds = camera.GetBounds();
   
   scene.Query<Transform, Sprite>([&](auto entity, auto& t, auto& s) {
       if (camera_bounds.Contains(t.position)) {
           renderer->DrawSprite(s);
       }
   });

Texture Atlases
~~~~~~~~~~~~~~~

Combine multiple textures into one:

.. code-block:: cpp

   auto atlas = engine::TextureAtlas::Create("assets/atlas.png");
   
   // Get region from atlas
   auto sprite_uv = atlas->GetRegion("player_idle_01");
   
   sprite.uv_rect = sprite_uv;

Advanced Topics
---------------

Render Targets
~~~~~~~~~~~~~~

Render to texture for effects:

.. code-block:: cpp

   auto render_target = engine::RenderTarget::Create(800, 600);
   
   // Render to target
   render_target->Bind();
   renderer->Clear();
   scene.Render(*renderer);
   render_target->Unbind();
   
   // Use as texture
   auto texture = render_target->GetTexture();

Post-Processing
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Render scene to texture
   scene_target->Bind();
   scene.Render(*renderer);
   scene_target->Unbind();
   
   // Apply post-processing shader
   post_process_shader->Use();
   post_process_shader->SetTexture("u_scene", scene_target->GetTexture());
   renderer->DrawFullscreenQuad();

Debugging
---------

Debug Rendering
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #ifdef DEBUG
   renderer->DrawDebugLine(start, end, color);
   renderer->DrawDebugCircle(center, radius, color);
   renderer->DrawDebugRect(min, max, color);
   #endif

Performance Metrics
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto stats = renderer->GetStats();
   std::cout << "Draw calls: " << stats.draw_calls << "\n";
   std::cout << "Vertices: " << stats.vertices << "\n";
   std::cout << "FPS: " << stats.fps << "\n";

See Also
--------

* :doc:`../api/rendering` - Complete rendering API reference
* :doc:`tilemap` - Rendering tilemaps efficiently
* :doc:`ui` - ImGui integration for debug rendering
