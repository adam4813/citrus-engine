#pragma once

#include "example_scene.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace examples {

/**
 * Factory function type for creating example scenes.
 */
using SceneFactory = std::function<std::unique_ptr<ExampleScene>()>;

/**
 * Information about a registered example scene.
 */
struct SceneInfo {
    std::string name;
    std::string description;
    SceneFactory factory;
};

/**
 * Registry for all available example scenes.
 * 
 * Scenes are automatically registered at program startup using the
 * REGISTER_EXAMPLE_SCENE() macro. The registry provides a central
 * location to query available scenes and create instances.
 */
class SceneRegistry {
public:
    /**
     * Get the singleton instance of the registry.
     */
    static SceneRegistry& Instance();

    /**
     * Register a new example scene.
     * 
     * @param name Scene name (must be unique)
     * @param description Brief description of what the scene demonstrates
     * @param factory Factory function to create scene instances
     */
    void RegisterScene(const std::string& name, 
                      const std::string& description,
                      SceneFactory factory);

    /**
     * Get information about all registered scenes.
     */
    const std::vector<SceneInfo>& GetAllScenes() const;

    /**
     * Find a scene by name.
     * 
     * @param name Scene name to search for
     * @return Pointer to SceneInfo if found, nullptr otherwise
     */
    const SceneInfo* FindScene(const std::string& name) const;

    /**
     * Create an instance of a scene by name.
     * 
     * @param name Scene name
     * @return Unique pointer to the created scene, or nullptr if not found
     */
    std::unique_ptr<ExampleScene> CreateScene(const std::string& name) const;

private:
    SceneRegistry() = default;
    std::vector<SceneInfo> scenes_;
};

/**
 * Helper class for automatic scene registration.
 * Creates a static instance that registers a scene at program startup.
 */
class SceneRegistrar {
public:
    SceneRegistrar(const std::string& name,
                  const std::string& description,
                  SceneFactory factory) {
        SceneRegistry::Instance().RegisterScene(name, description, factory);
    }
};

/**
 * Macro to register an example scene.
 * 
 * Usage:
 *   REGISTER_EXAMPLE_SCENE(MyScene, "My Example", "Demonstrates feature X")
 * 
 * Where MyScene is a class that inherits from ExampleScene.
 */
#define REGISTER_EXAMPLE_SCENE(SceneClass, SceneName, SceneDescription) \
    static examples::SceneRegistrar g_##SceneClass##_registrar( \
        SceneName, \
        SceneDescription, \
        []() -> std::unique_ptr<examples::ExampleScene> { \
            return std::make_unique<SceneClass>(); \
        } \
    )

} // namespace examples
