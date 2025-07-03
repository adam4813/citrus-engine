# MOD_ANIMATION_v1

> **2D/3D Animation System with State Machines - Engine Module**

## Executive Summary

The `engine.animation` module provides a comprehensive animation system supporting 2D sprite animations, 3D skeletal
animations, state machines, animation blending, and procedural animation with seamless ECS integration for
high-performance real-time animation. This module implements efficient keyframe interpolation, bone hierarchies,
animation state machines, and timeline-based animation sequencing to deliver smooth character animation, UI transitions,
and environmental effects for the Colony Game Engine's dynamic visual elements across desktop and WebAssembly platforms.

## Scope and Objectives

### In Scope

- [ ] 2D sprite animation with texture atlas and frame sequences
- [ ] 3D skeletal animation with bone hierarchies and skinning
- [ ] Animation state machines with transition conditions
- [ ] Animation blending and layering for smooth transitions
- [ ] Timeline-based animation sequencing and curves
- [ ] Procedural animation and inverse kinematics

### Out of Scope

- [ ] Advanced facial animation and lip-sync
- [ ] Motion capture data processing
- [ ] Complex physics-based animation simulation
- [ ] Animation retargeting between different skeletons

### Primary Objectives

1. **Performance**: Animate 1,000+ sprites and 100+ skeletal meshes at 60 FPS
2. **Quality**: Smooth animation playback with sub-frame interpolation accuracy
3. **Flexibility**: Data-driven animation system with hot-reload capabilities

### Secondary Objectives

- Memory usage under 128MB for typical animation loads
- Animation state transition latency under 16ms
- Cross-platform animation data compatibility

## Architecture/Design

### ECS Architecture Design

#### Entity Management

- **Entity Creation/Destruction**: Manages animation components for entities requiring animated behavior
- **Entity Queries**: Queries entities with AnimationController, SpriteAnimator, or SkeletalAnimator components
- **Component Dependencies**: Requires Transform for positioning; optional Renderer for visual output

#### Component Design

```cpp
// Animation-related components for ECS integration
struct AnimationController {
    AssetId animation_state_machine{0};
    std::string current_state{"idle"};
    std::unordered_map<std::string, AnimationParameter> parameters;
    float state_time{0.0f};
    bool is_playing{true};
    float playback_speed{1.0f};
};

struct SpriteAnimator {
    AssetId sprite_sheet{0};
    std::string current_animation{"default"};
    std::uint32_t current_frame{0};
    float frame_time{0.0f};
    float frames_per_second{12.0f};
    bool is_looping{true};
    bool is_playing{true};
};

struct SkeletalAnimator {
    AssetId skeleton{0};
    std::vector<BoneTransform> bone_transforms;
    std::vector<AnimationLayer> animation_layers;
    Mat4 root_transform{1.0f};
    bool use_root_motion{false};
};

struct AnimationLayer {
    AssetId animation_clip{0};
    float weight{1.0f};
    float time{0.0f};
    AnimationWrapMode wrap_mode{AnimationWrapMode::Loop};
    bool is_additive{false};
};

// Component traits for animation
template<>
struct ComponentTraits<AnimationController> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
};

template<>
struct ComponentTraits<SpriteAnimator> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 20000;
};

template<>
struct ComponentTraits<SkeletalAnimator> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;
};
```

#### System Integration

- **System Dependencies**: Runs after input/logic updates, before rendering
- **Component Access Patterns**: Read-write access to animation components; read-only to Transform
- **Inter-System Communication**: Provides animation events for gameplay and audio systems

### Multi-Threading Design

#### Threading Model

- **Execution Phase**: Animation batch - executes in parallel with other update systems
- **Thread Safety**: Animation updates are parallelizable; bone calculations use thread-local storage
- **Data Dependencies**: Reads animation assets; writes to animation component state

#### Parallel Execution Strategy

```cpp
// Multi-threaded animation processing with efficient bone calculations
class AnimationSystem : public ISystem {
public:
    // Parallel animation updates
    void UpdateAnimations(const ComponentManager& components,
                         std::span<EntityId> entities,
                         const ThreadContext& context) override;
    
    // Parallel bone hierarchy updates
    void UpdateSkeletalAnimations(std::span<EntityId> skeletal_entities,
                                 const ThreadContext& context);
    
    // Parallel sprite frame updates
    void UpdateSpriteAnimations(std::span<EntityId> sprite_entities,
                               const ThreadContext& context);

private:
    struct AnimationJob {
        Entity entity;
        AnimationType type;
        float delta_time;
        std::uint32_t thread_id;
    };
    
    // Per-thread bone calculation workspace
    thread_local std::vector<BoneTransform> bone_workspace_;
    std::atomic<std::uint32_t> jobs_completed_{0};
};
```

