#include <gtest/gtest.h>

#include <filesystem>
#include <flecs.h>
#include <fstream>

import engine;
import glm;
import engine.ecs;
import engine.scene;
import engine.components;
import engine.rendering;
import engine.platform;

using namespace engine;
using namespace engine::ecs;
using namespace engine::scene;
using namespace engine::components;
using namespace engine::rendering;

class SceneSerializationTest : public ::testing::Test {
protected:
	std::unique_ptr<ECSWorld> ecs_world_;
	std::unique_ptr<SceneManager> scene_manager_;
	std::filesystem::path temp_file_;

	void SetUp() override {
		ecs_world_ = std::make_unique<ECSWorld>();
		scene_manager_ = std::make_unique<SceneManager>(*ecs_world_);

		// Create temp file path
		temp_file_ = std::filesystem::temp_directory_path() / "test_scene.json";
	}

	void TearDown() override {
		// Clean up temp file
		if (std::filesystem::exists(temp_file_)) {
			std::filesystem::remove(temp_file_);
		}
		scene_manager_.reset();
		ecs_world_.reset();
	}
};

TEST_F(SceneSerializationTest, SaveAndLoad_EmptyScene_Succeeds) {
	// Create an empty scene
	const SceneId scene_id = scene_manager_->CreateScene("EmptyScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	// Save the scene
	const platform::fs::Path path(temp_file_.string());
	const bool save_result = scene_manager_->SaveScene(scene_id, path);
	EXPECT_TRUE(save_result);

	// Verify file exists
	EXPECT_TRUE(std::filesystem::exists(temp_file_));

	// Load the scene
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	EXPECT_NE(loaded_id, INVALID_SCENE);

	// Verify scene name
	const auto* loaded_scene = scene_manager_->TryGetScene(loaded_id);
	ASSERT_NE(loaded_scene, nullptr);
	EXPECT_EQ(loaded_scene->GetName(), "EmptyScene");
}

TEST_F(SceneSerializationTest, SaveAndLoad_EntityWithTransform_PreservesValues) {
	// Create scene with an entity
	const SceneId scene_id = scene_manager_->CreateScene("TransformTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	const auto& scene = scene_manager_->GetScene(scene_id);
	const auto entity = scene.CreateEntity("TestEntity");

	// Set transform values
	auto& transform = entity.get_mut<Transform>();
	transform.position = glm::vec3(10.0f, 20.0f, 30.0f);
	transform.rotation = glm::vec3(0.1f, 0.2f, 0.3f);
	transform.scale = glm::vec3(2.0f, 3.0f, 4.0f);

	// Save the scene
	const platform::fs::Path path(temp_file_.string());
	const bool save_result = scene_manager_->SaveScene(scene_id, path);
	EXPECT_TRUE(save_result);

	// Destroy original scene
	scene_manager_->DestroyScene(scene_id);

	// Load the scene
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	// Find the entity and verify transform
	const auto& loaded_scene = scene_manager_->GetScene(loaded_id);
	const auto loaded_entity = loaded_scene.FindEntityByName("TestEntity");
	ASSERT_TRUE(loaded_entity.is_valid());
	ASSERT_TRUE(loaded_entity.has<Transform>());

	const auto& loaded_transform = loaded_entity.get<Transform>();

	EXPECT_FLOAT_EQ(loaded_transform.position.x, 10.0f);
	EXPECT_FLOAT_EQ(loaded_transform.position.y, 20.0f);
	EXPECT_FLOAT_EQ(loaded_transform.position.z, 30.0f);

	EXPECT_FLOAT_EQ(loaded_transform.rotation.x, 0.1f);
	EXPECT_FLOAT_EQ(loaded_transform.rotation.y, 0.2f);
	EXPECT_FLOAT_EQ(loaded_transform.rotation.z, 0.3f);

	EXPECT_FLOAT_EQ(loaded_transform.scale.x, 2.0f);
	EXPECT_FLOAT_EQ(loaded_transform.scale.y, 3.0f);
	EXPECT_FLOAT_EQ(loaded_transform.scale.z, 4.0f);
}

TEST_F(SceneSerializationTest, SaveAndLoad_EntityWithSprite_PreservesAllFields) {
	// Create scene with a sprite entity
	SceneId scene_id = scene_manager_->CreateScene("SpriteTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	auto& scene = scene_manager_->GetScene(scene_id);
	auto entity = scene.CreateEntity("SpriteEntity");

	// Add and configure sprite
	entity.set<Sprite>(
			{.texture = 42,
			 .position = glm::vec2(100.0f, 200.0f),
			 .size = glm::vec2(64.0f, 64.0f),
			 .rotation = 1.5f,
			 .color = glm::vec4(1.0f, 0.5f, 0.25f, 0.8f),
			 .texture_offset = glm::vec2(0.1f, 0.2f),
			 .texture_scale = glm::vec2(2.0f, 2.0f),
			 .layer = 10,
			 .pivot = glm::vec2(0.5f, 0.5f),
			 .flip_x = true,
			 .flip_y = false});

	// Save
	platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	// Destroy and reload
	scene_manager_->DestroyScene(scene_id);
	SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	// Verify sprite
	auto& loaded_scene = scene_manager_->GetScene(loaded_id);
	auto loaded_entity = loaded_scene.FindEntityByName("SpriteEntity");
	ASSERT_TRUE(loaded_entity.is_valid());
	ASSERT_TRUE(loaded_entity.has<Sprite>());

	const auto& sprite = loaded_entity.get<Sprite>();

	EXPECT_EQ(sprite.texture, 42u);
	EXPECT_FLOAT_EQ(sprite.position.x, 100.0f);
	EXPECT_FLOAT_EQ(sprite.position.y, 200.0f);
	EXPECT_FLOAT_EQ(sprite.size.x, 64.0f);
	EXPECT_FLOAT_EQ(sprite.size.y, 64.0f);
	EXPECT_FLOAT_EQ(sprite.rotation, 1.5f);
	EXPECT_FLOAT_EQ(sprite.color.r, 1.0f);
	EXPECT_FLOAT_EQ(sprite.color.g, 0.5f);
	EXPECT_FLOAT_EQ(sprite.color.b, 0.25f);
	EXPECT_FLOAT_EQ(sprite.color.a, 0.8f);
	EXPECT_FLOAT_EQ(sprite.texture_offset.x, 0.1f);
	EXPECT_FLOAT_EQ(sprite.texture_offset.y, 0.2f);
	EXPECT_FLOAT_EQ(sprite.texture_scale.x, 2.0f);
	EXPECT_FLOAT_EQ(sprite.texture_scale.y, 2.0f);
	EXPECT_EQ(sprite.layer, 10);
	EXPECT_FLOAT_EQ(sprite.pivot.x, 0.5f);
	EXPECT_FLOAT_EQ(sprite.pivot.y, 0.5f);
	EXPECT_TRUE(sprite.flip_x);
	EXPECT_FALSE(sprite.flip_y);
}

TEST_F(SceneSerializationTest, SaveAndLoad_EntityWithCamera_PreservesValues) {
	const SceneId scene_id = scene_manager_->CreateScene("CameraTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	const auto& scene = scene_manager_->GetScene(scene_id);
	const auto entity = scene.CreateEntity("CameraEntity");

	entity.set<Camera>(
			{.target = glm::vec3(5.0f, 10.0f, 15.0f),
			 .up = glm::vec3(0.0f, 1.0f, 0.0f),
			 .fov = 75.0f,
			 .aspect_ratio = 16.0f / 10.0f,
			 .near_plane = 0.5f,
			 .far_plane = 500.0f});

	const platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	scene_manager_->DestroyScene(scene_id);
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	const auto& loaded_scene = scene_manager_->GetScene(loaded_id);
	const auto loaded_entity = loaded_scene.FindEntityByName("CameraEntity");
	ASSERT_TRUE(loaded_entity.is_valid());
	ASSERT_TRUE(loaded_entity.has<Camera>());

	const auto& camera = loaded_entity.get<Camera>();

	EXPECT_FLOAT_EQ(camera.target.x, 5.0f);
	EXPECT_FLOAT_EQ(camera.target.y, 10.0f);
	EXPECT_FLOAT_EQ(camera.target.z, 15.0f);
	EXPECT_FLOAT_EQ(camera.fov, 75.0f);
	EXPECT_FLOAT_EQ(camera.aspect_ratio, 16.0f / 10.0f);
	EXPECT_FLOAT_EQ(camera.near_plane, 0.5f);
	EXPECT_FLOAT_EQ(camera.far_plane, 500.0f);
}

TEST_F(SceneSerializationTest, SaveAndLoad_EntityWithLight_PreservesValues) {
	const SceneId scene_id = scene_manager_->CreateScene("LightTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	const auto& scene = scene_manager_->GetScene(scene_id);
	const auto entity = scene.CreateEntity("LightEntity");

	entity.set<Light>(
			{.type = Light::Type::Point,
			 .color = glm::vec4(1.0f, 0.8f, 0.6f, 1.0f),
			 .intensity = 2.5f,
			 .range = 50.0f,
			 .attenuation = 0.5f,
			 .spot_angle = 30.0f});

	const platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	scene_manager_->DestroyScene(scene_id);
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	const auto& loaded_scene = scene_manager_->GetScene(loaded_id);
	const auto loaded_entity = loaded_scene.FindEntityByName("LightEntity");
	ASSERT_TRUE(loaded_entity.is_valid());
	ASSERT_TRUE(loaded_entity.has<Light>());

	const auto& light = loaded_entity.get<Light>();

	EXPECT_FLOAT_EQ(light.color.r, 1.0f);
	EXPECT_FLOAT_EQ(light.color.g, 0.8f);
	EXPECT_FLOAT_EQ(light.color.b, 0.6f);
	EXPECT_FLOAT_EQ(light.intensity, 2.5f);
	EXPECT_FLOAT_EQ(light.range, 50.0f);
}

TEST_F(SceneSerializationTest, SaveAndLoad_MultipleEntities_PreservesAll) {
	SceneId scene_id = scene_manager_->CreateScene("MultiEntityScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	auto& scene = scene_manager_->GetScene(scene_id);

	// Create multiple entities with different components
	auto entity1 = scene.CreateEntity("Entity1");
	entity1.get_mut<Transform>().position = glm::vec3(1.0f, 1.0f, 1.0f);

	auto entity2 = scene.CreateEntity("Entity2");
	entity2.get_mut<Transform>().position = glm::vec3(2.0f, 2.0f, 2.0f);
	entity2.set<Sprite>({.layer = 5});

	auto entity3 = scene.CreateEntity("Entity3");
	entity3.get_mut<Transform>().position = glm::vec3(3.0f, 3.0f, 3.0f);
	entity3.set<Light>({.intensity = 3.0f});

	platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	scene_manager_->DestroyScene(scene_id);
	SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	auto& loaded_scene = scene_manager_->GetScene(loaded_id);

	// Verify all entities exist with correct data
	auto loaded1 = loaded_scene.FindEntityByName("Entity1");
	ASSERT_TRUE(loaded1.is_valid());
	ASSERT_TRUE(loaded1.has<Transform>());
	EXPECT_FLOAT_EQ(loaded1.get<Transform>().position.x, 1.0f);

	auto loaded2 = loaded_scene.FindEntityByName("Entity2");
	ASSERT_TRUE(loaded2.is_valid());
	ASSERT_TRUE(loaded2.has<Transform>());
	EXPECT_FLOAT_EQ(loaded2.get<Transform>().position.x, 2.0f);
	ASSERT_TRUE(loaded2.has<Sprite>());
	EXPECT_EQ(loaded2.get<Sprite>().layer, 5);

	auto loaded3 = loaded_scene.FindEntityByName("Entity3");
	ASSERT_TRUE(loaded3.is_valid());
	ASSERT_TRUE(loaded3.has<Transform>());
	EXPECT_FLOAT_EQ(loaded3.get<Transform>().position.x, 3.0f);
	ASSERT_TRUE(loaded3.has<Light>());
	EXPECT_FLOAT_EQ(loaded3.get<Light>().intensity, 3.0f);
}

TEST_F(SceneSerializationTest, SaveAndLoad_SceneWithHierarchy_PreservesParentChild) {
	const SceneId scene_id = scene_manager_->CreateScene("HierarchyScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	const auto& scene = scene_manager_->GetScene(scene_id);

	// Create parent-child hierarchy
	const auto parent = scene.CreateEntity("Parent");
	parent.get_mut<Transform>().position = glm::vec3(10.0f, 0.0f, 0.0f);

	const auto child = scene.CreateEntity("Child", parent);
	child.get_mut<Transform>().position = glm::vec3(5.0f, 0.0f, 0.0f);

	const platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	scene_manager_->DestroyScene(scene_id);
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	const auto& loaded_scene = scene_manager_->GetScene(loaded_id);

	const auto loaded_parent = loaded_scene.FindEntityByName("Parent");
	const auto loaded_child = loaded_scene.FindEntityByName("Child");

	ASSERT_TRUE(loaded_parent.is_valid());
	ASSERT_TRUE(loaded_child.is_valid());

	// Verify parent-child relationship
	EXPECT_EQ(loaded_child.parent(), loaded_parent);

	// Verify positions
	ASSERT_TRUE(loaded_parent.has<Transform>());
	ASSERT_TRUE(loaded_child.has<Transform>());
	EXPECT_FLOAT_EQ(loaded_parent.get<Transform>().position.x, 10.0f);
	EXPECT_FLOAT_EQ(loaded_child.get<Transform>().position.x, 5.0f);
}

TEST_F(SceneSerializationTest, Load_NonexistentFile_ReturnsInvalidScene) {
	const platform::fs::Path path("nonexistent_file_that_does_not_exist.json");
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	EXPECT_EQ(loaded_id, INVALID_SCENE);
}

TEST_F(SceneSerializationTest, SavedFile_ContainsValidJson) {
	SceneId scene_id = scene_manager_->CreateScene("JsonTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	// Read and verify JSON structure
	std::ifstream file(temp_file_);
	ASSERT_TRUE(file.is_open());

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	// Basic validation - should contain expected keys
	EXPECT_TRUE(content.find("\"version\"") != std::string::npos);
	EXPECT_TRUE(content.find("\"name\"") != std::string::npos);
	EXPECT_TRUE(content.find("\"JsonTestScene\"") != std::string::npos);
	EXPECT_TRUE(content.find("\"flecs_data\"") != std::string::npos);
}

TEST_F(SceneSerializationTest, SaveAndLoad_ActiveCamera_PreservesSelection) {
	const SceneId scene_id = scene_manager_->CreateScene("ActiveCameraTestScene");
	ASSERT_NE(scene_id, INVALID_SCENE);

	const auto& scene = scene_manager_->GetScene(scene_id);

	// Create two cameras
	const auto camera1 = scene.CreateEntity("Camera1");
	camera1.set<Camera>({.fov = 60.0f});

	const auto camera2 = scene.CreateEntity("Camera2");
	camera2.set<Camera>({.fov = 90.0f});

	// Set camera2 as active
	ecs_world_->SetActiveCamera(camera2);
	EXPECT_EQ(ecs_world_->GetActiveCamera(), camera2);

	// Save and reload
	const platform::fs::Path path(temp_file_.string());
	EXPECT_TRUE(scene_manager_->SaveScene(scene_id, path));

	scene_manager_->DestroyScene(scene_id);
	const SceneId loaded_id = scene_manager_->LoadSceneFromFile(path);
	ASSERT_NE(loaded_id, INVALID_SCENE);

	// Verify the active camera was restored
	const auto loaded_active = ecs_world_->GetActiveCamera();
	ASSERT_TRUE(loaded_active.is_valid());

	// Should be the entity named "Camera2"
	EXPECT_STREQ(loaded_active.name().c_str(), "Camera2");

	// Verify it has the correct FOV
	ASSERT_TRUE(loaded_active.has<Camera>());
	EXPECT_FLOAT_EQ(loaded_active.get<Camera>().fov, 90.0f);
}
