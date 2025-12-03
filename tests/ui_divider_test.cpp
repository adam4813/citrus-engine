/**
 * @file ui_divider_test.cpp
 * @brief Tests for Divider UI element
 */

#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Divider Tests
// ============================================================================

class DividerTest : public ::testing::Test {
protected:
	void SetUp() override { divider = std::make_unique<Divider>(); }

	void TearDown() override { divider.reset(); }

	std::unique_ptr<Divider> divider;
};

// ============================================================================
// Constructor Tests
// ============================================================================

TEST_F(DividerTest, DefaultConstructor_CreatesHorizontalDivider) {
	EXPECT_EQ(divider->GetOrientation(), Orientation::Horizontal);
	EXPECT_FLOAT_EQ(divider->GetThickness(), 2.0f);
}

TEST_F(DividerTest, DefaultConstructor_SetsCorrectDimensions) {
	// Horizontal divider: width=0 (stretched by layout), height=thickness
	EXPECT_FLOAT_EQ(divider->GetWidth(), 0.0f);
	EXPECT_FLOAT_EQ(divider->GetHeight(), 2.0f);
}

TEST_F(DividerTest, ThicknessConstructor_SetsThickness) {
	auto thick_divider = std::make_unique<Divider>(5.0f);
	EXPECT_FLOAT_EQ(thick_divider->GetThickness(), 5.0f);
	EXPECT_EQ(thick_divider->GetOrientation(), Orientation::Horizontal);
}

TEST_F(DividerTest, OrientationConstructor_Horizontal) {
	auto h_divider = std::make_unique<Divider>(Orientation::Horizontal, 3.0f);
	EXPECT_EQ(h_divider->GetOrientation(), Orientation::Horizontal);
	EXPECT_FLOAT_EQ(h_divider->GetThickness(), 3.0f);
	EXPECT_FLOAT_EQ(h_divider->GetWidth(), 0.0f);
	EXPECT_FLOAT_EQ(h_divider->GetHeight(), 3.0f);
}

TEST_F(DividerTest, OrientationConstructor_Vertical) {
	auto v_divider = std::make_unique<Divider>(Orientation::Vertical, 4.0f);
	EXPECT_EQ(v_divider->GetOrientation(), Orientation::Vertical);
	EXPECT_FLOAT_EQ(v_divider->GetThickness(), 4.0f);
	EXPECT_FLOAT_EQ(v_divider->GetWidth(), 4.0f);
	EXPECT_FLOAT_EQ(v_divider->GetHeight(), 0.0f);
}

// ============================================================================
// Property Tests
// ============================================================================

TEST_F(DividerTest, SetColor_UpdatesColor) {
	divider->SetColor(Colors::RED);
	Color color = divider->GetColor();
	EXPECT_FLOAT_EQ(color.r, Colors::RED.r);
	EXPECT_FLOAT_EQ(color.g, Colors::RED.g);
	EXPECT_FLOAT_EQ(color.b, Colors::RED.b);
	EXPECT_FLOAT_EQ(color.a, Colors::RED.a);
}

TEST_F(DividerTest, SetThickness_UpdatesThickness) {
	divider->SetThickness(10.0f);
	EXPECT_FLOAT_EQ(divider->GetThickness(), 10.0f);
}

TEST_F(DividerTest, SetThickness_UpdatesHeight_WhenHorizontal) {
	divider->SetThickness(8.0f);
	EXPECT_FLOAT_EQ(divider->GetHeight(), 8.0f);
}

TEST_F(DividerTest, SetThickness_UpdatesWidth_WhenVertical) {
	divider->SetOrientation(Orientation::Vertical);
	divider->SetThickness(6.0f);
	EXPECT_FLOAT_EQ(divider->GetWidth(), 6.0f);
}

TEST_F(DividerTest, SetThickness_ClampsToMinimum) {
	divider->SetThickness(0.0f);
	EXPECT_FLOAT_EQ(divider->GetThickness(), 1.0f);

	divider->SetThickness(-5.0f);
	EXPECT_FLOAT_EQ(divider->GetThickness(), 1.0f);
}

TEST_F(DividerTest, SetOrientation_ToVertical_SwapsDimensions) {
	divider->SetThickness(5.0f);
	divider->SetOrientation(Orientation::Vertical);

	EXPECT_EQ(divider->GetOrientation(), Orientation::Vertical);
	EXPECT_FLOAT_EQ(divider->GetWidth(), 5.0f);
	EXPECT_FLOAT_EQ(divider->GetHeight(), 0.0f);
}

TEST_F(DividerTest, SetOrientation_ToHorizontal_SwapsDimensions) {
	auto v_divider = std::make_unique<Divider>(Orientation::Vertical, 5.0f);
	v_divider->SetOrientation(Orientation::Horizontal);

	EXPECT_EQ(v_divider->GetOrientation(), Orientation::Horizontal);
	EXPECT_FLOAT_EQ(v_divider->GetWidth(), 0.0f);
	EXPECT_FLOAT_EQ(v_divider->GetHeight(), 5.0f);
}

// ============================================================================
// Visibility Tests
// ============================================================================

TEST_F(DividerTest, IsVisible_DefaultTrue) {
	EXPECT_TRUE(divider->IsVisible());
}

TEST_F(DividerTest, SetVisible_UpdatesVisibility) {
	divider->SetVisible(false);
	EXPECT_FALSE(divider->IsVisible());

	divider->SetVisible(true);
	EXPECT_TRUE(divider->IsVisible());
}

// ============================================================================
// Orientation Enum Tests
// ============================================================================

TEST(OrientationTest, EnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(Orientation::Horizontal), 0);
	EXPECT_EQ(static_cast<uint8_t>(Orientation::Vertical), 1);
}