#### Memory Access Patterns

- **Cache Efficiency**: Animation data stored contiguously for optimal bone hierarchy traversal
- **Memory Ordering**: Animation state updates use relaxed ordering for performance
- **Lock-Free Sections**: Animation playback and frame updates are lock-free

### Public APIs

#### Primary Interface: `AnimationSystemInterface`

```cpp
#pragma once

#include <optional>
#include <span>
#include <concepts>
#include <functional>

namespace engine::animation {

template<typename T>
concept AnimationData = requires(T t) {
    { t.GetDuration() } -> std::convertible_to<float>;
    { t.IsLooping() } -> std::convertible_to<bool>;
    { t.GetKeyframeCount() } -> std::convertible_to<std::size_t>;
};

class AnimationSystemInterface {
public:
    [[nodiscard]] auto Initialize(const AnimationConfig& config) -> std::optional<std::string>;
    void Shutdown() noexcept;
    
    // Animation playback control
    void PlayAnimation(Entity entity, std::string_view animation_name, float fade_time = 0.0f);
    void StopAnimation(Entity entity, float fade_time = 0.0f);
    void PauseAnimation(Entity entity);
    void ResumeAnimation(Entity entity);
    
    // Animation state management
    void SetAnimationState(Entity entity, std::string_view state_name);
    [[nodiscard]] auto GetAnimationState(Entity entity) const -> std::optional<std::string>;
    void SetAnimationParameter(Entity entity, std::string_view param_name, float value);
    void SetAnimationParameter(Entity entity, std::string_view param_name, bool value);
    
    // Animation blending and layering
    void AddAnimationLayer(Entity entity, const AnimationLayer& layer);
    void RemoveAnimationLayer(Entity entity, std::uint32_t layer_index);
    void SetLayerWeight(Entity entity, std::uint32_t layer_index, float weight);
    
    // Sprite animation control
    void SetSpriteAnimation(Entity entity, std::string_view animation_name);
    void SetSpriteFrame(Entity entity, std::uint32_t frame_index);
    [[nodiscard]] auto GetSpriteFrame(Entity entity) const -> std::optional<std::uint32_t>;
    
    // Timeline and curve evaluation
    template<AnimationData T>
    [[nodiscard]] auto EvaluateAnimation(const T& animation, float time) -> typename T::ValueType;
    
    [[nodiscard]] auto EvaluateCurve(const AnimationCurve& curve, float time) -> float;
    
    // Animation events
    [[nodiscard]] auto RegisterAnimationEvent(Entity entity, std::string_view event_name,
                                             std::function<void()> callback) -> CallbackId;
    void UnregisterAnimationEvent(CallbackId id);
    
    // Performance monitoring
    [[nodiscard]] auto GetAnimationStats() const -> AnimationStatistics;
    
    // Scripting interface exposure
    [[nodiscard]] auto GetScriptInterface() const -> const ScriptInterface&;

private:
    std::unique_ptr<AnimationSystemImpl> impl_;
    bool is_initialized_ = false;
};

} // namespace engine::animation
```

#### Scripting Interface Requirements

```cpp
// Animation scripting interface for dynamic animation control
class AnimationScriptInterface {
public:
    // Type-safe animation control from scripts
    void PlayAnimation(std::uint32_t entity_id, std::string_view animation_name, float fade_time = 0.0f);
    void StopAnimation(std::uint32_t entity_id, float fade_time = 0.0f);
    void PauseAnimation(std::uint32_t entity_id);
    void ResumeAnimation(std::uint32_t entity_id);
    
    // Animation state control
    void SetAnimationState(std::uint32_t entity_id, std::string_view state_name);
    [[nodiscard]] auto GetAnimationState(std::uint32_t entity_id) const -> std::string;
    
    // Animation parameters
    void SetFloatParameter(std::uint32_t entity_id, std::string_view param_name, float value);
    void SetBoolParameter(std::uint32_t entity_id, std::string_view param_name, bool value);
    [[nodiscard]] auto GetFloatParameter(std::uint32_t entity_id, std::string_view param_name) const -> float;
    
    // Sprite animation
    void SetSpriteAnimation(std::uint32_t entity_id, std::string_view animation_name);
    void SetSpriteFrame(std::uint32_t entity_id, std::uint32_t frame);
    [[nodiscard]] auto GetSpriteFrame(std::uint32_t entity_id) const -> std::uint32_t;
    
    // Animation events
    [[nodiscard]] auto OnAnimationEvent(std::uint32_t entity_id, std::string_view event_name,
                                       std::string_view callback_function) -> bool;
    
    // Configuration-driven behavior
    void LoadConfiguration(const ConfigData& data);
    [[nodiscard]] auto GetConfiguration() const -> const ConfigData&;

private:
    ConfigData current_config_;
    std::weak_ptr<AnimationSystemInterface> animation_system_;
};
```

