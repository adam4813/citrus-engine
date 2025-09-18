#include <gtest/gtest.h>

import engine.components;

class SanityCheckTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(SanityCheckTest, google_test_framework_works) {
    EXPECT_TRUE(true);
    EXPECT_EQ(1 + 1, 2);
    EXPECT_NE(0, 1);
}

TEST_F(SanityCheckTest, basic_math_operations_work) {
    int a = 5;
    int b = 3;

    EXPECT_EQ(a + b, 8);
    EXPECT_EQ(a - b, 2);
    EXPECT_EQ(a * b, 15);
    EXPECT_EQ(a / b, 1);
}

TEST_F(SanityCheckTest, string_operations_work) {
    std::string hello = "Hello";
    std::string world = "World";
    std::string combined = hello + " " + world;
    EXPECT_EQ(combined, "Hello World");
    EXPECT_EQ(hello.length(), 5);
    EXPECT_FALSE(world.empty());
}

TEST_F(SanityCheckTest, container_operations_work) {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    EXPECT_EQ(numbers.size(), 5);
    EXPECT_EQ(numbers[0], 1);
    EXPECT_EQ(numbers.back(), 5);

    numbers.push_back(6);
    EXPECT_EQ(numbers.size(), 6);
}

TEST_F(SanityCheckTest, tilemap_add_layer_updates_layer_count) {
    engine::components::Tilemap tilemap;

    EXPECT_EQ(tilemap.GetLayerCount(), static_cast<size_t>(0));

    auto firstIndex = tilemap.AddLayer();
    EXPECT_EQ(firstIndex, static_cast<size_t>(0));
    EXPECT_EQ(tilemap.GetLayerCount(), static_cast<size_t>(1));
    EXPECT_NE(tilemap.GetLayer(0), nullptr);

    auto secondIndex = tilemap.AddLayer();
    EXPECT_EQ(secondIndex, static_cast<size_t>(1));
    EXPECT_EQ(tilemap.GetLayerCount(), static_cast<size_t>(2));
    EXPECT_NE(tilemap.GetLayer(1), nullptr);
}

TEST_F(SanityCheckTest, tilemap_WithLayer_invokes_callback_for_valid_index_and_returns_false_for_invalid) {
    engine::components::Tilemap tilemap;

    auto idx = tilemap.AddLayer();
    int called = 0;

    bool ok_valid = tilemap.WithLayer(idx, [&](engine::components::TilemapLayer &layer) {
        called++;
        layer.visible = false;
    });
    EXPECT_TRUE(ok_valid);
    EXPECT_EQ(called, 1);
    ASSERT_NE(tilemap.GetLayer(idx), nullptr);
    EXPECT_FALSE(tilemap.GetLayer(idx)->visible);

    bool ok_invalid = tilemap.WithLayer(999, [&](engine::components::TilemapLayer &layer) {
        called++;
        layer.visible = true; // should not run
    });
    EXPECT_FALSE(ok_invalid);
    EXPECT_EQ(called, 1);
}
