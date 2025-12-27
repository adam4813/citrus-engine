#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

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

// ============================================================================
// UIJsonSerializer Tests
// ============================================================================

class UIJsonSerializerTest : public ::testing::Test {
protected:
	void SetUp() override {}
	void TearDown() override {}
};

// === Button JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, ButtonDescriptor_SerializeDeserialize) {
	auto desc = ButtonDescriptor{
		.bounds = {10, 20, 120, 40},
		.label = "Test Button",
		.text_style = {.font_size = 18.0f},
		.enabled = false
	};

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<ButtonDescriptor>(json);

	EXPECT_FLOAT_EQ(restored.bounds.x, 10.0f);
	EXPECT_FLOAT_EQ(restored.bounds.y, 20.0f);
	EXPECT_FLOAT_EQ(restored.bounds.width, 120.0f);
	EXPECT_FLOAT_EQ(restored.bounds.height, 40.0f);
	EXPECT_EQ(restored.label, "Test Button");
	EXPECT_FLOAT_EQ(restored.text_style.font_size, 18.0f);
	EXPECT_FALSE(restored.enabled);
}

// === Panel JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, PanelDescriptor_SerializeDeserialize) {
	auto desc = PanelDescriptor{
		.bounds = {0, 0, 400, 300},
		.padding = 15.0f,
		.opacity = 0.8f,
		.clip_children = true
	};

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<PanelDescriptor>(json);

	EXPECT_FLOAT_EQ(restored.bounds.width, 400.0f);
	EXPECT_FLOAT_EQ(restored.padding, 15.0f);
	EXPECT_FLOAT_EQ(restored.opacity, 0.8f);
	EXPECT_TRUE(restored.clip_children);
}

// === Label JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, LabelDescriptor_SerializeDeserialize) {
	auto desc = LabelDescriptor{
		.bounds = {10, 10, 200, 24},
		.text = "Hello World",
		.style = {.font_size = 20.0f}
	};

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<LabelDescriptor>(json);

	EXPECT_EQ(restored.text, "Hello World");
	EXPECT_FLOAT_EQ(restored.style.font_size, 20.0f);
}

// === Slider JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, SliderDescriptor_SerializeDeserialize) {
	auto desc = SliderDescriptor{
		.bounds = {10, 100, 200, 30},
		.min_value = 0.0f,
		.max_value = 100.0f,
		.initial_value = 75.0f,
		.label = "Volume"
	};

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<SliderDescriptor>(json);

	EXPECT_FLOAT_EQ(restored.min_value, 0.0f);
	EXPECT_FLOAT_EQ(restored.max_value, 100.0f);
	EXPECT_FLOAT_EQ(restored.initial_value, 75.0f);
	EXPECT_EQ(restored.label, "Volume");
}

// === Checkbox JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, CheckboxDescriptor_SerializeDeserialize) {
	auto desc = CheckboxDescriptor{
		.label = "Enable Feature",
		.initial_checked = true,
		.enabled = false
	};

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<CheckboxDescriptor>(json);

	EXPECT_EQ(restored.label, "Enable Feature");
	EXPECT_TRUE(restored.initial_checked);
	EXPECT_FALSE(restored.enabled);
}

// === Container JSON Serialization Tests ===

TEST_F(UIJsonSerializerTest, ContainerDescriptor_WithChildren_SerializeDeserialize) {
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

	std::string json = UIJsonSerializer::ToJsonString(desc);
	auto restored = UIJsonSerializer::FromJsonString<ContainerDescriptor>(json);

	EXPECT_FLOAT_EQ(restored.bounds.x, 100.0f);
	EXPECT_FLOAT_EQ(restored.bounds.y, 100.0f);
	EXPECT_FLOAT_EQ(restored.padding, 10.0f);
	EXPECT_EQ(restored.children.size(), 2);

	// Verify first child is a label
	EXPECT_TRUE(std::holds_alternative<LabelDescriptor>(restored.children[0]));
	const auto& label = std::get<LabelDescriptor>(restored.children[0]);
	EXPECT_EQ(label.text, "Title");

	// Verify second child is a button
	EXPECT_TRUE(std::holds_alternative<ButtonDescriptor>(restored.children[1]));
	const auto& button = std::get<ButtonDescriptor>(restored.children[1]);
	EXPECT_EQ(button.label, "OK");
}

// === Auto-Detection Tests ===

