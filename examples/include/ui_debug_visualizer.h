#pragma once

#include <imgui.h>

import engine;

/**
 * @brief UI Debug Visualizer - renders bounds and labels for UI elements
 *
 * A utility for visualizing UI element bounds and hierarchy without polluting UI element code with debug logic.
 *
 * # Features
 *
 * - **Bounds Visualization**: Renders colored outlines around each UI element
 * - **Type Labels**: Shows element type names above each element
 * - **Hierarchical Depth**: Color fades based on tree depth for visual hierarchy
 * - **No Code Pollution**: Separate from UIElement - keeps UI code clean
 * - **ImGui Controls**: Toggle options and adjust colors at runtime
 *
 * # Basic Usage
 *
 * @code{.cpp}
 * #include "ui_debug_visualizer.h"
 *
 * // Create debugger instance
 * UIDebugVisualizer ui_debugger;
 * ui_debugger.SetEnabled(true);  // Enable by default
 *
 * // In your render loop:
 * BatchRenderer::BeginFrame();
 *
 * // Render your normal UI
 * my_ui_element->Render();
 *
 * // Render debug overlay AFTER normal UI (appears on top)
 * ui_debugger.RenderDebugOverlay(my_ui_element.get());
 *
 * BatchRenderer::EndFrame();
 * @endcode
 *
 * # ImGui Controls
 *
 * Add debug controls to your ImGui panel:
 *
 * @code{.cpp}
 * void RenderUI() {
 *     ImGui::Begin("Debug Panel");
 *     
 *     ui_debugger.RenderImGuiControls();  // Adds checkboxes and color pickers
 *     
 *     ImGui::End();
 * }
 * @endcode
 *
 * This adds:
 * - Enable/disable checkbox
 * - Show bounds toggle
 * - Show labels toggle
 * - Color pickers for bounds, labels, and background
 *
 * # Programmatic Control
 *
 * @code{.cpp}
 * // Toggle on/off
 * ui_debugger.ToggleEnabled();
 * ui_debugger.SetEnabled(false);
 *
 * // Customize appearance
 * ui_debugger.SetShowLabels(true);
 * ui_debugger.SetShowBounds(true);
 * ui_debugger.SetBoundsColor({1.0f, 0.0f, 1.0f, 1.0f});  // Magenta
 *
 * // Check state
 * if (ui_debugger.IsEnabled()) {
 *     // ...
 * }
 * @endcode
 *
 * # How It Works
 *
 * 1. **Recursively traverses** UI element tree via GetChildren()
 * 2. **Renders bounds** using BatchRenderer::SubmitLine() (4 lines per element)
 * 3. **Renders labels** using BatchRenderer::SubmitText()
 * 4. **Type detection** via C++ RTTI (typeid) - no virtual method needed
 * 5. **Depth-based fading** makes nested elements easier to distinguish
 *
 * # Type Name Detection
 *
 * The visualizer uses typeid(*element).name() to identify element types:
 *
 * - **MSVC**: "class engine::ui::elements::Image" → extracts "Image"
 * - **GCC/Clang**: Mangled name → falls back to "UIElement"
 *
 * For better type names, elements can optionally add a virtual method:
 *
 * @code{.cpp}
 * class Image : public UIElement {
 *     virtual const char* GetDebugTypeName() const { return "Image"; }
 * };
 * @endcode
 *
 * But this is NOT required - the visualizer works with RTTI alone.
 *
 * # Performance
 *
 * - **Minimal overhead**: Only active when enabled
 * - **Efficient rendering**: Uses existing BatchRenderer
 * - **No allocations** per frame (except string for type name)
 * - **Skips invisible elements**: Only renders visible UI
 *
 * # Example Integration
 *
 * See examples/src/ui_image_scene.cpp for a complete working example.
 *
 * @note This visualizer is meant for development/debugging only. Disable it in production builds for optimal performance.
 *
 * @see engine::ui::UIElement
 * @see engine::ui::batch_renderer::BatchRenderer
 */
class UIDebugVisualizer {
private:
	bool enabled_ = false;
	bool show_labels_ = true;
	bool show_bounds_ = true;

	// Debug colors
	engine::ui::batch_renderer::Color bounds_color_{1.0f, 0.0f, 1.0f, 1.0f}; // Magenta
	engine::ui::batch_renderer::Color label_bg_color_{0.0f, 0.0f, 0.0f, 0.7f}; // Semi-transparent black
	engine::ui::batch_renderer::Color label_text_color_{1.0f, 1.0f, 0.0f, 1.0f}; // Yellow

