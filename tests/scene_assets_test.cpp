// Import modules first to avoid macro conflicts
import engine.scene;

#include <gtest/gtest.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
using engine::scene::AssetInfo;
using engine::scene::AssetPtr;
using engine::scene::AssetRegistry;
using engine::scene::AssetType;
using engine::scene::SceneAssets;
using engine::scene::ShaderAssetInfo;

class SceneAssetsTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Register asset types (normally done by InitializeSceneSystem)
		ShaderAssetInfo::RegisterType();
	}
};

// =============================================================================
// SceneAssets Container Tests
// =============================================================================

TEST_F(SceneAssetsTest, Add_SingleAsset_IncreasesSize) {
	SceneAssets assets;
	EXPECT_EQ(assets.Size(), 0);
	EXPECT_TRUE(assets.Empty());

	const auto shader = std::make_shared<ShaderAssetInfo>("test_shader", "test.vert", "test.frag");
	assets.Add(shader);

	EXPECT_EQ(assets.Size(), 1);
	EXPECT_FALSE(assets.Empty());
}

TEST_F(SceneAssetsTest, Find_ExistingAsset_ReturnsSharedPtr) {
	SceneAssets assets;
	const auto shader = std::make_shared<ShaderAssetInfo>("my_shader", "v.vert", "f.frag");
	assets.Add(shader);

	const auto found = assets.Find("my_shader", AssetType::SHADER);
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found->name, "my_shader");
	EXPECT_EQ(found->type, AssetType::SHADER);
}

TEST_F(SceneAssetsTest, Find_NonExistentAsset_ReturnsNull) {
	SceneAssets assets;
	const auto shader = std::make_shared<ShaderAssetInfo>("my_shader", "v.vert", "f.frag");
	assets.Add(shader);

	EXPECT_EQ(assets.Find("other_shader", AssetType::SHADER), nullptr);
}

TEST_F(SceneAssetsTest, FindTyped_ReturnsSharedPtr) {
	SceneAssets assets;
	const auto shader = std::make_shared<ShaderAssetInfo>("typed_shader", "v.vert", "f.frag");
	assets.Add(shader);

	const auto found = assets.FindTyped<ShaderAssetInfo>("typed_shader");
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found->vertex_path, "v.vert");
	EXPECT_EQ(found->fragment_path, "f.frag");
}

TEST_F(SceneAssetsTest, Remove_ExistingAsset_ReturnsTrue) {
	SceneAssets assets;
	assets.Add(std::make_shared<ShaderAssetInfo>("removable", "v.vert", "f.frag"));
	EXPECT_EQ(assets.Size(), 1);

	const bool removed = assets.Remove("removable", AssetType::SHADER);
	EXPECT_TRUE(removed);
	EXPECT_EQ(assets.Size(), 0);
}

TEST_F(SceneAssetsTest, Remove_NonExistentAsset_ReturnsFalse) {
	SceneAssets assets;
	assets.Add(std::make_shared<ShaderAssetInfo>("exists", "v.vert", "f.frag"));

	const bool removed = assets.Remove("does_not_exist", AssetType::SHADER);
	EXPECT_FALSE(removed);
	EXPECT_EQ(assets.Size(), 1);
}

TEST_F(SceneAssetsTest, Clear_RemovesAllAssets) {
	SceneAssets assets;
	assets.Add(std::make_shared<ShaderAssetInfo>("shader1", "v1.vert", "f1.frag"));
	assets.Add(std::make_shared<ShaderAssetInfo>("shader2", "v2.vert", "f2.frag"));
	EXPECT_EQ(assets.Size(), 2);

	assets.Clear();
	EXPECT_TRUE(assets.Empty());
}

TEST_F(SceneAssetsTest, GetAllOfType_ReturnsSharedPtrs) {
	SceneAssets assets;
	assets.Add(std::make_shared<ShaderAssetInfo>("shader1", "v1.vert", "f1.frag"));
	assets.Add(std::make_shared<ShaderAssetInfo>("shader2", "v2.vert", "f2.frag"));

	const auto shaders = assets.GetAllOfType<ShaderAssetInfo>();
	EXPECT_EQ(shaders.size(), 2);
	// Verify they're shared_ptrs we can hold onto
	ASSERT_NE(shaders[0], nullptr);
	EXPECT_EQ(shaders[0]->name, "shader1");
}

// =============================================================================
// ShaderAssetInfo Serialization Tests
// =============================================================================

