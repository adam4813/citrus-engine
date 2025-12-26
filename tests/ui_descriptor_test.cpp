#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::descriptor;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// Descriptor Creation Tests
// ============================================================================

class UIDescriptorTest : public ::testing::Test {
protected:
	void SetUp() override {}
	void TearDown() override {}
};

// === ButtonDescriptor Tests ===

TEST_F(UIDescriptorTest, ButtonDescriptor_DefaultValues) {
	ButtonDescriptor desc;

	EXPECT_FLOAT_EQ(desc.bounds.x, 0.0f);
	EXPECT_FLOAT_EQ(desc.bounds.y, 0.0f);
	EXPECT_FLOAT_EQ(desc.bounds.width, 100.0f);
	EXPECT_FLOAT_EQ(desc.bounds.height, 100.0f);
	EXPECT_TRUE(desc.label.empty());
	EXPECT_TRUE(desc.enabled);
	EXPECT_TRUE(desc.visible);
}

TEST_F(UIDescriptorTest, ButtonDescriptor_DesignatedInitializers) {
	auto desc = ButtonDescriptor{
		.bounds = {10, 20, 120, 40},
		.label = "Test Button",
		.text_style = {.font_size = 18.0f, .color = Colors::GOLD},
		.enabled = false
	};

	EXPECT_FLOAT_EQ(desc.bounds.x, 10.0f);
	EXPECT_FLOAT_EQ(desc.bounds.y, 20.0f);
	EXPECT_FLOAT_EQ(desc.bounds.width, 120.0f);
	EXPECT_FLOAT_EQ(desc.bounds.height, 40.0f);
	EXPECT_EQ(desc.label, "Test Button");
	EXPECT_FLOAT_EQ(desc.text_style.font_size, 18.0f);
	EXPECT_FALSE(desc.enabled);
	EXPECT_TRUE(desc.visible);
}

// === PanelDescriptor Tests ===

TEST_F(UIDescriptorTest, PanelDescriptor_DefaultValues) {
	PanelDescriptor desc;

	EXPECT_FLOAT_EQ(desc.padding, 0.0f);
	EXPECT_FLOAT_EQ(desc.opacity, 1.0f);
	EXPECT_FALSE(desc.clip_children);
	EXPECT_TRUE(desc.visible);
}

TEST_F(UIDescriptorTest, PanelDescriptor_DesignatedInitializers) {
	auto desc = PanelDescriptor{
		.bounds = {0, 0, 400, 300},
		.background = Colors::DARK_GRAY,
		.border = {.width = 2.0f, .color = Colors::GOLD},
		.padding = 10.0f,
		.clip_children = true
	};

	EXPECT_FLOAT_EQ(desc.bounds.width, 400.0f);
	EXPECT_FLOAT_EQ(desc.bounds.height, 300.0f);
	EXPECT_FLOAT_EQ(desc.border.width, 2.0f);
	EXPECT_FLOAT_EQ(desc.padding, 10.0f);
	EXPECT_TRUE(desc.clip_children);
}

// === SliderDescriptor Tests ===

TEST_F(UIDescriptorTest, SliderDescriptor_DefaultValues) {
	SliderDescriptor desc;

	EXPECT_FLOAT_EQ(desc.min_value, 0.0f);
	EXPECT_FLOAT_EQ(desc.max_value, 1.0f);
	EXPECT_FLOAT_EQ(desc.initial_value, 0.0f);
	EXPECT_TRUE(desc.visible);
}

TEST_F(UIDescriptorTest, SliderDescriptor_DesignatedInitializers) {
	auto desc = SliderDescriptor{
		.bounds = {10, 100, 200, 30},
		.min_value = 0.0f,
		.max_value = 100.0f,
		.initial_value = 50.0f,
		.label = "Volume"
	};

	EXPECT_FLOAT_EQ(desc.min_value, 0.0f);
	EXPECT_FLOAT_EQ(desc.max_value, 100.0f);
	EXPECT_FLOAT_EQ(desc.initial_value, 50.0f);
	EXPECT_EQ(desc.label, "Volume");
}

// === CheckboxDescriptor Tests ===

TEST_F(UIDescriptorTest, CheckboxDescriptor_DefaultValues) {
	CheckboxDescriptor desc;

	EXPECT_FALSE(desc.initial_checked);
	EXPECT_TRUE(desc.enabled);
	EXPECT_TRUE(desc.visible);
}

