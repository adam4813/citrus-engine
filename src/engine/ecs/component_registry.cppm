module;

#include <cstdint>
#include <flecs.h>
#include <string>
#include <type_traits>
#include <vector>

export module engine.ecs.component_registry;

import glm;

export namespace engine::ecs {

// === FIELD REFLECTION SYSTEM ===

/**
 * @brief Supported field types for component editing
 */
enum class FieldType {
	Bool,
	Int,
	Float,
	String,
	Vec2,
	Vec3,
	Vec4,
	Color, // Vec4 displayed as color picker
	ListInt,
	ListFloat,
	ListString,
	ReadOnly, // Display-only string
	AssetRef, // Reference to a scene asset (dropdown)
	Enum // Integer-backed enum displayed as combo box
};

/**
 * @brief Metadata about a single field within a component
 */
struct FieldInfo {
	std::string name;
	FieldType type{FieldType::ReadOnly};
	size_t offset{}; // Byte offset into component struct
	size_t size{}; // Size of the field in bytes
	std::string asset_type; // For AssetRef: the asset type key (e.g., "shader", "mesh")
	std::vector<std::string> enum_labels; // For Enum: display labels (index = integer value)
	std::vector<std::string> enum_tooltips; // For Enum: tooltips for each option (index = integer value)
};

/**
 * @brief Information about a registered component
 */
struct ComponentInfo {
	std::string name;
	std::string category; // User-defined category string
	flecs::entity_t id = 0;
	std::vector<FieldInfo> fields;
};

// === TYPE DEDUCTION TRAITS ===

template <typename T> struct FieldTypeTraits {
	static constexpr auto type = FieldType::ReadOnly; // Default fallback
};

template <> struct FieldTypeTraits<bool> {
	static constexpr auto type = FieldType::Bool;
};

template <> struct FieldTypeTraits<int> {
	static constexpr auto type = FieldType::Int;
};

template <> struct FieldTypeTraits<uint32_t> {
	static constexpr auto type = FieldType::Int;
};

template <> struct FieldTypeTraits<size_t> {
	static constexpr auto type = FieldType::Int;
};

template <> struct FieldTypeTraits<float> {
	static constexpr auto type = FieldType::Float;
};

template <> struct FieldTypeTraits<double> {
	static constexpr auto type = FieldType::Float;
};

template <> struct FieldTypeTraits<std::string> {
	static constexpr auto type = FieldType::String;
};

template <> struct FieldTypeTraits<glm::vec2> {
	static constexpr auto type = FieldType::Vec2;
};

template <> struct FieldTypeTraits<glm::vec3> {
	static constexpr auto type = FieldType::Vec3;
};

template <> struct FieldTypeTraits<glm::vec4> {
	static constexpr auto type = FieldType::Vec4;
};

template <> struct FieldTypeTraits<std::vector<int>> {
	static constexpr auto type = FieldType::ListInt;
};

template <> struct FieldTypeTraits<std::vector<float>> {
	static constexpr auto type = FieldType::ListFloat;
};

template <> struct FieldTypeTraits<std::vector<std::string>> {
	static constexpr auto type = FieldType::ListString;
};

// Forward declaration
class ComponentRegistry;

/**
 * @brief Builder for registering components with fluent API
 *
 * Usage:
 *   ComponentRegistry::Instance().Register<Transform>("Transform", world)
 *       .Category("Core")
 *       .Field("position", &Transform::position)
 *       .Field("color", &Light::color, FieldType::Color)  // Override type
 *       .Build();
 */
template <typename T> class ComponentRegistration {
public:
	ComponentRegistration(ComponentRegistry& registry, const std::string& name, const flecs::world& world);

	ComponentRegistration& Category(const std::string& cat) {
		info_.category = cat;
		return *this;
	}

	/**
     * @brief Register a field with automatic type deduction
     * Also registers the field with flecs reflection for JSON serialization
     */
	template <typename FieldT> ComponentRegistration& Field(const std::string& field_name, FieldT T::* member_ptr) {
		FieldInfo field;
		field.name = field_name;
		field.type = FieldTypeTraits<FieldT>::type;
		field.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member_ptr));
		field.size = sizeof(FieldT);
		info_.fields.push_back(std::move(field));

