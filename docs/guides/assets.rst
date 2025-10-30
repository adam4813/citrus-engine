Asset Management
================

Citrus Engine provides a comprehensive asset management system for loading
and managing game resources efficiently.

Overview
--------

The asset manager handles:

* Texture loading (PNG, JPG, WEBP)
* Shader loading and compilation
* Tileset and tilemap data
* Automatic resource caching
* Asynchronous loading
* Reference counting

Asset Manager
-------------

Initialization
~~~~~~~~~~~~~~

.. code-block:: cpp

   import engine.assets;
   
   auto asset_manager = engine::AssetManager::Create();
   asset_manager->SetAssetPath("assets/");

Loading Assets
~~~~~~~~~~~~~~

.. code-block:: cpp

   // Load texture
   auto texture = asset_manager->LoadTexture("textures/player.png");
   
   // Load shader
   auto shader = asset_manager->LoadShader(
       "shaders/sprite.vert",
       "shaders/sprite.frag"
   );
   
   // Load tileset
   auto tileset = asset_manager->LoadTileset("tilemaps/dungeon.json");

Asset IDs
~~~~~~~~~

Assets are identified by string IDs:

.. code-block:: cpp

   // Load with custom ID
   asset_manager->LoadTexture("player_texture", "textures/player.png");
   
   // Retrieve by ID
   auto texture = asset_manager->GetTexture("player_texture");
   
   // Check if loaded
   if (asset_manager->HasTexture("player_texture")) {
       // Use texture
   }

Textures
--------

Loading Textures
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Simple load
   auto texture = engine::Texture::LoadFromFile("assets/sprite.png");
   
   // With asset manager
   auto texture = asset_manager->LoadTexture("sprite", "sprite.png");

Texture Settings
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   texture->SetFilterMode(engine::FilterMode::Nearest);
   texture->SetWrapMode(engine::WrapMode::Clamp);
   texture->GenerateMipmaps();

Texture Properties
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto width = texture->GetWidth();
   auto height = texture->GetHeight();
   auto id = texture->GetId();

Async Loading
~~~~~~~~~~~~~

.. code-block:: cpp

   asset_manager->LoadTextureAsync("large_texture", "background.png",
       [](auto texture) {
           // Texture loaded, safe to use
           std::cout << "Loaded: " << texture->GetWidth() << "x" 
                     << texture->GetHeight() << "\n";
       }
   );

Tilesets and Tilemaps
---------------------

Loading Tilesets
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Load from Tiled JSON format
   auto tileset = asset_manager->LoadTileset("dungeon_tileset.json");
   
   // Access tileset properties
   auto tile_size = tileset->GetTileSize();
   auto tile_count = tileset->GetTileCount();

Working with Tiles
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get tile by ID
   auto tile = tileset->GetTile(42);
   
   // Get tile texture coordinates
   auto uv = tile->GetUVCoords();
   
   // Check tile properties
   if (tile->HasProperty("collision")) {
       // Tile is solid
   }

Loading Tilemaps
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto tilemap = asset_manager->LoadTilemap("level_01.tmx");
   
   // Get tilemap dimensions
   auto width = tilemap->GetWidth();
   auto height = tilemap->GetHeight();
   
   // Access layers
   auto ground_layer = tilemap->GetLayer("Ground");
   auto collision_layer = tilemap->GetLayer("Collision");

Resource Management
-------------------

Reference Counting
~~~~~~~~~~~~~~~~~~

Assets are automatically reference counted:

.. code-block:: cpp

   // Asset loaded (ref count = 1)
   auto texture1 = asset_manager->GetTexture("player");
   
   // Shared reference (ref count = 2)
   auto texture2 = asset_manager->GetTexture("player");
   
   // When both go out of scope, asset is freed

Manual Unloading
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Unload specific asset
   asset_manager->UnloadTexture("unused_texture");
   
   // Unload all unused assets
   asset_manager->UnloadUnused();
   
   // Clear all assets
   asset_manager->Clear();

Hot Reloading
~~~~~~~~~~~~~

Reload assets without restarting:

.. code-block:: cpp

   // Watch for file changes
   asset_manager->EnableHotReload(true);
   
   // Manually reload
   asset_manager->ReloadTexture("player");

Asset Bundles
-------------

Package multiple assets together:

.. code-block:: cpp

   // Create bundle
   engine::AssetBundle bundle;
   bundle.AddTexture("player", "textures/player.png");
   bundle.AddTexture("enemy", "textures/enemy.png");
   bundle.AddShader("sprite", "shaders/sprite.vert", "shaders/sprite.frag");
   
   // Load entire bundle
   asset_manager->LoadBundle(bundle);

Memory Management
-----------------

Memory Limits
~~~~~~~~~~~~~

.. code-block:: cpp

   // Set maximum texture memory (in MB)
   asset_manager->SetTextureMemoryLimit(512);
   
   // Get current usage
   auto used_mb = asset_manager->GetTextureMemoryUsage();

Streaming
~~~~~~~~~

For large assets:

.. code-block:: cpp

   // Stream texture in chunks
   auto streamer = engine::TextureStreamer::Create();
   streamer->Stream("huge_background.png", 
       [](float progress) {
           std::cout << "Loading: " << (progress * 100) << "%\n";
       },
       [](auto texture) {
           std::cout << "Complete!\n";
       }
   );

Asset Compression
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Enable compression for textures
   asset_manager->SetTextureCompression(true);
   
   // Compressed textures use less memory
   // but may have lower quality

Best Practices
--------------

1. **Load early**: Load assets during loading screens
2. **Unload unused**: Free assets when switching levels
3. **Use atlases**: Combine small textures into atlases
4. **Async loading**: Load large assets asynchronously
5. **Reference counting**: Let the manager handle lifetimes
6. **Hot reload**: Enable during development only

Asset Organization
------------------

Recommended directory structure:

.. code-block:: text

   assets/
   ├── textures/
   │   ├── characters/
   │   ├── environment/
   │   └── ui/
   ├── shaders/
   │   ├── sprite.vert
   │   ├── sprite.frag
   │   └── ...
   ├── tilemaps/
   │   ├── tilesets/
   │   └── levels/
   └── data/
       └── ...

See Also
--------

* :doc:`../api/assets` - Complete asset API reference
* :doc:`tilemap` - Detailed tilemap usage
* :doc:`rendering` - Using loaded assets for rendering
