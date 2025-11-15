module;

#include <string>
#include <memory>
#include <functional>

export module engine.ui:elements.confirmation_dialog;

import :ui_element;
import :elements.panel;
import :elements.button;
import :elements.label;
import :mouse_event;
import engine.ui.batch_renderer;

export namespace engine::ui::elements {
    /**
     * @brief Modal confirmation dialog with customizable buttons
     * 
     * ConfirmationDialog implements the Modal pattern (UI_DEVELOPMENT_BIBLE.md §3):
     * - Overlay that blocks interaction with lower layers
     * - Centered message with word wrapping
     * - Customizable button actions (OK/Cancel, Yes/No, etc.)
     * - Auto-centering on screen
     * 
     * **Usage Pattern (Declarative + Observer + Facade):**
     * @code
     * // Create dialog once (declarative)
     * auto dialog = std::make_unique<ConfirmationDialog>(
     *     "Delete Item",
     *     "Are you sure you want to delete this item? This action cannot be undone.",
     *     "Delete", "Cancel"
     * );
     * 
     * // Wire callbacks (observer pattern)
     * dialog->SetConfirmCallback([this]() {
     *     DeleteItem();
     *     dialog->Hide();
     * });
     * dialog->SetCancelCallback([this]() {
     *     dialog->Hide();
     * });
     * 
     * // Show/hide (facade pattern)
     * dialog->Show();  // Centers and displays
     * dialog->Hide();  // Hides
     * 
     * // Process events (blocks lower layers)
     * dialog->ProcessMouseEvent(event);
     * 
     * // Render
     * dialog->Render();
     * @endcode
     * 
     * **Features:**
     * - Modal overlay (blocks underlying UI)
     * - Auto-centering on screen
     * - Word wrapping for message
     * - Customizable button labels
     * - Confirm/cancel callbacks
     * - Escape key to cancel (future)
     * 
     * **CRITICAL Modal Pattern:**
     * ProcessMouseEvent() MUST return true to block events from reaching lower layers.
     * 
     * @see UI_DEVELOPMENT_BIBLE.md §3 for Modal pattern
     * @see UI_DEVELOPMENT_BIBLE.md §2.5 for Facade pattern
     */
    class ConfirmationDialog : public Panel {
    public:
        /**
         * @brief Callback type for confirm/cancel actions
         */
        using ActionCallback = std::function<void()>;

        /**
         * @brief Construct confirmation dialog
         * 
         * Dialog is initially hidden. Call Show() to display and center.
         * 
         * @param title Dialog title text
         * @param message Dialog message text (supports word wrapping)
         * @param confirm_label Confirm button label (default: "OK")
         * @param cancel_label Cancel button label (default: "Cancel")
         * @param width Dialog width (default: 400)
         * @param title_font_size Title font size (default: 18)
         * @param message_font_size Message font size (default: 14)
         * 
         * @code
         * auto dialog = std::make_unique<ConfirmationDialog>(
         *     "Confirm",
         *     "Are you sure?",
         *     "Yes", "No"
         * );
         * @endcode
         */
        ConfirmationDialog(
            const std::string& title,
            const std::string& message,
            const std::string& confirm_label = "OK",
            const std::string& cancel_label = "Cancel",
            float width = 400.0f,
            float title_font_size = 18.0f,
            float message_font_size = 14.0f
        )
            : Panel(0, 0, width, 200.0f)  // Height will be adjusted
            , title_text_(title)
            , message_text_(message)
            , dialog_width_(width)
            , title_font_size_(title_font_size)
            , message_font_size_(message_font_size)
            , title_label_(nullptr)
            , message_label_(nullptr)
            , confirm_button_(nullptr)
            , cancel_button_(nullptr)
            , confirm_callback_(nullptr)
            , cancel_callback_(nullptr) {
            
            // Configure panel appearance
            SetBackgroundColor(batch_renderer::Colors::DARK_GRAY);
            SetBorderColor(batch_renderer::Colors::GOLD);
            SetBorderWidth(2.0f);
            SetPadding(20.0f);
            SetClipChildren(false);  // Allow buttons to be fully visible
            
            // Initially hidden
            SetVisible(false);
            
            // Build dialog contents
            BuildDialog(title, message, confirm_label, cancel_label);
        }

