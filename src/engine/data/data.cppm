module;

export module engine.data;

// Re-export all data submodules
export import :data_asset;
export import :data_table;
export import :data_asset_registry;
export import :data_serializer;

// Main data module namespace
export namespace engine::data {
// Version information
inline constexpr int DATA_FORMAT_VERSION = 1;
} // namespace engine::data
