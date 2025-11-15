#include <gtest/gtest.h>
#include <memory>

import engine.ui;

using namespace engine::ui;
using namespace engine::ui::elements;
using namespace engine::ui::batch_renderer;

// ============================================================================
// ConfirmationDialog Tests
// ============================================================================

class ConfirmationDialogTest : public ::testing::Test {
protected:
	void SetUp() override {
		dialog = std::make_unique<ConfirmationDialog>(
				"Confirm Action", "Are you sure you want to proceed?", "Yes", "No");
	}

	void TearDown() override { dialog.reset(); }

	std::unique_ptr<ConfirmationDialog> dialog;
};

TEST_F(ConfirmationDialogTest, Constructor_CreatesDialog) { EXPECT_NE(dialog, nullptr); }

TEST_F(ConfirmationDialogTest, Constructor_InitiallyHidden) { EXPECT_FALSE(dialog->IsVisible()); }

TEST_F(ConfirmationDialogTest, Show_MakesVisible) {
	dialog->Show();
	EXPECT_TRUE(dialog->IsVisible());
}

TEST_F(ConfirmationDialogTest, Hide_MakesInvisible) {
	dialog->Show();
	EXPECT_TRUE(dialog->IsVisible());

	dialog->Hide();
	EXPECT_FALSE(dialog->IsVisible());
}

TEST_F(ConfirmationDialogTest, SetConfirmCallback_StoresCallback) {
	bool callback_triggered = false;

	dialog->SetConfirmCallback([&]() { callback_triggered = true; });

	// Note: We can't directly test callback execution without simulating
	// button clicks, which requires more complex event handling
	// This test just ensures the setter doesn't crash
}

TEST_F(ConfirmationDialogTest, SetCancelCallback_StoresCallback) {
	bool callback_triggered = false;

	dialog->SetCancelCallback([&]() { callback_triggered = true; });

	// Note: We can't directly test callback execution without simulating
	// button clicks, which requires more complex event handling
	// This test just ensures the setter doesn't crash
}

TEST_F(ConfirmationDialogTest, ProcessMouseEvent_Hidden_ReturnsFalse) {
	// Dialog is hidden by default
	MouseEvent event{100, 100, false, false, true, true, 0.0f};

	bool consumed = dialog->ProcessMouseEvent(event);

	EXPECT_FALSE(consumed);
}

TEST_F(ConfirmationDialogTest, ProcessMouseEvent_Visible_ReturnsTrue) {
	dialog->Show();

	// Any mouse event should be consumed when dialog is visible (modal behavior)
	MouseEvent event{100, 100, false, false, true, true, 0.0f};

	bool consumed = dialog->ProcessMouseEvent(event);

	EXPECT_TRUE(consumed);
}

TEST_F(ConfirmationDialogTest, ProcessMouseEvent_Visible_BlocksLowerLayers) {
	dialog->Show();

	// Click outside dialog bounds - should still be consumed (modal blocks all input)
	MouseEvent event{5000, 5000, false, false, true, true, 0.0f};

	bool consumed = dialog->ProcessMouseEvent(event);

	EXPECT_TRUE(consumed); // Modal pattern: consume ALL events when visible
}

TEST_F(ConfirmationDialogTest, Constructor_WithCustomSize_SetsSize) {
	auto custom_dialog = std::make_unique<ConfirmationDialog>(
			"Title",
			"Message",
			"OK",
			"Cancel",
			500.0f // Custom width
	);

	EXPECT_EQ(custom_dialog->GetWidth(), 500.0f);
}

TEST_F(ConfirmationDialogTest, Constructor_WithCustomFontSizes_DoesNotCrash) {
	auto custom_dialog = std::make_unique<ConfirmationDialog>(
			"Title",
			"Message",
			"OK",
			"Cancel",
			400.0f, // width
			24.0f, // title font size
			16.0f // message font size
	);

	EXPECT_NE(custom_dialog, nullptr);
}

TEST_F(ConfirmationDialogTest, HasChildren_AfterConstruction) {
	// Dialog should have children (title, message, buttons)
	EXPECT_GT(dialog->GetChildren().size(), 0);
}

TEST_F(ConfirmationDialogTest, InheritsFromPanel) {
	// ConfirmationDialog inherits from Panel
	// It should have Panel's properties
	dialog->SetBackgroundColor(Colors::BLUE);
	dialog->SetBorderColor(Colors::RED);
	dialog->SetBorderWidth(3.0f);

	// These should not crash
}

// Integration test: Verify modal behavior blocks lower UI
TEST_F(ConfirmationDialogTest, ModalBehavior_BlocksUnderlyingUI) {
	// Create a button "underneath" the dialog
	auto button = std::make_unique<Button>(100, 100, 100, 40, "Click");
	bool button_clicked = false;
	button->SetClickCallback([&](const MouseEvent&) {
		button_clicked = true;
		return true;
	});

	// Show dialog (modal)
	dialog->Show();

	// Try to click the button
	MouseEvent click_event{150, 120, false, false, true, true, 0.0f};

	// Dialog processes event first (modal)
	bool dialog_consumed = dialog->ProcessMouseEvent(click_event);

	// Only process button if dialog didn't consume
	if (!dialog_consumed) {
		button->ProcessMouseEvent(click_event);
	}

	EXPECT_TRUE(dialog_consumed); // Dialog should consume event
	EXPECT_FALSE(button_clicked); // Button should NOT receive event
}

// Test that hiding dialog allows lower UI to receive events
TEST_F(ConfirmationDialogTest, HidingDialog_AllowsUnderlyingUI) {
	// Create a button "underneath" the dialog
	auto button = std::make_unique<Button>(100, 100, 100, 40, "Click");
	bool button_clicked = false;
	button->SetClickCallback([&](const MouseEvent&) {
		button_clicked = true;
		return true;
	});

	// Dialog is hidden by default
	EXPECT_FALSE(dialog->IsVisible());

	// Try to click the button
	MouseEvent click_event{150, 120, false, false, true, true, 0.0f};

	// Dialog doesn't consume when hidden
	bool dialog_consumed = dialog->ProcessMouseEvent(click_event);

	// Process button
	if (!dialog_consumed) {
		button->ProcessMouseEvent(click_event);
	}

	EXPECT_FALSE(dialog_consumed); // Dialog should NOT consume when hidden
	EXPECT_TRUE(button_clicked); // Button should receive event
}