        virtual ~ConfirmationDialog() = default;

        // === Display Control ===

        /**
         * @brief Show and center dialog on screen
         * 
         * Centers dialog and makes it visible.
         * 
         * @code
         * dialog->Show();
         * @endcode
         */
        void Show() {
            SetVisible(true);
            CenterOnScreen();
        }

        /**
         * @brief Hide dialog
         * 
         * Makes dialog invisible (does not destroy it).
         * 
         * @code
         * dialog->Hide();
         * @endcode
         */
        void Hide() {
            SetVisible(false);
        }

        // === Callbacks ===

        /**
         * @brief Set confirm button callback
         * 
         * Called when user clicks confirm button.
         * 
         * @param callback Function to call on confirm
         * 
         * @code
         * dialog->SetConfirmCallback([this]() {
         *     ExecuteAction();
         *     dialog->Hide();
         * });
         * @endcode
         */
        void SetConfirmCallback(ActionCallback callback) {
            confirm_callback_ = std::move(callback);
        }

        /**
         * @brief Set cancel button callback
         * 
         * Called when user clicks cancel button.
         * 
         * @param callback Function to call on cancel
         * 
         * @code
         * dialog->SetCancelCallback([this]() {
         *     dialog->Hide();
         * });
         * @endcode
         */
        void SetCancelCallback(ActionCallback callback) {
            cancel_callback_ = std::move(callback);
        }

        // === Event Handling (Modal Pattern) ===

        /**
         * @brief Process mouse events (BLOCKS lower layers)
         * 
         * **CRITICAL**: Returns true when visible to consume ALL events
         * and prevent lower UI layers from receiving input.
         * This is the key to modal behavior.
         * 
         * @param event Mouse event data
         * @return True if visible (consumes event), false if hidden
         */
        bool ProcessMouseEvent(const MouseEvent& event) override {
            if (!IsVisible()) {
                return false;
            }
            
            // Process event normally
            Panel::ProcessMouseEvent(event);
            
            // ALWAYS consume event when visible (modal behavior)
            return true;
        }

        // === Rendering ===

        /**
         * @brief Render dialog with semi-transparent overlay
         * 
         * Rendering order:
         * 1. Overlay (semi-transparent fullscreen quad)
         * 2. Panel (background, border)
         * 3. Title
         * 4. Message
         * 5. Buttons
         * 
         * @code
         * dialog->Render();
         * @endcode
         */
        void Render() const override {
            using namespace batch_renderer;

            if (!IsVisible()) {
                return;
            }

            // Render semi-transparent overlay (covers entire screen)
            // This visually indicates modal state
            // TODO: Replace hardcoded dimensions with actual viewport/window size from window manager.
            // For now, use a large rectangle to attempt fullscreen coverage.
            const batch_renderer::Color overlay_color = 
                batch_renderer::Color::Alpha(batch_renderer::Colors::BLACK, 0.5f);
            
            // Note: This uses a large rectangle (10000x10000) as a workaround since
            // there's no window/viewport manager integration yet. In production,
            // this should query actual screen dimensions.
            const Rectangle overlay_rect{0, 0, 10000, 10000};  // Effectively fullscreen
            BatchRenderer::SubmitQuad(overlay_rect, overlay_color);

            // Render panel (background, border, contents)
            Panel::Render();
        }

