#include <gtest/gtest.h>

import engine.ui.batch_renderer;

using namespace engine::ui::batch_renderer;

// ============================================================================
// UITheme Color Tests
// ============================================================================

class UIThemeColorTest : public ::testing::Test {};

TEST_F(UIThemeColorTest, PrimaryColors_AreValid) {
	EXPECT_EQ(UITheme::Primary::NORMAL.r, 1.0f);
	EXPECT_EQ(UITheme::Primary::NORMAL.g, 0.84f);
	EXPECT_EQ(UITheme::Primary::NORMAL.b, 0.0f);
	EXPECT_EQ(UITheme::Primary::NORMAL.a, 1.0f);
}

TEST_F(UIThemeColorTest, PrimaryHover_IsBrighterOrMaxedOut) {
	// Hover should be brighter than normal, or clamped at max if already bright
	// GOLD has r=1.0, g=0.84, b=0.0, so r is already maxed out
	EXPECT_GE(UITheme::Primary::HOVER.r, UITheme::Primary::NORMAL.r);
	EXPECT_GT(UITheme::Primary::HOVER.g, UITheme::Primary::NORMAL.g);
	EXPECT_GE(UITheme::Primary::HOVER.b, UITheme::Primary::NORMAL.b);
}

TEST_F(UIThemeColorTest, PrimaryActive_IsDarker) {
	// Active should be darker than normal
	EXPECT_LT(UITheme::Primary::ACTIVE.r, UITheme::Primary::NORMAL.r);
	EXPECT_LT(UITheme::Primary::ACTIVE.g, UITheme::Primary::NORMAL.g);
}

TEST_F(UIThemeColorTest, PrimaryDisabled_IsTransparent) {
	// Disabled should have reduced alpha
	EXPECT_LT(UITheme::Primary::DISABLED.a, UITheme::Primary::NORMAL.a);
	EXPECT_EQ(UITheme::Primary::DISABLED.a, 0.5f);
}

TEST_F(UIThemeColorTest, BackgroundColors_AreValid) {
	EXPECT_GT(UITheme::Background::PANEL.r, 0.0f);
	EXPECT_GT(UITheme::Background::PANEL.g, 0.0f);
	EXPECT_GT(UITheme::Background::PANEL.b, 0.0f);
	EXPECT_EQ(UITheme::Background::PANEL.a, 1.0f);
}

TEST_F(UIThemeColorTest, BackgroundButton_HasHoverStates) {
	// Hover should be brighter
	EXPECT_GT(UITheme::Background::BUTTON_HOVER.r, UITheme::Background::BUTTON.r);
	
	// Active should be darker
	EXPECT_LT(UITheme::Background::BUTTON_ACTIVE.r, UITheme::Background::BUTTON.r);
}

TEST_F(UIThemeColorTest, TextColors_AreReadable) {
	// Primary text should be visible (white or near-white)
	EXPECT_GE(UITheme::Text::PRIMARY.r, 0.9f);
	EXPECT_GE(UITheme::Text::PRIMARY.g, 0.9f);
	EXPECT_GE(UITheme::Text::PRIMARY.b, 0.9f);
	
	// Disabled text should be semi-transparent
	EXPECT_LT(UITheme::Text::DISABLED.a, UITheme::Text::PRIMARY.a);
}

TEST_F(UIThemeColorTest, BorderColors_AreDistinct) {
	// Focus border should be accent color (gold)
	EXPECT_EQ(UITheme::Border::FOCUS.r, Colors::GOLD.r);
	EXPECT_EQ(UITheme::Border::FOCUS.g, Colors::GOLD.g);
	
	// Error border should be red
	EXPECT_EQ(UITheme::Border::ERROR.r, Colors::RED.r);
}

TEST_F(UIThemeColorTest, StateColors_AreDistinct) {
	// Selected should use accent color
	EXPECT_EQ(UITheme::State::SELECTED.r, Colors::GOLD.r);
	
	// Checked should be green
	EXPECT_EQ(UITheme::State::CHECKED.r, Colors::GREEN.r);
	
	// Hover overlay should be semi-transparent white
	EXPECT_LT(UITheme::State::HOVER_OVERLAY.a, 0.5f);
}

// ============================================================================
// UITheme Spacing Tests
// ============================================================================

class UIThemeSpacingTest : public ::testing::Test {};

