// Tests for UI scroll component
// Tests scroll state management and geometry calculations

#include <gtest/gtest.h>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;
using namespace engine::ui::components;

// ========================================
// ScrollState Tests
// ========================================

class ScrollStateTest : public ::testing::Test {
protected:
	ScrollState scroll_;

	void SetUp() override {
		// Standard setup: content larger than viewport
		scroll_.SetContentSize(500.0f, 1000.0f);
		scroll_.SetViewportSize(200.0f, 300.0f);
	}
};

TEST_F(ScrollStateTest, InitialScrollIsZero) {
	ScrollState s;
	EXPECT_FLOAT_EQ(s.GetScrollX(), 0.0f);
	EXPECT_FLOAT_EQ(s.GetScrollY(), 0.0f);
}

TEST_F(ScrollStateTest, CanScrollWhenContentLargerThanViewport) {
	EXPECT_TRUE(scroll_.CanScrollX());
	EXPECT_TRUE(scroll_.CanScrollY());
}

TEST_F(ScrollStateTest, CannotScrollWhenContentFitsViewport) {
	ScrollState s;
	s.SetContentSize(100.0f, 200.0f);
	s.SetViewportSize(200.0f, 300.0f);

	EXPECT_FALSE(s.CanScrollX());
	EXPECT_FALSE(s.CanScrollY());
}

TEST_F(ScrollStateTest, MaxScrollIsContentMinusViewport) {
	// Content 500x1000, viewport 200x300
	EXPECT_FLOAT_EQ(scroll_.GetMaxScrollX(), 300.0f);
	EXPECT_FLOAT_EQ(scroll_.GetMaxScrollY(), 700.0f);
}

TEST_F(ScrollStateTest, ScrollByAddsToPosition) {
	scroll_.ScrollBy(50.0f, 100.0f);

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 50.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 100.0f);
}

TEST_F(ScrollStateTest, ScrollToSetsPosition) {
	scroll_.ScrollTo(150.0f, 250.0f);

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 150.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 250.0f);
}

TEST_F(ScrollStateTest, ScrollClampsToMinimum) {
	scroll_.SetScroll(-100.0f, -100.0f);

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 0.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 0.0f);
}

TEST_F(ScrollStateTest, ScrollClampsToMaximum) {
	scroll_.SetScroll(1000.0f, 2000.0f);

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 300.0f);   // Max X
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 700.0f);  // Max Y
}

TEST_F(ScrollStateTest, ScrollToStartResetsToZero) {
	scroll_.ScrollTo(150.0f, 250.0f);
	scroll_.ScrollToStart();

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 0.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 0.0f);
}

TEST_F(ScrollStateTest, ScrollToEndGoesToMax) {
	scroll_.ScrollToEnd();

	EXPECT_FLOAT_EQ(scroll_.GetScrollX(), 300.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 700.0f);
}

TEST_F(ScrollStateTest, NormalizedScrollIsZeroAtStart) {
	EXPECT_FLOAT_EQ(scroll_.GetScrollXNormalized(), 0.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollYNormalized(), 0.0f);
}

TEST_F(ScrollStateTest, NormalizedScrollIsOneAtEnd) {
	scroll_.ScrollToEnd();

	EXPECT_FLOAT_EQ(scroll_.GetScrollXNormalized(), 1.0f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollYNormalized(), 1.0f);
}

TEST_F(ScrollStateTest, NormalizedScrollIsMiddle) {
	scroll_.SetScroll(150.0f, 350.0f);  // Half of max

	EXPECT_FLOAT_EQ(scroll_.GetScrollXNormalized(), 0.5f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollYNormalized(), 0.5f);
}

TEST_F(ScrollStateTest, ThumbRatioIsViewportDividedByContent) {
	// 200/500 = 0.4, 300/1000 = 0.3
	EXPECT_FLOAT_EQ(scroll_.GetScrollXThumbRatio(), 0.4f);
	EXPECT_FLOAT_EQ(scroll_.GetScrollYThumbRatio(), 0.3f);
}

TEST_F(ScrollStateTest, ThumbRatioIsOneWhenContentFitsViewport) {
	ScrollState s;
	s.SetContentSize(100.0f, 200.0f);
	s.SetViewportSize(200.0f, 300.0f);

	EXPECT_FLOAT_EQ(s.GetScrollXThumbRatio(), 1.0f);
	EXPECT_FLOAT_EQ(s.GetScrollYThumbRatio(), 1.0f);
}

TEST_F(ScrollStateTest, HandleScrollVertical) {
	scroll_.SetDirection(ScrollDirection::Vertical);

	MouseEvent event;
	event.scroll_delta_y = -2.0f;  // Scroll down

	bool handled = scroll_.HandleScroll(event);

	EXPECT_TRUE(handled);
	EXPECT_GT(scroll_.GetScrollY(), 0.0f);
}

TEST_F(ScrollStateTest, HandleScrollHorizontal) {
	scroll_.SetDirection(ScrollDirection::Horizontal);

	MouseEvent event;
	event.scroll_delta_x = -2.0f;

	bool handled = scroll_.HandleScroll(event);

	EXPECT_TRUE(handled);
	EXPECT_GT(scroll_.GetScrollX(), 0.0f);
}

TEST_F(ScrollStateTest, HandleScrollNoContentReturnsFalse) {
	ScrollState s;
	s.SetContentSize(100.0f, 100.0f);
	s.SetViewportSize(200.0f, 200.0f);  // Content fits
	s.SetDirection(ScrollDirection::Vertical);

	MouseEvent event;
	event.scroll_delta_y = -2.0f;

	bool handled = s.HandleScroll(event);

	EXPECT_FALSE(handled);
}

