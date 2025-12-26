module;

#include <memory>
#include <optional>
#include <sstream>
#include <string>
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
 * auto desc = UIJsonSerializer::FromJson<ContainerDescriptor>(json_str);
 * auto element = UIFactory::Create(desc);
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
	 * @brief Serialize a descriptor to JSON string
	 *
	 * @tparam T Descriptor type (ButtonDescriptor, PanelDescriptor, etc.)
	 * @param desc Descriptor to serialize
	 * @param indent Indentation for pretty printing (-1 for compact)
	 * @return JSON string
	 */
	template <typename T> static std::string ToJson(const T& desc, const int indent = 2) {
		nlohmann::json j = ToJsonObject(desc);
		return j.dump(indent);
	}

	/**
	 * @brief Serialize a descriptor variant to JSON string
	 */
	static std::string ToJson(const CompleteUIDescriptor& desc, const int indent = 2) {
		nlohmann::json j = std::visit([](const auto& d) { return ToJsonObject(d); }, desc);
		return j.dump(indent);
	}

	// ========================================================================
	// Deserialization (JSON -> Descriptor)
	// ========================================================================

	/**
	 * @brief Deserialize a descriptor from JSON string
	 *
	 * @tparam T Descriptor type to deserialize to
	 * @param json_str JSON string
	 * @return Deserialized descriptor
	 */
	template <typename T> static T FromJson(const std::string& json_str) {
		const nlohmann::json j = nlohmann::json::parse(json_str);
		return FromJsonObject<T>(j);
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
		return Bounds{
			j.value("x", 0.0f), j.value("y", 0.0f), j.value("width", 100.0f), j.value("height", 100.0f)};
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
	// Button Serialization
	// ========================================================================

	static nlohmann::json ToJsonObject(const ButtonDescriptor& desc) {
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

	template <> static ButtonDescriptor FromJsonObject<ButtonDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const PanelDescriptor& desc) {
		return nlohmann::json{{"type", "panel"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"background", ColorToJson(desc.background)},
							  {"border", BorderStyleToJson(desc.border)},
							  {"padding", desc.padding},
							  {"opacity", desc.opacity},
							  {"clip_children", desc.clip_children},
							  {"visible", desc.visible}};
	}

	template <> static PanelDescriptor FromJsonObject<PanelDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const LabelDescriptor& desc) {
		return nlohmann::json{{"type", "label"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"text", desc.text},
							  {"style", TextStyleToJson(desc.style)},
							  {"visible", desc.visible}};
	}

	template <> static LabelDescriptor FromJsonObject<LabelDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const SliderDescriptor& desc) {
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

	template <> static SliderDescriptor FromJsonObject<SliderDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const CheckboxDescriptor& desc) {
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

	template <> static CheckboxDescriptor FromJsonObject<CheckboxDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const DividerDescriptor& desc) {
		return nlohmann::json{{"type", "divider"},
							  {"bounds", BoundsToJson(desc.bounds)},
							  {"color", ColorToJson(desc.color)},
							  {"horizontal", desc.horizontal},
							  {"visible", desc.visible}};
	}

	template <> static DividerDescriptor FromJsonObject<DividerDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const ProgressBarDescriptor& desc) {
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

	template <> static ProgressBarDescriptor FromJsonObject<ProgressBarDescriptor>(const nlohmann::json& j) {
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

	static nlohmann::json ToJsonObject(const ImageDescriptor& desc) {
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

	template <> static ImageDescriptor FromJsonObject<ImageDescriptor>(const nlohmann::json& j) {
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
			desc.uv_coords = UVCoords{uv.value("u0", 0.0f), uv.value("v0", 0.0f), uv.value("u1", 1.0f),
									  uv.value("v1", 1.0f)};
		}
		desc.visible = j.value("visible", true);
		return desc;
	}

	// ========================================================================
	// Container Serialization (with children)
	// ========================================================================

	static nlohmann::json ChildVariantToJson(const UIDescriptorVariant& child) {
		return std::visit([](const auto& c) { return ToJsonObject(c); }, child);
	}

	static UIDescriptorVariant ChildVariantFromJson(const nlohmann::json& j) {
		const std::string type = j.value("type", "panel");

		if (type == "button") {
			return FromJsonObject<ButtonDescriptor>(j);
		}
		if (type == "label") {
			return FromJsonObject<LabelDescriptor>(j);
		}
		if (type == "slider") {
			return FromJsonObject<SliderDescriptor>(j);
		}
		if (type == "checkbox") {
			return FromJsonObject<CheckboxDescriptor>(j);
		}
		if (type == "divider") {
			return FromJsonObject<DividerDescriptor>(j);
		}
		if (type == "progress_bar") {
			return FromJsonObject<ProgressBarDescriptor>(j);
		}
		if (type == "image") {
			return FromJsonObject<ImageDescriptor>(j);
		}
		// Default to panel
		return FromJsonObject<PanelDescriptor>(j);
	}

	static nlohmann::json ToJsonObject(const ContainerDescriptor& desc) {
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

	template <> static ContainerDescriptor FromJsonObject<ContainerDescriptor>(const nlohmann::json& j) {
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
			return FromJsonObject<ButtonDescriptor>(j);
		}
		if (type == "label") {
			return FromJsonObject<LabelDescriptor>(j);
		}
		if (type == "slider") {
			return FromJsonObject<SliderDescriptor>(j);
		}
		if (type == "checkbox") {
			return FromJsonObject<CheckboxDescriptor>(j);
		}
		if (type == "divider") {
			return FromJsonObject<DividerDescriptor>(j);
		}
		if (type == "progress_bar") {
			return FromJsonObject<ProgressBarDescriptor>(j);
		}
		if (type == "image") {
			return FromJsonObject<ImageDescriptor>(j);
		}
		if (type == "container") {
			return FromJsonObject<ContainerDescriptor>(j);
		}
		// Default to panel
		return FromJsonObject<PanelDescriptor>(j);
	}

	// Generic template for unknown types
	template <typename T> static T FromJsonObject(const nlohmann::json& /*j*/) {
		static_assert(sizeof(T) == 0, "No FromJsonObject specialization for this type");
		return T{};
	}
};

} // namespace engine::ui::descriptor
