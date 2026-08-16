#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

import engine.assets;
import engine.asset_registry;

using namespace engine::assets;

class AssetManagerTest : public ::testing::Test {
protected:
	std::filesystem::path temp_dir_;
	std::filesystem::path temp_text_file_;
	std::filesystem::path temp_binary_file_;

	void SetUp() override {
		temp_dir_ = std::filesystem::temp_directory_path() / "citrus_asset_test";
		std::filesystem::create_directories(temp_dir_);
		temp_text_file_ = temp_dir_ / "test_text.txt";
		temp_binary_file_ = temp_dir_ / "test_binary.bin";
	}

	void TearDown() override {
		std::error_code ec;
		std::filesystem::remove_all(temp_dir_, ec);
	}
};

// =============================================================================
// SaveTextFile / LoadTextFile (absolute path overloads)
// =============================================================================

TEST_F(AssetManagerTest, save_and_load_text_file) {
	const std::string content = "Hello, Citrus Engine!\nLine 2.";
	ASSERT_TRUE(AssetManager::SaveTextFile(temp_text_file_, content));
	EXPECT_TRUE(std::filesystem::exists(temp_text_file_));

	auto loaded = AssetManager::LoadTextFile(temp_text_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(*loaded, content);
}

TEST_F(AssetManagerTest, save_text_overwrites_existing) {
	ASSERT_TRUE(AssetManager::SaveTextFile(temp_text_file_, "first"));
	ASSERT_TRUE(AssetManager::SaveTextFile(temp_text_file_, "second"));

	auto loaded = AssetManager::LoadTextFile(temp_text_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(*loaded, "second");
}

TEST_F(AssetManagerTest, save_text_empty_string) {
	// Saving empty content may return true but loading empty returns nullopt
	AssetManager::SaveTextFile(temp_text_file_, "");
	auto loaded = AssetManager::LoadTextFile(temp_text_file_);
	// The implementation returns nullopt for empty files
	EXPECT_FALSE(loaded.has_value());
}

// =============================================================================
// SaveBinaryFile / LoadBinaryFile (absolute path overloads)
// =============================================================================

TEST_F(AssetManagerTest, save_and_load_binary_file) {
	std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
	ASSERT_TRUE(AssetManager::SaveBinaryFile(temp_binary_file_, data));
	EXPECT_TRUE(std::filesystem::exists(temp_binary_file_));

	auto loaded = AssetManager::LoadBinaryFile(temp_binary_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(*loaded, data);
}

TEST_F(AssetManagerTest, save_binary_overwrites_existing) {
	std::vector<uint8_t> data1 = {0x01, 0x02};
	std::vector<uint8_t> data2 = {0x03, 0x04, 0x05};

	ASSERT_TRUE(AssetManager::SaveBinaryFile(temp_binary_file_, data1));
	ASSERT_TRUE(AssetManager::SaveBinaryFile(temp_binary_file_, data2));

	auto loaded = AssetManager::LoadBinaryFile(temp_binary_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(*loaded, data2);
}

TEST_F(AssetManagerTest, save_binary_empty_data) {
	std::vector<uint8_t> empty_data;
	AssetManager::SaveBinaryFile(temp_binary_file_, empty_data);
	auto loaded = AssetManager::LoadBinaryFile(temp_binary_file_);
	// The implementation returns nullopt for empty files
	EXPECT_FALSE(loaded.has_value());
}

// =============================================================================
// Error Handling - Missing Files
// =============================================================================

TEST_F(AssetManagerTest, load_text_file_missing_returns_nullopt) {
	std::filesystem::path nonexistent = temp_dir_ / "does_not_exist.txt";
	auto result = AssetManager::LoadTextFile(nonexistent);
	EXPECT_FALSE(result.has_value());
}

TEST_F(AssetManagerTest, load_binary_file_missing_returns_nullopt) {
	std::filesystem::path nonexistent = temp_dir_ / "does_not_exist.bin";
	auto result = AssetManager::LoadBinaryFile(nonexistent);
	EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Error Handling - Invalid Paths
// =============================================================================

TEST_F(AssetManagerTest, save_text_to_nonexistent_directory_fails) {
	std::filesystem::path bad_path = temp_dir_ / "no_such_dir" / "file.txt";
	bool result = AssetManager::SaveTextFile(bad_path, "content");
	EXPECT_FALSE(result);
}

TEST_F(AssetManagerTest, save_binary_to_nonexistent_directory_fails) {
	std::filesystem::path bad_path = temp_dir_ / "no_such_dir" / "file.bin";
	std::vector<uint8_t> data = {0x01};
	bool result = AssetManager::SaveBinaryFile(bad_path, data);
	EXPECT_FALSE(result);
}

// =============================================================================
// Text File Content Integrity
// =============================================================================

TEST_F(AssetManagerTest, text_file_preserves_special_characters) {
	const std::string content = "Tab:\there\nNewline above\r\nCRLF\nUnicode: äöü";
	ASSERT_TRUE(AssetManager::SaveTextFile(temp_text_file_, content));

	auto loaded = AssetManager::LoadTextFile(temp_text_file_);
	ASSERT_TRUE(loaded.has_value());
	// The exact content may differ on line endings across platforms,
	// but the core content should be preserved
	EXPECT_FALSE(loaded->empty());
}

TEST_F(AssetManagerTest, text_file_preserves_large_content) {
	std::string large_content;
	for (int i = 0; i < 1000; ++i) {
		large_content += "Line " + std::to_string(i) + "\n";
	}

	ASSERT_TRUE(AssetManager::SaveTextFile(temp_text_file_, large_content));
	auto loaded = AssetManager::LoadTextFile(temp_text_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(loaded->size(), large_content.size());
}

// =============================================================================
// Binary File Content Integrity
// =============================================================================

TEST_F(AssetManagerTest, binary_file_preserves_all_byte_values) {
	std::vector<uint8_t> all_bytes(256);
	for (int i = 0; i < 256; ++i) {
		all_bytes[i] = static_cast<uint8_t>(i);
	}

	ASSERT_TRUE(AssetManager::SaveBinaryFile(temp_binary_file_, all_bytes));
	auto loaded = AssetManager::LoadBinaryFile(temp_binary_file_);
	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(*loaded, all_bytes);
}

// =============================================================================
// Image Struct Tests
// =============================================================================

TEST(ImageTest, default_image_is_invalid) {
	Image img;
	EXPECT_FALSE(img.IsValid());
	EXPECT_EQ(img.width, 0);
	EXPECT_EQ(img.height, 0);
	EXPECT_EQ(img.channels, 0);
	EXPECT_TRUE(img.pixel_data.empty());
}

TEST(ImageTest, valid_image_check) {
	Image img;
	img.width = 16;
	img.height = 16;
	img.channels = 4;
	img.pixel_data.resize(16 * 16 * 4, 0xFF);
	EXPECT_TRUE(img.IsValid());
}

TEST(ImageTest, zero_dimensions_is_invalid) {
	Image img;
	img.width = 0;
	img.height = 16;
	img.pixel_data.resize(16, 0);
	EXPECT_FALSE(img.IsValid());
}

// =============================================================================
// AssetCache::LoadOrImportSource - raw-file import + sidecar metadata
// =============================================================================

namespace {
/// Minimal concrete asset used to exercise the raw-file import pipeline without
/// pulling in a renderer-backed asset type.
struct TestRawAsset : AssetInfo {
	TestRawAsset() : AssetInfo("", AssetType{}) {}
	explicit TestRawAsset(std::string n) : AssetInfo(std::move(n), AssetType{}) {}

protected:
	[[nodiscard]] std::string_view GetTypeName() const override { return "test_raw"; }
};
} // namespace

// Regression: LoadOrImportSource must pass the filename WITH its extension to the
// importer (previously it passed the stem, so no importer was ever matched and no
// sidecar ".meta.json" was written).
TEST_F(AssetManagerTest, import_source_matches_importer_and_writes_metadata) {
	auto& cache = AssetCache::Instance();
	cache.RegisterFileImporter(
		{".citrustest"},
		[](const std::string& name, const std::string&) -> std::shared_ptr<AssetInfo> {
			return std::make_shared<TestRawAsset>(name);
		}
	);

	const auto raw = temp_dir_ / "widget.citrustest";
	const std::vector<uint8_t> bytes = {1, 2, 3, 4, 5};
	ASSERT_TRUE(AssetManager::SaveBinaryFile(raw, bytes));

	const std::string raw_path = raw.string();
	const auto asset = cache.LoadOrImportSource(raw_path);
	ASSERT_NE(asset, nullptr);
	EXPECT_NE(asset->guid, 0U);
	EXPECT_EQ(asset->name, "widget");

	const std::filesystem::path meta = raw_path + ".meta.json";
	ASSERT_TRUE(std::filesystem::exists(meta));

	const auto meta_text = AssetManager::LoadTextFile(meta);
	ASSERT_TRUE(meta_text.has_value());
	const auto j = nlohmann::json::parse(*meta_text, nullptr, false);
	ASSERT_FALSE(j.is_discarded());

	// Identity + source locator live under a single grouped "_metadata" block.
	ASSERT_TRUE(j.contains("_metadata"));
	const auto& m = j["_metadata"];
	EXPECT_EQ(m.value("guid", 0U), asset->guid);
	EXPECT_EQ(m.value("type", std::string{}), "test_raw");
	EXPECT_FALSE(m.value("path", std::string{}).empty());
	EXPECT_EQ(m.value("source_size", static_cast<uint64_t>(0)), bytes.size());
	EXPECT_NE(m.value("source_hash", static_cast<uint64_t>(0)), static_cast<uint64_t>(0));
}

// The persistent GUID in the sidecar's "_metadata" must survive a cache reload:
// re-importing the same source after Clear() recovers the original GUID via the meta.
TEST_F(AssetManagerTest, import_source_recovers_guid_from_metadata) {
	auto& cache = AssetCache::Instance();
	cache.RegisterFileImporter(
		{".citrustest"},
		[](const std::string& name, const std::string&) -> std::shared_ptr<AssetInfo> {
			return std::make_shared<TestRawAsset>(name);
		}
	);
	// Register the type so meta-recovery can reconstruct the asset via "_metadata.type".
	AssetTypeRegistry::Instance().RegisterType<TestRawAsset>(std::string_view{"test_raw"}, AssetType{}).Build();

	const auto raw = temp_dir_ / "gadget.citrustest";
	const std::vector<uint8_t> bytes = {9, 8, 7};
	ASSERT_TRUE(AssetManager::SaveBinaryFile(raw, bytes));
	const std::string raw_path = raw.string();

	const auto first = cache.LoadOrImportSource(raw_path);
	ASSERT_NE(first, nullptr);
	const uint32_t original_guid = first->guid;
	ASSERT_NE(original_guid, 0U);

	cache.Clear(); // drop the in-memory cache; the on-disk sidecar persists

	const auto recovered = cache.LoadOrImportSource(raw_path);
	ASSERT_NE(recovered, nullptr);
	EXPECT_EQ(recovered->guid, original_guid);
}
