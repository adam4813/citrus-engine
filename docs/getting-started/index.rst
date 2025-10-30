Getting Started
===============

Welcome to Citrus Engine! This section will help you get up and running
with the engine quickly.

What is Citrus Engine?
----------------------

Citrus Engine is a modern C++20 game engine that focuses on:

* **Modern C++20**: Leveraging the latest C++ features for clean, safe code
* **ECS Architecture**: Entity-Component-System design using flecs
* **Cross-Platform**: Windows, Linux, macOS, and WebAssembly support
* **Immediate Mode UI**: ImGui integration for debugging and tooling
* **Asset Management**: Efficient resource loading and management
* **2D Rendering**: OpenGL-based rendering with tilemap support

Prerequisites
-------------

Before you begin, you'll need:

* **C++20 Compiler**:
  
  * Linux: Clang 18+ (recommended) or GCC 10+
  * Windows: MSVC 2022 or Clang 18+
  * macOS: Xcode 12+ / Clang 10+

* **CMake 3.28+**: Build system generator
* **vcpkg**: C++ package manager
* **Git**: Version control

Optional tools:

* **Ninja**: Fast build tool (recommended)
* **Doxygen**: API documentation generation
* **Python 3.11+**: Documentation building

Next Steps
----------

1. :doc:`installation` - Install dependencies and set up your development environment
2. :doc:`first-project` - Create your first game with Citrus Engine
3. :doc:`building` - Build and run your project

.. toctree::
   :hidden:
   :maxdepth: 1

   installation
   first-project
   building
