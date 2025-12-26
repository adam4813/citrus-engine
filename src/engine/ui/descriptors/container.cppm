module;

#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

export module engine.ui:descriptors.container;

import :descriptors.common;
import :descriptors.button;
import :descriptors.panel;
import :descriptors.label;
import :descriptors.slider;
import :descriptors.checkbox;
import :descriptors.divider;
import :descriptors.progress_bar;
import :descriptors.image;
import engine.ui.batch_renderer;

export namespace engine::ui::descriptor {

/**
 * @brief Variant type for child UI element descriptors
 *
 * Used in ContainerDescriptor to describe nested children.
 * Does not include ContainerDescriptor to avoid circular dependency.
 */
using UIDescriptorVariant = std::variant<ButtonDescriptor,
										 PanelDescriptor,
										 LabelDescriptor,
										 SliderDescriptor,
										 CheckboxDescriptor,
										 DividerDescriptor,
										 ProgressBarDescriptor,
										 ImageDescriptor>;

/**
 * @brief Declarative descriptor for Container elements with children
 *
 * Supports nested child descriptors for building complex UI hierarchies:
 * @code
 * auto desc = ContainerDescriptor{
 *     .bounds = {100, 100, 300, 400},
 *     .background = Colors::DARK_GRAY,
 *     .padding = 10.0f,
 *     .children = {
 *         LabelDescriptor{
 *             .bounds = {0, 0, 200, 24},
 *             .text = "Settings",
 *             .style = {.font_size = 20.0f, .color = Colors::GOLD}
 *         },
 *         SliderDescriptor{
 *             .bounds = {0, 40, 200, 30},
 *             .label = "Volume",
 *             .min_value = 0.0f,
 *             .max_value = 100.0f
 *         },
 *         ButtonDescriptor{
 *             .bounds = {0, 100, 100, 40},
 *             .label = "Apply"
 *         }
 *     }
 * };
 * auto container = UIFactory::Create(desc);
 * @endcode
 */
struct ContainerDescriptor {
	/// Bounds (position and size)
	Bounds bounds;

	/// Background color
	batch_renderer::Color background{batch_renderer::Colors::DARK_GRAY};

	/// Border styling
	BorderStyle border;

	/// Inner padding
	float padding{0.0f};

	/// Opacity (0.0-1.0)
	float opacity{1.0f};

	/// Whether to clip children to bounds
	bool clip_children{false};

	/// Whether container is initially visible
	bool visible{true};

	/// Child element descriptors
	std::vector<UIDescriptorVariant> children;
};

/**
 * @brief Complete variant type including ContainerDescriptor
 *
 * Use this for JSON serialization and factory creation of any UI element.
 */
using CompleteUIDescriptor = std::variant<ButtonDescriptor,
										  PanelDescriptor,
										  LabelDescriptor,
										  SliderDescriptor,
										  CheckboxDescriptor,
										  DividerDescriptor,
										  ProgressBarDescriptor,
										  ImageDescriptor,
										  ContainerDescriptor>;

// --- JSON Serialization for variants ---

// Forward declarations for recursive serialization
inline void to_json(nlohmann::json& j, const UIDescriptorVariant& v);
inline void from_json(const nlohmann::json& j, UIDescriptorVariant& v);

inline void to_json(nlohmann::json& j, const ContainerDescriptor& d) {
	j = nlohmann::json{
		{"type", "container"},
		{"bounds", detail::bounds_to_json(d.bounds)},
		{"background", detail::color_to_json(d.background)},
		{"border", detail::border_to_json(d.border)},
		{"padding", d.padding},
		{"opacity", d.opacity},
		{"clip_children", d.clip_children},
		{"visible", d.visible}
	};
	if (!d.children.empty()) {
		j["children"] = nlohmann::json::array();
		for (const auto& child : d.children) {
			nlohmann::json child_json;
			to_json(child_json, child);
			j["children"].push_back(child_json);
		}
	}
}

inline void from_json(const nlohmann::json& j, ContainerDescriptor& d) {
	if (j.contains("bounds")) {
		d.bounds = detail::bounds_from_json(j["bounds"]);
	}
	if (j.contains("background")) {
		d.background = detail::color_from_json(j["background"]);
	}
	if (j.contains("border")) {
		d.border = detail::border_from_json(j["border"]);
	}
	d.padding = j.value("padding", 0.0f);
	d.opacity = j.value("opacity", 1.0f);
	d.clip_children = j.value("clip_children", false);
	d.visible = j.value("visible", true);
	if (j.contains("children") && j["children"].is_array()) {
		for (const auto& child_json : j["children"]) {
			UIDescriptorVariant child;
			from_json(child_json, child);
			d.children.push_back(std::move(child));
		}
	}
}

// --- Variant serialization ---

inline void to_json(nlohmann::json& j, const UIDescriptorVariant& v) {
	std::visit([&j](const auto& d) { to_json(j, d); }, v);
}

inline void from_json(const nlohmann::json& j, UIDescriptorVariant& v) {
	const std::string type = j.value("type", "panel");
	if (type == "button") {
		v = j.get<ButtonDescriptor>();
	}
	else if (type == "label") {
		v = j.get<LabelDescriptor>();
	}
	else if (type == "slider") {
		v = j.get<SliderDescriptor>();
	}
	else if (type == "checkbox") {
		v = j.get<CheckboxDescriptor>();
	}
	else if (type == "divider") {
		v = j.get<DividerDescriptor>();
	}
	else if (type == "progress_bar") {
		v = j.get<ProgressBarDescriptor>();
	}
	else if (type == "image") {
		v = j.get<ImageDescriptor>();
	}
	else {
		v = j.get<PanelDescriptor>();
	}
}

inline void to_json(nlohmann::json& j, const CompleteUIDescriptor& v) {
	std::visit([&j](const auto& d) { to_json(j, d); }, v);
}

inline void from_json(const nlohmann::json& j, CompleteUIDescriptor& v) {
	const std::string type = j.value("type", "panel");
	if (type == "button") {
		v = j.get<ButtonDescriptor>();
	}
	else if (type == "panel") {
		v = j.get<PanelDescriptor>();
	}
	else if (type == "label") {
		v = j.get<LabelDescriptor>();
	}
	else if (type == "slider") {
		v = j.get<SliderDescriptor>();
	}
	else if (type == "checkbox") {
		v = j.get<CheckboxDescriptor>();
	}
	else if (type == "divider") {
		v = j.get<DividerDescriptor>();
	}
	else if (type == "progress_bar") {
		v = j.get<ProgressBarDescriptor>();
	}
	else if (type == "image") {
		v = j.get<ImageDescriptor>();
	}
	else if (type == "container") {
		v = j.get<ContainerDescriptor>();
	}
	else {
		v = j.get<PanelDescriptor>();
	}
}

} // namespace engine::ui::descriptor
