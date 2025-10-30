Module Overview
===============

Citrus Engine is organized into modules using C++20 modules.

Core Modules
------------

engine.platform
~~~~~~~~~~~~~~~

Platform abstraction for windowing, input, and system functions.

.. code-block:: cpp

   import engine.platform;

**Key Classes:**

* ``Platform`` - Platform initialization and management
* ``Window`` - Window creation and management
* ``Input`` - Keyboard and mouse input

engine.rendering
~~~~~~~~~~~~~~~~

Graphics rendering system using OpenGL.

.. code-block:: cpp

   import engine.rendering;

**Key Classes:**

* ``Renderer`` - Main rendering interface
* ``Shader`` - Shader program management
* ``Texture`` - Texture loading and management
* ``Material`` - Material system
* ``TilemapRenderer`` - Tilemap rendering

engine.scene
~~~~~~~~~~~~

Scene management and entity organization.

.. code-block:: cpp

   import engine.scene;

**Key Classes:**

* ``Scene`` - Scene container and management
* ``Entity`` - Entity handle for ECS

engine.components
~~~~~~~~~~~~~~~~~

Standard ECS components.

.. code-block:: cpp

   import engine.components;

**Key Components:**

* ``Transform`` - Position, rotation, scale
* ``Sprite`` - 2D sprite rendering
* ``Camera`` - Camera component
* ``Tilemap`` - Tilemap data

engine.assets
~~~~~~~~~~~~~

Asset loading and management system.

.. code-block:: cpp

   import engine.assets;

**Key Classes:**

* ``AssetManager`` - Central asset management
* ``Tileset`` - Tileset data for tilemaps

engine.ui
~~~~~~~~~

User interface system using ImGui.

.. code-block:: cpp

   import engine.ui;

**Key Features:**

* ImGui integration
* Batch UI rendering
* Custom widgets

Module Dependencies
-------------------

The modules have the following dependency structure:

.. code-block:: text

   engine.platform (base)
       ↓
   engine.rendering
       ↓
   engine.scene
       ↓
   engine.components
       ↓
   engine.assets
       ↓
   engine.ui

Usage Example
-------------

Importing modules in your code:

.. code-block:: cpp

   // Import specific modules
   import engine.platform;
   import engine.rendering;
   import engine.scene;
   import engine.components;
   
   // Standard library
   #include <iostream>
   
   int main() {
       // Use imported modules
       auto platform = engine::Platform::Create();
       // ...
   }

See Also
--------

* :doc:`rendering` - Rendering API details
* :doc:`components` - Component reference
* :doc:`assets` - Asset management API