    private:
        /**
         * @brief Build dialog UI structure
         * 
         * Creates title, message, and button layout.
         * 
         * @param title Title text
         * @param message Message text
         * @param confirm_label Confirm button label
         * @param cancel_label Cancel button label
         */
        void BuildDialog(
            const std::string& title,
            const std::string& message,
            const std::string& confirm_label,
            const std::string& cancel_label
        ) {
            const float padding = GetPadding();
            const float content_width = dialog_width_ - padding * 2.0f;
            float y_offset = padding;

            // Create title label
            auto title_label = std::make_unique<Label>(
                padding, y_offset, title, title_font_size_, batch_renderer::Colors::GOLD
            );
            title_label->SetMaxWidth(content_width);
            title_label->SetAlignment(Label::Alignment::Center);
            title_label_ = title_label.get();
            y_offset += title_label->GetHeight() + 15.0f;
            AddChild(std::move(title_label));

            // Create message label with word wrapping
            auto message_label = std::make_unique<Label>(
                padding, y_offset, message, message_font_size_, batch_renderer::Colors::WHITE
            );
            message_label->SetMaxWidth(content_width);
            message_label->SetAlignment(Label::Alignment::Left);
            message_label_ = message_label.get();
            y_offset += message_label->GetHeight() + 20.0f;
            AddChild(std::move(message_label));

            // Create buttons
            const float button_width = 100.0f;
            const float button_height = 35.0f;
            const float button_spacing = 10.0f;
            const float buttons_total_width = button_width * 2.0f + button_spacing;
            const float buttons_x = (dialog_width_ - buttons_total_width) * 0.5f;

            // Confirm button
            auto confirm_btn = std::make_unique<Button>(
                buttons_x, y_offset, button_width, button_height, confirm_label
            );
            confirm_btn->SetNormalColor(batch_renderer::Colors::GOLD);
            confirm_btn->SetHoverColor(batch_renderer::Color::Brightness(batch_renderer::Colors::GOLD, 0.2f));
            confirm_btn->SetPressedColor(batch_renderer::Color::Brightness(batch_renderer::Colors::GOLD, -0.2f));
            confirm_btn->SetClickCallback([this](const MouseEvent&) {
                if (confirm_callback_) {
                    confirm_callback_();
                }
                return true;
            });
            confirm_button_ = confirm_btn.get();
            AddChild(std::move(confirm_btn));

            // Cancel button
            auto cancel_btn = std::make_unique<Button>(
                buttons_x + button_width + button_spacing, y_offset, 
                button_width, button_height, cancel_label
            );
            cancel_btn->SetClickCallback([this](const MouseEvent&) {
                if (cancel_callback_) {
                    cancel_callback_();
                }
                return true;
            });
            cancel_button_ = cancel_btn.get();
            AddChild(std::move(cancel_btn));

            y_offset += button_height + padding;

            // Set final dialog height
            SetSize(dialog_width_, y_offset);
        }

        /**
         * @brief Center dialog on screen
         * 
         * TODO: Replace hardcoded screen dimensions with actual values from window/viewport manager.
         * 
         * Currently uses hardcoded 1920x1080 dimensions, which will cause misalignment
         * on different screen sizes. This should be updated to query actual screen dimensions
         * from a window manager or use parent-relative positioning for proper centering.
         */
        void CenterOnScreen() {
            // FIXME: Hardcoded screen dimensions - will not work on non-1080p displays
            const float screen_width = 1920.0f;
            const float screen_height = 1080.0f;
            
            const float x = (screen_width - width_) * 0.5f;
            const float y = (screen_height - height_) * 0.5f;
            
            SetRelativePosition(x, y);
        }

        std::string title_text_;
        std::string message_text_;
        float dialog_width_;
        float title_font_size_;
        float message_font_size_;

        // Child elements (raw pointers for access, Panel owns via unique_ptr)
        Label* title_label_;
        Label* message_label_;
        Button* confirm_button_;
        Button* cancel_button_;

        // Callbacks
        ActionCallback confirm_callback_;
        ActionCallback cancel_callback_;
    };

} // namespace engine::ui::elements
