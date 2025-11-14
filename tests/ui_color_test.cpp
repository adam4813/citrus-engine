#include <gtest/gtest.h>

import engine.ui.batch_renderer;

using namespace engine::ui::batch_renderer;

class ColorTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

// === Constructor Tests ===

TEST_F(ColorTest, DefaultConstructor_CreatesWhite) {
    Color color;
    
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_FLOAT_EQ(color.g, 1.0f);
    EXPECT_FLOAT_EQ(color.b, 1.0f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

TEST_F(ColorTest, ParameterizedConstructor_SetsValues) {
    Color color(0.5f, 0.6f, 0.7f, 0.8f);
    
    EXPECT_FLOAT_EQ(color.r, 0.5f);
    EXPECT_FLOAT_EQ(color.g, 0.6f);
    EXPECT_FLOAT_EQ(color.b, 0.7f);
    EXPECT_FLOAT_EQ(color.a, 0.8f);
}

TEST_F(ColorTest, ConstructorWithDefaultAlpha_SetsAlphaToOne) {
    Color color(0.5f, 0.6f, 0.7f);
    
    EXPECT_FLOAT_EQ(color.a, 1.0f);
}

// === Alpha Helper Tests ===

TEST_F(ColorTest, Alpha_ModifiesAlphaOnly) {
    Color original(0.5f, 0.6f, 0.7f, 1.0f);
    
    Color modified = Color::Alpha(original, 0.3f);
    
    EXPECT_FLOAT_EQ(modified.r, 0.5f);  // RGB unchanged
    EXPECT_FLOAT_EQ(modified.g, 0.6f);
    EXPECT_FLOAT_EQ(modified.b, 0.7f);
    EXPECT_FLOAT_EQ(modified.a, 0.3f);  // Alpha changed
}

TEST_F(ColorTest, Alpha_ClampsToZero) {
    Color original(0.5f, 0.6f, 0.7f, 1.0f);
    
    Color modified = Color::Alpha(original, -0.5f);
    
    EXPECT_FLOAT_EQ(modified.a, 0.0f);
}

TEST_F(ColorTest, Alpha_ClampsToOne) {
    Color original(0.5f, 0.6f, 0.7f, 0.5f);
    
    Color modified = Color::Alpha(original, 1.5f);
    
    EXPECT_FLOAT_EQ(modified.a, 1.0f);
}

TEST_F(ColorTest, Alpha_PreservesOriginal) {
    Color original(0.5f, 0.6f, 0.7f, 1.0f);
    
    Color modified = Color::Alpha(original, 0.3f);
    
    // Original should be unchanged
    EXPECT_FLOAT_EQ(original.a, 1.0f);
}

// === Brightness Helper Tests ===

TEST_F(ColorTest, Brightness_IncreasesRGBValues) {
    Color original(0.5f, 0.5f, 0.5f, 1.0f);
    
    Color modified = Color::Brightness(original, 0.2f);
    
    EXPECT_FLOAT_EQ(modified.r, 0.7f);
    EXPECT_FLOAT_EQ(modified.g, 0.7f);
    EXPECT_FLOAT_EQ(modified.b, 0.7f);
    EXPECT_FLOAT_EQ(modified.a, 1.0f);  // Alpha unchanged
}

TEST_F(ColorTest, Brightness_DecreasesRGBValues) {
    Color original(0.5f, 0.5f, 0.5f, 1.0f);
    
    Color modified = Color::Brightness(original, -0.2f);
    
    EXPECT_FLOAT_EQ(modified.r, 0.3f);
    EXPECT_FLOAT_EQ(modified.g, 0.3f);
    EXPECT_FLOAT_EQ(modified.b, 0.3f);
    EXPECT_FLOAT_EQ(modified.a, 1.0f);
}

TEST_F(ColorTest, Brightness_ClampsToZero) {
    Color original(0.3f, 0.3f, 0.3f, 1.0f);
    
    Color modified = Color::Brightness(original, -0.5f);
    
    EXPECT_FLOAT_EQ(modified.r, 0.0f);
    EXPECT_FLOAT_EQ(modified.g, 0.0f);
    EXPECT_FLOAT_EQ(modified.b, 0.0f);
}

TEST_F(ColorTest, Brightness_ClampsToOne) {
    Color original(0.8f, 0.8f, 0.8f, 1.0f);
    
    Color modified = Color::Brightness(original, 0.5f);
    
    EXPECT_FLOAT_EQ(modified.r, 1.0f);
    EXPECT_FLOAT_EQ(modified.g, 1.0f);
    EXPECT_FLOAT_EQ(modified.b, 1.0f);
}

TEST_F(ColorTest, Brightness_PreservesOriginal) {
    Color original(0.5f, 0.5f, 0.5f, 1.0f);
    
    Color modified = Color::Brightness(original, 0.2f);
    
    // Original should be unchanged
    EXPECT_FLOAT_EQ(original.r, 0.5f);
    EXPECT_FLOAT_EQ(original.g, 0.5f);
    EXPECT_FLOAT_EQ(original.b, 0.5f);
}

TEST_F(ColorTest, Brightness_WorksWithDifferentRGBValues) {
    Color original(0.2f, 0.5f, 0.8f, 0.7f);
    
    Color modified = Color::Brightness(original, 0.1f);
    
    EXPECT_FLOAT_EQ(modified.r, 0.3f);
    EXPECT_FLOAT_EQ(modified.g, 0.6f);
    EXPECT_FLOAT_EQ(modified.b, 0.9f);
    EXPECT_FLOAT_EQ(modified.a, 0.7f);  // Alpha unchanged
}

// === Color Constants Tests ===

TEST_F(ColorTest, Constants_WhiteIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::WHITE.r, 1.0f);
    EXPECT_FLOAT_EQ(Colors::WHITE.g, 1.0f);
    EXPECT_FLOAT_EQ(Colors::WHITE.b, 1.0f);
    EXPECT_FLOAT_EQ(Colors::WHITE.a, 1.0f);
}

TEST_F(ColorTest, Constants_BlackIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::BLACK.r, 0.0f);
    EXPECT_FLOAT_EQ(Colors::BLACK.g, 0.0f);
    EXPECT_FLOAT_EQ(Colors::BLACK.b, 0.0f);
    EXPECT_FLOAT_EQ(Colors::BLACK.a, 1.0f);
}

