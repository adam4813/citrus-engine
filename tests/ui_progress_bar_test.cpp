#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// ProgressBar Tests
// ============================================================================

class ProgressBarTest : public ::testing::Test {
protected:
	void SetUp() override { progress_bar = std::make_unique<ProgressBar>(10, 10, 200, 20, 0.5f); }

	void TearDown() override { progress_bar.reset(); }

	std::unique_ptr<ProgressBar> progress_bar;
};

TEST_F(ProgressBarTest, Constructor_SetsInitialProgress) { EXPECT_FLOAT_EQ(progress_bar->GetProgress(), 0.5f); }

TEST_F(ProgressBarTest, Constructor_ClampsProgress) {
	auto bar1 = std::make_unique<ProgressBar>(0, 0, 100, 20, 1.5f);
	EXPECT_FLOAT_EQ(bar1->GetProgress(), 1.0f); // Clamped to max

	auto bar2 = std::make_unique<ProgressBar>(0, 0, 100, 20, -0.5f);
	EXPECT_FLOAT_EQ(bar2->GetProgress(), 0.0f); // Clamped to min
}

TEST_F(ProgressBarTest, SetProgress_UpdatesValue) {
	progress_bar->SetProgress(0.75f);
	EXPECT_FLOAT_EQ(progress_bar->GetProgress(), 0.75f);
}

TEST_F(ProgressBarTest, SetProgress_ClampsToRange) {
	progress_bar->SetProgress(1.5f);
	EXPECT_FLOAT_EQ(progress_bar->GetProgress(), 1.0f);

	progress_bar->SetProgress(-0.5f);
	EXPECT_FLOAT_EQ(progress_bar->GetProgress(), 0.0f);
}

TEST_F(ProgressBarTest, SetLabel_StoresLabel) {
	progress_bar->SetLabel("Loading...");
	EXPECT_EQ(progress_bar->GetLabel(), "Loading...");
}

TEST_F(ProgressBarTest, SetShowPercentage_TogglesDisplay) {
	EXPECT_FALSE(progress_bar->GetShowPercentage());

	progress_bar->SetShowPercentage(true);
	EXPECT_TRUE(progress_bar->GetShowPercentage());

	progress_bar->SetShowPercentage(false);
	EXPECT_FALSE(progress_bar->GetShowPercentage());
}

TEST_F(ProgressBarTest, ColorSetters_StoreColors) {
	Color track_color{0.1f, 0.2f, 0.3f, 1.0f};
	Color fill_color{0.4f, 0.5f, 0.6f, 1.0f};

	progress_bar->SetTrackColor(track_color);
	progress_bar->SetFillColor(fill_color);

	EXPECT_FLOAT_EQ(progress_bar->GetTrackColor().r, 0.1f);
	EXPECT_FLOAT_EQ(progress_bar->GetFillColor().r, 0.4f);
}

TEST_F(ProgressBarTest, LayoutConstructor_SetsZeroPosition) {
	auto bar = std::make_unique<ProgressBar>(200, 20, 0.0f);
	auto bounds = bar->GetRelativeBounds();

	EXPECT_FLOAT_EQ(bounds.x, 0.0f);
	EXPECT_FLOAT_EQ(bounds.y, 0.0f);
	EXPECT_FLOAT_EQ(bounds.width, 200.0f);
	EXPECT_FLOAT_EQ(bounds.height, 20.0f);
}
