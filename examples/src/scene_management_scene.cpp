#include "example_scene.h"
#include "scene_registry.h"

import engine;
import glm;

#include <flecs.h>
#include <imgui.h>
#include <iostream>
#include <string>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif

/**
 * Scene Management Example
 * 
 * Demonstrates the engine's scene management system including:
 * - Creating and managing multiple scenes via SceneManager
 * - Scene transitions and activation
 * - Scene lifecycle (load/unload)
 * - Entity management within scenes
 * - Scene hierarchy with parent-child relationships
 */
class SceneManagementScene : public examples::ExampleScene {
private:
	flecs::entity camera_entity_;

public:
	const char* GetName() const override { return "Scene Management"; }

	const char* GetDescription() const override {
		return "Demonstrates scene creation, transitions, and lifecycle management";
	}

	void Initialize(engine::Engine& engine) override {
		std::cout << "SceneManagementScene: Initialize" << std::endl;

		// Get the engine's scene manager
		auto& scene_manager = engine::scene::GetSceneManager();

		// Create sub-scenes to demonstrate transitions
		scene_a_id_ = scene_manager.CreateScene("Scene A");
		scene_b_id_ = scene_manager.CreateScene("Scene B");
		scene_c_id_ = scene_manager.CreateScene("Scene C");

		// Populate Scene A
		PopulateSceneA(scene_manager.GetScene(scene_a_id_));

		// Populate Scene B
		PopulateSceneB(scene_manager.GetScene(scene_b_id_));

		// Populate Scene C
		PopulateSceneC(scene_manager.GetScene(scene_c_id_));

		// Activate Scene A by default
		scene_manager.SetActiveScene(scene_a_id_);
		current_scene_id_ = scene_a_id_;

		std::cout << "SceneManagementScene: Created " << scene_manager.GetSceneCount() << " scenes" << std::endl;

		auto& ecs = engine.ecs;
		camera_entity_ = ecs.CreateEntity("MainCamera");
		camera_entity_.set<engine::components::Transform>({{0.0f, 0.0f, 5.0f}});
		camera_entity_.set<engine::components::Camera>({.target = {0.0f, 0.0f, 4.0f}});
		ecs.SetActiveCamera(camera_entity_);
	}

	void Shutdown(engine::Engine& engine) override {
		std::cout << "SceneManagementScene: Shutdown" << std::endl;

		camera_entity_.destruct();

		// Clean up all created scenes
		auto& scene_manager = engine::scene::GetSceneManager();

		if (scene_a_id_ != engine::scene::INVALID_SCENE) {
			scene_manager.DestroyScene(scene_a_id_);
		}
		if (scene_b_id_ != engine::scene::INVALID_SCENE) {
			scene_manager.DestroyScene(scene_b_id_);
		}
		if (scene_c_id_ != engine::scene::INVALID_SCENE) {
			scene_manager.DestroyScene(scene_c_id_);
		}

		scene_a_id_ = engine::scene::INVALID_SCENE;
		scene_b_id_ = engine::scene::INVALID_SCENE;
		scene_c_id_ = engine::scene::INVALID_SCENE;
		current_scene_id_ = engine::scene::INVALID_SCENE;
	}

	void Update(engine::Engine& engine, float delta_time) override {
		// Update the scene manager (updates all active scenes)
		auto& scene_manager = engine::scene::GetSceneManager();
		scene_manager.Update(delta_time);
	}

	void Render(engine::Engine& engine) override {
		// Render current scene entities
		auto& scene_manager = engine::scene::GetSceneManager();

		if (current_scene_id_ == engine::scene::INVALID_SCENE) {
			return;
		}

		auto* scene = scene_manager.TryGetScene(current_scene_id_);
		if (!scene) {
			return;
		}

		// Get all entities in the current scene and render debug information
		auto entities = scene->GetAllEntities();

		// For this example, we just demonstrate the scene management
		// In a real game, rendering would be handled by the rendering system
	}

