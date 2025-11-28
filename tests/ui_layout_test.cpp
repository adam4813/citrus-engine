// Tests for UI layout components
// Tests the layout strategy pattern (vertical, horizontal, grid, center, etc.)

#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;
using namespace engine::ui::components;
using namespace engine::ui::elements;

// Simple test element that can be positioned
class TestElement : public UIElement {
public:
	TestElement(float x, float y, float width, float height) : UIElement(x, y, width, height) {}
	void Render() const override {}
};

// Test container with configurable size (no padding by default)
class TestContainer : public UIElement {
public:
	TestContainer(float width, float height) : UIElement(0, 0, width, height) {}
	void Render() const override {}
};

// ========================================
// VerticalLayout Tests
// ========================================

class VerticalLayoutTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(200.0f, 400.0f); }

	void CreateChildren(int count, float width = 100.0f, float height = 30.0f) {
		children_.clear();
		for (int i = 0; i < count; i++) {
			children_.push_back(std::make_unique<TestElement>(0, 0, width, height));
		}
	}
};

TEST_F(VerticalLayoutTest, AppliesVerticalStacking) {
	CreateChildren(3);
	VerticalLayout layout(0.0f);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().y, 30.0f);
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().y, 60.0f);
}

TEST_F(VerticalLayoutTest, AppliesGap) {
	CreateChildren(3);
	VerticalLayout layout(10.0f);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().y, 40.0f);  // 30 + 10
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().y, 80.0f);  // 30 + 10 + 30 + 10
}

TEST_F(VerticalLayoutTest, CentersHorizontally) {
	CreateChildren(3);
	VerticalLayout layout(0.0f, Alignment::Center);
	layout.Apply(children_, container_.get());

	// Children are 100px wide, container is 200px wide
	// Centered: x = (200 - 100) / 2 = 50
	for (const auto& child : children_) {
		EXPECT_FLOAT_EQ(child->GetRelativeBounds().x, 50.0f);
	}
}

TEST_F(VerticalLayoutTest, AlignsToEnd) {
	CreateChildren(3);
	VerticalLayout layout(0.0f, Alignment::End);
	layout.Apply(children_, container_.get());

	// Children are 100px wide, container is 200px wide
	// End: x = 200 - 100 = 100
	for (const auto& child : children_) {
		EXPECT_FLOAT_EQ(child->GetRelativeBounds().x, 100.0f);
	}
}

TEST_F(VerticalLayoutTest, StretchesWidth) {
	CreateChildren(3);
	VerticalLayout layout(0.0f, Alignment::Stretch);
	layout.Apply(children_, container_.get());

	for (const auto& child : children_) {
		EXPECT_FLOAT_EQ(child->GetWidth(), 200.0f);
		EXPECT_FLOAT_EQ(child->GetRelativeBounds().x, 0.0f);
	}
}

TEST_F(VerticalLayoutTest, SkipsInvisibleChildren) {
	CreateChildren(3);
	children_[1]->SetVisible(false);
	VerticalLayout layout(0.0f);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 0.0f);
	// children_[1] is invisible, so children_[2] follows directly
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().y, 30.0f);
}

// ========================================
// HorizontalLayout Tests
// ========================================

class HorizontalLayoutTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(400.0f, 200.0f); }

	void CreateChildren(int count, float width = 50.0f, float height = 30.0f) {
		children_.clear();
		for (int i = 0; i < count; i++) {
			children_.push_back(std::make_unique<TestElement>(0, 0, width, height));
		}
	}
};

TEST_F(HorizontalLayoutTest, AppliesHorizontalStacking) {
	CreateChildren(3);
	HorizontalLayout layout(0.0f);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 50.0f);
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().x, 100.0f);
}

TEST_F(HorizontalLayoutTest, AppliesGap) {
	CreateChildren(3);
	HorizontalLayout layout(10.0f);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 60.0f);
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().x, 120.0f);
}

TEST_F(HorizontalLayoutTest, CentersVertically) {
	CreateChildren(3);
	HorizontalLayout layout(0.0f, Alignment::Center);
	layout.Apply(children_, container_.get());

	// Children are 30px tall, container is 200px tall
	// Centered: y = (200 - 30) / 2 = 85
	for (const auto& child : children_) {
		EXPECT_FLOAT_EQ(child->GetRelativeBounds().y, 85.0f);
	}
}

TEST_F(HorizontalLayoutTest, StretchesHeight) {
	CreateChildren(3);
	HorizontalLayout layout(0.0f, Alignment::Stretch);
	layout.Apply(children_, container_.get());

	for (const auto& child : children_) {
		EXPECT_FLOAT_EQ(child->GetHeight(), 200.0f);
	}
}

