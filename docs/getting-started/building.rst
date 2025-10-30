Building the Engine
===================

This guide covers how to configure and build Citrus Engine for different platforms.

Quick Build
-----------

For the impatient, here's a quick build for Linux:

.. code-block:: bash

   export CC=clang-18 CXX=clang++-18 VCPKG_ROOT=/path/to/vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux
   cmake --build --preset cli-native-debug

Configuration
-------------

Citrus Engine uses CMake presets for configuration. Use the ``cli-native`` preset
for command-line builds:

Linux Native
~~~~~~~~~~~~

.. code-block:: bash

   export CC=clang-18
   export CXX=clang++-18
   export VCPKG_ROOT=/path/to/vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux

Windows Native
~~~~~~~~~~~~~~

.. code-block:: cmd

   set VCPKG_ROOT=C:\path\to\vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows

macOS Native
~~~~~~~~~~~~

.. code-block:: bash

   export VCPKG_ROOT=/path/to/vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-osx

WebAssembly (Emscripten)
~~~~~~~~~~~~~~~~~~~~~~~~~

First, install and activate the Emscripten SDK:

.. code-block:: bash

   cd /opt
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh

Then configure:

.. code-block:: bash

   export VCPKG_ROOT=/path/to/vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten

Building
--------

After configuration, build the project:

Debug Build
~~~~~~~~~~~

.. code-block:: bash

   cmake --build --preset cli-native-debug

For faster builds, use parallel compilation:

.. code-block:: bash

   cmake --build --preset cli-native-debug --parallel 4

Release Build
~~~~~~~~~~~~~

.. code-block:: bash

   cmake --build --preset cli-native-release --parallel 4

Build Targets
-------------

You can build specific targets:

.. code-block:: bash

   cmake --build --preset cli-native-debug --target engine

Available targets:

* ``engine`` - The core engine library
* ``game`` - Example game/demo application

Running Tests
-------------

To build and run tests:

.. code-block:: bash

   # Configure for testing
   cmake --preset cli-native-test -DVCPKG_TARGET_TRIPLET=x64-linux
   
   # Build tests
   cmake --build --preset cli-native-test-debug
   
   # Run all tests
   ctest --preset cli-native-test-debug

Or run tests directly from the test directory:

.. code-block:: bash

   cd build/cli-native-test
   ctest -C Debug --output-on-failure

Troubleshooting
---------------

Build Fails with Module Errors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you get C++20 module-related errors, ensure you're using:

* Clang 18+ on Linux
* MSVC 2022 on Windows
* Latest Emscripten for WebAssembly

**Note:** GCC has incomplete C++20 module support.

vcpkg Errors
~~~~~~~~~~~~

If vcpkg fails to find packages:

1. Verify ``VCPKG_ROOT`` is set correctly
2. Ensure you specified the correct triplet for your platform
3. Try bootstrapping vcpkg again

Missing X11 Libraries (Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you get linker errors about X11:

.. code-block:: bash

   sudo apt-get install -y libx11-dev libxrandr-dev libxinerama-dev \
     libxcursor-dev libxi-dev libgl1-mesa-dev

Next Steps
----------

Now that you have the engine built, continue to :doc:`first-project` to create
your first game!