	void RenderUI(engine::Engine& engine) override {
		auto& scene_manager = engine::scene::GetSceneManager();

		ImGui::Begin("Scene Management Example");

		ImGui::Text("This example demonstrates the engine's scene management system.");
		ImGui::Separator();

		// Scene information
		ImGui::Text("Total Scenes: %zu", scene_manager.GetSceneCount());
		ImGui::Text("Active Scenes: %zu", scene_manager.GetActiveSceneCount());
		ImGui::Text("Total Entities: %zu", scene_manager.GetTotalEntityCount());

		ImGui::Separator();

		// Current scene info
		if (current_scene_id_ != engine::scene::INVALID_SCENE) {
			auto* scene = scene_manager.TryGetScene(current_scene_id_);
			if (scene) {
				ImGui::Text("Current Scene: %s", scene->GetName().c_str());
				ImGui::Text("Active: %s", scene->IsActive() ? "Yes" : "No");
				ImGui::Text("Loaded: %s", scene->IsLoaded() ? "Yes" : "No");

				auto entities = scene->GetAllEntities();
				ImGui::Text("Entities in Scene: %zu", entities.size());
			}
		}

		ImGui::Separator();

		// Scene transition controls
		ImGui::Text("Scene Transitions:");

		if (ImGui::Button("Switch to Scene A")) {
			SwitchToScene(scene_manager, scene_a_id_, "Scene A");
		}
		ImGui::SameLine();
		if (ImGui::Button("Switch to Scene B")) {
			SwitchToScene(scene_manager, scene_b_id_, "Scene B");
		}
		ImGui::SameLine();
		if (ImGui::Button("Switch to Scene C")) {
			SwitchToScene(scene_manager, scene_c_id_, "Scene C");
		}

		ImGui::Separator();

		// Scene lifecycle controls
		ImGui::Text("Scene Lifecycle:");

		if (current_scene_id_ != engine::scene::INVALID_SCENE) {
			auto* scene = scene_manager.TryGetScene(current_scene_id_);
			if (scene) {
				bool is_loaded = scene->IsLoaded();
				if (ImGui::Button(is_loaded ? "Unload Current Scene" : "Load Current Scene")) {
					if (is_loaded) {
						scene_manager.UnloadScene(current_scene_id_);
						std::cout << "Unloaded scene: " << scene->GetName() << std::endl;
					}
					else {
						scene_manager.LoadScene(current_scene_id_);
						std::cout << "Loaded scene: " << scene->GetName() << std::endl;
					}
				}
			}
		}

		ImGui::Separator();

		// Entity information for current scene
		if (current_scene_id_ != engine::scene::INVALID_SCENE) {
			auto* scene = scene_manager.TryGetScene(current_scene_id_);
			if (scene) {
				ImGui::Text("Scene Entities:");
				auto entities = scene->GetAllEntities();

				if (ImGui::BeginChild("EntityList", ImVec2(0, 150), true)) {
					for (const auto& entity : entities) {
						if (entity.is_valid()) {
							const char* name = entity.name();
							ImGui::BulletText(
									"%s (ID: %llu)",
									name ? name : "<unnamed>",
									static_cast<unsigned long long>(entity.id()));
						}
					}
				}
				ImGui::EndChild();
			}
		}

		ImGui::Separator();

		// Instructions
		ImGui::TextWrapped(
				"This example demonstrates:\n"
				"- Creating multiple scenes using SceneManager\n"
				"- Switching between scenes\n"
				"- Scene activation and deactivation\n"
				"- Entity management within scenes\n"
				"- Scene lifecycle (load/unload)\n"
				"\n"
				"Use the buttons above to switch between scenes and observe "
				"how the scene manager handles transitions.");

		ImGui::End();
	}

private:
	void PopulateSceneA(engine::scene::Scene& scene) {
		// Create some example entities in Scene A
		auto entity1 = scene.CreateEntity("Scene_A_Entity_1");
		auto entity2 = scene.CreateEntity("Scene_A_Entity_2");
		auto entity3 = scene.CreateEntity("Scene_A_Entity_3");

		// Create a parent-child hierarchy
		auto parent = scene.CreateEntity("Scene_A_Parent");
		auto child1 = scene.CreateEntity("Scene_A_Child_1", parent);
		auto child2 = scene.CreateEntity("Scene_A_Child_2", parent);

		scene.SetLoaded(true);
	}

	void PopulateSceneB(engine::scene::Scene& scene) {
		// Create different entities in Scene B
		auto entity1 = scene.CreateEntity("Scene_B_Entity_1");
		auto entity2 = scene.CreateEntity("Scene_B_Entity_2");

		// Create a more complex hierarchy
		auto root = scene.CreateEntity("Scene_B_Root");
		auto branch1 = scene.CreateEntity("Scene_B_Branch_1", root);
		auto branch2 = scene.CreateEntity("Scene_B_Branch_2", root);
		auto leaf1 = scene.CreateEntity("Scene_B_Leaf_1", branch1);
		auto leaf2 = scene.CreateEntity("Scene_B_Leaf_2", branch1);

		scene.SetLoaded(true);
	}

	void PopulateSceneC(engine::scene::Scene& scene) {
		// Create entities in Scene C
		auto entity1 = scene.CreateEntity("Scene_C_Entity_1");
		auto entity2 = scene.CreateEntity("Scene_C_Entity_2");
		auto entity3 = scene.CreateEntity("Scene_C_Entity_3");
		auto entity4 = scene.CreateEntity("Scene_C_Entity_4");

		scene.SetLoaded(true);
	}

	void
	SwitchToScene(engine::scene::SceneManager& scene_manager, engine::scene::SceneId scene_id, const char* scene_name) {
		if (scene_id == engine::scene::INVALID_SCENE) {
			std::cerr << "Cannot switch to invalid scene" << std::endl;
			return;
		}

		std::cout << "Switching to " << scene_name << std::endl;
		scene_manager.SetActiveScene(scene_id);
		current_scene_id_ = scene_id;
	}

	// Scene IDs for the sub-scenes we manage
	engine::scene::SceneId scene_a_id_ = engine::scene::INVALID_SCENE;
	engine::scene::SceneId scene_b_id_ = engine::scene::INVALID_SCENE;
	engine::scene::SceneId scene_c_id_ = engine::scene::INVALID_SCENE;
	engine::scene::SceneId current_scene_id_ = engine::scene::INVALID_SCENE;
};

// Register the scene
REGISTER_EXAMPLE_SCENE(
		SceneManagementScene, "Scene Management", "Demonstrates scene creation, transitions, and lifecycle management");