TEST_F(UIJsonSerializerTest, FromJsonAuto_DetectsButtonType) {
	std::string json = R"({
		"type": "button",
		"bounds": {"x": 0, "y": 0, "width": 100, "height": 30},
		"label": "Test"
	})";

	auto variant = UIJsonSerializer::FromJsonAuto(json);

	EXPECT_TRUE(std::holds_alternative<ButtonDescriptor>(variant));
	const auto& button = std::get<ButtonDescriptor>(variant);
	EXPECT_EQ(button.label, "Test");
}

TEST_F(UIJsonSerializerTest, FromJsonAuto_DetectsContainerType) {
	std::string json = R"({
		"type": "container",
		"bounds": {"x": 0, "y": 0, "width": 400, "height": 300},
		"padding": 10,
		"children": [
			{"type": "label", "text": "Header"}
		]
	})";

	auto variant = UIJsonSerializer::FromJsonAuto(json);

	EXPECT_TRUE(std::holds_alternative<ContainerDescriptor>(variant));
	const auto& container = std::get<ContainerDescriptor>(variant);
	EXPECT_FLOAT_EQ(container.padding, 10.0f);
	EXPECT_EQ(container.children.size(), 1);
}

// === Roundtrip Tests (Serialize -> Deserialize -> Create) ===

TEST_F(UIJsonSerializerTest, Roundtrip_CreateElementFromJson) {
	std::string json = R"({
		"type": "button",
		"bounds": {"x": 10, "y": 20, "width": 120, "height": 40},
		"label": "Click Me",
		"enabled": true,
		"visible": true
	})";

	auto variant = UIJsonSerializer::FromJsonAuto(json);
	auto element = UIFactory::CreateFromVariant(variant);

	ASSERT_NE(element, nullptr);
	EXPECT_FLOAT_EQ(element->GetRelativeX(), 10.0f);
	EXPECT_FLOAT_EQ(element->GetRelativeY(), 20.0f);
	EXPECT_FLOAT_EQ(element->GetWidth(), 120.0f);
	EXPECT_FLOAT_EQ(element->GetHeight(), 40.0f);
}

// === nlohmann ADL Pattern Tests ===

TEST_F(UIJsonSerializerTest, ADL_DirectSerialization) {
	// Test that nlohmann's ADL pattern works directly
	ButtonDescriptor desc{
		.bounds = {5, 10, 100, 50},
		.label = "ADL Test"
	};

	// Direct serialization via ADL
	nlohmann::json j = desc;

	EXPECT_EQ(j["type"], "button");
	EXPECT_EQ(j["label"], "ADL Test");
	EXPECT_FLOAT_EQ(j["bounds"]["x"].get<float>(), 5.0f);
}

TEST_F(UIJsonSerializerTest, ADL_DirectDeserialization) {
	// Test that nlohmann's ADL pattern works for deserialization
	nlohmann::json j = {
		{"type", "label"},
		{"bounds", {{"x", 10}, {"y", 20}, {"width", 200}, {"height", 30}}},
		{"text", "ADL Deserialized"},
		{"visible", true}
	};

	// Direct deserialization via ADL
	auto desc = j.get<LabelDescriptor>();

	EXPECT_EQ(desc.text, "ADL Deserialized");
	EXPECT_FLOAT_EQ(desc.bounds.x, 10.0f);
	EXPECT_FLOAT_EQ(desc.bounds.width, 200.0f);
}

// ============================================================================
// UIFactoryRegistry Tests
// ============================================================================

class UIFactoryRegistryTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Ensure registry is initialized
		UIFactoryRegistry::Initialize();
	}
};