TEST_F(UIThemeSpacingTest, SpacingValues_AreOrdered) {
	EXPECT_LT(UITheme::Spacing::NONE, UITheme::Spacing::TINY);
	EXPECT_LT(UITheme::Spacing::TINY, UITheme::Spacing::SMALL);
	EXPECT_LT(UITheme::Spacing::SMALL, UITheme::Spacing::MEDIUM);
	EXPECT_LT(UITheme::Spacing::MEDIUM, UITheme::Spacing::LARGE);
	EXPECT_LT(UITheme::Spacing::LARGE, UITheme::Spacing::XL);
	EXPECT_LT(UITheme::Spacing::XL, UITheme::Spacing::XXL);
	EXPECT_LT(UITheme::Spacing::XXL, UITheme::Spacing::HUGE);
}

TEST_F(UIThemeSpacingTest, SpacingValues_ArePositive) {
	EXPECT_GE(UITheme::Spacing::TINY, 0.0f);
	EXPECT_GT(UITheme::Spacing::SMALL, 0.0f);
	EXPECT_GT(UITheme::Spacing::MEDIUM, 0.0f);
	EXPECT_GT(UITheme::Spacing::LARGE, 0.0f);
}

TEST_F(UIThemeSpacingTest, SpacingNone_IsZero) {
	EXPECT_EQ(UITheme::Spacing::NONE, 0.0f);
}

// ============================================================================
// UITheme Padding Tests
// ============================================================================

class UIThemePaddingTest : public ::testing::Test {};

TEST_F(UIThemePaddingTest, ButtonPadding_IsReasonable) {
	EXPECT_GT(UITheme::Padding::BUTTON_HORIZONTAL, 0.0f);
	EXPECT_GT(UITheme::Padding::BUTTON_VERTICAL, 0.0f);
	
	// Horizontal padding typically larger than vertical
	EXPECT_GE(UITheme::Padding::BUTTON_HORIZONTAL, UITheme::Padding::BUTTON_VERTICAL);
}

TEST_F(UIThemePaddingTest, PanelPadding_IsReasonable) {
	EXPECT_GT(UITheme::Padding::PANEL_HORIZONTAL, 0.0f);
	EXPECT_GT(UITheme::Padding::PANEL_VERTICAL, 0.0f);
}

TEST_F(UIThemePaddingTest, InputPadding_IsReasonable) {
	EXPECT_GT(UITheme::Padding::INPUT_HORIZONTAL, 0.0f);
	EXPECT_GT(UITheme::Padding::INPUT_VERTICAL, 0.0f);
}

TEST_F(UIThemePaddingTest, GenericPadding_IsOrdered) {
	EXPECT_LT(UITheme::Padding::SMALL, UITheme::Padding::MEDIUM);
	EXPECT_LT(UITheme::Padding::MEDIUM, UITheme::Padding::LARGE);
}

// ============================================================================
// UITheme Font Size Tests
// ============================================================================

class UIThemeFontSizeTest : public ::testing::Test {};

TEST_F(UIThemeFontSizeTest, FontSizes_AreOrdered) {
	EXPECT_LT(UITheme::FontSize::TINY, UITheme::FontSize::SMALL);
	EXPECT_LT(UITheme::FontSize::SMALL, UITheme::FontSize::NORMAL);
	EXPECT_LT(UITheme::FontSize::NORMAL, UITheme::FontSize::MEDIUM);
	EXPECT_LT(UITheme::FontSize::MEDIUM, UITheme::FontSize::LARGE);
	EXPECT_LT(UITheme::FontSize::LARGE, UITheme::FontSize::XL);
	EXPECT_LT(UITheme::FontSize::XL, UITheme::FontSize::XXL);
}

TEST_F(UIThemeFontSizeTest, HeadingSizes_AreOrdered) {
	EXPECT_GT(UITheme::FontSize::HEADING_1, UITheme::FontSize::HEADING_2);
	EXPECT_GT(UITheme::FontSize::HEADING_2, UITheme::FontSize::HEADING_3);
	EXPECT_GT(UITheme::FontSize::HEADING_3, UITheme::FontSize::NORMAL);
}

TEST_F(UIThemeFontSizeTest, FontSizes_ArePositive) {
	EXPECT_GT(UITheme::FontSize::TINY, 0.0f);
	EXPECT_GT(UITheme::FontSize::SMALL, 0.0f);
	EXPECT_GT(UITheme::FontSize::NORMAL, 0.0f);
	EXPECT_GT(UITheme::FontSize::LARGE, 0.0f);
}

TEST_F(UIThemeFontSizeTest, NormalFontSize_IsReadable) {
	// Normal font size should be at least 12px for readability
	EXPECT_GE(UITheme::FontSize::NORMAL, 12.0f);
}

// ============================================================================
// UITheme Border Tests
// ============================================================================

class UIThemeBorderTest : public ::testing::Test {};