TEST_F(ScrollStateTest, ScrollSpeedAffectsScrollAmount) {
	scroll_.SetDirection(ScrollDirection::Vertical);
	scroll_.SetScrollSpeed(100.0f);

	MouseEvent event;
	event.scroll_delta_y = -1.0f;

	scroll_.HandleScroll(event);

	// With speed 100, delta -1, scroll should increase by 100
	EXPECT_FLOAT_EQ(scroll_.GetScrollY(), 100.0f);
}

// ========================================
// ScrollDirection Tests
// ========================================

TEST(ScrollDirectionTest, AllValuesAreDefined) {
	EXPECT_EQ(static_cast<int>(ScrollDirection::Vertical), 0);
	EXPECT_EQ(static_cast<int>(ScrollDirection::Horizontal), 1);
	EXPECT_EQ(static_cast<int>(ScrollDirection::Both), 2);
}

// ========================================
// ScrollbarStyle Tests
// ========================================

TEST(ScrollbarStyleTest, DefaultValues) {
	ScrollbarStyle style;

	EXPECT_FLOAT_EQ(style.width, 8.0f);
	EXPECT_FLOAT_EQ(style.min_thumb_length, 20.0f);
	EXPECT_TRUE(style.show_track);
}

// ========================================
// ScrollbarGeometry Tests
// ========================================

class ScrollbarGeometryTest : public ::testing::Test {
protected:
	ScrollState scroll_;
	Rectangle viewport_{50.0f, 50.0f, 200.0f, 300.0f};
	ScrollbarStyle style_;

	void SetUp() override {
		scroll_.SetContentSize(200.0f, 1000.0f);
		scroll_.SetViewportSize(200.0f, 300.0f);
	}
};

TEST_F(ScrollbarGeometryTest, VerticalThumbAtTop) {
	scroll_.SetScroll(0.0f, 0.0f);
	auto thumb = ScrollbarGeometry::CalculateVerticalThumb(scroll_, viewport_, style_);

	EXPECT_FLOAT_EQ(thumb.x, viewport_.x + viewport_.width - style_.width);  // 50 + 200 - 8 = 242
	EXPECT_FLOAT_EQ(thumb.y, viewport_.y);  // At top
}

TEST_F(ScrollbarGeometryTest, VerticalThumbAtBottom) {
	scroll_.ScrollToEnd();
	auto thumb = ScrollbarGeometry::CalculateVerticalThumb(scroll_, viewport_, style_);

	// Thumb height = viewport * thumb_ratio = 300 * 0.3 = 90
	// Available height = 300 - 90 = 210
	// At end, thumb_y = 50 + 210 = 260
	float expected_y = viewport_.y + (viewport_.height - viewport_.height * scroll_.GetScrollYThumbRatio());
	EXPECT_FLOAT_EQ(thumb.y, expected_y);
}

TEST_F(ScrollbarGeometryTest, VerticalThumbSize) {
	auto thumb = ScrollbarGeometry::CalculateVerticalThumb(scroll_, viewport_, style_);

	// Thumb height = viewport * ratio = 300 * 0.3 = 90
	EXPECT_FLOAT_EQ(thumb.height, 90.0f);
	EXPECT_FLOAT_EQ(thumb.width, style_.width);
}

TEST_F(ScrollbarGeometryTest, VerticalTrackCoversFullHeight) {
	auto track = ScrollbarGeometry::CalculateVerticalTrack(viewport_, style_);

	EXPECT_FLOAT_EQ(track.x, viewport_.x + viewport_.width - style_.width);
	EXPECT_FLOAT_EQ(track.y, viewport_.y);
	EXPECT_FLOAT_EQ(track.width, style_.width);
	EXPECT_FLOAT_EQ(track.height, viewport_.height);
}

TEST_F(ScrollbarGeometryTest, HorizontalScrollbarWhenNeeded) {
	ScrollState h_scroll;
	h_scroll.SetContentSize(500.0f, 200.0f);
	h_scroll.SetViewportSize(200.0f, 200.0f);

	auto thumb = ScrollbarGeometry::CalculateHorizontalThumb(h_scroll, viewport_, style_);

	EXPECT_FLOAT_EQ(thumb.y, viewport_.y + viewport_.height - style_.width);  // At bottom
	EXPECT_GT(thumb.width, 0.0f);
}

TEST_F(ScrollbarGeometryTest, MinThumbLength) {
	// Very small content ratio
	ScrollState small;
	small.SetContentSize(200.0f, 10000.0f);  // Huge content
	small.SetViewportSize(200.0f, 300.0f);

	auto thumb = ScrollbarGeometry::CalculateVerticalThumb(small, viewport_, style_);

	// Thumb should be at least min_thumb_length
	EXPECT_GE(thumb.height, style_.min_thumb_length);
}

TEST_F(ScrollbarGeometryTest, NoThumbWhenContentFits) {
	ScrollState small;
	small.SetContentSize(100.0f, 200.0f);
	small.SetViewportSize(200.0f, 300.0f);

	auto thumb = ScrollbarGeometry::CalculateVerticalThumb(small, viewport_, style_);

	// Returns empty rectangle when no scroll needed
	EXPECT_FLOAT_EQ(thumb.width, 0.0f);
	EXPECT_FLOAT_EQ(thumb.height, 0.0f);
}
