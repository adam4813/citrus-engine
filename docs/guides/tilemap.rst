Tilemap System
==============

The tilemap system provides efficient 2D tile-based rendering capabilities for
Citrus Engine with support for multiple layers, sparse storage, and batch rendering.

Overview
--------

The tilemap system is designed with these principles:

* **Decoupled Architecture**: Renderer separate from tilemap data
* **Multi-layer Support**: Multiple layers with different tilesets
* **Sparse Storage**: Only stores tiles that exist, saving memory
* **Batch Rendering**: Efficient GPU usage through batched draw calls
* **Flexible Layering**: Multiple tiles can exist in the same grid cell

For detailed implementation documentation, see the original tilemap-system.md file.

See Also
--------

* :doc:`assets` - Loading tilesets and tilemap data
* :doc:`rendering` - General rendering concepts