// ========================================
// GridLayout Tests
// ========================================

class GridLayoutTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(300.0f, 400.0f); }

	void CreateChildren(int count, float width = 80.0f, float height = 40.0f) {
		children_.clear();
		for (int i = 0; i < count; i++) {
			children_.push_back(std::make_unique<TestElement>(0, 0, width, height));
		}
	}
};

TEST_F(GridLayoutTest, ArrangesInColumns) {
	CreateChildren(6);
	GridLayout layout(3, 0.0f, 0.0f);
	layout.Apply(children_, container_.get());

	// Row 0
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 100.0f);  // 300/3 = 100
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().x, 200.0f);

	// Row 1
	EXPECT_FLOAT_EQ(children_[3]->GetRelativeBounds().y, 40.0f);  // Next row
	EXPECT_FLOAT_EQ(children_[4]->GetRelativeBounds().x, 100.0f);
	EXPECT_FLOAT_EQ(children_[5]->GetRelativeBounds().x, 200.0f);
}

TEST_F(GridLayoutTest, AppliesGaps) {
	CreateChildren(6);
	GridLayout layout(3, 10.0f, 15.0f);  // 10px horizontal, 15px vertical gap
	layout.Apply(children_, container_.get());

	// Cell width = (300 - 2*10) / 3 = 93.33...
	EXPECT_FLOAT_EQ(children_[3]->GetRelativeBounds().y, 55.0f);  // 40 + 15
}

// ========================================
// StackLayout Center Tests
// ========================================

class StackLayoutCenterTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(200.0f, 200.0f); }
};

TEST_F(StackLayoutCenterTest, CentersChildren) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));
	StackLayout layout(Alignment::Center, Alignment::Center);
	layout.Apply(children_, container_.get());

	// Centered: x = (200 - 100) / 2 = 50, y = (200 - 50) / 2 = 75
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 50.0f);
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 75.0f);
}

TEST_F(StackLayoutCenterTest, CentersMultipleChildren) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));
	children_.push_back(std::make_unique<TestElement>(0, 0, 60.0f, 30.0f));
	StackLayout layout(Alignment::Center, Alignment::Center);
	layout.Apply(children_, container_.get());

	// Both centered
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 50.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 70.0f);  // (200 - 60) / 2
}

// ========================================
// JustifyLayout Tests
// ========================================

class JustifyLayoutTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(300.0f, 200.0f); }
};

TEST_F(JustifyLayoutTest, DistributesHorizontally) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));
	children_.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));
	children_.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));

	JustifyLayout layout(JustifyDirection::Horizontal);
	layout.Apply(children_, container_.get());

	// 3 children, 50px each = 150px total
	// 300 - 150 = 150px to distribute between 2 gaps = 75px each
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 125.0f);  // 50 + 75
	EXPECT_FLOAT_EQ(children_[2]->GetRelativeBounds().x, 250.0f); // 50 + 75 + 50 + 75
}

TEST_F(JustifyLayoutTest, SingleChildCenters) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));

	JustifyLayout layout(JustifyDirection::Horizontal);
	layout.Apply(children_, container_.get());

	// Single child should be centered
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 125.0f);  // (300 - 50) / 2
}

// ========================================
// StackLayout Tests
// ========================================

class StackLayoutTest : public ::testing::Test {
protected:
	std::vector<std::unique_ptr<UIElement>> children_;
	std::unique_ptr<TestContainer> container_;

	void SetUp() override { container_ = std::make_unique<TestContainer>(200.0f, 200.0f); }
};

TEST_F(StackLayoutTest, StacksAtCenter) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));
	children_.push_back(std::make_unique<TestElement>(0, 0, 80.0f, 40.0f));

	StackLayout layout(Alignment::Center, Alignment::Center);
	layout.Apply(children_, container_.get());

	// Both centered
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 50.0f);
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 75.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 60.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().y, 80.0f);
}

TEST_F(StackLayoutTest, StacksAtTopLeft) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));
	children_.push_back(std::make_unique<TestElement>(0, 0, 80.0f, 40.0f));

	StackLayout layout(Alignment::Start, Alignment::Start);
	layout.Apply(children_, container_.get());

	// Both at origin (no padding on TestContainer)
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().x, 0.0f);
	EXPECT_FLOAT_EQ(children_[1]->GetRelativeBounds().y, 0.0f);
}

TEST_F(StackLayoutTest, StacksAtBottomRight) {
	children_.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));

	StackLayout layout(Alignment::End, Alignment::End);
	layout.Apply(children_, container_.get());

	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().x, 100.0f);  // 200 - 100
	EXPECT_FLOAT_EQ(children_[0]->GetRelativeBounds().y, 150.0f);  // 200 - 50
}

