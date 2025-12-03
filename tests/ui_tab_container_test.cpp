#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// TabContainer Tests
// ============================================================================

class TabContainerTest : public ::testing::Test {
protected:
	void SetUp() override { tab_container = std::make_unique<TabContainer>(10, 10, 400, 300); }

	void TearDown() override { tab_container.reset(); }

	std::unique_ptr<TabContainer> tab_container;
};

TEST_F(TabContainerTest, Constructor_InitializesEmpty) { EXPECT_EQ(tab_container->GetTabCount(), 0); }

TEST_F(TabContainerTest, AddTab_IncreasesCount) {
	auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel));

	EXPECT_EQ(tab_container->GetTabCount(), 1);
}

TEST_F(TabContainerTest, AddTab_ReturnsCorrectIndex) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	auto panel2 = std::make_unique<Panel>(0, 0, 380, 250);

	size_t index1 = tab_container->AddTab("Tab 1", std::move(panel1));
	size_t index2 = tab_container->AddTab("Tab 2", std::move(panel2));

	EXPECT_EQ(index1, 0);
	EXPECT_EQ(index2, 1);
}

TEST_F(TabContainerTest, GetTabLabel_ReturnsCorrectLabel) {
	auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Settings", std::move(panel));

	EXPECT_EQ(tab_container->GetTabLabel(0), "Settings");
}

TEST_F(TabContainerTest, GetTabLabel_ReturnsEmptyForInvalidIndex) {
	EXPECT_EQ(tab_container->GetTabLabel(0), "");
	EXPECT_EQ(tab_container->GetTabLabel(100), "");
}

TEST_F(TabContainerTest, AddTab_FirstTabBecomesActive) {
	auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel));

	EXPECT_EQ(tab_container->GetActiveTab(), 0);
}

TEST_F(TabContainerTest, SetActiveTab_ChangesActiveTab) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	auto panel2 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));
	tab_container->AddTab("Tab 2", std::move(panel2));

	tab_container->SetActiveTab(1);

	EXPECT_EQ(tab_container->GetActiveTab(), 1);
}

TEST_F(TabContainerTest, SetActiveTab_IgnoresInvalidIndex) {
	auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel));

	tab_container->SetActiveTab(100); // Invalid

	EXPECT_EQ(tab_container->GetActiveTab(), 0); // Unchanged
}

TEST_F(TabContainerTest, RemoveTab_DecreasesCount) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	auto panel2 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));
	tab_container->AddTab("Tab 2", std::move(panel2));

	bool removed = tab_container->RemoveTab(0);

	EXPECT_TRUE(removed);
	EXPECT_EQ(tab_container->GetTabCount(), 1);
}

TEST_F(TabContainerTest, RemoveTab_ReturnsFalseForInvalidIndex) {
	bool removed = tab_container->RemoveTab(0);

	EXPECT_FALSE(removed);
}

TEST_F(TabContainerTest, RemoveTab_AdjustsActiveTabIndex) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	auto panel2 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));
	tab_container->AddTab("Tab 2", std::move(panel2));
	tab_container->SetActiveTab(1);

	tab_container->RemoveTab(1);

	EXPECT_EQ(tab_container->GetActiveTab(), 0); // Adjusted to last valid
}

TEST_F(TabContainerTest, TabChangedCallback_TriggersOnSetActiveTab) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	auto panel2 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));
	tab_container->AddTab("Tab 2", std::move(panel2));

	bool callback_triggered = false;
	size_t callback_index = 0;
	std::string callback_label;

	tab_container->SetTabChangedCallback([&](size_t index, const std::string& label) {
		callback_triggered = true;
		callback_index = index;
		callback_label = label;
	});

	tab_container->SetActiveTab(1);

	EXPECT_TRUE(callback_triggered);
	EXPECT_EQ(callback_index, 1);
	EXPECT_EQ(callback_label, "Tab 2");
}

TEST_F(TabContainerTest, LayoutConstructor_SetsZeroPosition) {
	auto tabs = std::make_unique<TabContainer>(400, 300);
	auto bounds = tabs->GetRelativeBounds();

	EXPECT_FLOAT_EQ(bounds.x, 0.0f);
	EXPECT_FLOAT_EQ(bounds.y, 0.0f);
	EXPECT_FLOAT_EQ(bounds.width, 400.0f);
	EXPECT_FLOAT_EQ(bounds.height, 300.0f);
}

TEST_F(TabContainerTest, TabChangedCallback_TriggersOnFirstTabAdded) {
	bool callback_triggered = false;
	size_t callback_index = 99;
	std::string callback_label;

	tab_container->SetTabChangedCallback([&](size_t index, const std::string& label) {
		callback_triggered = true;
		callback_index = index;
		callback_label = label;
	});

	auto panel = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("First Tab", std::move(panel));

	EXPECT_TRUE(callback_triggered);
	EXPECT_EQ(callback_index, 0);
	EXPECT_EQ(callback_label, "First Tab");
}

TEST_F(TabContainerTest, SetActiveTab_IgnoresSameTabWithoutForce) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));

	int callback_count = 0;
	tab_container->SetTabChangedCallback([&](size_t, const std::string&) { callback_count++; });

	// First call counts (from AddTab setting first tab active)
	// Reset counter after setup
	callback_count = 0;

	tab_container->SetActiveTab(0);  // Already active, should not trigger

	EXPECT_EQ(callback_count, 0);
}

TEST_F(TabContainerTest, SetActiveTab_TriggersCallbackWithForce) {
	auto panel1 = std::make_unique<Panel>(0, 0, 380, 250);
	tab_container->AddTab("Tab 1", std::move(panel1));

	int callback_count = 0;
	tab_container->SetTabChangedCallback([&](size_t, const std::string&) { callback_count++; });

	// Reset counter after setup
	callback_count = 0;

	tab_container->SetActiveTab(0, true);  // Force trigger even though already active

	EXPECT_EQ(callback_count, 1);
}

TEST_F(TabContainerTest, GetContentHeight_ComputesDynamically) {
	auto tabs = std::make_unique<TabContainer>(400, 300);

	// Default tab bar height is 30, so content height should be 270
	EXPECT_FLOAT_EQ(tabs->GetContentHeight(), 270.0f);

	tabs->SetTabBarHeight(50.0f);
	EXPECT_FLOAT_EQ(tabs->GetContentHeight(), 250.0f);
}
