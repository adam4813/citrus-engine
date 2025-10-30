Platform-Specific Features
==========================

Platform-specific considerations and features for Citrus Engine.

Supported Platforms
-------------------

* **Windows** (x64, MSVC 2022 / Clang 18+)
* **Linux** (x64, Clang 18+ / GCC 10+)
* **macOS** (x64/ARM, Xcode 12+ / Clang 10+)
* **WebAssembly** (Emscripten 3.1.40+)

Windows
-------

Compiler Support
~~~~~~~~~~~~~~~~

**MSVC 2022** (Recommended):

.. code-block:: bat

   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
   cmake --build --preset cli-native-debug

**Clang for Windows**:

.. code-block:: bat

   set CC=clang
   set CXX=clang++
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-windows
   cmake --build --preset cli-native-debug

Platform APIs
~~~~~~~~~~~~~

.. code-block:: cpp

   #ifdef _WIN32
   import engine.platform;
   
   // Windows-specific features
   auto window = platform->CreateWindow("Game", 800, 600);
   window->SetIcon("assets/icon.ico");
   #endif

File Paths
~~~~~~~~~~

Use forward slashes in asset paths (converted automatically):

.. code-block:: cpp

   // Works on all platforms
   auto texture = LoadTexture("assets/textures/player.png");

Linux
-----

Compiler Support
~~~~~~~~~~~~~~~~

**Clang 18+** (Recommended for C++20 modules):

.. code-block:: bash

   export CC=clang-18
   export CXX=clang++-18
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux
   cmake --build --preset cli-native-debug

**GCC 10+** (Limited module support):

.. code-block:: bash

   export CC=gcc-10
   export CXX=g++-10
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-linux
   cmake --build --preset cli-native-debug

System Dependencies
~~~~~~~~~~~~~~~~~~~

Install required packages:

.. code-block:: bash

   sudo apt-get install -y \
     libx11-dev \
     libxrandr-dev \
     libxinerama-dev \
     libxcursor-dev \
     libxi-dev \
     libgl1-mesa-dev

Graphics Drivers
~~~~~~~~~~~~~~~~

Ensure up-to-date OpenGL drivers:

.. code-block:: bash

   # Check OpenGL version
   glxinfo | grep "OpenGL version"
   
   # Should be 3.3 or higher

macOS
-----

Compiler Support
~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Install Xcode Command Line Tools
   xcode-select --install
   
   # Or use Homebrew Clang
   brew install llvm
   export CC=/opt/homebrew/opt/llvm/bin/clang
   export CXX=/opt/homebrew/opt/llvm/bin/clang++

Building
~~~~~~~~

.. code-block:: bash

   export VCPKG_ROOT=/path/to/vcpkg
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=x64-osx
   cmake --build --preset cli-native-debug

Retina Displays
~~~~~~~~~~~~~~~

Handle high-DPI displays:

.. code-block:: cpp

   auto window = platform->CreateWindow("Game", 800, 600);
   
   // Get actual framebuffer size
   auto fb_width = window->GetFramebufferWidth();
   auto fb_height = window->GetFramebufferHeight();
   
   // Use for viewport and rendering
   renderer->SetViewport(0, 0, fb_width, fb_height);

WebAssembly
-----------

Setup
~~~~~

Install Emscripten SDK:

.. code-block:: bash

   cd /opt
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh

Building for Web
~~~~~~~~~~~~~~~~

.. code-block:: bash

   export VCPKG_ROOT=/path/to/vcpkg
   source /opt/emsdk/emsdk_env.sh
   
   cmake --preset cli-native -DVCPKG_TARGET_TRIPLET=wasm32-emscripten
   cmake --build --preset cli-native-debug

Generating HTML
~~~~~~~~~~~~~~~

.. code-block:: bash

   # Build generates .wasm and .js files
   # Create HTML wrapper:
   emcc game.wasm -o game.html \
     -s USE_GLFW=3 \
     -s USE_WEBGL2=1 \
     -s FULL_ES3=1

