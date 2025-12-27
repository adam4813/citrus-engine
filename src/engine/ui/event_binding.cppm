module;

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

export module engine.ui:event_binding;

import :ui_element;
import :mouse_event;
import :elements.button;
import :elements.slider;
import :elements.checkbox;

export namespace engine::ui {

/**
 * @brief Registry for named actions that can be referenced from JSON
 *
 * ActionRegistry provides a way to specify event handlers declaratively in JSON
 * by referencing named actions. This bridges the gap between JSON (which cannot
 * contain code) and runtime callbacks.
 *
 * ## How It Works
 *
 * 1. Register named actions at application startup
 * 2. Reference action names in JSON via `on_click_action`, `on_change_action`, etc.
 * 3. When UI is created from JSON, actions are automatically wired up
 *
 * ## JSON Schema
 *
 * Descriptors can include action name fields:
 * - `on_click_action: string` - For buttons
 * - `on_change_action: string` - For sliders
 * - `on_toggle_action: string` - For checkboxes
 *
 * ## Usage
 *
 * @code
 * // Step 1: Register actions at startup
 * ActionRegistry::RegisterClickAction("save_game", [](const MouseEvent&) {
 *     SaveGame();
 *     return true;
 * });
 * ActionRegistry::RegisterFloatAction("set_volume", [](float v) {
 *     SetVolume(v);
 * });
 * ActionRegistry::RegisterBoolAction("toggle_fullscreen", [](bool v) {
 *     SetFullscreen(v);
 * });
 *
 * // Step 2: Reference in JSON
 * // {
 * //   "type": "button",
 * //   "label": "Save",
 * //   "on_click_action": "save_game"
 * // }
 *
 * // Step 3: Create UI from JSON (actions auto-wired)
 * auto ui = UIFactoryRegistry::CreateFromJson(json);
 *
 * // Step 4: Apply action bindings
 * ActionRegistry::ApplyTo(ui.get());
 * @endcode
 *
 * ## Note
 *
 * For one-off or context-specific callbacks, continue using EventBindings
 * after loading. ActionRegistry is best for reusable, global actions.
 */
class ActionRegistry {
public:
	/// Click action callback type
	using ClickAction = std::function<bool(const MouseEvent&)>;
	/// Float value changed callback type
	using FloatAction = std::function<void(float)>;
	/// Bool toggled callback type
	using BoolAction = std::function<void(bool)>;

	/**
	 * @brief Register a named click action
	 *
	 * @param name Action name to reference in JSON
	 * @param action Callback function
	 */
	static void RegisterClickAction(const std::string& name, ClickAction action) {
		GetClickActions()[name] = std::move(action);
	}

	/**
	 * @brief Register a named float value action (for sliders)
	 *
	 * @param name Action name to reference in JSON
	 * @param action Callback function
	 */
	static void RegisterFloatAction(const std::string& name, FloatAction action) {
		GetFloatActions()[name] = std::move(action);
	}

	/**
	 * @brief Register a named bool action (for checkboxes)
	 *
	 * @param name Action name to reference in JSON
	 * @param action Callback function
	 */
	static void RegisterBoolAction(const std::string& name, BoolAction action) {
		GetBoolActions()[name] = std::move(action);
	}

	/**
	 * @brief Get a registered click action by name
	 *
	 * @param name Action name
	 * @return Pointer to action, or nullptr if not found
	 */
	static const ClickAction* GetClickAction(const std::string& name) {
		const auto& actions = GetClickActions();
		const auto it = actions.find(name);
		return it != actions.end() ? &it->second : nullptr;
	}

	/**
	 * @brief Get a registered float action by name
	 *
	 * @param name Action name
	 * @return Pointer to action, or nullptr if not found
	 */
	static const FloatAction* GetFloatAction(const std::string& name) {
		const auto& actions = GetFloatActions();
		const auto it = actions.find(name);
		return it != actions.end() ? &it->second : nullptr;
	}

	/**
	 * @brief Get a registered bool action by name
	 *
	 * @param name Action name
	 * @return Pointer to action, or nullptr if not found
	 */
	static const BoolAction* GetBoolAction(const std::string& name) {
		const auto& actions = GetBoolActions();
		const auto it = actions.find(name);
		return it != actions.end() ? &it->second : nullptr;
	}

	/**
	 * @brief Clear all registered actions
	 */
	static void Clear() {
		GetClickActions().clear();
		GetFloatActions().clear();
		GetBoolActions().clear();
	}

	/**
	 * @brief Apply registered actions to a UI tree based on JSON definition
	 *
	 * Recursively traverses the JSON and the UI tree, wiring up actions
	 * based on `on_click_action`, `on_change_action`, and `on_toggle_action` fields.
	 *
	 * @param json The JSON used to create the UI
	 * @param root The root element of the created UI tree
	 * @return Number of actions successfully applied
	 */
	static int ApplyActionsFromJson(const nlohmann::json& json, UIElement* root) {
		if (!root) {
			return 0;
		}

		int applied = 0;

		// Apply click action if specified
		if (json.contains("on_click_action")) {
			const std::string action_name = json["on_click_action"];
			if (const auto* action = GetClickAction(action_name)) {
				if (auto* button = dynamic_cast<elements::Button*>(root)) {
					button->SetClickCallback(*action);
					applied++;
				}
			}
		}

		// Apply change action for sliders
		if (json.contains("on_change_action")) {
			const std::string action_name = json["on_change_action"];
			if (const auto* action = GetFloatAction(action_name)) {
				if (auto* slider = dynamic_cast<elements::Slider*>(root)) {
					slider->SetValueChangedCallback(*action);
					applied++;
				}
			}
		}

		// Apply toggle action for checkboxes
		if (json.contains("on_toggle_action")) {
			const std::string action_name = json["on_toggle_action"];
			if (const auto* action = GetBoolAction(action_name)) {
				if (auto* checkbox = dynamic_cast<elements::Checkbox*>(root)) {
					checkbox->SetToggleCallback(*action);
					applied++;
				}
			}
		}

		// Recursively apply to children
		if (json.contains("children") && json["children"].is_array()) {
			const auto& children = root->GetChildren();
			const auto& json_children = json["children"];
			for (size_t i = 0; i < json_children.size() && i < children.size(); ++i) {
				applied += ApplyActionsFromJson(json_children[i], children[i].get());
			}
		}

		return applied;
	}

private:
	static std::unordered_map<std::string, ClickAction>& GetClickActions() {
		static std::unordered_map<std::string, ClickAction> actions;
		return actions;
	}

	static std::unordered_map<std::string, FloatAction>& GetFloatActions() {
		static std::unordered_map<std::string, FloatAction> actions;
		return actions;
	}

	static std::unordered_map<std::string, BoolAction>& GetBoolActions() {
		static std::unordered_map<std::string, BoolAction> actions;
		return actions;
	}
};

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