TEST_F(UIThemeBorderTest, BorderSizes_AreOrdered) {
	EXPECT_LT(UITheme::BorderSize::NONE, UITheme::BorderSize::THIN);
	EXPECT_LT(UITheme::BorderSize::THIN, UITheme::BorderSize::MEDIUM);
	EXPECT_LT(UITheme::BorderSize::MEDIUM, UITheme::BorderSize::THICK);
}

TEST_F(UIThemeBorderTest, BorderSizes_AreNonNegative) {
	EXPECT_GE(UITheme::BorderSize::NONE, 0.0f);
	EXPECT_GE(UITheme::BorderSize::THIN, 0.0f);
	EXPECT_GE(UITheme::BorderSize::MEDIUM, 0.0f);
	EXPECT_GE(UITheme::BorderSize::THICK, 0.0f);
}

TEST_F(UIThemeBorderTest, BorderRadius_AreOrdered) {
	EXPECT_LT(UITheme::BorderRadius::NONE, UITheme::BorderRadius::SMALL);
	EXPECT_LT(UITheme::BorderRadius::SMALL, UITheme::BorderRadius::MEDIUM);
	EXPECT_LT(UITheme::BorderRadius::MEDIUM, UITheme::BorderRadius::LARGE);
}

TEST_F(UIThemeBorderTest, BorderRadius_RoundIsLarge) {
	// Round should be a very large value for fully rounded corners
	EXPECT_GT(UITheme::BorderRadius::ROUND, 100.0f);
}

// ============================================================================
// UITheme Component Constants Tests
// ============================================================================

class UIThemeComponentTest : public ::testing::Test {};

TEST_F(UIThemeComponentTest, Button_HasMinimumDimensions) {
	EXPECT_GT(UITheme::Button::MIN_WIDTH, 0.0f);
	EXPECT_GT(UITheme::Button::MIN_HEIGHT, 0.0f);
	EXPECT_GT(UITheme::Button::DEFAULT_HEIGHT, 0.0f);
}

TEST_F(UIThemeComponentTest, Button_DefaultHeightIsReasonable) {
	// Default button height should be at least the minimum
	EXPECT_GE(UITheme::Button::DEFAULT_HEIGHT, UITheme::Button::MIN_HEIGHT);
	
	// And should be touchable (at least 32px for accessibility)
	EXPECT_GE(UITheme::Button::DEFAULT_HEIGHT, 32.0f);
}

TEST_F(UIThemeComponentTest, Panel_HasMinimumDimensions) {
	EXPECT_GT(UITheme::Panel::MIN_WIDTH, 0.0f);
	EXPECT_GT(UITheme::Panel::MIN_HEIGHT, 0.0f);
}

TEST_F(UIThemeComponentTest, Slider_HasReasonableDimensions) {
	EXPECT_GT(UITheme::Slider::TRACK_HEIGHT, 0.0f);
	EXPECT_GT(UITheme::Slider::THUMB_SIZE, UITheme::Slider::TRACK_HEIGHT);
	EXPECT_GT(UITheme::Slider::MIN_WIDTH, 0.0f);
}

TEST_F(UIThemeComponentTest, Checkbox_HasReasonableSize) {
	EXPECT_GT(UITheme::Checkbox::SIZE, 0.0f);
	EXPECT_GT(UITheme::Checkbox::CHECK_MARK_THICKNESS, 0.0f);
	
	// Checkbox should be touchable (at least 16px)
	EXPECT_GE(UITheme::Checkbox::SIZE, 16.0f);
}

TEST_F(UIThemeComponentTest, Input_HasMinimumDimensions) {
	EXPECT_GT(UITheme::Input::MIN_WIDTH, 0.0f);
	EXPECT_GT(UITheme::Input::DEFAULT_HEIGHT, 0.0f);
	
	// Input should be at least 32px tall for accessibility
	EXPECT_GE(UITheme::Input::DEFAULT_HEIGHT, 32.0f);
}

// ============================================================================
// UITheme Animation Tests
// ============================================================================

class UIThemeAnimationTest : public ::testing::Test {};

TEST_F(UIThemeAnimationTest, AnimationDurations_AreOrdered) {
	EXPECT_LT(UITheme::Animation::FAST, UITheme::Animation::NORMAL);
	EXPECT_LT(UITheme::Animation::NORMAL, UITheme::Animation::SLOW);
}

TEST_F(UIThemeAnimationTest, AnimationDurations_ArePositive) {
	EXPECT_GT(UITheme::Animation::FAST, 0.0f);
	EXPECT_GT(UITheme::Animation::NORMAL, 0.0f);
	EXPECT_GT(UITheme::Animation::SLOW, 0.0f);
}