// ========================================
// Alignment Enum Tests
// ========================================

TEST(AlignmentTest, AllValuesAreDefined) {
	EXPECT_EQ(static_cast<int>(Alignment::Start), 0);
	EXPECT_EQ(static_cast<int>(Alignment::Center), 1);
	EXPECT_EQ(static_cast<int>(Alignment::End), 2);
	EXPECT_EQ(static_cast<int>(Alignment::Stretch), 3);
}

TEST(JustifyDirectionTest, AllValuesAreDefined) {
	EXPECT_EQ(static_cast<int>(JustifyDirection::Horizontal), 0);
	EXPECT_EQ(static_cast<int>(JustifyDirection::Vertical), 1);
}

// ========================================
// Padding Tests (using Panel)
// ========================================
// These tests verify that layouts correctly handle padding from Panel

TEST(PaddingTest, VerticalLayoutAppliesPaddingOnPrimaryAxis) {
	auto panel = std::make_unique<Panel>(0, 0, 200.0f, 400.0f);
	panel->SetPadding(10.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 30.0f));
	children.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 30.0f));

	VerticalLayout layout(5.0f, Alignment::Start);
	layout.Apply(children, panel.get());

	// Primary axis (Y): starts at padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 10.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().y, 45.0f);  // 10 + 30 + 5

	// Cross axis Start: respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 10.0f);
}

TEST(PaddingTest, VerticalLayoutCenterIgnoresPaddingOnCrossAxis) {
	auto panel = std::make_unique<Panel>(0, 0, 200.0f, 400.0f);
	panel->SetPadding(10.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 100.0f, 30.0f));

	VerticalLayout layout(0.0f, Alignment::Center);
	layout.Apply(children, panel.get());

	// Cross axis Center: uses full width, ignores padding
	// x = (200 - 100) / 2 = 50
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 50.0f);

	// Primary axis: still respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 10.0f);
}

TEST(PaddingTest, HorizontalLayoutAppliesPaddingOnPrimaryAxis) {
	auto panel = std::make_unique<Panel>(0, 0, 400.0f, 200.0f);
	panel->SetPadding(15.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));

	HorizontalLayout layout(10.0f, Alignment::Start);
	layout.Apply(children, panel.get());

	// Primary axis (X): starts at padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 15.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().x, 75.0f);  // 15 + 50 + 10

	// Cross axis Start: respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 15.0f);
}

TEST(PaddingTest, HorizontalLayoutCenterIgnoresPaddingOnCrossAxis) {
	auto panel = std::make_unique<Panel>(0, 0, 400.0f, 200.0f);
	panel->SetPadding(15.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));

	HorizontalLayout layout(0.0f, Alignment::Center);
	layout.Apply(children, panel.get());

	// Cross axis Center: uses full height, ignores padding
	// y = (200 - 30) / 2 = 85
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 85.0f);

	// Primary axis: still respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 15.0f);
}

TEST(BoundsOffsetTest, HorizontalLayoutRespectsOffset) {
	// Use a panel with padding to test offset behavior
	auto panel = std::make_unique<Panel>(0, 0, 200.0f, 200.0f);
	panel->SetPadding(15.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 30.0f));

	HorizontalLayout layout(10.0f, Alignment::Start);
	layout.Apply(children, panel.get());

	// Primary axis starts at padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 15.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().x, 75.0f);  // 15 + 50 + 10

	// Cross axis Start: respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 15.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().y, 15.0f);
}

TEST(BoundsOffsetTest, GridLayoutRespectsOffset) {
	auto panel = std::make_unique<Panel>(0, 0, 220.0f, 220.0f);
	panel->SetPadding(10.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 40.0f));
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 40.0f));
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 40.0f));

	GridLayout layout(2, 5.0f, 5.0f);
	layout.Apply(children, panel.get());

	// First child at padding offset
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 10.0f);
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 10.0f);

	// Content width = 220 - 2*10 = 200
	// cell_width = (200 - 5) / 2 = 97.5
	float cell_width = (200.0f - 5.0f) / 2.0f;
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().x, 10.0f + cell_width + 5.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().y, 10.0f);

	// Third child: next row
	EXPECT_FLOAT_EQ(children[2]->GetRelativeBounds().x, 10.0f);
	EXPECT_FLOAT_EQ(children[2]->GetRelativeBounds().y, 55.0f);  // 10 + 40 + 5
}

