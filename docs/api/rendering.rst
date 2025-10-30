Rendering API
=============

The rendering module provides OpenGL-based 2D graphics rendering.

Module: engine.rendering
-------------------------

.. code-block:: cpp

   import engine.rendering;

Classes
-------

Renderer
~~~~~~~~

Main rendering interface.

.. doxygenclass:: engine::rendering::Renderer
   :members:
   :undoc-members:

Shader
~~~~~~

Shader program management.

.. doxygenclass:: engine::rendering::Shader
   :members:
   :undoc-members:

Texture
~~~~~~~

Texture loading and management.

.. doxygenclass:: engine::rendering::Texture
   :members:
   :undoc-members:

Material
~~~~~~~~

Material system for rendering.

.. doxygenclass:: engine::rendering::Material
   :members:
   :undoc-members:

TilemapRenderer
~~~~~~~~~~~~~~~

Tilemap rendering system.

.. doxygenclass:: engine::rendering::TilemapRenderer
   :members:
   :undoc-members:

Mesh
~~~~

Mesh data and rendering.

.. doxygenclass:: engine::rendering::Mesh
   :members:
   :undoc-members:

Types
-----

.. doxygentypedef:: engine::rendering::TextureId

Enums
-----

.. doxygenenum:: engine::rendering::FilterMode
.. doxygenenum:: engine::rendering::WrapMode

See Also
--------

* :doc:`../guides/rendering` - Rendering guide
* :doc:`components` - Sprite and render components
