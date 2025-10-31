#pragma once

#include "example_scene.h"
#include <memory>
#include <string>

namespace engine {
    class Engine;
}

namespace examples {

/**
 * Manages the active example scene and provides UI for switching between scenes.
 * 
 * The SceneSwitcher handles:
 * - Activating and deactivating scenes
 * - Rendering the ImGui scene selection menu
 * - Command-line/WASM argument parsing for default scene
 * - Forwarding update/render calls to the active scene
 */
class SceneSwitcher {
public:
    SceneSwitcher();
    ~SceneSwitcher();

    /**
     * Initialize the scene switcher.
     * 
     * @param engine Reference to the engine instance
     * @param default_scene_name Optional name of scene to activate by default
     */
    void Initialize(engine::Engine& engine, const std::string& default_scene_name = "");

    /**
     * Shut down the scene switcher and clean up the active scene.
     * 
     * @param engine Reference to the engine instance
     */
    void Shutdown(engine::Engine& engine);

    /**
     * Update the active scene.
     * 
     * @param engine Reference to the engine instance
     * @param delta_time Time elapsed since last frame in seconds
     */
    void Update(engine::Engine& engine, float delta_time);

    /**
     * Render the active scene.
     * 
     * @param engine Reference to the engine instance
     */
    void Render(engine::Engine& engine);

    /**
     * Render the scene switcher UI and active scene UI.
     * 
     * @param engine Reference to the engine instance
     */
    void RenderUI(engine::Engine& engine);

    /**
     * Switch to a different scene.
     * 
     * @param engine Reference to the engine instance
     * @param scene_name Name of the scene to switch to
     * @return true if the switch was successful, false otherwise
     */
    bool SwitchToScene(engine::Engine& engine, const std::string& scene_name);

    /**
     * Get the name of the currently active scene.
     */
    const std::string& GetActiveSceneName() const;

    /**
     * Check if a scene is currently active.
     */
    bool HasActiveScene() const;

private:
    std::unique_ptr<ExampleScene> active_scene_;
    std::string active_scene_name_;
    bool show_scene_menu_;
};

} // namespace examples