TEST_F(UIThemeAnimationTest, AnimationDurations_AreReasonable) {
	// Animations should be fast enough to feel responsive (< 1 second)
	EXPECT_LT(UITheme::Animation::FAST, 0.5f);
	EXPECT_LT(UITheme::Animation::NORMAL, 0.5f);
	EXPECT_LT(UITheme::Animation::SLOW, 0.5f);
}

// ============================================================================
// UITheme Layer Tests
// ============================================================================

class UIThemeLayerTest : public ::testing::Test {};

TEST_F(UIThemeLayerTest, Layers_AreOrdered) {
	EXPECT_LT(UITheme::Layer::BACKGROUND, UITheme::Layer::CONTENT);
	EXPECT_LT(UITheme::Layer::CONTENT, UITheme::Layer::OVERLAY);
	EXPECT_LT(UITheme::Layer::OVERLAY, UITheme::Layer::MODAL);
	EXPECT_LT(UITheme::Layer::MODAL, UITheme::Layer::TOOLTIP);
	EXPECT_LT(UITheme::Layer::TOOLTIP, UITheme::Layer::NOTIFICATION);
}

TEST_F(UIThemeLayerTest, Layers_AreNonNegative) {
	EXPECT_GE(UITheme::Layer::BACKGROUND, 0);
	EXPECT_GE(UITheme::Layer::CONTENT, 0);
	EXPECT_GE(UITheme::Layer::OVERLAY, 0);
	EXPECT_GE(UITheme::Layer::MODAL, 0);
}

TEST_F(UIThemeLayerTest, Layers_HaveProperSpacing) {
	// Layers should have enough spacing for intermediate layers if needed
	EXPECT_GE(UITheme::Layer::CONTENT - UITheme::Layer::BACKGROUND, 5);
	EXPECT_GE(UITheme::Layer::OVERLAY - UITheme::Layer::CONTENT, 5);
}

// ============================================================================
// UITheme Usage Examples Tests
// ============================================================================

class UIThemeUsageTest : public ::testing::Test {};

TEST_F(UIThemeUsageTest, CanUseThemeColorsForButton) {
	// Example: Button color based on state
	Color button_color = UITheme::Background::BUTTON;
	
	// On hover
	button_color = UITheme::Background::BUTTON_HOVER;
	EXPECT_GT(button_color.r, UITheme::Background::BUTTON.r);
	
	// On press
	button_color = UITheme::Background::BUTTON_ACTIVE;
	EXPECT_LT(button_color.r, UITheme::Background::BUTTON.r);
}

TEST_F(UIThemeUsageTest, CanUseThemePaddingForLayout) {
	// Example: Panel with theme padding
	float panel_width = 200.0f;
	float panel_height = 100.0f;
	float content_width = panel_width - 2 * UITheme::Padding::PANEL_HORIZONTAL;
	float content_height = panel_height - 2 * UITheme::Padding::PANEL_VERTICAL;
	
	EXPECT_GT(content_width, 0.0f);
	EXPECT_GT(content_height, 0.0f);
	EXPECT_LT(content_width, panel_width);
	EXPECT_LT(content_height, panel_height);
}

TEST_F(UIThemeUsageTest, CanUseThemeFontSizes) {
	// Example: Text elements with theme font sizes
	float title_size = UITheme::FontSize::HEADING_1;
	float subtitle_size = UITheme::FontSize::HEADING_3;
	float body_size = UITheme::FontSize::NORMAL;
	
	EXPECT_GT(title_size, subtitle_size);
	EXPECT_GT(subtitle_size, body_size);
}

TEST_F(UIThemeUsageTest, CanCombineThemeValues) {
	// Example: Complete button styling
	Color bg_color = UITheme::Background::BUTTON;
	Color text_color = UITheme::Text::PRIMARY;
	Color border_color = UITheme::Border::DEFAULT;
	float padding_h = UITheme::Padding::BUTTON_HORIZONTAL;
	float padding_v = UITheme::Padding::BUTTON_VERTICAL;
	float font_size = UITheme::FontSize::NORMAL;
	float border_width = UITheme::BorderSize::THIN;
	
	// All values should be valid
	EXPECT_GT(bg_color.a, 0.0f);
	EXPECT_GT(text_color.a, 0.0f);
	EXPECT_GT(border_color.a, 0.0f);
	EXPECT_GT(padding_h, 0.0f);
	EXPECT_GT(padding_v, 0.0f);
	EXPECT_GT(font_size, 0.0f);
	EXPECT_GE(border_width, 0.0f);
}
