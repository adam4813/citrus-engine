// Tests for UI anchoring/constraint components
// Tests anchor constraints and size constraints

#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::batch_renderer;
using namespace engine::ui::components;

// Simple test element
class TestElement : public UIElement {
public:
	TestElement(float x, float y, float width, float height) : UIElement(x, y, width, height) {}
	void Render() const override {}
};

// ========================================
// Anchor Tests
// ========================================

class AnchorTest : public ::testing::Test {
protected:
	std::unique_ptr<TestElement> element_;
	Rectangle parent_bounds_{0.0f, 0.0f, 400.0f, 300.0f};

	void SetUp() override {
		element_ = std::make_unique<TestElement>(0, 0, 100.0f, 50.0f);
	}
};

TEST_F(AnchorTest, FixedFromLeft) {
	Anchor anchor;
	anchor.SetLeft(20.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 20.0f);
}

TEST_F(AnchorTest, FixedFromRight) {
	Anchor anchor;
	anchor.SetRight(20.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	// x = 400 - 100 - 20 = 280
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 280.0f);
}

TEST_F(AnchorTest, FixedFromTop) {
	Anchor anchor;
	anchor.SetTop(30.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 30.0f);
}

TEST_F(AnchorTest, FixedFromBottom) {
	Anchor anchor;
	anchor.SetBottom(30.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	// y = 300 - 50 - 30 = 220
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 220.0f);
}

TEST_F(AnchorTest, StretchHorizontally) {
	Anchor anchor;
	anchor.SetLeft(10.0f);
	anchor.SetRight(20.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 10.0f);
	// width = 400 - 10 - 20 = 370
	EXPECT_FLOAT_EQ(element_->GetWidth(), 370.0f);
}

TEST_F(AnchorTest, StretchVertically) {
	Anchor anchor;
	anchor.SetTop(15.0f);
	anchor.SetBottom(25.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 15.0f);
	// height = 300 - 15 - 25 = 260
	EXPECT_FLOAT_EQ(element_->GetHeight(), 260.0f);
}

TEST_F(AnchorTest, StretchBothDirections) {
	Anchor anchor;
	anchor.SetLeft(10.0f);
	anchor.SetRight(10.0f);
	anchor.SetTop(10.0f);
	anchor.SetBottom(10.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 10.0f);
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 10.0f);
	EXPECT_FLOAT_EQ(element_->GetWidth(), 380.0f);
	EXPECT_FLOAT_EQ(element_->GetHeight(), 280.0f);
}

TEST_F(AnchorTest, TopLeftCorner) {
	auto anchor = Anchor::TopLeft(15.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 15.0f);
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 15.0f);
}

TEST_F(AnchorTest, TopRightCorner) {
	auto anchor = Anchor::TopRight(15.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	// x = 400 - 100 - 15 = 285
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 285.0f);
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 15.0f);
}

TEST_F(AnchorTest, BottomLeftCorner) {
	auto anchor = Anchor::BottomLeft(15.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 15.0f);
	// y = 300 - 50 - 15 = 235
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 235.0f);
}

TEST_F(AnchorTest, BottomRightCorner) {
	auto anchor = Anchor::BottomRight(15.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 285.0f);
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 235.0f);
}

TEST_F(AnchorTest, StretchHorizontalFactory) {
	auto anchor = Anchor::StretchHorizontal(20.0f, 30.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 20.0f);
	// width = 400 - 20 - 30 = 350
	EXPECT_FLOAT_EQ(element_->GetWidth(), 350.0f);
}

TEST_F(AnchorTest, FillFactory) {
	auto anchor = Anchor::Fill(10.0f);
	anchor.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().x, 10.0f);
	EXPECT_FLOAT_EQ(element_->GetRelativeBounds().y, 10.0f);
	EXPECT_FLOAT_EQ(element_->GetWidth(), 380.0f);
	EXPECT_FLOAT_EQ(element_->GetHeight(), 280.0f);
}

TEST_F(AnchorTest, HasAnchor_ReturnsTrueWhenSet) {
	Anchor anchor;
	EXPECT_FALSE(anchor.HasAnchor());

	anchor.SetLeft(10.0f);
	EXPECT_TRUE(anchor.HasAnchor());
}

TEST_F(AnchorTest, Clear_RemovesAllAnchors) {
	Anchor anchor;
	anchor.SetLeft(10.0f);
	anchor.SetTop(20.0f);
	EXPECT_TRUE(anchor.HasAnchor());

	anchor.Clear();
	EXPECT_FALSE(anchor.HasAnchor());
}

// ========================================
// SizeConstraint Tests
// ========================================

class SizeConstraintTest : public ::testing::Test {};

TEST_F(SizeConstraintTest, FixedSize) {
	auto constraint = SizeConstraint::Fixed(150.0f);
	float result = constraint.Calculate(400.0f, 100.0f);

	EXPECT_FLOAT_EQ(result, 150.0f);
}

TEST_F(SizeConstraintTest, PercentageSize) {
	auto constraint = SizeConstraint::Percent(0.5f);
	float result = constraint.Calculate(400.0f, 100.0f);

	EXPECT_FLOAT_EQ(result, 200.0f);  // 50% of 400
}

TEST_F(SizeConstraintTest, PercentageClamped) {
	// Percentage > 1.0 should be clamped
	auto constraint = SizeConstraint::Percent(1.5f);
	float result = constraint.Calculate(400.0f, 100.0f);

	EXPECT_FLOAT_EQ(result, 400.0f);  // Clamped to 100%
}

