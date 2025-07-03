export module engine;

// Re-export all engine subsystems for convenient access
export import engine.assets;
export import engine.platform;
export import engine.ecs.flecs;
export import engine.scene;
export import engine.rendering;
export import engine.os;

// Main engine namespace
export namespace engine {
    // Engine-wide utilities and constants can go here
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    [[nodiscard]] constexpr auto GetVersionString() noexcept -> const char * { return "1.0.0"; }
} // namespace engine
