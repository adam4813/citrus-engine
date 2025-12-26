module;

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

export module engine.ui:json_serializer;

import :descriptor;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief JSON serializer for UI descriptors
 *
 * Provides bidirectional JSON serialization for all UI descriptor types.
 * This enables:
 * - Saving UI layouts to JSON files
 * - Loading UI definitions from resource files
 * - Hot-reloading UI during development
 * - Data-driven UI configuration
 *
 * **Usage:**
 * @code
 * using namespace engine::ui::descriptor;
 *
 * // Serialize a container to JSON
 * auto container = ContainerDescriptor{
 *     .bounds = {0, 0, 400, 300},
 *     .children = {
 *         ButtonDescriptor{.bounds = {10, 10, 100, 30}, .label = "OK"}
 *     }
 * };
 * std::string json_str = UIJsonSerializer::ToJson(container);
 *
 * // Deserialize from JSON
 * auto desc = UIJsonSerializer::ButtonFromJson(json_str);  // Type-specific
 * auto element = UIFactory::Create(desc);
 *
 * // Or auto-detect from "type" field
 * auto variant = UIJsonSerializer::FromJsonAuto(json_str);
 * @endcode
 *
 * **JSON Format Example:**
 * @code
 * {
 *   "type": "container",
 *   "bounds": {"x": 0, "y": 0, "width": 400, "height": 300},
 *   "background": {"r": 0.2, "g": 0.2, "b": 0.2, "a": 1.0},
 *   "padding": 10,
 *   "children": [
 *     {
 *       "type": "button",
 *       "bounds": {"x": 10, "y": 10, "width": 100, "height": 30},
 *       "label": "OK"
 *     }
 *   ]
 * }
 * @endcode
 */
class UIJsonSerializer {
public:
	// ========================================================================
	// Serialization (Descriptor -> JSON)
	// ========================================================================