TEST_F(SizeConstraintTest, FitContent) {
	auto constraint = SizeConstraint::FitContent();
	float result = constraint.Calculate(400.0f, 150.0f);

	EXPECT_FLOAT_EQ(result, 150.0f);  // Uses content size
}

TEST_F(SizeConstraintTest, FitContentWithMin) {
	auto constraint = SizeConstraint::FitContent(200.0f, std::nullopt);
	float result = constraint.Calculate(400.0f, 100.0f);

	EXPECT_FLOAT_EQ(result, 200.0f);  // Min applied
}

TEST_F(SizeConstraintTest, FitContentWithMax) {
	auto constraint = SizeConstraint::FitContent(std::nullopt, 80.0f);
	float result = constraint.Calculate(400.0f, 100.0f);

	EXPECT_FLOAT_EQ(result, 80.0f);  // Max applied
}

TEST_F(SizeConstraintTest, FitContentWithMinAndMax) {
	auto constraint = SizeConstraint::FitContent(50.0f, 150.0f);

	EXPECT_FLOAT_EQ(constraint.Calculate(400.0f, 100.0f), 100.0f);  // Within range
	EXPECT_FLOAT_EQ(constraint.Calculate(400.0f, 30.0f), 50.0f);   // Below min
	EXPECT_FLOAT_EQ(constraint.Calculate(400.0f, 200.0f), 150.0f); // Above max
}

// ========================================
// SizeConstraints Tests
// ========================================

class SizeConstraintsTest : public ::testing::Test {
protected:
	std::unique_ptr<TestElement> element_;
	Rectangle parent_bounds_{0.0f, 0.0f, 400.0f, 300.0f};

	void SetUp() override {
		element_ = std::make_unique<TestElement>(0, 0, 100.0f, 50.0f);
	}
};

TEST_F(SizeConstraintsTest, FixedFactory) {
	auto constraints = SizeConstraints::Fixed(200.0f, 100.0f);
	constraints.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetWidth(), 200.0f);
	EXPECT_FLOAT_EQ(element_->GetHeight(), 100.0f);
}

TEST_F(SizeConstraintsTest, PercentFactory) {
	auto constraints = SizeConstraints::Percent(0.5f, 0.25f);
	constraints.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetWidth(), 200.0f);   // 50% of 400
	EXPECT_FLOAT_EQ(element_->GetHeight(), 75.0f);  // 25% of 300
}

TEST_F(SizeConstraintsTest, FullFactory) {
	auto constraints = SizeConstraints::Full();
	constraints.Apply(element_.get(), parent_bounds_);

	EXPECT_FLOAT_EQ(element_->GetWidth(), 400.0f);
	EXPECT_FLOAT_EQ(element_->GetHeight(), 300.0f);
}

// ========================================
// Edge Flag Tests
// ========================================

TEST(EdgeTest, BitwiseOperators) {
	Edge combined = Edge::Left | Edge::Top;
	EXPECT_TRUE(HasEdge(combined, Edge::Left));
	EXPECT_TRUE(HasEdge(combined, Edge::Top));
	EXPECT_FALSE(HasEdge(combined, Edge::Right));
	EXPECT_FALSE(HasEdge(combined, Edge::Bottom));
}

TEST(EdgeTest, PrebuiltCombinations) {
	EXPECT_TRUE(HasEdge(Edge::TopLeft, Edge::Top));
	EXPECT_TRUE(HasEdge(Edge::TopLeft, Edge::Left));

	EXPECT_TRUE(HasEdge(Edge::Horizontal, Edge::Left));
	EXPECT_TRUE(HasEdge(Edge::Horizontal, Edge::Right));
	EXPECT_FALSE(HasEdge(Edge::Horizontal, Edge::Top));

	EXPECT_TRUE(HasEdge(Edge::All, Edge::Left));
	EXPECT_TRUE(HasEdge(Edge::All, Edge::Right));
	EXPECT_TRUE(HasEdge(Edge::All, Edge::Top));
	EXPECT_TRUE(HasEdge(Edge::All, Edge::Bottom));
}

// ========================================
// Integration Tests
// ========================================

TEST(AnchorIntegrationTest, CombineAnchorAndSizeConstraints) {
	auto element = std::make_unique<TestElement>(0, 0, 100.0f, 50.0f);
	Rectangle parent_bounds{0.0f, 0.0f, 400.0f, 300.0f};

	// First apply size constraint (percentage-based)
	SizeConstraints size_constraints;
	size_constraints.width = SizeConstraint::Percent(0.5f);  // 200px
	size_constraints.height = SizeConstraint::Percent(0.3f);  // 90px
	size_constraints.Apply(element.get(), parent_bounds);

	EXPECT_FLOAT_EQ(element->GetWidth(), 200.0f);
	EXPECT_FLOAT_EQ(element->GetHeight(), 90.0f);

	// Then apply anchor (center bottom)
	auto anchor = Anchor::BottomLeft(20.0f);
	anchor.Apply(element.get(), parent_bounds);

	EXPECT_FLOAT_EQ(element->GetRelativeBounds().x, 20.0f);
	// y = 300 - 90 - 20 = 190
	EXPECT_FLOAT_EQ(element->GetRelativeBounds().y, 190.0f);
}
