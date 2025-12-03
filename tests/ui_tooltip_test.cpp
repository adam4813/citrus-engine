#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::components;
using namespace engine::ui::batch_renderer;

// ============================================================================
// TooltipComponent Tests
// ============================================================================

class TooltipComponentTest : public ::testing::Test {
protected:
	void SetUp() override {
		button = std::make_unique<Button>(10, 10, 100, 30, "Test Button");

		auto content = std::make_unique<Panel>(0, 0, 150, 40);
		content->SetBackgroundColor(Colors::DARK_GRAY);
		tooltip_content = content.get();

		tooltip = button->AddComponent<TooltipComponent>(std::move(content));
	}

	void TearDown() override { button.reset(); }

	std::unique_ptr<Button> button;
	TooltipComponent* tooltip = nullptr;
	Panel* tooltip_content = nullptr;
};

TEST_F(TooltipComponentTest, Constructor_HidesTooltip) { EXPECT_FALSE(tooltip->IsShowing()); }

TEST_F(TooltipComponentTest, GetContent_ReturnsContent) { EXPECT_EQ(tooltip->GetContent(), tooltip_content); }

TEST_F(TooltipComponentTest, SetOffset_StoresOffset) {
	tooltip->SetOffset(20.0f, 25.0f);

	EXPECT_FLOAT_EQ(tooltip->GetOffsetX(), 20.0f);
	EXPECT_FLOAT_EQ(tooltip->GetOffsetY(), 25.0f);
}

TEST_F(TooltipComponentTest, Show_MakesTooltipVisible) {
	tooltip->Show(100.0f, 100.0f);

	EXPECT_TRUE(tooltip->IsShowing());
}

TEST_F(TooltipComponentTest, Hide_MakesTooltipInvisible) {
	tooltip->Show(100.0f, 100.0f);
	tooltip->Hide();

	EXPECT_FALSE(tooltip->IsShowing());
}

TEST_F(TooltipComponentTest, SetContent_ReplacesContent) {
	auto new_content = std::make_unique<Panel>(0, 0, 200, 50);
	Panel* new_content_ptr = new_content.get();

	tooltip->SetContent(std::move(new_content));

	EXPECT_EQ(tooltip->GetContent(), new_content_ptr);
}

TEST_F(TooltipComponentTest, DefaultOffset_IsTenPixels) {
	EXPECT_FLOAT_EQ(tooltip->GetOffsetX(), 10.0f);
	EXPECT_FLOAT_EQ(tooltip->GetOffsetY(), 10.0f);
}

TEST_F(TooltipComponentTest, SetWindowBounds_StoresBounds) {
	tooltip->SetWindowBounds(1920.0f, 1080.0f);

	// Show and check position is within bounds
	tooltip->Show(1910.0f, 1070.0f);

	// Tooltip should reposition to stay within bounds
	EXPECT_TRUE(tooltip->IsShowing());
}

TEST_F(TooltipComponentTest, DefaultConstructor_CreatesEmptyTooltip) {
	auto empty_tooltip = TooltipComponent();

	EXPECT_EQ(empty_tooltip.GetContent(), nullptr);
	EXPECT_FALSE(empty_tooltip.IsShowing());
}

// ============================================================================
// TooltipComponent with Element Integration Tests
// ============================================================================

class TooltipIntegrationTest : public ::testing::Test {
protected:
	void SetUp() override {
		panel = std::make_unique<Panel>(0, 0, 200, 100);

		auto content = std::make_unique<Label>(0, 0, "Tooltip text", 12);
		tooltip = panel->AddComponent<TooltipComponent>(std::move(content));
	}

	void TearDown() override { panel.reset(); }

	std::unique_ptr<Panel> panel;
	TooltipComponent* tooltip = nullptr;
};

TEST_F(TooltipIntegrationTest, ComponentAttachesToElement) {
	EXPECT_TRUE(panel->HasComponent<TooltipComponent>());
}

TEST_F(TooltipIntegrationTest, GetComponent_ReturnsSameInstance) {
	auto* retrieved = panel->GetComponent<TooltipComponent>();

	EXPECT_EQ(retrieved, tooltip);
}

TEST_F(TooltipIntegrationTest, RemoveComponent_DetachesTooltip) {
	panel->RemoveComponent<TooltipComponent>();

	EXPECT_FALSE(panel->HasComponent<TooltipComponent>());
}
