Installation
============

This guide will walk you through installing Citrus Engine and its dependencies.

System Requirements
-------------------

**Minimum:**

* 4 GB RAM
* 2 GB disk space
* Graphics card with OpenGL 3.3+ support

**Recommended:**

* 8 GB RAM
* 5 GB disk space (includes build cache)
* Graphics card with OpenGL 4.5+ support

Installing Dependencies
-----------------------

Linux
~~~~~

Install system dependencies:

.. code-block:: bash

   sudo apt-get update && sudo apt-get install -y \
     build-essential \
     cmake \
     ninja-build \
     pkg-config \
     clang-18 \
     libx11-dev \
     libxrandr-dev \
     libxinerama-dev \
     libxcursor-dev \
     libxi-dev \
     libgl1-mesa-dev \
     libglu1-mesa-dev

Set up Clang as the default compiler:

.. code-block:: bash

   export CC=clang-18
   export CXX=clang++-18

Windows
~~~~~~~

1. Install `Visual Studio 2022 <https://visualstudio.microsoft.com/>`_ with C++ tools
2. Or install `Clang 18+ <https://releases.llvm.org/download.html>`_
3. Install `CMake <https://cmake.org/download/>`_ (3.28 or later)
4. Install `Git <https://git-scm.com/download/win>`_

macOS
~~~~~

Install Xcode Command Line Tools and Homebrew:

.. code-block:: bash

   xcode-select --install
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   brew install cmake ninja llvm

Installing vcpkg
----------------

vcpkg is required for managing C++ dependencies. Clone it in the parent directory
of your project:

.. code-block:: bash

   cd /path/to/parent/directory
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # Linux/macOS
   # or
   bootstrap-vcpkg.bat   # Windows

Set the environment variable:

.. code-block:: bash

   export VCPKG_ROOT=/path/to/vcpkg  # Linux/macOS
   # or
   set VCPKG_ROOT=C:\path\to\vcpkg   # Windows

Getting Citrus Engine
---------------------

Clone the repository:

.. code-block:: bash

   git clone https://github.com/adam4813/citrus-engine.git
   cd citrus-engine

Verifying Installation
----------------------

Verify all tools are installed:

.. code-block:: bash

   cmake --version    # Should be 3.28+
   clang --version    # Should be 10+
   ninja --version    # Optional but recommended

Next Steps
----------

Continue to :doc:`building` to configure and build the engine.
