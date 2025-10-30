Your First Project
==================

Let's create a simple game project using Citrus Engine!

Project Structure
-----------------

A typical Citrus Engine project looks like this:

.. code-block:: text

   my-game/
   ├── CMakeLists.txt
   ├── src/
   │   └── main.cpp
   └── assets/
       ├── textures/
       └── data/

Setting Up CMake
----------------

Create a ``CMakeLists.txt`` file:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.28)
   project(my-game LANGUAGES CXX)

   # Find Citrus Engine
   find_package(citrus-engine CONFIG REQUIRED)

   # Create executable
   add_executable(my-game src/main.cpp)
   target_link_libraries(my-game PRIVATE citrus-engine::engine)
   target_compile_features(my-game PRIVATE cxx_std_20)

Creating Your First Game
-------------------------

Create ``src/main.cpp``:

.. code-block:: cpp

   import engine.platform;
   import engine.rendering;
   import engine.scene;
   import engine.components;
   
   #include <iostream>

   int main() {
       // Initialize the platform
       auto platform = engine::Platform::Create();
       if (!platform) {
           std::cerr << "Failed to create platform\n";
           return 1;
       }

       // Create a window
       auto window = platform->CreateWindow(
           "My First Citrus Game",
           800, 600
       );
       
       // Initialize renderer
       auto renderer = engine::Renderer::Create();
       renderer->Initialize();

       // Create a simple scene
       engine::Scene scene;
       
       // Create an entity
       auto entity = scene.CreateEntity("Player");
       entity.AddComponent<engine::Transform>();
       entity.AddComponent<engine::Sprite>("player.png");

       // Game loop
       while (!window->ShouldClose()) {
           platform->PollEvents();
           
           // Update scene
           scene.Update(0.016f);  // ~60 FPS
           
           // Render
           renderer->BeginFrame();
           scene.Render(*renderer);
           renderer->EndFrame();
           
           window->SwapBuffers();
       }

       return 0;
   }

This creates a basic game with:

* A window
* A renderer
* A scene with one entity (the player)
* A simple game loop

Building Your Project
---------------------

.. code-block:: bash

   mkdir build && cd build
   cmake ..
   cmake --build .

Running Your Game
-----------------

.. code-block:: bash

   ./my-game

You should see a window open with the title "My First Citrus Game"!

Next Steps
----------

Now that you have a basic project running, explore:

* :doc:`../guides/ecs` - Learn about the Entity-Component-System
* :doc:`../guides/rendering` - Advanced rendering techniques
* :doc:`../guides/assets` - Loading and managing assets
* :doc:`../guides/ui` - Creating user interfaces

Common Issues
-------------

**Import errors**: Ensure you're using a C++20-compatible compiler with module support.

**Linking errors**: Verify that Citrus Engine is properly installed and 
``CMAKE_PREFIX_PATH`` includes the installation directory.

**Window doesn't appear**: Check console output for error messages about 
OpenGL or platform initialization.