TEST_F(UIDescriptorTest, CheckboxDescriptor_DesignatedInitializers) {
	auto desc = CheckboxDescriptor{
		.bounds = {10, 50, 150, 24},
		.label = "Enable Feature",
		.initial_checked = true
	};

	EXPECT_EQ(desc.label, "Enable Feature");
	EXPECT_TRUE(desc.initial_checked);
}

// === ContainerDescriptor Tests ===

TEST_F(UIDescriptorTest, ContainerDescriptor_WithChildren) {
	auto desc = ContainerDescriptor{
		.bounds = {100, 100, 300, 400},
		.padding = 10.0f,
		.children = {
			LabelDescriptor{
				.bounds = {0, 0, 200, 24},
				.text = "Title"
			},
			ButtonDescriptor{
				.bounds = {0, 40, 100, 30},
				.label = "OK"
			}
		}
	};

	EXPECT_FLOAT_EQ(desc.bounds.x, 100.0f);
	EXPECT_FLOAT_EQ(desc.bounds.y, 100.0f);
	EXPECT_FLOAT_EQ(desc.padding, 10.0f);
	EXPECT_EQ(desc.children.size(), 2);
}

// === Bounds Helper Tests ===

TEST_F(UIDescriptorTest, Bounds_FromPositionAndSize) {
	Position pos{10, 20};
	Size size{100, 50};

	auto bounds = Bounds::From(pos, size);

	EXPECT_FLOAT_EQ(bounds.x, 10.0f);
	EXPECT_FLOAT_EQ(bounds.y, 20.0f);
	EXPECT_FLOAT_EQ(bounds.width, 100.0f);
	EXPECT_FLOAT_EQ(bounds.height, 50.0f);
}

TEST_F(UIDescriptorTest, Bounds_WithSize) {
	auto bounds = Bounds::WithSize(200, 100);

	EXPECT_FLOAT_EQ(bounds.x, 0.0f);
	EXPECT_FLOAT_EQ(bounds.y, 0.0f);
	EXPECT_FLOAT_EQ(bounds.width, 200.0f);
	EXPECT_FLOAT_EQ(bounds.height, 100.0f);
}

// ============================================================================
// UIFactory Tests
// ============================================================================

class UIFactoryTest : public ::testing::Test {
protected:
	void SetUp() override {}
	void TearDown() override {}
};

// === Button Factory Tests ===

TEST_F(UIFactoryTest, CreateButton_FromDescriptor) {
	auto desc = ButtonDescriptor{
		.bounds = {10, 20, 120, 40},
		.label = "Click Me",
		.text_style = {.font_size = 18.0f}
	};

	auto button = UIFactory::Create(desc);

	ASSERT_NE(button, nullptr);
	EXPECT_FLOAT_EQ(button->GetRelativeX(), 10.0f);
	EXPECT_FLOAT_EQ(button->GetRelativeY(), 20.0f);
	EXPECT_FLOAT_EQ(button->GetWidth(), 120.0f);
	EXPECT_FLOAT_EQ(button->GetHeight(), 40.0f);
	EXPECT_EQ(button->GetLabel(), "Click Me");
}

TEST_F(UIFactoryTest, CreateButton_WithCallback) {
	bool callback_fired = false;

	auto desc = ButtonDescriptor{
		.bounds = {0, 0, 100, 30},
		.label = "Test",
		.on_click = [&callback_fired](const MouseEvent&) {
			callback_fired = true;
			return true;
		}
	};

	auto button = UIFactory::Create(desc);

	// Simulate click inside bounds
	MouseEvent event{50, 15, false, false, true, false, 0.0f};
	button->OnClick(event);

	EXPECT_TRUE(callback_fired);
}

// === Panel Factory Tests ===

TEST_F(UIFactoryTest, CreatePanel_FromDescriptor) {
	auto desc = PanelDescriptor{
		.bounds = {0, 0, 400, 300},
		.padding = 10.0f,
		.clip_children = true
	};

	auto panel = UIFactory::Create(desc);

	ASSERT_NE(panel, nullptr);
	EXPECT_FLOAT_EQ(panel->GetWidth(), 400.0f);
	EXPECT_FLOAT_EQ(panel->GetHeight(), 300.0f);
	EXPECT_FLOAT_EQ(panel->GetPadding(), 10.0f);
	EXPECT_TRUE(panel->GetClipChildren());
}

// === Label Factory Tests ===

