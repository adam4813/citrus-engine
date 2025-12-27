module;

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

export module engine.ui:event_binding;

import :ui_element;
import :mouse_event;
import :elements.button;
import :elements.slider;
import :elements.checkbox;

export namespace engine::ui {

/**
 * @brief Event binding system for wiring up callbacks after JSON loading
 *
 * Since callbacks cannot be serialized to JSON, this class provides a way to
 * bind event handlers to UI elements after they are created from descriptors.
 *
 * ## Usage Patterns
 *
 * ### Pattern 1: Named Element Binding
 * Use element IDs to bind handlers after loading:
 * @code
 * // Define handlers
 * EventBindings bindings;
 * bindings.OnClick("save_button", [](const MouseEvent&) {
 *     SaveGame();
 *     return true;
 * });
 * bindings.OnSliderChanged("volume_slider", [](float v) {
 *     SetVolume(v);
 * });
 *
 * // Load UI from JSON
 * auto ui = UIFactory::CreateFromVariant(UIJsonSerializer::FromJsonAuto(json));
 *
 * // Apply bindings to element tree
 * bindings.ApplyTo(ui.get());
 * @endcode
 *
 * ### Pattern 2: Direct Element Access
 * Store element references during creation:
 * @code
 * auto ui = UIFactory::CreateFromVariant(desc);
 *
 * // Find specific elements by ID and bind directly
 * if (auto* btn = ui->FindChildById("ok_button")) {
 *     if (auto* button = dynamic_cast<elements::Button*>(btn)) {
 *         button->SetClickCallback([](const MouseEvent&) { ... });
 *     }
 * }
 * @endcode
 *
 * ### Pattern 3: Data Binding (Reactive)
 * For reactive data binding, use lambdas that capture state:
 * @code
 * struct GameSettings {
 *     float volume = 0.5f;
 *     bool fullscreen = false;
 * };
 * GameSettings* settings = &game_settings;
 *
 * bindings.OnSliderChanged("volume", [settings](float v) {
 *     settings->volume = v;
 * });
 * bindings.OnCheckboxToggled("fullscreen", [settings](bool v) {
 *     settings->fullscreen = v;
 * });
 * @endcode
 */
class EventBindings {
public:
	// === Click Handlers (for Buttons) ===

	/**
	 * @brief Register a click handler for an element ID
	 *
	 * @param element_id The ID of the element to bind to
	 * @param handler Click callback function
	 * @return Reference to self for chaining
	 */
	EventBindings& OnClick(const std::string& element_id, UIElement::ClickCallback handler) {
		click_handlers_[element_id] = std::move(handler);
		return *this;
	}

	// === Value Changed Handlers (for Sliders) ===

	/**
	 * @brief Register a value changed handler for a slider ID
	 *
	 * @param element_id The ID of the slider to bind to
	 * @param handler Value changed callback function
	 * @return Reference to self for chaining
	 */
	EventBindings& OnSliderChanged(const std::string& element_id, std::function<void(float)> handler) {
		slider_handlers_[element_id] = std::move(handler);
		return *this;
	}

	// === Toggle Handlers (for Checkboxes) ===

	/**
	 * @brief Register a toggle handler for a checkbox ID
	 *
	 * @param element_id The ID of the checkbox to bind to
	 * @param handler Toggle callback function
	 * @return Reference to self for chaining
	 */
	EventBindings& OnCheckboxToggled(const std::string& element_id, std::function<void(bool)> handler) {
		checkbox_handlers_[element_id] = std::move(handler);
		return *this;
	}

	// === Application ===

	/**
	 * @brief Apply all registered bindings to an element tree
	 *
	 * Recursively traverses the element tree and applies matching handlers.
	 *
	 * @param root Root element of the UI tree
	 * @return Number of bindings successfully applied
	 */
	int ApplyTo(UIElement* root) {
		if (!root) {
			return 0;
		}

		int applied = 0;
		applied += ApplyToElement(root);

		// Recursively apply to children
		for (const auto& child : root->GetChildren()) {
			applied += ApplyTo(child.get());
		}

		return applied;
	}

	/**
	 * @brief Clear all registered bindings
	 */
	void Clear() {
		click_handlers_.clear();
		slider_handlers_.clear();
		checkbox_handlers_.clear();
	}

