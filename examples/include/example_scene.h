#pragma once

#include <memory>
#include <string>

import engine;

namespace examples {

/**
 * Base interface for all example scenes.
 * 
 * Each example scene demonstrates a specific feature or set of features
 * of the Citrus Engine. Scenes are registered in the SceneRegistry and
 * can be switched between using the ImGui scene switcher.
 * 
 * To create a new example scene:
 * 1. Inherit from ExampleScene
 * 2. Implement all virtual methods
 * 3. Register in scene_registry.cpp using REGISTER_EXAMPLE_SCENE()
 * 4. Add a brief description for the scene switcher menu
 */
class ExampleScene {
public:
	virtual ~ExampleScene() = default;

	/**
     * Get the display name of this example scene.
     * This name appears in the scene switcher menu.
     */
	virtual const char* GetName() const = 0;

	/**
     * Get a brief description of what this example demonstrates.
     * Displayed in the scene switcher UI.
     */
	virtual const char* GetDescription() const = 0;

	/**
     * Initialize the scene.
     * Called once when the scene is first activated.
     * 
     * @param engine Reference to the engine instance
     */
	virtual void Initialize(engine::Engine& engine) = 0;

	/**
     * Clean up the scene.
     * Called when switching away from this scene.
     * 
     * @param engine Reference to the engine instance
     */
	virtual void Shutdown(engine::Engine& engine) = 0;

	/**
     * Update the scene logic.
     * Called every frame while this scene is active.
     * 
     * @param engine Reference to the engine instance
     * @param delta_time Time elapsed since last frame in seconds
     */
	virtual void Update(engine::Engine& engine, float delta_time) = 0;

	/**
     * Render the scene.
     * Called every frame while this scene is active.
     * 
     * @param engine Reference to the engine instance
     */
	virtual void Render(engine::Engine& engine) = 0;

	/**
     * Render scene-specific ImGui UI.
     * Called every frame after the main scene switcher UI.
     * Use this to display debug info, controls, or scene-specific UI.
     * 
     * @param engine Reference to the engine instance
     */
	virtual void RenderUI(engine::Engine& engine) = 0;
};

} // namespace examples
