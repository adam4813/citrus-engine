Extending the Engine
====================

How to extend Citrus Engine with custom functionality.

Creating Custom Components
--------------------------

Define New Components
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // In your game code
   struct Health {
       float current;
       float maximum;
       
       bool IsDead() const { return current <= 0.0f; }
   };
   
   struct Velocity {
       glm::vec2 velocity;
       float max_speed;
   };
   
   // Register with ECS
   entity.AddComponent<Health>({100.0f, 100.0f});
   entity.AddComponent<Velocity>({{0.0f, 0.0f}, 10.0f});

Component Groups
~~~~~~~~~~~~~~~~

Group related components:

.. code-block:: cpp

   struct PlayerTag {};
   struct EnemyTag {};
   
   // Tag entities
   player.AddComponent<PlayerTag>();
   enemy.AddComponent<EnemyTag>();
   
   // Query by tag
   scene.Query<PlayerTag, Transform>([](auto e, auto& t) {
       // Player-specific logic
   });

Creating Custom Systems
-----------------------

System Base Class
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class System {
   public:
       virtual ~System() = default;
       virtual void Update(Scene& scene, float delta_time) = 0;
   };

Movement System
~~~~~~~~~~~~~~~

.. code-block:: cpp

   class MovementSystem : public System {
   public:
       void Update(Scene& scene, float delta_time) override {
           scene.Query<Transform, Velocity>([dt = delta_time]
               (auto entity, auto& transform, auto& velocity) {
               
               transform.position.x += velocity.velocity.x * dt;
               transform.position.y += velocity.velocity.y * dt;
           });
       }
   };

Health System
~~~~~~~~~~~~~

.. code-block:: cpp

   class HealthSystem : public System {
   public:
       void Update(Scene& scene, float delta_time) override {
           scene.Query<Health>([&scene](auto entity, auto& health) {
               if (health.IsDead()) {
                   scene.DestroyEntity(entity);
               }
           });
       }
   };

System Manager
~~~~~~~~~~~~~~

.. code-block:: cpp

   class SystemManager {
       std::vector<std::unique_ptr<System>> systems_;
       
   public:
       template<typename T, typename... Args>
       T* AddSystem(Args&&... args) {
           auto system = std::make_unique<T>(std::forward<Args>(args)...);
           auto* ptr = system.get();
           systems_.push_back(std::move(system));
           return ptr;
       }
       
       void Update(Scene& scene, float delta_time) {
           for (auto& system : systems_) {
               system->Update(scene, delta_time);
           }
       }
   };

Custom Rendering
----------------

Custom Shaders
~~~~~~~~~~~~~~

Create custom shader files:

.. code-block:: glsl

   // custom.vert
   #version 330 core
   layout(location = 0) in vec3 a_position;
   layout(location = 1) in vec2 a_texcoord;
   
   out vec2 v_texcoord;
   
   uniform mat4 u_mvp;
   
   void main() {
       gl_Position = u_mvp * vec4(a_position, 1.0);
       v_texcoord = a_texcoord;
   }

.. code-block:: glsl

   // custom.frag
   #version 330 core
   in vec2 v_texcoord;
   out vec4 frag_color;
   
   uniform sampler2D u_texture;
   uniform float u_time;
   
   void main() {
       vec4 color = texture(u_texture, v_texcoord);
       // Custom effect
       color.rgb *= 0.5 + 0.5 * sin(u_time);
       frag_color = color;
   }

Load and use:

.. code-block:: cpp

   auto shader = Shader::LoadFromFiles(
       "assets/shaders/custom.vert",
       "assets/shaders/custom.frag"
   );
   
   shader->Use();
   shader->SetUniform("u_time", current_time);

Custom Renderer
~~~~~~~~~~~~~~~

.. code-block:: cpp

   class CustomRenderer {
       Shader shader_;
       
   public:
       void Initialize() {
           shader_ = Shader::LoadFromFiles("custom.vert", "custom.frag");
       }
       
       void Render(const Mesh& mesh, const Transform& transform) {
           shader_.Use();
           shader_.SetUniform("u_mvp", CalculateMVP(transform));
           mesh.Draw();
       }
   };

Asset Loaders
-------------

Custom Asset Type
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct LevelData {
       std::vector<EntityData> entities;
       std::string background_music;
       std::unordered_map<std::string, std::string> properties;
   };
   
   class LevelLoader {
   public:
       static std::unique_ptr<LevelData> LoadFromFile(
           const std::string& path) {
           
           auto level = std::make_unique<LevelData>();
           
           // Parse JSON/XML/custom format
           auto json = LoadJSON(path);
           level->background_music = json["music"];
           
           for (auto& entity_json : json["entities"]) {
               level->entities.push_back(ParseEntity(entity_json));
           }
           
           return level;
       }
   };