TEST_F(UIFactoryRegistryTest, Initialize_RegistersBuiltinTypes) {
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("button"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("panel"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("label"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("slider"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("checkbox"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("divider"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("progress_bar"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("image"));
	EXPECT_TRUE(UIFactoryRegistry::IsRegistered("container"));
}

TEST_F(UIFactoryRegistryTest, CreateFromJson_Button) {
	nlohmann::json j = {
		{"type", "button"},
		{"id", "test_button"},
		{"bounds", {{"x", 10}, {"y", 20}, {"width", 120}, {"height", 40}}},
		{"label", "Click Me"}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);

	ASSERT_NE(element, nullptr);
	EXPECT_EQ(element->GetId(), "test_button");
	EXPECT_FLOAT_EQ(element->GetRelativeX(), 10.0f);
	EXPECT_FLOAT_EQ(element->GetWidth(), 120.0f);
}

TEST_F(UIFactoryRegistryTest, CreateFromJson_ContainerWithChildren) {
	nlohmann::json j = {
		{"type", "container"},
		{"id", "settings_container"},
		{"bounds", {{"x", 0}, {"y", 0}, {"width", 400}, {"height", 300}}},
		{"children", {
			{
				{"type", "label"},
				{"id", "title"},
				{"text", "Settings"}
			},
			{
				{"type", "slider"},
				{"id", "volume_slider"},
				{"min_value", 0.0f},
				{"max_value", 100.0f}
			}
		}}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);

	ASSERT_NE(element, nullptr);
	EXPECT_EQ(element->GetId(), "settings_container");
	EXPECT_EQ(element->GetChildren().size(), 2);

	// Check children have IDs
	auto* title = element->FindChildById("title");
	EXPECT_NE(title, nullptr);

	auto* slider = element->FindChildById("volume_slider");
	EXPECT_NE(slider, nullptr);
}

TEST_F(UIFactoryRegistryTest, CreateFromJson_UnknownType_ReturnsNull) {
	nlohmann::json j = {
		{"type", "unknown_widget"},
		{"bounds", {{"x", 0}, {"y", 0}, {"width", 100}, {"height", 100}}}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);

	EXPECT_EQ(element, nullptr);
}

TEST_F(UIFactoryRegistryTest, GetRegisteredTypes_ContainsAllBuiltins) {
	auto types = UIFactoryRegistry::GetRegisteredTypes();

	EXPECT_GE(types.size(), 9u); // At least 9 built-in types
}

// ============================================================================
// EventBindings Tests
// ============================================================================

class EventBindingsTest : public ::testing::Test {
protected:
	void SetUp() override {}
};

TEST_F(EventBindingsTest, BindingCount_TracksRegisteredBindings) {
	EventBindings bindings;

	EXPECT_EQ(bindings.BindingCount(), 0u);

	bindings.OnClick("button1", [](const MouseEvent&) { return true; });
	EXPECT_EQ(bindings.BindingCount(), 1u);

	bindings.OnSliderChanged("slider1", [](float) {});
	EXPECT_EQ(bindings.BindingCount(), 2u);

	bindings.OnCheckboxToggled("checkbox1", [](bool) {});
	EXPECT_EQ(bindings.BindingCount(), 3u);
}

TEST_F(EventBindingsTest, Clear_RemovesAllBindings) {
	EventBindings bindings;

	bindings.OnClick("button1", [](const MouseEvent&) { return true; });
	bindings.OnSliderChanged("slider1", [](float) {});

	bindings.Clear();

	EXPECT_EQ(bindings.BindingCount(), 0u);
}

TEST_F(EventBindingsTest, ApplyTo_NullRoot_ReturnsZero) {
	EventBindings bindings;
	bindings.OnClick("button1", [](const MouseEvent&) { return true; });

	int applied = bindings.ApplyTo(nullptr);

	EXPECT_EQ(applied, 0);
}

TEST_F(EventBindingsTest, ApplyTo_ButtonBinding) {
	// Create a button with ID
	auto button = UIFactory::Create(ButtonDescriptor{
		.id = "save_button",
		.bounds = {10, 10, 100, 30},
		.label = "Save"
	});

	bool clicked = false;
	EventBindings bindings;
	bindings.OnClick("save_button", [&clicked](const MouseEvent&) {
		clicked = true;
		return true;
	});

	int applied = bindings.ApplyTo(button.get());

	EXPECT_EQ(applied, 1);

	// The callback should be wired up - simulate a click
	MouseEvent event;
	event.x = 50.0f;
	event.y = 25.0f;
	event.left_pressed = true;
	button->ProcessMouseEvent(event);

	EXPECT_TRUE(clicked);
}

TEST_F(EventBindingsTest, ApplyTo_SliderBinding) {
	auto slider = UIFactory::Create(SliderDescriptor{
		.id = "volume_slider",
		.bounds = {10, 10, 200, 30},
		.min_value = 0.0f,
		.max_value = 100.0f
	});

	float captured_value = -1.0f;
	EventBindings bindings;
	bindings.OnSliderChanged("volume_slider", [&captured_value](float v) {
		captured_value = v;
	});

	int applied = bindings.ApplyTo(slider.get());

	EXPECT_EQ(applied, 1);
}

TEST_F(EventBindingsTest, ApplyTo_CheckboxBinding) {
	auto checkbox = UIFactory::Create(CheckboxDescriptor{
		.id = "fullscreen_checkbox",
		.label = "Fullscreen"
	});

	bool captured_state = false;
	EventBindings bindings;
	bindings.OnCheckboxToggled("fullscreen_checkbox", [&captured_state](bool v) {
		captured_state = v;
	});

	int applied = bindings.ApplyTo(checkbox.get());

	EXPECT_EQ(applied, 1);
}

TEST_F(EventBindingsTest, ApplyTo_ContainerWithMultipleBindings) {
	auto container = UIFactory::Create(ContainerDescriptor{
		.id = "settings_panel",
		.bounds = {0, 0, 400, 300},
		.children = {
			ButtonDescriptor{
				.id = "apply_button",
				.bounds = {10, 10, 100, 30},
				.label = "Apply"
			},
			SliderDescriptor{
				.id = "brightness_slider",
				.bounds = {10, 50, 200, 30}
			},
			CheckboxDescriptor{
				.id = "vsync_checkbox",
				.label = "VSync"
			}
		}
	});

	EventBindings bindings;
	bindings.OnClick("apply_button", [](const MouseEvent&) { return true; });
	bindings.OnSliderChanged("brightness_slider", [](float) {});
	bindings.OnCheckboxToggled("vsync_checkbox", [](bool) {});

	int applied = bindings.ApplyTo(container.get());

	EXPECT_EQ(applied, 3);
}

// ============================================================================
// DataBinder Tests
// ============================================================================

TEST_F(EventBindingsTest, DataBinder_BindFloat) {
	float volume = 0.5f;

	DataBinder binder;
	binder.BindFloat("volume_slider", volume);

	EXPECT_EQ(binder.GetBindings().BindingCount(), 1u);
}

TEST_F(EventBindingsTest, DataBinder_BindBool) {
	bool fullscreen = false;

	DataBinder binder;
	binder.BindBool("fullscreen_checkbox", fullscreen);

	EXPECT_EQ(binder.GetBindings().BindingCount(), 1u);
}

TEST_F(EventBindingsTest, DataBinder_BindAction) {
	bool action_called = false;

	DataBinder binder;
	binder.BindAction("apply_button", [&action_called]() { action_called = true; });

	EXPECT_EQ(binder.GetBindings().BindingCount(), 1u);
}

TEST_F(EventBindingsTest, DataBinder_Chaining) {
	float volume = 0.5f;
	bool muted = false;

	DataBinder binder;
	binder.BindFloat("volume", volume)
		  .BindBool("muted", muted)
		  .BindAction("apply", []() {});

	EXPECT_EQ(binder.GetBindings().BindingCount(), 3u);
}

// ============================================================================
// ID Tests
// ============================================================================

TEST_F(UIFactoryTest, CreateWithId_ButtonHasId) {
	auto button = UIFactory::Create(ButtonDescriptor{
		.id = "my_button",
		.label = "Test"
	});

	EXPECT_EQ(button->GetId(), "my_button");
}

TEST_F(UIFactoryTest, CreateWithId_ContainerChildrenHaveIds) {
	auto container = UIFactory::Create(ContainerDescriptor{
		.id = "parent",
		.children = {
			ButtonDescriptor{.id = "child1", .label = "Button1"},
			LabelDescriptor{.id = "child2", .text = "Label2"}
		}
	});

	EXPECT_EQ(container->GetId(), "parent");

	auto* child1 = container->FindChildById("child1");
	EXPECT_NE(child1, nullptr);

	auto* child2 = container->FindChildById("child2");
	EXPECT_NE(child2, nullptr);
}

TEST_F(UIFactoryTest, FindChildById_ReturnsNullForUnknownId) {
	auto container = UIFactory::Create(ContainerDescriptor{
		.id = "parent",
		.children = {
			ButtonDescriptor{.id = "child1", .label = "Button1"}
		}
	});

	auto* unknown = container->FindChildById("nonexistent");
	EXPECT_EQ(unknown, nullptr);
}

// ============================================================================
// ActionRegistry Tests
// ============================================================================

class ActionRegistryTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Clear any previously registered actions
		ActionRegistry::Clear();
	}
	void TearDown() override {
		ActionRegistry::Clear();
	}
};

TEST_F(ActionRegistryTest, RegisterClickAction_CanBeRetrieved) {
	bool called = false;
	ActionRegistry::RegisterClickAction("test_click", [&called](const MouseEvent&) {
		called = true;
		return true;
	});

	const auto* action = ActionRegistry::GetClickAction("test_click");
	ASSERT_NE(action, nullptr);

	MouseEvent event{};
	(*action)(event);
	EXPECT_TRUE(called);
}

TEST_F(ActionRegistryTest, RegisterFloatAction_CanBeRetrieved) {
	float received_value = 0.0f;
	ActionRegistry::RegisterFloatAction("test_float", [&received_value](float v) {
		received_value = v;
	});

	const auto* action = ActionRegistry::GetFloatAction("test_float");
	ASSERT_NE(action, nullptr);

	(*action)(42.5f);
	EXPECT_FLOAT_EQ(received_value, 42.5f);
}

TEST_F(ActionRegistryTest, RegisterBoolAction_CanBeRetrieved) {
	bool received_value = false;
	ActionRegistry::RegisterBoolAction("test_bool", [&received_value](bool v) {
		received_value = v;
	});

	const auto* action = ActionRegistry::GetBoolAction("test_bool");
	ASSERT_NE(action, nullptr);

	(*action)(true);
	EXPECT_TRUE(received_value);
}

TEST_F(ActionRegistryTest, GetUnregisteredAction_ReturnsNull) {
	EXPECT_EQ(ActionRegistry::GetClickAction("nonexistent"), nullptr);
	EXPECT_EQ(ActionRegistry::GetFloatAction("nonexistent"), nullptr);
	EXPECT_EQ(ActionRegistry::GetBoolAction("nonexistent"), nullptr);
}

TEST_F(ActionRegistryTest, Clear_RemovesAllActions) {
	ActionRegistry::RegisterClickAction("test_click", [](const MouseEvent&) { return true; });
	ActionRegistry::RegisterFloatAction("test_float", [](float) {});
	ActionRegistry::RegisterBoolAction("test_bool", [](bool) {});

	ActionRegistry::Clear();

	EXPECT_EQ(ActionRegistry::GetClickAction("test_click"), nullptr);
	EXPECT_EQ(ActionRegistry::GetFloatAction("test_float"), nullptr);
	EXPECT_EQ(ActionRegistry::GetBoolAction("test_bool"), nullptr);
}

TEST_F(ActionRegistryTest, ApplyActionsFromJson_WiresClickAction) {
	bool clicked = false;
	ActionRegistry::RegisterClickAction("do_click", [&clicked](const MouseEvent&) {
		clicked = true;
		return true;
	});

	nlohmann::json j = {
		{"type", "button"},
		{"id", "my_button"},
		{"label", "Click Me"},
		{"on_click_action", "do_click"}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);
	ASSERT_NE(element, nullptr);

	int applied = ActionRegistry::ApplyActionsFromJson(j, element.get());
	EXPECT_EQ(applied, 1);

	// Simulate click - create event within button bounds
	auto* button = dynamic_cast<Button*>(element.get());
	ASSERT_NE(button, nullptr);
	MouseEvent event{5, 5, false, false, true, false, 0.0f};  // left_pressed = true at button position
	button->OnClick(event);
	EXPECT_TRUE(clicked);
}

TEST_F(ActionRegistryTest, ApplyActionsFromJson_WiresSliderAction) {
	float value = 0.0f;
	ActionRegistry::RegisterFloatAction("set_value", [&value](float v) {
		value = v;
	});

	nlohmann::json j = {
		{"type", "slider"},
		{"id", "my_slider"},
		{"min_value", 0},
		{"max_value", 100},
		{"on_change_action", "set_value"}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);
	ASSERT_NE(element, nullptr);

	int applied = ActionRegistry::ApplyActionsFromJson(j, element.get());
	EXPECT_EQ(applied, 1);

	// Note: Slider callbacks are only triggered during user drag interaction,
	// not via SetValue(). We verify the action was applied (count = 1).
	// The callback wiring is verified by the applied count.
}

TEST_F(ActionRegistryTest, ApplyActionsFromJson_WiresCheckboxAction) {
	bool toggled = false;
	ActionRegistry::RegisterBoolAction("toggle_it", [&toggled](bool v) {
		toggled = v;
	});

	nlohmann::json j = {
		{"type", "checkbox"},
		{"id", "my_checkbox"},
		{"label", "Enable"},
		{"on_toggle_action", "toggle_it"}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);
	ASSERT_NE(element, nullptr);

	int applied = ActionRegistry::ApplyActionsFromJson(j, element.get());
	EXPECT_EQ(applied, 1);

	// Use Toggle() to trigger the callback (simulates user click)
	auto* checkbox = dynamic_cast<Checkbox*>(element.get());
	ASSERT_NE(checkbox, nullptr);
	checkbox->Toggle();
	EXPECT_TRUE(toggled);
}

TEST_F(ActionRegistryTest, ApplyActionsFromJson_ContainerWithActions) {
	bool button_clicked = false;
	float slider_value = 0.0f;

	ActionRegistry::RegisterClickAction("btn_action", [&button_clicked](const MouseEvent&) {
		button_clicked = true;
		return true;
	});
	ActionRegistry::RegisterFloatAction("slider_action", [&slider_value](float v) {
		slider_value = v;
	});

	nlohmann::json j = {
		{"type", "container"},
		{"id", "parent"},
		{"children", {
			{
				{"type", "button"},
				{"id", "child_btn"},
				{"label", "Child"},
				{"on_click_action", "btn_action"}
			},
			{
				{"type", "slider"},
				{"id", "child_slider"},
				{"on_change_action", "slider_action"}
			}
		}}
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);
	ASSERT_NE(element, nullptr);

	int applied = ActionRegistry::ApplyActionsFromJson(j, element.get());
	EXPECT_EQ(applied, 2);

	// Test button - click at button position
	auto* btn = element->FindChildById("child_btn");
	ASSERT_NE(btn, nullptr);
	auto* button = dynamic_cast<Button*>(btn);
	ASSERT_NE(button, nullptr);
	MouseEvent event{5, 5, false, false, true, false, 0.0f};  // left_pressed = true
	button->OnClick(event);
	EXPECT_TRUE(button_clicked);

	// Test slider - verify action was wired (callback is only triggered during user drag)
	auto* sldr = element->FindChildById("child_slider");
	ASSERT_NE(sldr, nullptr);
	auto* slider = dynamic_cast<Slider*>(sldr);
	ASSERT_NE(slider, nullptr);
	// Note: Slider callback is triggered by drag interaction, not SetValue()
	// The action wiring is verified by applied count above
}

TEST_F(ActionRegistryTest, ApplyActionsFromJson_UnregisteredAction_DoesNothing) {
	nlohmann::json j = {
		{"type", "button"},
		{"id", "my_button"},
		{"label", "Click Me"},
		{"on_click_action", "nonexistent_action"}  // Not registered
	};

	auto element = UIFactoryRegistry::CreateFromJson(j);
	ASSERT_NE(element, nullptr);

	int applied = ActionRegistry::ApplyActionsFromJson(j, element.get());
	EXPECT_EQ(applied, 0);
}

// ============================================================================
// Action Name in Descriptor Tests
// ============================================================================

TEST_F(UIDescriptorTest, ButtonDescriptor_OnClickAction_SerializesToJson) {
	auto desc = ButtonDescriptor{
		.id = "save_btn",
		.label = "Save",
		.on_click_action = "save_game"
	};

	nlohmann::json j = desc;

	EXPECT_EQ(j["on_click_action"], "save_game");
}

TEST_F(UIDescriptorTest, ButtonDescriptor_OnClickAction_DeserializesFromJson) {
	nlohmann::json j = {
		{"type", "button"},
		{"label", "Save"},
		{"on_click_action", "save_game"}
	};

	auto desc = j.get<ButtonDescriptor>();

	EXPECT_EQ(desc.on_click_action, "save_game");
}

TEST_F(UIDescriptorTest, SliderDescriptor_OnChangeAction_SerializesToJson) {
	auto desc = SliderDescriptor{
		.id = "volume",
		.on_change_action = "set_volume"
	};

	nlohmann::json j = desc;

	EXPECT_EQ(j["on_change_action"], "set_volume");
}

TEST_F(UIDescriptorTest, CheckboxDescriptor_OnToggleAction_SerializesToJson) {
	auto desc = CheckboxDescriptor{
		.id = "fullscreen",
		.label = "Fullscreen",
		.on_toggle_action = "toggle_fullscreen"
	};

	nlohmann::json j = desc;

	EXPECT_EQ(j["on_toggle_action"], "toggle_fullscreen");
}