## Success Criteria

### Functional Requirements

- [ ] **2D Animation**: Smooth sprite animation playback with frame-perfect timing
- [ ] **3D Animation**: Accurate skeletal animation with proper bone hierarchy evaluation
- [ ] **State Machines**: Reliable animation state transitions with condition evaluation

### Performance Requirements

- [ ] **Animation Scale**: Animate 1,000+ sprites and 100+ skeletal meshes at 60 FPS
- [ ] **Memory Usage**: Animation system memory under 128MB for typical game content
- [ ] **Transition Speed**: Animation state transitions under 16ms for responsive gameplay
- [ ] **Update Time**: Animation processing under 4ms per frame (25% of 16ms budget)

### Quality Requirements

- [ ] **Smoothness**: Sub-frame interpolation accuracy with no visible stuttering
- [ ] **Maintainability**: All animation code covered by automated tests
- [ ] **Testability**: Deterministic animation playback for automated testing
- [ ] **Documentation**: Complete animation system guide with state machine examples

### Acceptance Tests

```cpp
// Performance requirement validation
TEST(AnimationTest, AnimationScale) {
    auto animation_system = engine::animation::AnimationSystem{};
    animation_system.Initialize(AnimationConfig{});
    
    // Create many animated entities
    std::vector<Entity> sprite_entities;
    std::vector<Entity> skeletal_entities;
    
    // Create 1000 sprite animations
    for (int i = 0; i < 1000; ++i) {
        auto entity = CreateTestEntity();
        AddSpriteAnimator(entity, "test_sprite_anim");
        sprite_entities.push_back(entity);
    }
    
    // Create 100 skeletal animations
    for (int i = 0; i < 100; ++i) {
        auto entity = CreateTestEntity();
        AddSkeletalAnimator(entity, "test_skeletal_anim");
        skeletal_entities.push_back(entity);
    }
    
    // Measure animation performance
    auto start = std::chrono::high_resolution_clock::now();
    for (int frame = 0; frame < 60; ++frame) {
        animation_system.Update(1.0f / 60.0f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    auto avg_frame_time = total_time / 60.0f;
    
    EXPECT_LT(avg_frame_time, 4.0f); // Under 4ms per frame
}

TEST(AnimationTest, StateTransitionSpeed) {
    auto animation_system = engine::animation::AnimationSystem{};
    animation_system.Initialize(AnimationConfig{});
    
    auto entity = CreateTestEntity();
    AddAnimationController(entity, "test_state_machine");
    
    // Start in idle state
    animation_system.SetAnimationState(entity, "idle");
    
    // Measure state transition time
    auto start = std::chrono::high_resolution_clock::now();
    animation_system.SetAnimationState(entity, "running");
    
    // Wait for transition to complete
    while (animation_system.GetAnimationState(entity) != "running") {
        animation_system.Update(1.0f / 60.0f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto transition_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    EXPECT_LT(transition_time, 16); // Under 16ms
}

TEST(AnimationTest, FrameAccuracy) {
    auto animation_system = engine::animation::AnimationSystem{};
    animation_system.Initialize(AnimationConfig{});
    
    auto entity = CreateTestEntity();
    AddSpriteAnimator(entity, "precise_anim");
    
    // Set animation with known frame count
    animation_system.SetSpriteAnimation(entity, "test_anim"); // 10 frames, 10 FPS
    
    // Advance by exact frame duration
    for (int frame = 0; frame < 10; ++frame) {
        animation_system.Update(0.1f); // 0.1s = 1 frame at 10 FPS
        auto current_frame = animation_system.GetSpriteFrame(entity);
        ASSERT_TRUE(current_frame.has_value());
        EXPECT_EQ(*current_frame, static_cast<std::uint32_t>(frame + 1) % 10);
    }
}
```

## Implementation Strategy

### Development Phases