	/**
	 * @brief Recursively render debug overlay for element and its children
	 */
	void RenderElementDebug(const engine::ui::UIElement* element, int depth = 0) {
		using namespace engine::ui::batch_renderer;

		if (!element || !element->IsVisible()) {
			return;
		}

		const auto bounds = element->GetAbsoluteBounds();

		// Render bounds as outline (4 lines forming a rectangle)
		if (show_bounds_) {
			const float x = bounds.x;
			const float y = bounds.y;
			const float w = bounds.width;
			const float h = bounds.height;
			const float thickness = 1.0f;

			// Adjust bounds color based on depth for visual hierarchy
			Color depth_color = bounds_color_;
			depth_color.a = 1.0f - (depth * 0.1f); // Fade deeper elements
			if (depth_color.a < 0.3f)
				depth_color.a = 0.3f;

			BatchRenderer::SubmitLine(x, y, x + w, y, thickness, depth_color); // Top
			BatchRenderer::SubmitLine(x + w, y, x + w, y + h, thickness, depth_color); // Right
			BatchRenderer::SubmitLine(x + w, y + h, x, y + h, thickness, depth_color); // Bottom
			BatchRenderer::SubmitLine(x, y + h, x, y, thickness, depth_color); // Left
		}

		// Render label above bounds
		if (show_labels_) {
			const std::string label = GetElementTypeName(element);
			const float label_x = bounds.x;
			float label_y = bounds.y - 18.0f; // Above the element
			const int label_font_size = 12;

			// Get default font for measurement
			auto* font = engine::ui::text_renderer::FontManager::GetDefaultFont();
			if (font && font->IsValid()) {
				// Measure text bounds properly
				auto text_bounds = engine::ui::text_renderer::TextLayout::MeasureText(
						label, *font, 0.0f // No wrapping
				);

				label_y = bounds.y - text_bounds.height - 4.0f; // Use measured height with a small offset

				// Background for label (with padding)
				const float padding = 4.0f;
				const Rectangle label_bg{
						label_x - padding,
						label_y - padding,
						text_bounds.width + padding * 2,
						text_bounds.height + padding * 2};
				BatchRenderer::SubmitQuad(label_bg, label_bg_color_);

				// Label text
				BatchRenderer::SubmitText(label, label_x, label_y, label_font_size, label_text_color_);
			}
			else {
				// Fallback if font not available (crude estimate)
				const Rectangle label_bg{
						label_x - 2.0f, label_y - 2.0f, static_cast<float>(label.length() * 8 + 4), 18.0f};
				BatchRenderer::SubmitQuad(label_bg, label_bg_color_);
				BatchRenderer::SubmitText(label, label_x, label_y, label_font_size, label_text_color_);
			}
		}

		// Recursively render children
		for (const auto& child : element->GetChildren()) {
			RenderElementDebug(child.get(), depth + 1);
		}
	}

	/**
	 * @brief Get a human-readable type name for the element
	 *
	 * Uses simple RTTI to identify element type. In the future, could use
	 * a virtual GetTypeName() method for more accurate identification.
	 */
	std::string GetElementTypeName(const engine::ui::UIElement* element) {
		// Try to identify type using typeid (basic RTTI)
		// This is not ideal but works without modifying UIElement
		const std::string type_name = typeid(*element).name();

		// Clean up the type name (compiler-specific mangling)
		// For MSVC: "class engine::ui::elements::Image" -> "Image"
		// For GCC/Clang: "N6engine2ui8elements5ImageE" -> needs demangling

		std::string clean_name;

#ifdef _MSC_VER
		// MSVC format: "class engine::ui::elements::Image"
		size_t last_colon = type_name.find_last_of(':');
		if (last_colon != std::string::npos) {
			clean_name = type_name.substr(last_colon + 1);
		}
		else {
			// Remove "class " prefix if present
			if (type_name.find("class ") == 0) {
				clean_name = type_name.substr(6);
			}
			else {
				clean_name = type_name;
			}
		}
#else
		// GCC/Clang format: mangled name - for now just use "UIElement"
		// Could use __cxa_demangle for proper demangling
		clean_name = "UIElement";
#endif

		// Fallback if something went wrong
		if (clean_name.empty()) {
			clean_name = "UIElement";
		}

		return clean_name;
	}

public:
	/**
	 * @brief Enable/disable debug visualization
	 */
	void SetEnabled(bool enabled) { enabled_ = enabled; }

	/**
	 * @brief Check if debug visualization is enabled
	 */
	bool IsEnabled() const { return enabled_; }

	/**
	 * @brief Toggle debug visualization on/off
	 */
	void ToggleEnabled() { enabled_ = !enabled_; }

	/**
	 * @brief Enable/disable label rendering
	 */
	void SetShowLabels(bool show) { show_labels_ = show; }

	/**
	 * @brief Enable/disable bounds rendering
	 */
	void SetShowBounds(bool show) { show_bounds_ = show; }

	/**
	 * @brief Set color for bounds lines
	 */
	void SetBoundsColor(const engine::ui::batch_renderer::Color& color) { bounds_color_ = color; }

	/**
	 * @brief Render debug overlay for UI element tree
	 *
	 * Call this AFTER rendering your normal UI, so debug overlay appears on top.
	 * This must be called between BatchRenderer::BeginFrame() and EndFrame().
	 *
	 * @param root Root UI element to visualize (will recursively visualize children)
	 *
	 * @code
	 * BatchRenderer::BeginFrame();
	 *
	 * // Render normal UI
	 * my_ui_element->Render();
	 *
	 * // Render debug overlay on top
	 * debugger.RenderDebugOverlay(my_ui_element.get());
	 *
	 * BatchRenderer::EndFrame();
	 * @endcode
	 */
	void RenderDebugOverlay(const engine::ui::UIElement* root) {
		if (!enabled_ || !root) {
			return;
		}

		RenderElementDebug(root, 0);
	}

	/**
	 * @brief Render ImGui controls for debug options
	 *
	 * Call this in your ImGui rendering code to add a debug panel.
	 *
	 * @code
	 * ImGui::Begin("Debug Controls");
	 * debugger.RenderImGuiControls();
	 * ImGui::End();
	 * @endcode
	 */
	void RenderImGuiControls() {

		ImGui::Checkbox("Enable UI Debug", &enabled_);

		if (enabled_) {
			ImGui::Indent();
			ImGui::Checkbox("Show Bounds", &show_bounds_);
			ImGui::Checkbox("Show Labels", &show_labels_);

			ImGui::Text("Bounds Color:");
			ImGui::ColorEdit4("##BoundsColor", &bounds_color_.r);

			ImGui::Text("Label Text Color:");
			ImGui::ColorEdit4("##LabelTextColor", &label_text_color_.r);

			ImGui::Text("Label Background:");
			ImGui::ColorEdit4("##LabelBgColor", &label_bg_color_.r);

			ImGui::Unindent();
		}
	}
};