Running Locally
~~~~~~~~~~~~~~~

.. code-block:: bash

   # Start local web server
   python3 -m http.server 8000
   
   # Open browser to http://localhost:8000/game.html

Memory Management
~~~~~~~~~~~~~~~~~

WebAssembly has limited memory:

.. code-block:: cpp

   // Set initial memory (default 16MB)
   // Add to emcc flags:
   // -s INITIAL_MEMORY=64MB
   
   // Enable memory growth
   // -s ALLOW_MEMORY_GROWTH=1

Asset Loading
~~~~~~~~~~~~~

Preload assets or use async loading:

.. code-block:: cpp

   // Preload (emcc flag):
   // --preload-file assets@/assets
   
   // Or use async loading
   asset_manager->LoadTextureAsync("sprite.png", [](auto texture) {
       // Texture loaded
   });

Performance Tips
~~~~~~~~~~~~~~~~

* Use ``-O3`` for production builds
* Enable SIMD: ``-msimd128``
* Use WebGL 2.0 (OpenGL ES 3.0)
* Minimize file sizes (use texture compression)

Cross-Platform Code
-------------------

Conditional Compilation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #if defined(_WIN32)
       // Windows-specific
   #elif defined(__linux__)
       // Linux-specific
   #elif defined(__APPLE__)
       // macOS-specific
   #elif defined(__EMSCRIPTEN__)
       // WebAssembly-specific
   #endif

Platform Detection
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.platform;
   
   auto platform_type = engine::Platform::GetType();
   
   switch (platform_type) {
       case PlatformType::Windows:
           // Windows
           break;
       case PlatformType::Linux:
           // Linux
           break;
       case PlatformType::macOS:
           // macOS
           break;
       case PlatformType::Web:
           // WebAssembly
           break;
   }

File System Abstraction
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Use platform-independent paths
   import engine.filesystem;
   
   auto path = fs::path("assets") / "textures" / "player.png";
   auto texture = LoadTexture(path.string());

Input Handling
--------------

Keyboard
~~~~~~~~

.. code-block:: cpp

   // Cross-platform key codes
   if (input->IsKeyPressed(Key::Space)) {
       // Jump
   }

Mouse
~~~~~

.. code-block:: cpp

   auto mouse_pos = input->GetMousePosition();
   if (input->IsMouseButtonPressed(MouseButton::Left)) {
       // Handle click
   }

Touch (Mobile/Web)
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || defined(__APPLE__)
   auto touches = input->GetTouches();
   for (auto& touch : touches) {
       // Handle touch
   }
   #endif

Graphics API Abstraction
------------------------

OpenGL Versions
~~~~~~~~~~~~~~~

* **Desktop**: OpenGL 3.3+
* **WebAssembly**: OpenGL ES 3.0 (WebGL 2.0)

.. code-block:: cpp

   // Engine handles differences automatically
   renderer->Initialize();  // Uses appropriate GL version

Shader Compatibility
~~~~~~~~~~~~~~~~~~~~

Use ``#version`` directives:

.. code-block:: glsl

   // Desktop
   #version 330 core
   
   // Web
   #version 300 es
   precision mediump float;

Audio
-----

Platform Support
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.audio;
   
   // Cross-platform audio
   auto audio = engine::Audio::Create();
   audio->PlaySound("assets/sounds/jump.wav");

Debugging
---------

Platform-Specific Debuggers
~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **Windows**: Visual Studio Debugger, WinDbg
* **Linux**: GDB, LLDB
* **macOS**: Xcode Debugger, LLDB
* **Web**: Browser DevTools

Graphics Debugging
~~~~~~~~~~~~~~~~~~

* **RenderDoc** (Windows, Linux)
* **Xcode Instruments** (macOS)
* **Chrome DevTools** (WebAssembly)

See Also
--------

* :doc:`optimization` - Performance optimization tips
* :doc:`extending` - Extending engine functionality
* :doc:`../getting-started/building` - Build instructions