#### Phase 1: Core Animation Framework (Estimated: 6 days)

- [ ] **Task 1.1**: Implement animation timeline and keyframe interpolation
- [ ] **Task 1.2**: Add 2D sprite animation with texture atlas support
- [ ] **Task 1.3**: Create animation asset loading and management
- [ ] **Deliverable**: Basic 2D sprite animation working

#### Phase 2: Skeletal Animation (Estimated: 5 days)

- [ ] **Task 2.1**: Implement bone hierarchy and skeletal structure
- [ ] **Task 2.2**: Add skeletal animation playback and bone transforms
- [ ] **Task 2.3**: Create skinning and mesh deformation
- [ ] **Deliverable**: Full 3D skeletal animation system

#### Phase 3: State Machines (Estimated: 4 days)

- [ ] **Task 3.1**: Implement animation state machine framework
- [ ] **Task 3.2**: Add state transition conditions and parameters
- [ ] **Task 3.3**: Create animation blending and layering
- [ ] **Deliverable**: Complete state machine-driven animation

#### Phase 4: Advanced Features (Estimated: 3 days)

- [ ] **Task 4.1**: Add animation events and timeline markers
- [ ] **Task 4.2**: Implement procedural animation and curves
- [ ] **Task 4.3**: Add performance optimization and debugging tools
- [ ] **Deliverable**: Production-ready animation system

### File Structure

```
src/engine/animation/
├── animation.cppm              // Primary module interface
├── animation_system.cpp        // Core animation management
├── animation_timeline.cpp      // Keyframe and curve evaluation
├── sprite_animator.cpp         // 2D sprite animation
├── skeletal_animator.cpp       // 3D skeletal animation
├── animation_state_machine.cpp // State transition logic
├── animation_blending.cpp      // Animation layer blending
├── bone_hierarchy.cpp          // Skeletal structure management
├── animation_events.cpp        // Timeline event system
└── tests/
    ├── animation_system_tests.cpp
    ├── sprite_animation_tests.cpp
    └── animation_benchmarks.cpp
```

### Code Organization Patterns

- **Namespace**: `engine::animation`
- **Header Guards**: Use `#pragma once`
- **Module Structure**: Single primary module with specialized animation algorithms
- **Build Integration**: Links with engine.ecs, engine.assets, engine.scene

### Testing Strategy

- **Unit Tests**: Isolated testing of animation algorithms with known expected results
- **Integration Tests**: Full animation testing with ECS components and asset loading
- **Performance Tests**: Animation scale testing and timing benchmarks
- **Visual Tests**: Animation playback validation with reference frames

## Risk Assessment

### Technical Risks

| Risk                           | Probability | Impact | Mitigation                                           |
|--------------------------------|-------------|--------|------------------------------------------------------|
| **Performance Degradation**    | Medium      | High   | Efficient bone calculations, animation LOD system    |
| **Memory Usage**               | Medium      | Medium | Animation compression, streaming, asset optimization |
| **Synchronization Issues**     | Low         | Medium | Deterministic animation timing, thread-safe updates  |
| **Cross-Platform Differences** | Low         | Low    | Consistent animation data formats, platform testing  |

### Integration Risks

- **ECS Performance**: Risk that animation component updates impact frame rate
    - *Mitigation*: Efficient component layout, parallel animation processing
- **Asset Loading**: Risk of large animation assets blocking gameplay
    - *Mitigation*: Streaming animation data, progressive loading

## Dependencies

### Internal Dependencies

- **Required Systems**:
    - engine.platform (timing, memory management)
    - engine.ecs (component access, entity management)
    - engine.assets (animation asset loading and management)
    - engine.scene (transform synchronization)

- **Optional Systems**:
    - engine.profiling (animation performance monitoring)

### External Dependencies

- **Standard Library Features**: C++20 ranges, mathematical functions, containers
- **Math Libraries**: GLM or similar for matrix and quaternion operations

### Build System Dependencies

- **CMake Targets**: Links with engine.platform, engine.ecs, engine.assets, engine.scene
- **vcpkg Packages**: glm (for mathematical operations)
- **Platform-Specific**: Platform-optimized math libraries where available
- **Module Dependencies**: Imports engine.platform, engine.ecs, engine.assets, engine.scene

### Asset Pipeline Dependencies

- **Animation Assets**: Sprite sheets, skeletal animation data, state machine definitions
- **Configuration Files**: Animation settings and state machine configurations in JSON
- **Resource Loading**: Animation assets loaded through engine.assets with hot-reload