Asset Manager Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename T>
   class AssetCache {
       std::unordered_map<std::string, std::shared_ptr<T>> cache_;
       
   public:
       std::shared_ptr<T> Load(const std::string& id,
                               const std::string& path) {
           
           // Check cache
           auto it = cache_.find(id);
           if (it != cache_.end()) {
               return it->second;
           }
           
           // Load asset
           auto asset = T::LoadFromFile(path);
           cache_[id] = asset;
           return asset;
       }
   };

Scripting Integration
---------------------

Lua Scripting
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <lua.hpp>
   
   class ScriptSystem {
       lua_State* L_;
       
   public:
       void Initialize() {
           L_ = luaL_newstate();
           luaL_openlibs(L_);
           
           // Register engine functions
           RegisterEngineFunctions();
       }
       
       void RunScript(const std::string& script) {
           luaL_dofile(L_, script.c_str());
       }
       
       void CallFunction(const std::string& name) {
           lua_getglobal(L_, name.c_str());
           lua_call(L_, 0, 0);
       }
   };

Expose Components
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void RegisterEngineFunctions() {
       // Expose transform
       lua_register(L_, "SetPosition", [](lua_State* L) -> int {
           int entity_id = lua_tointeger(L, 1);
           float x = lua_tonumber(L, 2);
           float y = lua_tonumber(L, 3);
           
           auto entity = GetEntity(entity_id);
           auto* transform = entity.GetComponent<Transform>();
           transform->position = {x, y, 0.0f};
           
           return 0;
       });
   }

Custom Input Handling
---------------------

Input Actions
~~~~~~~~~~~~~

.. code-block:: cpp

   class InputAction {
   public:
       virtual bool IsActive(const Input& input) const = 0;
   };
   
   class KeyAction : public InputAction {
       Key key_;
   public:
       KeyAction(Key key) : key_(key) {}
       
       bool IsActive(const Input& input) const override {
           return input.IsKeyPressed(key_);
       }
   };
   
   class InputActionMap {
       std::unordered_map<std::string, std::unique_ptr<InputAction>> actions_;
       
   public:
       void Register(const std::string& name, 
                    std::unique_ptr<InputAction> action) {
           actions_[name] = std::move(action);
       }
       
       bool IsActionActive(const std::string& name, 
                          const Input& input) const {
           auto it = actions_.find(name);
           if (it != actions_.end()) {
               return it->second->IsActive(input);
           }
           return false;
       }
   };

Usage:

.. code-block:: cpp

   InputActionMap input_map;
   input_map.Register("jump", std::make_unique<KeyAction>(Key::Space));
   input_map.Register("shoot", std::make_unique<KeyAction>(Key::X));
   
   // In game loop
   if (input_map.IsActionActive("jump", input)) {
       player.Jump();
   }

Custom UI Widgets
-----------------

ImGui Extension
~~~~~~~~~~~~~~~

.. code-block:: cpp

   namespace ImGui {
       bool ColoredButton(const char* label, const ImVec4& color) {
           ImGui::PushStyleColor(ImGuiCol_Button, color);
           bool result = ImGui::Button(label);
           ImGui::PopStyleColor();
           return result;
       }
       
       void HealthBar(float current, float maximum) {
           char buf[32];
           snprintf(buf, sizeof(buf), "%.0f/%.0f", current, maximum);
           
           float fraction = current / maximum;
           ImVec4 color = fraction > 0.5f 
               ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f)  // Green
               : ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
           
           ImGui::ProgressBar(fraction, ImVec2(-1, 0), buf);
       }
   }

Plugin System
-------------

Plugin Interface
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class IPlugin {
   public:
       virtual ~IPlugin() = default;
       virtual void Initialize() = 0;
       virtual void Shutdown() = 0;
       virtual const char* GetName() const = 0;
   };

Plugin Manager
~~~~~~~~~~~~~~

.. code-block:: cpp

   class PluginManager {
       std::vector<std::unique_ptr<IPlugin>> plugins_;
       
   public:
       void LoadPlugin(std::unique_ptr<IPlugin> plugin) {
           plugin->Initialize();
           plugins_.push_back(std::move(plugin));
       }
       
       void UnloadAll() {
           for (auto& plugin : plugins_) {
               plugin->Shutdown();
           }
           plugins_.clear();
       }
   };

Example Plugin
~~~~~~~~~~~~~~

.. code-block:: cpp

   class ParticleSystemPlugin : public IPlugin {
   public:
       void Initialize() override {
           // Register particle system
       }
       
       void Shutdown() override {
           // Cleanup
       }
       
       const char* GetName() const override {
           return "ParticleSystem";
       }
   };

Best Practices
--------------

1. **Follow engine patterns**: Match existing code style
2. **Use RAII**: Manage resources automatically
3. **Avoid globals**: Use dependency injection
4. **Document APIs**: Add comments for public functions
5. **Write tests**: Ensure extensions work correctly

See Also
--------

* :doc:`../guides/ecs` - ECS architecture
* :doc:`../api/index` - Engine API reference
* :doc:`optimization` - Performance considerations