TEST_F(ColorTest, Constants_RedIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::RED.r, 1.0f);
    EXPECT_FLOAT_EQ(Colors::RED.g, 0.0f);
    EXPECT_FLOAT_EQ(Colors::RED.b, 0.0f);
    EXPECT_FLOAT_EQ(Colors::RED.a, 1.0f);
}

TEST_F(ColorTest, Constants_GreenIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::GREEN.r, 0.0f);
    EXPECT_FLOAT_EQ(Colors::GREEN.g, 1.0f);
    EXPECT_FLOAT_EQ(Colors::GREEN.b, 0.0f);
    EXPECT_FLOAT_EQ(Colors::GREEN.a, 1.0f);
}

TEST_F(ColorTest, Constants_BlueIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::BLUE.r, 0.0f);
    EXPECT_FLOAT_EQ(Colors::BLUE.g, 0.0f);
    EXPECT_FLOAT_EQ(Colors::BLUE.b, 1.0f);
    EXPECT_FLOAT_EQ(Colors::BLUE.a, 1.0f);
}

TEST_F(ColorTest, Constants_TransparentIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::TRANSPARENT.r, 0.0f);
    EXPECT_FLOAT_EQ(Colors::TRANSPARENT.g, 0.0f);
    EXPECT_FLOAT_EQ(Colors::TRANSPARENT.b, 0.0f);
    EXPECT_FLOAT_EQ(Colors::TRANSPARENT.a, 0.0f);
}

TEST_F(ColorTest, Constants_GoldIsCorrect) {
    EXPECT_FLOAT_EQ(Colors::GOLD.r, 1.0f);
    EXPECT_FLOAT_EQ(Colors::GOLD.g, 0.84f);
    EXPECT_FLOAT_EQ(Colors::GOLD.b, 0.0f);
    EXPECT_FLOAT_EQ(Colors::GOLD.a, 1.0f);
}

// === Combined Operations Tests ===

TEST_F(ColorTest, AlphaAndBrightness_CanBeCombined) {
    Color original(0.5f, 0.5f, 0.5f, 1.0f);
    
    // Brighten, then reduce alpha
    Color step1 = Color::Brightness(original, 0.2f);
    Color final = Color::Alpha(step1, 0.5f);
    
    EXPECT_FLOAT_EQ(final.r, 0.7f);
    EXPECT_FLOAT_EQ(final.g, 0.7f);
    EXPECT_FLOAT_EQ(final.b, 0.7f);
    EXPECT_FLOAT_EQ(final.a, 0.5f);
}