		// Register with flecs reflection for JSON serialization
		RegisterFlecsMember<FieldT>(field_name);
		return *this;
	}

	/**
     * @brief Register a field with explicit type override
     * Also registers the field with flecs reflection for JSON serialization
     */
	template <typename FieldT>
	ComponentRegistration& Field(const std::string& field_name, FieldT T::* member_ptr, const FieldType type_override) {
		FieldInfo field;
		field.name = field_name;
		field.type = type_override;
		field.offset = reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*member_ptr));
		field.size = sizeof(FieldT);
		info_.fields.push_back(std::move(field));

		// Register with flecs reflection for JSON serialization
		RegisterFlecsMember<FieldT>(field_name);
		return *this;
	}

	/**
	 * @brief Mark the last field as an asset reference with the given asset type key
	 * Should be chained after Field() for AssetRef fields
	 */
	ComponentRegistration& AssetRef(const std::string& asset_type_key) {
		if (!info_.fields.empty()) {
			info_.fields.back().type = FieldType::AssetRef;
			info_.fields.back().asset_type = asset_type_key;
		}
		return *this;
	}
	ComponentRegistration& AssetRef(const std::string_view asset_type_key) {
		return AssetRef(std::string(asset_type_key));
	}

	/**
	 * @brief Mark the last field as an enum with the given display labels
	 * Labels are indexed by the integer value of the enum. Chain after Field().
	 */
	ComponentRegistration& EnumLabels(std::vector<std::string> labels) {
		if (!info_.fields.empty()) {
			info_.fields.back().type = FieldType::Enum;
			info_.fields.back().enum_labels = std::move(labels);
		}
		return *this;
	}

	/**
	 * @brief Add tooltips for the last field's enum options
	 * Tooltips are indexed by the integer value of the enum. Chain after EnumLabels().
	 */
	ComponentRegistration& EnumTooltips(std::vector<std::string> tips) {
		if (!info_.fields.empty()) {
			info_.fields.back().enum_tooltips = std::move(tips);
		}
		return *this;
	}

	void Build();

private:
	/**
     * @brief Register a member with flecs reflection system
     */
	template <typename FieldT> void RegisterFlecsMember(const std::string& field_name) {
		if constexpr (std::is_same_v<FieldT, bool>) {
			component_.member<bool>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, int>) {
			component_.member<int>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, uint32_t>) {
			component_.member<uint32_t>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, size_t>) {
			component_.member<size_t>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, float>) {
			component_.member<float>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, double>) {
			component_.member<double>(field_name.c_str());
		}
		else if constexpr (std::is_enum_v<FieldT>) {
			using UnderlyingT = std::underlying_type_t<FieldT>;
			component_.member<UnderlyingT>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, glm::vec2>) {
			component_.member<glm::vec2>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, glm::vec3>) {
			component_.member<glm::vec3>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, glm::vec4>) {
			component_.member<glm::vec4>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, glm::ivec2>) {
			component_.member<glm::ivec2>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, glm::mat4>) {
			component_.member<glm::mat4>(field_name.c_str());
		}
		else if constexpr (std::is_same_v<FieldT, std::string>) {
			component_.member<std::string>(field_name.c_str());
		}
		// Unknown types are skipped - they won't be serialized
	}

	ComponentRegistry& registry_;
	ComponentInfo info_;
	flecs::untyped_component component_;
};

/**
 * @brief Singleton registry of all available components
 *
 * Components register themselves using the builder pattern in ECSWorld constructor.
 * Registry only stores metadata - use flecs APIs directly for adding/checking components.
 */
class ComponentRegistry {
public:
	/**
     * @brief Get the singleton instance
     */
	static ComponentRegistry& Instance();

	/**
     * @brief Start registering a component with builder pattern
     */
	template <typename T> ComponentRegistration<T> Register(const std::string& name, flecs::world& world) {
		return ComponentRegistration<T>(*this, name, world);
	}

	/**
     * @brief Get list of all registered components
     */
	[[nodiscard]] const std::vector<ComponentInfo>& GetComponents() const { return components_; }

	/**
     * @brief Get unique category names (sorted)
     */
	[[nodiscard]] std::vector<std::string> GetCategories() const;

	/**
     * @brief Get components filtered by category
     */
	[[nodiscard]] std::vector<const ComponentInfo*> GetComponentsByCategory(const std::string& category) const;

	/**
     * @brief Find component by name
     * @return nullptr if not found
     */
	[[nodiscard]] const ComponentInfo* FindComponent(const std::string& name) const;

	// Called by ComponentRegistration::Build()
	void AddComponent(ComponentInfo info) { components_.push_back(std::move(info)); }

private:
	ComponentRegistry() = default;
	~ComponentRegistry() = default;
	ComponentRegistry(const ComponentRegistry&) = delete;
	ComponentRegistry& operator=(const ComponentRegistry&) = delete;

	std::vector<ComponentInfo> components_;
};

// Template implementation
template <typename T>
ComponentRegistration<T>::ComponentRegistration(
		ComponentRegistry& registry, const std::string& name, const flecs::world& world) :
		registry_(registry), component_(world.component<T>()) {
	info_.name = name;
	info_.category = "Other"; // Default category
	info_.id = component_.id();
}

template <typename T> void ComponentRegistration<T>::Build() { registry_.AddComponent(std::move(info_)); }

} // namespace engine::ecs