TEST_F(UIFactoryTest, CreateLabel_FromDescriptor) {
	auto desc = LabelDescriptor{
		.bounds = {10, 10, 0, 0},  // Size auto-calculated
		.text = "Hello World",
		.style = {.font_size = 16.0f, .color = Colors::GOLD}
	};

	auto label = UIFactory::Create(desc);

	ASSERT_NE(label, nullptr);
	EXPECT_FLOAT_EQ(label->GetRelativeX(), 10.0f);
	EXPECT_FLOAT_EQ(label->GetRelativeY(), 10.0f);
	EXPECT_EQ(label->GetText(), "Hello World");
}

// === Slider Factory Tests ===

TEST_F(UIFactoryTest, CreateSlider_FromDescriptor) {
	auto desc = SliderDescriptor{
		.bounds = {10, 100, 200, 30},
		.min_value = 0.0f,
		.max_value = 100.0f,
		.initial_value = 50.0f
	};

	auto slider = UIFactory::Create(desc);

	ASSERT_NE(slider, nullptr);
	EXPECT_FLOAT_EQ(slider->GetWidth(), 200.0f);
	EXPECT_FLOAT_EQ(slider->GetMinValue(), 0.0f);
	EXPECT_FLOAT_EQ(slider->GetMaxValue(), 100.0f);
	EXPECT_FLOAT_EQ(slider->GetValue(), 50.0f);
}

// === Checkbox Factory Tests ===

TEST_F(UIFactoryTest, CreateCheckbox_FromDescriptor) {
	auto desc = CheckboxDescriptor{
		.label = "Enable Feature",
		.initial_checked = true
	};

	auto checkbox = UIFactory::Create(desc);

	ASSERT_NE(checkbox, nullptr);
	EXPECT_EQ(checkbox->GetLabel(), "Enable Feature");
	EXPECT_TRUE(checkbox->IsChecked());
}

// === Container Factory Tests ===

TEST_F(UIFactoryTest, CreateContainer_WithChildren) {
	auto desc = ContainerDescriptor{
		.bounds = {100, 100, 300, 400},
		.padding = 10.0f,
		.children = {
			LabelDescriptor{
				.bounds = {0, 0, 200, 24},
				.text = "Title"
			},
			ButtonDescriptor{
				.bounds = {0, 40, 100, 30},
				.label = "OK"
			}
		}
	};

	auto container = UIFactory::Create(desc);

	ASSERT_NE(container, nullptr);
	EXPECT_FLOAT_EQ(container->GetWidth(), 300.0f);
	EXPECT_FLOAT_EQ(container->GetHeight(), 400.0f);
	EXPECT_EQ(container->GetChildren().size(), 2);
}

TEST_F(UIFactoryTest, CreateContainer_NestedContainers) {
	auto desc = ContainerDescriptor{
		.bounds = {0, 0, 800, 600},
		.children = {
			PanelDescriptor{
				.bounds = {10, 10, 200, 580}
			},
			PanelDescriptor{
				.bounds = {220, 10, 570, 580}
			}
		}
	};

	auto container = UIFactory::Create(desc);

	ASSERT_NE(container, nullptr);
	EXPECT_EQ(container->GetChildren().size(), 2);
}

// === Variant Factory Tests ===

TEST_F(UIFactoryTest, CreateFromVariant_Button) {
	UIDescriptorVariant variant = ButtonDescriptor{
		.bounds = {0, 0, 100, 30},
		.label = "Test"
	};

	auto element = UIFactory::CreateFromVariant(variant);

	ASSERT_NE(element, nullptr);
	// Element should be a button (we can't easily check type, but we can check it exists)
}

TEST_F(UIFactoryTest, CreateFromVariant_Label) {
	UIDescriptorVariant variant = LabelDescriptor{
		.bounds = {0, 0, 100, 20},
		.text = "Label"
	};

	auto element = UIFactory::CreateFromVariant(variant);

	ASSERT_NE(element, nullptr);
}

TEST_F(UIFactoryTest, CreateFromCompleteVariant_Container) {
	CompleteUIDescriptor variant = ContainerDescriptor{
		.bounds = {0, 0, 400, 300},
		.children = {
			ButtonDescriptor{.bounds = {0, 0, 100, 30}, .label = "Button"}
		}
	};

	auto element = UIFactory::CreateFromVariant(variant);

	ASSERT_NE(element, nullptr);
}