	/**
	 * @brief Get number of registered bindings
	 */
	[[nodiscard]] size_t BindingCount() const {
		return click_handlers_.size() + slider_handlers_.size() + checkbox_handlers_.size();
	}

private:
	int ApplyToElement(UIElement* element) {
		int applied = 0;
		const std::string& id = element->GetId();

		// Try to apply click handler (for buttons)
		if (const auto it = click_handlers_.find(id); it != click_handlers_.end()) {
			if (auto* button = dynamic_cast<elements::Button*>(element)) {
				button->SetClickCallback(it->second);
				applied++;
			}
		}

		// Try to apply slider handler
		if (const auto it = slider_handlers_.find(id); it != slider_handlers_.end()) {
			if (auto* slider = dynamic_cast<elements::Slider*>(element)) {
				slider->SetValueChangedCallback(it->second);
				applied++;
			}
		}

		// Try to apply checkbox handler
		if (const auto it = checkbox_handlers_.find(id); it != checkbox_handlers_.end()) {
			if (auto* checkbox = dynamic_cast<elements::Checkbox*>(element)) {
				checkbox->SetToggleCallback(it->second);
				applied++;
			}
		}

		return applied;
	}

	std::unordered_map<std::string, UIElement::ClickCallback> click_handlers_;
	std::unordered_map<std::string, std::function<void(float)>> slider_handlers_;
	std::unordered_map<std::string, std::function<void(bool)>> checkbox_handlers_;
};

/**
 * @brief Helper to create data-bound UI
 *
 * Provides a fluent interface for creating UI and binding data in one step.
 *
 * **Lifetime Warning**: The bound references (float&, bool&) must outlive
 * the DataBinder and any UI elements using these bindings. Typically,
 * bind to member variables of a long-lived object like a settings struct.
 *
 * @code
 * // Safe usage: settings outlives the UI
 * struct Settings {
 *     float volume = 0.5f;
 *     bool fullscreen = false;
 * };
 * Settings settings; // Must outlive UI
 *
 * DataBinder binder;
 * binder.BindFloat("volume", settings.volume)
 *       .BindBool("fullscreen", settings.fullscreen);
 * binder.ApplyTo(ui.get());
 * @endcode
 */
class DataBinder {
public:
	/**
	 * @brief Bind a float reference to a slider
	 *
	 * Creates a two-way binding where changing the slider updates the value.
	 *
	 * **Warning**: The referenced float must outlive the UI elements.
	 *
	 * @param element_id Slider element ID
	 * @param value Reference to float to bind (must remain valid)
	 * @return Reference to self for chaining
	 */
	DataBinder& BindFloat(const std::string& element_id, float& value) {
		bindings_.OnSliderChanged(element_id, [&value](float v) { value = v; });
		return *this;
	}

	/**
	 * @brief Bind a bool reference to a checkbox
	 *
	 * Creates a two-way binding where toggling the checkbox updates the value.
	 *
	 * **Warning**: The referenced bool must outlive the UI elements.
	 *
	 * @param element_id Checkbox element ID
	 * @param value Reference to bool to bind (must remain valid)
	 * @return Reference to self for chaining
	 */
	DataBinder& BindBool(const std::string& element_id, bool& value) {
		bindings_.OnCheckboxToggled(element_id, [&value](bool v) { value = v; });
		return *this;
	}

	/**
	 * @brief Bind a click action to a button
	 *
	 * @param element_id Button element ID
	 * @param action Action to perform on click
	 * @return Reference to self for chaining
	 */
	DataBinder& BindAction(const std::string& element_id, std::function<void()> action) {
		bindings_.OnClick(element_id, [action = std::move(action)](const MouseEvent&) {
			action();
			return true;
		});
		return *this;
	}

	/**
	 * @brief Get the configured bindings
	 *
	 * @return Reference to EventBindings
	 */
	EventBindings& GetBindings() { return bindings_; }

	/**
	 * @brief Apply bindings to a UI element tree
	 *
	 * @param root Root element
	 * @return Number of bindings applied
	 */
	int ApplyTo(UIElement* root) { return bindings_.ApplyTo(root); }

private:
	EventBindings bindings_;
};

} // namespace engine::ui