TEST(BoundsOffsetTest, StackLayoutCenterRespectsOffset) {
	// Center alignment uses full dimensions (ignores padding)
	auto panel = std::make_unique<Panel>(0, 0, 200.0f, 200.0f);
	panel->SetPadding(20.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 60.0f, 40.0f));

	StackLayout layout(Alignment::Center, Alignment::Center);
	layout.Apply(children, panel.get());

	// Centered using full dimensions (padding ignored for center)
	// x = (200 - 60) / 2 = 70
	// y = (200 - 40) / 2 = 80
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 70.0f);
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 80.0f);
}

TEST(BoundsOffsetTest, StackLayoutRespectsOffset) {
	auto panel = std::make_unique<Panel>(0, 0, 200.0f, 200.0f);
	panel->SetPadding(10.0f);

	std::vector<std::unique_ptr<UIElement>> children;
	children.push_back(std::make_unique<TestElement>(0, 0, 50.0f, 50.0f));

	// End alignment should position at bounds.width - padding - child.width
	StackLayout layout(Alignment::End, Alignment::End);
	layout.Apply(children, panel.get());

	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 140.0f);  // 200 - 10 - 50
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 140.0f);  // 200 - 10 - 50
}

// ========================================
// LayoutComponent Integration Tests
// ========================================
// Tests that LayoutComponent correctly applies layouts to Container children
// with proper padding handling

using namespace engine::ui::elements;

TEST(LayoutComponentIntegrationTest, AppliesLayoutWithPadding) {
	// Create a container with padding
	auto container = std::make_unique<Container>(50.0f, 50.0f, 300.0f, 400.0f);
	container->SetPadding(20.0f);

	// Add children
	container->AddChild(std::make_unique<TestElement>(0, 0, 100.0f, 40.0f));
	container->AddChild(std::make_unique<TestElement>(0, 0, 100.0f, 40.0f));

	// Add layout component with Start alignment
	container->AddComponent<LayoutComponent>(std::make_unique<VerticalLayout>(10.0f, Alignment::Start));

	// Trigger layout
	container->Update();

	const auto& children = container->GetChildren();

	// Primary axis (Y): starts at padding
	// Cross axis Start: respects padding
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 20.0f);
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 20.0f);

	// Second child: y = padding + height + gap = 20 + 40 + 10 = 70
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().x, 20.0f);
	EXPECT_FLOAT_EQ(children[1]->GetRelativeBounds().y, 70.0f);

	// Absolute bounds = container position + relative position
	// (Panel::GetContentBounds no longer adds padding offset)
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().x, 70.0f);  // 50 + 20
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().y, 70.0f);  // 50 + 20
	EXPECT_FLOAT_EQ(children[1]->GetAbsoluteBounds().x, 70.0f);  // 50 + 20
	EXPECT_FLOAT_EQ(children[1]->GetAbsoluteBounds().y, 120.0f); // 50 + 70
}

TEST(LayoutComponentIntegrationTest, CentersWithinPaddedArea) {
	auto container = std::make_unique<Container>(0.0f, 0.0f, 200.0f, 200.0f);
	container->SetPadding(10.0f);

	container->AddChild(std::make_unique<TestElement>(0, 0, 80.0f, 60.0f));

	container->AddComponent<LayoutComponent>(std::make_unique<StackLayout>(Alignment::Center, Alignment::Center));
	container->Update();

	const auto& children = container->GetChildren();

	// Center uses full dimensions (ignores padding)
	// x = (200 - 80) / 2 = 60
	// y = (200 - 60) / 2 = 70
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 60.0f);
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 70.0f);

	// Absolute bounds = container position + relative position
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().x, 60.0f);
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().y, 70.0f);
}

TEST(LayoutComponentIntegrationTest, StretchUsesContentWidth) {
	auto container = std::make_unique<Container>(0.0f, 0.0f, 300.0f, 400.0f);
	container->SetPadding(25.0f);

	// Content area width is 300 - 2*25 = 250
	container->AddChild(std::make_unique<TestElement>(0, 0, 100.0f, 50.0f));

	container->AddComponent<LayoutComponent>(std::make_unique<VerticalLayout>(0.0f, Alignment::Stretch));
	container->Update();

	const auto& children = container->GetChildren();

	// Child should be stretched to content width (250px)
	EXPECT_FLOAT_EQ(children[0]->GetWidth(), 250.0f);
	// Positioned at padding offset
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().x, 25.0f);
	EXPECT_FLOAT_EQ(children[0]->GetRelativeBounds().y, 25.0f);

	// Absolute bounds = container position + relative position
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().x, 25.0f);
	EXPECT_FLOAT_EQ(children[0]->GetAbsoluteBounds().y, 25.0f);
}