	/**
	 * @brief Serialize a Button descriptor to JSON string
	 */
	static std::string ToJson(const ButtonDescriptor& desc, const int indent = 2) {
		return ButtonToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Panel descriptor to JSON string
	 */
	static std::string ToJson(const PanelDescriptor& desc, const int indent = 2) {
		return PanelToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Label descriptor to JSON string
	 */
	static std::string ToJson(const LabelDescriptor& desc, const int indent = 2) {
		return LabelToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Slider descriptor to JSON string
	 */
	static std::string ToJson(const SliderDescriptor& desc, const int indent = 2) {
		return SliderToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Checkbox descriptor to JSON string
	 */
	static std::string ToJson(const CheckboxDescriptor& desc, const int indent = 2) {
		return CheckboxToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Divider descriptor to JSON string
	 */
	static std::string ToJson(const DividerDescriptor& desc, const int indent = 2) {
		return DividerToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a ProgressBar descriptor to JSON string
	 */
	static std::string ToJson(const ProgressBarDescriptor& desc, const int indent = 2) {
		return ProgressBarToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize an Image descriptor to JSON string
	 */
	static std::string ToJson(const ImageDescriptor& desc, const int indent = 2) {
		return ImageToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a Container descriptor to JSON string
	 */
	static std::string ToJson(const ContainerDescriptor& desc, const int indent = 2) {
		return ContainerToJsonObject(desc).dump(indent);
	}

	/**
	 * @brief Serialize a descriptor variant to JSON string
	 */
	static std::string ToJson(const CompleteUIDescriptor& desc, const int indent = 2) {
		nlohmann::json j = std::visit([](const auto& d) { return DescriptorToJsonObject(d); }, desc);
		return j.dump(indent);
	}

	// ========================================================================
	// Deserialization (JSON -> Descriptor) - Type-specific methods
	// ========================================================================

	/**
	 * @brief Deserialize a Button descriptor from JSON string
	 */
	static ButtonDescriptor ButtonFromJson(const std::string& json_str) {
		return ButtonFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Panel descriptor from JSON string
	 */
	static PanelDescriptor PanelFromJson(const std::string& json_str) {
		return PanelFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Label descriptor from JSON string
	 */
	static LabelDescriptor LabelFromJson(const std::string& json_str) {
		return LabelFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Slider descriptor from JSON string
	 */
	static SliderDescriptor SliderFromJson(const std::string& json_str) {
		return SliderFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Checkbox descriptor from JSON string
	 */
	static CheckboxDescriptor CheckboxFromJson(const std::string& json_str) {
		return CheckboxFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Divider descriptor from JSON string
	 */
	static DividerDescriptor DividerFromJson(const std::string& json_str) {
		return DividerFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a ProgressBar descriptor from JSON string
	 */
	static ProgressBarDescriptor ProgressBarFromJson(const std::string& json_str) {
		return ProgressBarFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize an Image descriptor from JSON string
	 */
	static ImageDescriptor ImageFromJson(const std::string& json_str) {
		return ImageFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize a Container descriptor from JSON string
	 */
	static ContainerDescriptor ContainerFromJson(const std::string& json_str) {
		return ContainerFromJsonObject(nlohmann::json::parse(json_str));
	}

	/**
	 * @brief Deserialize any descriptor type from JSON (uses "type" field)
	 *
	 * @param json_str JSON string with "type" field
	 * @return Variant containing the appropriate descriptor
	 */
	static CompleteUIDescriptor FromJsonAuto(const std::string& json_str) {
		const nlohmann::json j = nlohmann::json::parse(json_str);
		return FromJsonObjectAuto(j);
	}

private:
	// ========================================================================
	// Color Serialization
	// ========================================================================

	static nlohmann::json ColorToJson(const batch_renderer::Color& color) {
		return nlohmann::json{{"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a}};
	}

	static batch_renderer::Color ColorFromJson(const nlohmann::json& j) {
		return batch_renderer::Color{j.value("r", 1.0f), j.value("g", 1.0f), j.value("b", 1.0f), j.value("a", 1.0f)};
	}

	// ========================================================================
	// Bounds Serialization
	// ========================================================================

	static nlohmann::json BoundsToJson(const Bounds& bounds) {
		return nlohmann::json{{"x", bounds.x}, {"y", bounds.y}, {"width", bounds.width}, {"height", bounds.height}};
	}

	static Bounds BoundsFromJson(const nlohmann::json& j) {
		return Bounds{j.value("x", 0.0f), j.value("y", 0.0f), j.value("width", 100.0f), j.value("height", 100.0f)};
	}

	// ========================================================================
	// BorderStyle Serialization
	// ========================================================================

	static nlohmann::json BorderStyleToJson(const BorderStyle& border) {
		return nlohmann::json{{"width", border.width}, {"color", ColorToJson(border.color)}};
	}

	static BorderStyle BorderStyleFromJson(const nlohmann::json& j) {
		BorderStyle border;
		border.width = j.value("width", 0.0f);
		if (j.contains("color")) {
			border.color = ColorFromJson(j["color"]);
		}
		return border;
	}

	// ========================================================================
	// TextStyle Serialization
	// ========================================================================

	static nlohmann::json TextStyleToJson(const TextStyle& style) {
		return nlohmann::json{{"font_size", style.font_size}, {"color", ColorToJson(style.color)}};
	}

	static TextStyle TextStyleFromJson(const nlohmann::json& j) {
		TextStyle style;
		style.font_size = j.value("font_size", 16.0f);
		if (j.contains("color")) {
			style.color = ColorFromJson(j["color"]);
		}
		return style;
	}

	// ========================================================================
	// Generic Descriptor to JSON (used for variant serialization)
	// ========================================================================

	/**
	 * @brief Helper template to convert any descriptor type to JSON object
	 * 
	 * Uses if-constexpr to dispatch to the appropriate type-specific function.
	 * This eliminates code duplication between CompleteUIDescriptor and
	 * UIDescriptorVariant serialization.
	 */
	template <typename T>
	static nlohmann::json DescriptorToJsonObject(const T& desc) {
		if constexpr (std::is_same_v<T, ButtonDescriptor>) {
			return ButtonToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, PanelDescriptor>) {
			return PanelToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, LabelDescriptor>) {
			return LabelToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, SliderDescriptor>) {
			return SliderToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, CheckboxDescriptor>) {
			return CheckboxToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, DividerDescriptor>) {
			return DividerToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, ProgressBarDescriptor>) {
			return ProgressBarToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, ImageDescriptor>) {
			return ImageToJsonObject(desc);
		}
		else if constexpr (std::is_same_v<T, ContainerDescriptor>) {
			return ContainerToJsonObject(desc);
		}
		else {
			return nlohmann::json{};
		}
	}

	// ========================================================================
	// Button Serialization
	// ========================================================================

	static nlohmann::json ButtonToJsonObject(const ButtonDescriptor& desc) {
		return nlohmann::json{{"type", "button"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"label", desc.label},
							  {"text_style", TextStyleToJson(desc.text_style)},
							  {"normal_color", ColorToJson(desc.normal_color)},
							  {"hover_color", ColorToJson(desc.hover_color)},
							  {"pressed_color", ColorToJson(desc.pressed_color)},
							  {"disabled_color", ColorToJson(desc.disabled_color)},
							  {"border", BorderStyleToJson(desc.border)},
							  {"enabled", desc.enabled},
							  {"visible", desc.visible}};
	}

	static ButtonDescriptor ButtonFromJsonObject(const nlohmann::json& j) {
		ButtonDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.label = j.value("label", "");
		if (j.contains("text_style")) {
			desc.text_style = TextStyleFromJson(j["text_style"]);
		}
		if (j.contains("normal_color")) {
			desc.normal_color = ColorFromJson(j["normal_color"]);
		}
		if (j.contains("hover_color")) {
			desc.hover_color = ColorFromJson(j["hover_color"]);
		}
		if (j.contains("pressed_color")) {
			desc.pressed_color = ColorFromJson(j["pressed_color"]);
		}
		if (j.contains("disabled_color")) {
			desc.disabled_color = ColorFromJson(j["disabled_color"]);
		}
		if (j.contains("border")) {
			desc.border = BorderStyleFromJson(j["border"]);
		}
		desc.enabled = j.value("enabled", true);
		desc.visible = j.value("visible", true);
		// Note: Callbacks cannot be serialized
		return desc;
	}

	// ========================================================================
	// Panel Serialization
	// ========================================================================

	static nlohmann::json PanelToJsonObject(const PanelDescriptor& desc) {
		return nlohmann::json{{"type", "panel"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"background", ColorToJson(desc.background)},
							  {"border", BorderStyleToJson(desc.border)},
							  {"padding", desc.padding},
							  {"opacity", desc.opacity},
							  {"clip_children", desc.clip_children},
							  {"visible", desc.visible}};
	}

	static PanelDescriptor PanelFromJsonObject(const nlohmann::json& j) {
		PanelDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		if (j.contains("background")) {
			desc.background = ColorFromJson(j["background"]);
		}
		if (j.contains("border")) {
			desc.border = BorderStyleFromJson(j["border"]);
		}
		desc.padding = j.value("padding", 0.0f);
		desc.opacity = j.value("opacity", 1.0f);
		desc.clip_children = j.value("clip_children", false);
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// Label Serialization
	// ========================================================================

	static nlohmann::json LabelToJsonObject(const LabelDescriptor& desc) {
		return nlohmann::json{{"type", "label"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"text", desc.text},
							  {"style", TextStyleToJson(desc.style)},
							  {"visible", desc.visible}};
	}

	static LabelDescriptor LabelFromJsonObject(const nlohmann::json& j) {
		LabelDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.text = j.value("text", "");
		if (j.contains("style")) {
			desc.style = TextStyleFromJson(j["style"]);
		}
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// Slider Serialization
	// ========================================================================

	static nlohmann::json SliderToJsonObject(const SliderDescriptor& desc) {
		return nlohmann::json{{"type", "slider"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"min_value", desc.min_value},
							  {"max_value", desc.max_value},
							  {"initial_value", desc.initial_value},
							  {"label", desc.label},
							  {"track_color", ColorToJson(desc.track_color)},
							  {"fill_color", ColorToJson(desc.fill_color)},
							  {"thumb_color", ColorToJson(desc.thumb_color)},
							  {"visible", desc.visible}};
	}

	static SliderDescriptor SliderFromJsonObject(const nlohmann::json& j) {
		SliderDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.min_value = j.value("min_value", 0.0f);
		desc.max_value = j.value("max_value", 1.0f);
		desc.initial_value = j.value("initial_value", 0.0f);
		desc.label = j.value("label", "");
		if (j.contains("track_color")) {
			desc.track_color = ColorFromJson(j["track_color"]);
		}
		if (j.contains("fill_color")) {
			desc.fill_color = ColorFromJson(j["fill_color"]);
		}
		if (j.contains("thumb_color")) {
			desc.thumb_color = ColorFromJson(j["thumb_color"]);
		}
		desc.visible = j.value("visible", true);
		// Note: Callbacks cannot be serialized
		return desc;
	}

	// ========================================================================
	// Checkbox Serialization
	// ========================================================================

	static nlohmann::json CheckboxToJsonObject(const CheckboxDescriptor& desc) {
		return nlohmann::json{{"type", "checkbox"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"label", desc.label},
							  {"initial_checked", desc.initial_checked},
							  {"unchecked_color", ColorToJson(desc.unchecked_color)},
							  {"checked_color", ColorToJson(desc.checked_color)},
							  {"text_style", TextStyleToJson(desc.text_style)},
							  {"enabled", desc.enabled},
							  {"visible", desc.visible}};
	}

	static CheckboxDescriptor CheckboxFromJsonObject(const nlohmann::json& j) {
		CheckboxDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.label = j.value("label", "");
		desc.initial_checked = j.value("initial_checked", false);
		if (j.contains("unchecked_color")) {
			desc.unchecked_color = ColorFromJson(j["unchecked_color"]);
		}
		if (j.contains("checked_color")) {
			desc.checked_color = ColorFromJson(j["checked_color"]);
		}
		if (j.contains("text_style")) {
			desc.text_style = TextStyleFromJson(j["text_style"]);
		}
		desc.enabled = j.value("enabled", true);
		desc.visible = j.value("visible", true);
		// Note: Callbacks cannot be serialized
		return desc;
	}

	// ========================================================================
	// Divider Serialization
	// ========================================================================

	static nlohmann::json DividerToJsonObject(const DividerDescriptor& desc) {
		return nlohmann::json{{"type", "divider"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"color", ColorToJson(desc.color)},
							  {"horizontal", desc.horizontal},
							  {"visible", desc.visible}};
	}

	static DividerDescriptor DividerFromJsonObject(const nlohmann::json& j) {
		DividerDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		if (j.contains("color")) {
			desc.color = ColorFromJson(j["color"]);
		}
		desc.horizontal = j.value("horizontal", true);
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// ProgressBar Serialization
	// ========================================================================

	static nlohmann::json ProgressBarToJsonObject(const ProgressBarDescriptor& desc) {
		return nlohmann::json{{"type", "progress_bar"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"initial_progress", desc.initial_progress},
							  {"label", desc.label},
							  {"show_percentage", desc.show_percentage},
							  {"track_color", ColorToJson(desc.track_color)},
							  {"fill_color", ColorToJson(desc.fill_color)},
							  {"text_style", TextStyleToJson(desc.text_style)},
							  {"border", BorderStyleToJson(desc.border)},
							  {"visible", desc.visible}};
	}

	static ProgressBarDescriptor ProgressBarFromJsonObject(const nlohmann::json& j) {
		ProgressBarDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.initial_progress = j.value("initial_progress", 0.0f);
		desc.label = j.value("label", "");
		desc.show_percentage = j.value("show_percentage", false);
		if (j.contains("track_color")) {
			desc.track_color = ColorFromJson(j["track_color"]);
		}
		if (j.contains("fill_color")) {
			desc.fill_color = ColorFromJson(j["fill_color"]);
		}
		if (j.contains("text_style")) {
			desc.text_style = TextStyleFromJson(j["text_style"]);
		}
		if (j.contains("border")) {
			desc.border = BorderStyleFromJson(j["border"]);
		}
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// Image Serialization
	// ========================================================================

	static nlohmann::json ImageToJsonObject(const ImageDescriptor& desc) {
		nlohmann::json j{{"type", "image"},
						 {"bounds", BoundsToJson(desc.bounds)},
						 {"texture_id", desc.texture_id},
						 {"tint", ColorToJson(desc.tint)},
						 {"visible", desc.visible}};

		if (desc.uv_coords) {
			j["uv_coords"] = nlohmann::json{{"u0", desc.uv_coords->u0},
											{"v0", desc.uv_coords->v0},
											{"u1", desc.uv_coords->u1},
											{"v1", desc.uv_coords->v1}};
		}

		return j;
	}

	static ImageDescriptor ImageFromJsonObject(const nlohmann::json& j) {
		ImageDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		desc.texture_id = j.value("texture_id", 0u);
		if (j.contains("tint")) {
			desc.tint = ColorFromJson(j["tint"]);
		}
		if (j.contains("uv_coords")) {
			const auto& uv = j["uv_coords"];
			desc.uv_coords =
				UVCoords{uv.value("u0", 0.0f), uv.value("v0", 0.0f), uv.value("u1", 1.0f), uv.value("v1", 1.0f)};
		}
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// Container Serialization (with children)
	// ========================================================================

	static nlohmann::json ChildVariantToJson(const UIDescriptorVariant& child) {
		return std::visit([](const auto& c) { return DescriptorToJsonObject(c); }, child);
	}

	static UIDescriptorVariant ChildVariantFromJson(const nlohmann::json& j) {
		const std::string type = j.value("type", "panel");

		if (type == "button") {
			return ButtonFromJsonObject(j);
		}
		if (type == "label") {
			return LabelFromJsonObject(j);
		}
		if (type == "slider") {
			return SliderFromJsonObject(j);
		}
		if (type == "checkbox") {
			return CheckboxFromJsonObject(j);
		}
		if (type == "divider") {
			return DividerFromJsonObject(j);
		}
		if (type == "progress_bar") {
			return ProgressBarFromJsonObject(j);
		}
		if (type == "image") {
			return ImageFromJsonObject(j);
		}
		// Default to panel
		return PanelFromJsonObject(j);
	}

	static nlohmann::json ContainerToJsonObject(const ContainerDescriptor& desc) {
		nlohmann::json j{{"type", "container"},
						 {"bounds", BoundsToJson(desc.bounds)},
						 {"background", ColorToJson(desc.background)},
						 {"border", BorderStyleToJson(desc.border)},
						 {"padding", desc.padding},
						 {"opacity", desc.opacity},
						 {"clip_children", desc.clip_children},
						 {"visible", desc.visible}};

		if (!desc.children.empty()) {
			nlohmann::json children_array = nlohmann::json::array();
			for (const auto& child : desc.children) {
				children_array.push_back(ChildVariantToJson(child));
			}
			j["children"] = children_array;
		}

		return j;
	}

	static ContainerDescriptor ContainerFromJsonObject(const nlohmann::json& j) {
		ContainerDescriptor desc;
		if (j.contains("bounds")) {
			desc.bounds = BoundsFromJson(j["bounds"]);
		}
		if (j.contains("background")) {
			desc.background = ColorFromJson(j["background"]);
		}
		if (j.contains("border")) {
			desc.border = BorderStyleFromJson(j["border"]);
		}
		desc.padding = j.value("padding", 0.0f);
		desc.opacity = j.value("opacity", 1.0f);
		desc.clip_children = j.value("clip_children", false);
		desc.visible = j.value("visible", true);

		if (j.contains("children") && j["children"].is_array()) {
			for (const auto& child_json : j["children"]) {
				desc.children.push_back(ChildVariantFromJson(child_json));
			}
		}

		return desc;
	}

	// ========================================================================
	// Auto-detection from "type" field
	// ========================================================================

	static CompleteUIDescriptor FromJsonObjectAuto(const nlohmann::json& j) {
		const std::string type = j.value("type", "panel");

		if (type == "button") {
			return ButtonFromJsonObject(j);
		}
		if (type == "label") {
			return LabelFromJsonObject(j);
		}
		if (type == "slider") {
			return SliderFromJsonObject(j);
		}
		if (type == "checkbox") {
			return CheckboxFromJsonObject(j);
		}
		if (type == "divider") {
			return DividerFromJsonObject(j);
		}
		if (type == "progress_bar") {
			return ProgressBarFromJsonObject(j);
		}
		if (type == "image") {
			return ImageFromJsonObject(j);
		}
		if (type == "container") {
			return ContainerFromJsonObject(j);
		}
		// Default to panel
		return PanelFromJsonObject(j);
	}
};

} // namespace engine::ui::descriptor