TEST_F(SceneAssetsTest, ShaderAssetInfo_ToJson_ContainsAllFields) {
	const ShaderAssetInfo shader("colored_3d", "shaders/colored_3d.vert", "shaders/colored_3d.frag");

	json j;
	shader.ToJson(j);

	EXPECT_EQ(j["type"], "shader");
	EXPECT_EQ(j["name"], "colored_3d");
	EXPECT_EQ(j["vertex_path"], "shaders/colored_3d.vert");
	EXPECT_EQ(j["fragment_path"], "shaders/colored_3d.frag");
	// id should NOT be serialized
	EXPECT_FALSE(j.contains("id"));
}

TEST_F(SceneAssetsTest, ShaderAssetInfo_FromJson_ParsesAllFields) {
	const json j = {
			{"type", "shader"},
			{"name", "unlit_sprite"},
			{"vertex_path", "shaders/unlit.vert"},
			{"fragment_path", "shaders/unlit.frag"}};

	const auto asset = AssetInfo::FromJson(j);
	ASSERT_NE(asset, nullptr);
	EXPECT_EQ(asset->type, AssetType::SHADER);
	EXPECT_EQ(asset->name, "unlit_sprite");

	auto* shader = dynamic_cast<ShaderAssetInfo*>(asset.get());
	ASSERT_NE(shader, nullptr);
	EXPECT_EQ(shader->vertex_path, "shaders/unlit.vert");
	EXPECT_EQ(shader->fragment_path, "shaders/unlit.frag");
	EXPECT_EQ(shader->id, 0); // Transient, not loaded yet
}

TEST_F(SceneAssetsTest, ShaderAssetInfo_ToJsonFromJson_Roundtrip) {
	const ShaderAssetInfo original("roundtrip_shader", "path/to/v.vert", "path/to/f.frag");

	json j;
	original.ToJson(j);

	const auto restored = AssetInfo::FromJson(j);
	ASSERT_NE(restored, nullptr);

	auto* shader = dynamic_cast<ShaderAssetInfo*>(restored.get());
	ASSERT_NE(shader, nullptr);
	EXPECT_EQ(shader->name, original.name);
	EXPECT_EQ(shader->vertex_path, original.vertex_path);
	EXPECT_EQ(shader->fragment_path, original.fragment_path);
}

// =============================================================================
// AssetRegistry Tests
// =============================================================================

TEST_F(SceneAssetsTest, AssetRegistry_IsRegistered_ReturnsTrueForShader) {
	EXPECT_TRUE(AssetRegistry::Instance().IsRegistered("shader"));
}

TEST_F(SceneAssetsTest, AssetRegistry_IsRegistered_ReturnsFalseForUnknown) {
	EXPECT_FALSE(AssetRegistry::Instance().IsRegistered("unknown_type"));
}

TEST_F(SceneAssetsTest, AssetRegistry_Create_UnknownType_ReturnsNull) {
	const json j = {{"type", "nonexistent_type"}, {"name", "test"}};
	const auto asset = AssetRegistry::Instance().Create(j);
	EXPECT_EQ(asset, nullptr);
}

TEST_F(SceneAssetsTest, AssetRegistry_Create_EmptyType_ReturnsNull) {
	const json j = {{"name", "test"}}; // No type field
	const auto asset = AssetRegistry::Instance().Create(j);
	EXPECT_EQ(asset, nullptr);
}

// =============================================================================
// FindTypedIf Predicate Tests
// =============================================================================

TEST_F(SceneAssetsTest, FindTypedIf_WithMatchingPredicate_ReturnsSharedPtr) {
	SceneAssets assets;
	const auto shader = std::make_shared<ShaderAssetInfo>("target", "v.vert", "f.frag");
	assets.Add(shader);
	assets.Add(std::make_shared<ShaderAssetInfo>("other", "v2.vert", "f2.frag"));

	const auto found = assets.FindTypedIf<ShaderAssetInfo>(
			[target_id = shader->id](const ShaderAssetInfo& s) { return s.id == target_id; });

	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found->name, "target");
}

TEST_F(SceneAssetsTest, FindTypedIf_NoMatch_ReturnsNull) {
	SceneAssets assets;
	assets.Add(std::make_shared<ShaderAssetInfo>("shader1", "v.vert", "f.frag"));

	const auto found = assets.FindTypedIf<ShaderAssetInfo>([](const ShaderAssetInfo& s) {
		return s.id == 9999; // No shader has this ID
	});

	EXPECT_EQ(found, nullptr);
}
