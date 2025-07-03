# SYS_ANIMATION_v1

> **System-Level Design Document for Cross-Platform Animation System**

## Executive Summary

This document defines a comprehensive animation system for the modern C++20 game engine, designed to provide
high-performance 2D sprite animation and 3D skeletal animation capabilities across Windows, Linux, and WebAssembly
platforms. The system emphasizes state machine-driven animation control, seamless ECS integration, and script-driven
animation logic while maintaining 60+ FPS performance with thousands of animated entities. The design supports both
data-driven animation workflows and runtime animation composition, with comprehensive hot-reload capabilities for rapid
iteration during development.

## Scope and Objectives

### In Scope

- [ ] 2D sprite animation with atlas support and frame-based sequencing
- [ ] 3D skeletal animation with bone hierarchies, skinning, and blend trees
- [ ] Animation state machines with transition logic and condition evaluation
- [ ] Procedural animation through tweening, easing, and property interpolation
- [ ] ECS integration with animation components and system-driven processing
- [ ] Scripting interface enabling runtime animation control and custom animation logic
- [ ] Asset pipeline integration for animation data loading and hot-reload support
- [ ] Cross-platform optimization with WebAssembly-specific performance considerations
- [ ] Animation compression and memory optimization for large-scale entity animation
- [ ] Development tools including animation debugging and performance profiling

### Out of Scope

- [ ] Advanced inverse kinematics (IK) solving beyond basic two-bone chains
- [ ] Physics-based animation or ragdoll simulation integration
- [ ] Real-time motion capture integration or retargeting
- [ ] Complex facial animation or blend shape systems
- [ ] Animation authoring tools or in-engine animation editors
- [ ] Video playback or cinematic cutscene systems

### Primary Objectives

1. **Performance**: Animate 1000+ entities simultaneously while maintaining 60+ FPS
2. **Flexibility**: Support both 2D and 3D animation workflows through unified interface
3. **ECS Integration**: Seamless component-based animation with minimal overhead
4. **Scripting Support**: Full animation control through Lua, Python, and AngelScript interfaces
5. **Cross-Platform**: Identical animation behavior and performance across all target platforms

### Secondary Objectives

- Memory-efficient animation data storage suitable for WebAssembly heap constraints
- Hot-reload animation assets for rapid development iteration workflows
- Comprehensive debugging tools for animation state visualization and performance analysis
- Future extensibility for advanced animation features and rendering integration
- Integration with physics system for animation-driven collision shape updates

## Architecture/Design

### High-Level Overview

```
Animation System Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                 Animation Control Layer                         │
├─────────────────────────────────────────────────────────────────┤
│  Script API    │ State Machine │ Animation     │ Debug         │
│                │ Controller    │ Blending      │ Interface     │
│                │               │               │               │
│  Lua/Python/   │ State Trans-  │ Blend Trees   │ Visualization │
│  AngelScript   │ itions &      │ & Weight      │ & Profiling   │
│  Integration   │ Conditions    │ Management    │ Tools         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                 Animation Processing Core                       │
├─────────────────────────────────────────────────────────────────┤
│  Sprite         │ Skeletal      │ Procedural    │ Timeline      │
│  Animation      │ Animation     │ Animation     │ System        │
│                 │               │               │               │
│  Frame-Based    │ Bone          │ Tweening &    │ Keyframe      │
│  Sequencing     │ Hierarchies   │ Easing        │ Interpolation │
│  Atlas Support  │ Skinning      │ Property      │ Curve         │
│                 │               │ Animation     │ Evaluation    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ECS Integration                              │
├─────────────────────────────────────────────────────────────────┤
│  Animation      │ Sprite        │ Skeleton      │ Transform     │
│  Component      │ Component     │ Component     │ Integration   │
│                 │               │               │               │
│  State Machine  │ Frame Data    │ Bone Data     │ World Matrix  │
│  Current State  │ Atlas Info    │ Joint Trans-  │ Hierarchical  │
│  Blend Weights  │ UV Coords     │ forms         │ Updates       │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                 Foundation Integration                          │
├─────────────────────────────────────────────────────────────────┤
│  Assets         │ Rendering     │ Threading     │ Scene         │
│                 │               │               │               │
│  Animation      │ Mesh &        │ Parallel      │ Transform     │
│  Asset Loading  │ Texture       │ Animation     │ Hierarchy     │
│  Hot-Reload     │ Updates       │ Processing    │ Management    │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Animation State Machine Controller

- **Purpose**: High-level animation state management with transition logic and condition evaluation
- **Responsibilities**: State transitions, condition evaluation, animation scheduling, blend weight management
- **Key Classes/Interfaces**: `AnimationStateMachine`, `AnimationState`, `StateTransition`, `AnimationController`
- **Data Flow**: Input conditions → State evaluation → Transition triggers → Animation updates → Output transforms

#### Component 2: Animation Processing Engine

- **Purpose**: Core animation calculation and interpolation for both 2D and 3D animation types
- **Responsibilities**: Keyframe interpolation, bone transformation, sprite frame sequencing, blend tree evaluation
- **Key Classes/Interfaces**: `AnimationProcessor`, `SkeletalAnimator`, `SpriteAnimator`, `BlendTree`
- **Data Flow**: Animation assets → Time sampling → Interpolation → Transform generation → Component updates

#### Component 3: ECS Animation Integration

- **Purpose**: Component-based animation system with efficient batch processing and transform propagation
- **Responsibilities**: Animation component management, batch processing, transform hierarchy updates
- **Key Classes/Interfaces**: `AnimationSystem`, `AnimationComponent`, `SkeletonComponent`, `SpriteAnimationComponent`
- **Data Flow**: ECS queries → Component processing → Animation calculations → Transform updates → Rendering integration

#### Component 4: Asset Pipeline Integration

- **Purpose**: Animation asset loading, compression, and hot-reload capabilities for development workflows
- **Responsibilities**: Asset loading, format conversion, compression, hot-reload monitoring, dependency management
- **Key Classes/Interfaces**: `AnimationAssetLoader`, `AnimationAsset`, `AnimationCompressor`, `AnimationHotReloader`
- **Data Flow**: Asset files → Loading/parsing → Compression → Memory storage → Hot-reload monitoring

### ECS Animation Components

#### Core Animation Components

```cpp
namespace engine::animation {

// Primary animation component for entities with animation capabilities
struct AnimationComponent {
    std::string current_state = "idle";
    std::string target_state = "idle";
    float transition_time = 0.0f;
    float transition_duration = 0.3f;
    bool is_transitioning = false;
    
    // Animation timing
    float current_time = 0.0f;
    float playback_speed = 1.0f;
    bool is_playing = true;
    bool is_looping = true;
    
    // State machine reference
    AssetId state_machine_asset = AssetId::Invalid;
    
    // Blend weights for multi-animation blending
    std::unordered_map<std::string, float> blend_weights;
    
    // Script callbacks for animation events
    std::unordered_map<std::string, std::function<void()>> event_callbacks;
};

// Component traits for ECS integration
template<>
struct ComponentTraits<AnimationComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 10000;
    static constexpr size_t alignment = alignof(std::string);
};

// Sprite animation component for 2D frame-based animation
struct SpriteAnimationComponent {
    AssetId sprite_atlas_asset = AssetId::Invalid;
    std::string current_animation = "default";
    
    // Frame data
    uint32_t current_frame = 0;
    uint32_t frame_count = 1;
    float frame_duration = 0.1f;
    float frame_timer = 0.0f;
    
    // Atlas information
    glm::vec2 frame_size{32.0f, 32.0f};
    glm::vec2 atlas_size{256.0f, 256.0f};
    uint32_t frames_per_row = 8;
    
    // Animation state
    bool is_playing = true;
    bool is_looping = true;
    float playback_speed = 1.0f;
};

template<>
struct ComponentTraits<SpriteAnimationComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 5000;
    static constexpr size_t alignment = alignof(AssetId);
};

// Skeletal animation component for 3D bone-based animation
struct SkeletonComponent {
    AssetId skeleton_asset = AssetId::Invalid;
    std::string current_animation = "bind_pose";
    
    // Bone data
    std::vector<glm::mat4> bone_transforms;      // Local bone transforms
    std::vector<glm::mat4> world_bone_transforms; // World space transforms
    std::vector<glm::mat4> inverse_bind_poses;   // Bind pose inverse matrices
    
    // Bone hierarchy
    std::vector<int32_t> bone_parents;           // Parent bone indices (-1 for root)
    std::vector<std::string> bone_names;         // Bone names for identification
    
    // Animation state
    bool requires_update = true;
    bool is_playing = true;
    float playback_speed = 1.0f;
    
    // LOD support
    uint32_t lod_level = 0;                      // 0 = full quality, higher = reduced quality
    float lod_distance = 0.0f;                   // Distance from camera for LOD calculation
};

template<>
struct ComponentTraits<SkeletonComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 1000;
    static constexpr size_t alignment = alignof(std::vector<glm::mat4>);
};

// Procedural animation component for tween-based property animation
struct TweenComponent {
    enum class PropertyType { Position, Rotation, Scale, Color, Custom };
    enum class EasingType { Linear, EaseIn, EaseOut, EaseInOut, Bounce, Elastic };
    
    struct TweenData {
        PropertyType property;
        EasingType easing;
        glm::vec4 start_value;
        glm::vec4 target_value;
        float duration;
        float current_time;
        bool is_active;
        bool is_looping;
        std::function<void(const glm::vec4&)> setter;
    };
    
    std::vector<TweenData> active_tweens;
    bool auto_remove_completed = true;
};

template<>
struct ComponentTraits<TweenComponent> {
    static constexpr bool is_trivially_copyable = false;
    static constexpr size_t max_instances = 2000;
    static constexpr size_t alignment = alignof(std::vector<TweenComponent::TweenData>);
};

} // namespace engine::animation
```

### Animation State Machine System

#### State Machine Architecture

```cpp
namespace engine::animation {

// Animation state definition with entry/exit actions and transitions
class AnimationState {
public:
    explicit AnimationState(std::string name) : name_(std::move(name)) {}
    
    // State configuration
    void SetAnimation(const std::string& animation_name, bool looping = true);
    void SetPlaybackSpeed(float speed);
    void AddEntryAction(std::function<void()> action);
    void AddExitAction(std::function<void()> action);
    void AddUpdateAction(std::function<void(float)> action);
    
    // Transition management
    void AddTransition(std::string target_state, std::function<bool()> condition, float transition_time = 0.3f);
    void AddTransitionOnEvent(std::string target_state, std::string event_name, float transition_time = 0.3f);
    void AddTransitionOnAnimationEnd(std::string target_state, float transition_time = 0.3f);
    
    // State execution
    void OnEnter();
    void OnExit();
    void Update(float delta_time);
    [[nodiscard]] auto CheckTransitions() -> std::optional<StateTransition>;
    
    // Accessors
    [[nodiscard]] auto GetName() const -> const std::string& { return name_; }
    [[nodiscard]] auto GetAnimationName() const -> const std::string& { return animation_name_; }
    [[nodiscard]] auto IsLooping() const -> bool { return is_looping_; }
    [[nodiscard]] auto GetPlaybackSpeed() const -> float { return playback_speed_; }

private:
    std::string name_;
    std::string animation_name_;
    bool is_looping_ = true;
    float playback_speed_ = 1.0f;
    
    // Actions
    std::vector<std::function<void()>> entry_actions_;
    std::vector<std::function<void()>> exit_actions_;
    std::vector<std::function<void(float)>> update_actions_;
    
    // Transitions
    struct TransitionInfo {
        std::string target_state;
        std::function<bool()> condition;
        float transition_time;
        bool is_event_based;
        std::string event_name;
    };
    std::vector<TransitionInfo> transitions_;
};

// State transition data
struct StateTransition {
    std::string from_state;
    std::string to_state;
    float transition_time;
    float blend_curve_type; // For future blend curve support
};

// Complete animation state machine
class AnimationStateMachine {
public:
    // State management
    void AddState(std::unique_ptr<AnimationState> state);
    void RemoveState(const std::string& state_name);
    [[nodiscard]] auto GetState(const std::string& state_name) -> AnimationState*;
    [[nodiscard]] auto GetCurrentState() -> AnimationState* { return current_state_; }
    
    // State machine execution
    void SetInitialState(const std::string& state_name);
    void Update(float delta_time);
    void TriggerEvent(const std::string& event_name);
    void ForceTransition(const std::string& target_state, float transition_time = 0.3f);
    
    // Transition management
    [[nodiscard]] auto IsTransitioning() const -> bool { return is_transitioning_; }
    [[nodiscard]] auto GetTransitionProgress() const -> float;
    
    // Scripting integration
    void RegisterLuaFunctions(lua_State* L);
    void RegisterPythonFunctions(pybind11::module& m);
    void RegisterAngelScriptFunctions(asIScriptEngine* engine);

private:
    std::unordered_map<std::string, std::unique_ptr<AnimationState>> states_;
    AnimationState* current_state_ = nullptr;
    AnimationState* target_state_ = nullptr;
    
    // Transition state
    bool is_transitioning_ = false;
    float transition_timer_ = 0.0f;
    float transition_duration_ = 0.0f;
    StateTransition current_transition_;
    
    // Event system
    std::unordered_set<std::string> triggered_events_;
    
    void ProcessStateTransition(const StateTransition& transition);
    void UpdateTransition(float delta_time);
};

// Asset representation for state machines
struct AnimationStateMachineAsset {
    std::string name;
    std::string initial_state;
    
    struct StateData {
        std::string animation_name;
        bool is_looping = true;
        float playback_speed = 1.0f;
        std::vector<std::string> entry_scripts;
        std::vector<std::string> exit_scripts;
        std::vector<std::string> update_scripts;
    };
    
    struct TransitionData {
        std::string from_state;
        std::string to_state;
        std::string condition_script;
        std::string event_name;
        float transition_time = 0.3f;
        bool is_event_based = false;
        bool triggers_on_animation_end = false;
    };
    
    std::unordered_map<std::string, StateData> states;
    std::vector<TransitionData> transitions;
};

} // namespace engine::animation
```

### Animation Processing Engine

#### Core Animation Systems

```cpp
namespace engine::animation {

// Main animation system for ECS integration
class AnimationSystem : public ISystem {
public:
    [[nodiscard]] auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                ComponentTypeId::GetId<TransformComponent>(),
                ComponentTypeId::GetId<AnimationComponent>(),
                ComponentTypeId::GetId<SpriteAnimationComponent>(),
                ComponentTypeId::GetId<SkeletonComponent>()
            },
            .write_components = {
                ComponentTypeId::GetId<AnimationComponent>(),
                ComponentTypeId::GetId<SpriteAnimationComponent>(),
                ComponentTypeId::GetId<SkeletonComponent>(),
                ComponentTypeId::GetId<TransformComponent>()
            },
            .execution_phase = ExecutionPhase::Update,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Process animation state machines
        ProcessAnimationComponents(world, delta_time);
        
        // Process sprite animations
        ProcessSpriteAnimations(world, delta_time);
        
        // Process skeletal animations
        ProcessSkeletalAnimations(world, delta_time);
        
        // Process procedural tweens
        ProcessTweenComponents(world, delta_time);
        
        // Update render components with animation data
        UpdateRenderComponents(world);
    }

private:
    void ProcessAnimationComponents(World& world, float delta_time) {
        world.ForEachComponent<AnimationComponent>([this, delta_time](EntityId entity, AnimationComponent& anim) {
            if (!anim.is_playing) return;
            
            // Load state machine if needed
            auto* state_machine = GetOrLoadStateMachine(anim.state_machine_asset);
            if (!state_machine) return;
            
            // Update state machine
            state_machine->Update(delta_time * anim.playback_speed);
            
            // Update animation timing
            anim.current_time += delta_time * anim.playback_speed;
            
            // Handle state transitions
            if (anim.target_state != anim.current_state) {
                if (!anim.is_transitioning) {
                    StartStateTransition(entity, anim);
                } else {
                    UpdateStateTransition(entity, anim, delta_time);
                }
            }
        });
    }
    
    void ProcessSpriteAnimations(World& world, float delta_time) {
        world.ForEachComponent<SpriteAnimationComponent>([this, delta_time](EntityId entity, SpriteAnimationComponent& sprite) {
            if (!sprite.is_playing) return;
            
            // Update frame timer
            sprite.frame_timer += delta_time * sprite.playback_speed;
            
            // Check for frame advancement
            if (sprite.frame_timer >= sprite.frame_duration) {
                sprite.frame_timer -= sprite.frame_duration;
                sprite.current_frame++;
                
                // Handle looping
                if (sprite.current_frame >= sprite.frame_count) {
                    if (sprite.is_looping) {
                        sprite.current_frame = 0;
                    } else {
                        sprite.current_frame = sprite.frame_count - 1;
                        sprite.is_playing = false;
                    }
                }
                
                // Update UV coordinates for rendering
                UpdateSpriteUVCoordinates(entity, sprite);
            }
        });
    }
    
    void ProcessSkeletalAnimations(World& world, float delta_time) {
        world.ForEachComponent<SkeletonComponent>([this, delta_time](EntityId entity, SkeletonComponent& skeleton) {
            if (!skeleton.is_playing || !skeleton.requires_update) return;
            
            // Get animation asset
            auto* animation_asset = GetAnimationAsset(skeleton.current_animation);
            if (!animation_asset) return;
            
            // Calculate animation time
            float animation_time = CalculateAnimationTime(animation_asset, delta_time * skeleton.playback_speed);
            
            // Sample bone transforms from animation
            SampleBoneTransforms(skeleton, *animation_asset, animation_time);
            
            // Calculate world transforms from local transforms
            CalculateWorldBoneTransforms(skeleton);
            
            // Apply LOD if necessary
            ApplySkeletalLOD(skeleton);
            
            skeleton.requires_update = false;
        });
    }
    
    void ProcessTweenComponents(World& world, float delta_time) {
        world.ForEachComponent<TweenComponent>([this, delta_time](EntityId entity, TweenComponent& tween) {
            auto it = tween.active_tweens.begin();
            while (it != tween.active_tweens.end()) {
                auto& tween_data = *it;
                
                if (!tween_data.is_active) {
                    ++it;
                    continue;
                }
                
                // Update tween time
                tween_data.current_time += delta_time;
                
                // Calculate progress (0.0 to 1.0)
                float progress = std::clamp(tween_data.current_time / tween_data.duration, 0.0f, 1.0f);
                
                // Apply easing function
                float eased_progress = ApplyEasing(progress, tween_data.easing);
                
                // Interpolate value
                glm::vec4 current_value = glm::mix(tween_data.start_value, tween_data.target_value, eased_progress);
                
                // Apply value through setter
                if (tween_data.setter) {
                    tween_data.setter(current_value);
                }
                
                // Check for completion
                if (progress >= 1.0f) {
                    if (tween_data.is_looping) {
                        tween_data.current_time = 0.0f;
                    } else {
                        tween_data.is_active = false;
                        if (tween.auto_remove_completed) {
                            it = tween.active_tweens.erase(it);
                            continue;
                        }
                    }
                }
                
                ++it;
            }
        });
    }
    
    void UpdateRenderComponents(World& world) {
        // Update sprite render components with UV coordinates
        world.ForEachComponent<SpriteAnimationComponent, RenderComponent>(
            [](EntityId entity, const SpriteAnimationComponent& sprite, RenderComponent& render) {
                // Update texture coordinates based on current frame
                UpdateSpriteRenderData(render, sprite);
            });
        
        // Update skeletal mesh render components with bone matrices
        world.ForEachComponent<SkeletonComponent, RenderComponent>(
            [](EntityId entity, const SkeletonComponent& skeleton, RenderComponent& render) {
                // Update bone matrix uniform data for GPU skinning
                UpdateSkeletalRenderData(render, skeleton);
            });
    }
    
    // Helper methods
    auto GetOrLoadStateMachine(AssetId asset_id) -> AnimationStateMachine*;
    auto GetAnimationAsset(const std::string& animation_name) -> AnimationAsset*;
    void StartStateTransition(EntityId entity, AnimationComponent& anim);
    void UpdateStateTransition(EntityId entity, AnimationComponent& anim, float delta_time);
    void UpdateSpriteUVCoordinates(EntityId entity, const SpriteAnimationComponent& sprite);
    float CalculateAnimationTime(const AnimationAsset* asset, float delta_time);
    void SampleBoneTransforms(SkeletonComponent& skeleton, const AnimationAsset& asset, float time);
    void CalculateWorldBoneTransforms(SkeletonComponent& skeleton);
    void ApplySkeletalLOD(SkeletonComponent& skeleton);
    float ApplyEasing(float progress, TweenComponent::EasingType easing);
    void UpdateSpriteRenderData(RenderComponent& render, const SpriteAnimationComponent& sprite);
    void UpdateSkeletalRenderData(RenderComponent& render, const SkeletonComponent& skeleton);
};

// Specialized systems for performance-critical animation processing
class SkeletalAnimationProcessor {
public:
    // SIMD-optimized bone transformation calculations
    void ProcessBoneHierarchy(std::span<glm::mat4> local_transforms,
                             std::span<glm::mat4> world_transforms,
                             std::span<const int32_t> parent_indices);
    
    // Parallel bone calculation using job system
    void ProcessBonesParallel(SkeletonComponent& skeleton, 
                             const AnimationAsset& asset, 
                             float animation_time);
    
    // LOD system for skeletal animation
    void CalculateLOD(SkeletonComponent& skeleton, float distance_to_camera);
    void ApplyLOD(SkeletonComponent& skeleton, uint32_t lod_level);

private:
    // Memory pools for temporary bone calculations
    std::vector<glm::mat4> temp_transforms_;
    std::vector<glm::quat> temp_rotations_;
    std::vector<glm::vec3> temp_positions_;
    std::vector<glm::vec3> temp_scales_;
};

class SpriteAnimationProcessor {
public:
    // Batch processing for sprite animations
    void ProcessSpriteBatch(std::span<SpriteAnimationComponent> sprites, float delta_time);
    
    // Atlas management and UV calculation
    void CalculateUVCoordinates(const SpriteAnimationComponent& sprite, glm::vec4& uv_coords);
    void UpdateTextureAtlas(EntityId entity, const SpriteAnimationComponent& sprite);
    
    // Animation event handling
    void ProcessAnimationEvents(EntityId entity, const SpriteAnimationComponent& sprite);

private:
    // Cached atlas data for performance
    std::unordered_map<AssetId, SpriteAtlasData> cached_atlases_;
};

} // namespace engine::animation
```

### Scripting Integration

#### Animation Script Interface

```cpp
namespace engine::scripting {

// Animation API for scripting languages
class ScriptAnimationAPI {
public:
    explicit ScriptAnimationAPI(AnimationSystem& animation_system) 
        : animation_system_(animation_system) {}
    
    // Animation control
    void PlayAnimation(EntityId entity, const std::string& animation_name);
    void StopAnimation(EntityId entity);
    void PauseAnimation(EntityId entity);
    void ResumeAnimation(EntityId entity);
    void SetAnimationSpeed(EntityId entity, float speed);
    [[nodiscard]] auto IsAnimationPlaying(EntityId entity) const -> bool;
    [[nodiscard]] auto GetCurrentAnimation(EntityId entity) const -> std::string;
    
    // State machine control
    void SetAnimationState(EntityId entity, const std::string& state_name);
    void TriggerAnimationEvent(EntityId entity, const std::string& event_name);
    void ForceStateTransition(EntityId entity, const std::string& target_state, float transition_time = 0.3f);
    [[nodiscard]] auto GetCurrentState(EntityId entity) const -> std::string;
    [[nodiscard]] auto IsTransitioning(EntityId entity) const -> bool;
    
    // Sprite animation control
    void SetSpriteFrame(EntityId entity, uint32_t frame);
    [[nodiscard]] auto GetSpriteFrame(EntityId entity) const -> uint32_t;
    void SetSpriteAnimation(EntityId entity, const std::string& animation_name);
    
    // Skeletal animation control
    void SetBoneTransform(EntityId entity, const std::string& bone_name, const glm::mat4& transform);
    [[nodiscard]] auto GetBoneTransform(EntityId entity, const std::string& bone_name) const -> glm::mat4;
    void AttachObjectToBone(EntityId entity, const std::string& bone_name, EntityId attached_object);
    
    // Procedural animation (tweening)
    void TweenPosition(EntityId entity, const glm::vec3& target, float duration, const std::string& easing = "linear");
    void TweenRotation(EntityId entity, const glm::quat& target, float duration, const std::string& easing = "linear");
    void TweenScale(EntityId entity, const glm::vec3& target, float duration, const std::string& easing = "linear");
    void TweenProperty(EntityId entity, const std::string& property, const glm::vec4& target, float duration, const std::string& easing = "linear");
    void StopTween(EntityId entity, const std::string& property = "");
    
    // Animation events and callbacks
    void RegisterAnimationCallback(EntityId entity, const std::string& event_name, std::function<void()> callback);
    void UnregisterAnimationCallback(EntityId entity, const std::string& event_name);
    void RegisterAnimationEndCallback(EntityId entity, std::function<void()> callback);
    
    // Animation asset management
    void LoadAnimationAsset(const std::string& asset_path);
    void UnloadAnimationAsset(const std::string& asset_path);
    void ReloadAnimationAsset(const std::string& asset_path);

private:
    AnimationSystem& animation_system_;
    
    // Helper methods for script interface
    [[nodiscard]] auto GetAnimationComponent(EntityId entity) -> AnimationComponent*;
    [[nodiscard]] auto GetSpriteAnimationComponent(EntityId entity) -> SpriteAnimationComponent*;
    [[nodiscard]] auto GetSkeletonComponent(EntityId entity) -> SkeletonComponent*;
    [[nodiscard]] auto GetTweenComponent(EntityId entity) -> TweenComponent*;
    
    TweenComponent::EasingType ParseEasingType(const std::string& easing_name);
};

// Integration with unified scripting interface
void RegisterAnimationAPI(ScriptInterface& script_interface, AnimationSystem& animation_system) {
    auto animation_api = std::make_unique<ScriptAnimationAPI>(animation_system);
    script_interface.RegisterAPI("animation", std::move(animation_api));
}

} // namespace engine::scripting

// Example Lua integration
/*
-- Lua script example for animation control
function on_entity_created(entity_id)
    -- Play idle animation
    animation.play_animation(entity_id, "idle")
    
    -- Set up animation event callbacks
    animation.register_animation_callback(entity_id, "footstep", function()
        audio.play_sound("footstep.wav")
    end)
    
    -- Set up state machine
    animation.set_animation_state(entity_id, "idle")
end

function on_player_input(entity_id, input_action)
    if input_action == "move_forward" then
        animation.set_animation_state(entity_id, "walking")
    elseif input_action == "jump" then
        animation.set_animation_state(entity_id, "jumping")
    elseif input_action == "attack" then
        animation.set_animation_state(entity_id, "attacking")
    else
        animation.set_animation_state(entity_id, "idle")
    end
end

function tween_to_position(entity_id, target_pos)
    animation.tween_position(entity_id, target_pos, 1.0, "ease_out")
end
*/
```

### Asset Pipeline Integration

#### Animation Asset System

```cpp
namespace engine::animation {

// Animation asset types
struct KeyFrame {
    float time;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct AnimationTrack {
    std::string bone_name;
    std::vector<KeyFrame> keyframes;
};

struct AnimationAsset {
    std::string name;
    float duration;
    float frame_rate;
    bool is_looping;
    std::vector<AnimationTrack> tracks;
    
    // Compressed animation data for memory efficiency
    std::vector<uint8_t> compressed_data;
    bool is_compressed = false;
    
    // Animation events (for sound effects, particle effects, etc.)
    struct AnimationEvent {
        float time;
        std::string event_name;
        std::unordered_map<std::string, std::string> parameters;
    };
    std::vector<AnimationEvent> events;
};

struct SpriteAnimationAsset {
    std::string name;
    AssetId atlas_asset;
    
    struct FrameData {
        uint32_t frame_index;
        float duration;
        glm::vec4 uv_coordinates; // x, y, width, height in normalized coordinates
    };
    std::vector<FrameData> frames;
    
    bool is_looping = true;
    float total_duration = 0.0f;
};

struct SkeletonAsset {
    std::string name;
    
    struct BoneData {
        std::string name;
        int32_t parent_index;
        glm::mat4 bind_pose;
        glm::mat4 inverse_bind_pose;
    };
    std::vector<BoneData> bones;
    
    // Default animations for this skeleton
    std::vector<std::string> default_animations;
};

// Animation asset loader
class AnimationAssetLoader : public IAssetLoader {
public:
    [[nodiscard]] auto CanLoad(const std::string& file_extension) const -> bool override {
        return file_extension == ".anim" || 
               file_extension == ".skelanim" || 
               file_extension == ".spriteanim" ||
               file_extension == ".skeleton";
    }
    
    [[nodiscard]] auto LoadAsset(const std::string& file_path) -> std::unique_ptr<IAsset> override {
        auto extension = std::filesystem::path(file_path).extension().string();
        
        if (extension == ".anim" || extension == ".skelanim") {
            return LoadSkeletalAnimation(file_path);
        } else if (extension == ".spriteanim") {
            return LoadSpriteAnimation(file_path);
        } else if (extension == ".skeleton") {
            return LoadSkeleton(file_path);
        }
        
        return nullptr;
    }
    
    void UnloadAsset(IAsset* asset) override {
        // Animation assets are managed by the animation system
        delete asset;
    }

private:
    [[nodiscard]] auto LoadSkeletalAnimation(const std::string& file_path) -> std::unique_ptr<AnimationAsset> {
        auto file_data = platform::FileSystem::ReadFile(file_path);
        if (!file_data) {
            return nullptr;
        }
        
        // Parse animation data from JSON or binary format
        try {
            std::string content(file_data->begin(), file_data->end());
            auto json_data = nlohmann::json::parse(content);
            
            auto animation = std::make_unique<AnimationAsset>();
            animation->name = json_data["name"];
            animation->duration = json_data["duration"];
            animation->frame_rate = json_data.value("frame_rate", 30.0f);
            animation->is_looping = json_data.value("looping", true);
            
            // Parse animation tracks
            for (const auto& track_data : json_data["tracks"]) {
                AnimationTrack track;
                track.bone_name = track_data["bone"];
                
                for (const auto& keyframe_data : track_data["keyframes"]) {
                    KeyFrame keyframe;
                    keyframe.time = keyframe_data["time"];
                    
                    if (keyframe_data.contains("position")) {
                        auto pos = keyframe_data["position"];
                        keyframe.position = glm::vec3(pos[0], pos[1], pos[2]);
                    }
                    
                    if (keyframe_data.contains("rotation")) {
                        auto rot = keyframe_data["rotation"];
                        keyframe.rotation = glm::quat(rot[3], rot[0], rot[1], rot[2]); // w, x, y, z
                    }
                    
                    if (keyframe_data.contains("scale")) {
                        auto scale = keyframe_data["scale"];
                        keyframe.scale = glm::vec3(scale[0], scale[1], scale[2]);
                    }
                    
                    track.keyframes.push_back(keyframe);
                }
                
                animation->tracks.push_back(std::move(track));
            }
            
            // Parse animation events
            if (json_data.contains("events")) {
                for (const auto& event_data : json_data["events"]) {
                    AnimationAsset::AnimationEvent event;
                    event.time = event_data["time"];
                    event.event_name = event_data["name"];
                    
                    if (event_data.contains("parameters")) {
                        for (const auto& [key, value] : event_data["parameters"].items()) {
                            event.parameters[key] = value.dump();
                        }
                    }
                    
                    animation->events.push_back(event);
                }
            }
            
            return animation;
        } catch (const std::exception& e) {
            LogMessage(LogLevel::Error, std::format("Failed to load animation {}: {}", file_path, e.what()));
            return nullptr;
        }
    }
    
    [[nodiscard]] auto LoadSpriteAnimation(const std::string& file_path) -> std::unique_ptr<SpriteAnimationAsset> {
        auto file_data = platform::FileSystem::ReadFile(file_path);
        if (!file_data) {
            return nullptr;
        }
        
        try {
            std::string content(file_data->begin(), file_data->end());
            auto json_data = nlohmann::json::parse(content);
            
            auto sprite_animation = std::make_unique<SpriteAnimationAsset>();
            sprite_animation->name = json_data["name"];
            sprite_animation->atlas_asset = AssetId{json_data.value("atlas_id", 0)};
            sprite_animation->is_looping = json_data.value("looping", true);
            
            for (const auto& frame_data : json_data["frames"]) {
                SpriteAnimationAsset::FrameData frame;
                frame.frame_index = frame_data["index"];
                frame.duration = frame_data.value("duration", 0.1f);
                
                if (frame_data.contains("uv")) {
                    auto uv = frame_data["uv"];
                    frame.uv_coordinates = glm::vec4(uv[0], uv[1], uv[2], uv[3]);
                }
                
                sprite_animation->frames.push_back(frame);
                sprite_animation->total_duration += frame.duration;
            }
            
            return sprite_animation;
        } catch (const std::exception& e) {
            LogMessage(LogLevel::Error, std::format("Failed to load sprite animation {}: {}", file_path, e.what()));
            return nullptr;
        }
    }
    
    [[nodiscard]] auto LoadSkeleton(const std::string& file_path) -> std::unique_ptr<SkeletonAsset> {
        auto file_data = platform::FileSystem::ReadFile(file_path);
        if (!file_data) {
            return nullptr;
        }
        
        try {
            std::string content(file_data->begin(), file_data->end());
            auto json_data = nlohmann::json::parse(content);
            
            auto skeleton = std::make_unique<SkeletonAsset>();
            skeleton->name = json_data["name"];
            
            for (const auto& bone_data : json_data["bones"]) {
                SkeletonAsset::BoneData bone;
                bone.name = bone_data["name"];
                bone.parent_index = bone_data.value("parent", -1);
                
                // Parse bind pose matrix
                if (bone_data.contains("bind_pose")) {
                    auto matrix_data = bone_data["bind_pose"];
                    bone.bind_pose = ParseMatrix4(matrix_data);
                }
                
                // Calculate inverse bind pose
                bone.inverse_bind_pose = glm::inverse(bone.bind_pose);
                
                skeleton->bones.push_back(bone);
            }
            
            if (json_data.contains("default_animations")) {
                for (const auto& anim_name : json_data["default_animations"]) {
                    skeleton->default_animations.push_back(anim_name);
                }
            }
            
            return skeleton;
        } catch (const std::exception& e) {
            LogMessage(LogLevel::Error, std::format("Failed to load skeleton {}: {}", file_path, e.what()));
            return nullptr;
        }
    }
    
    [[nodiscard]] auto ParseMatrix4(const nlohmann::json& matrix_data) -> glm::mat4 {
        glm::mat4 matrix(1.0f);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                matrix[i][j] = matrix_data[i * 4 + j];
            }
        }
        return matrix;
    }
};

// Animation hot-reload system
class AnimationHotReloader {
public:
    explicit AnimationHotReloader(AnimationSystem& animation_system)
        : animation_system_(animation_system) {}
    
    void Initialize(const std::filesystem::path& animation_directory) {
        animation_directory_ = animation_directory;
        
        file_watcher_ = platform::FileSystem::CreateWatcher(
            animation_directory.string(),
            [this](const std::string& changed_file) {
                OnAnimationFileChanged(changed_file);
            }
        );
    }
    
    void OnAnimationFileChanged(const std::string& changed_file) {
        auto file_path = animation_directory_ / changed_file;
        auto extension = file_path.extension().string();
        
        if (extension == ".anim" || extension == ".skelanim" || 
            extension == ".spriteanim" || extension == ".skeleton") {
            
            LogMessage(LogLevel::Info, std::format("Reloading animation asset: {}", changed_file));
            
            // Reload the animation asset
            animation_system_.ReloadAnimationAsset(file_path.string());
            
            // Notify any registered callbacks
            auto it = reload_callbacks_.find(changed_file);
            if (it != reload_callbacks_.end()) {
                for (const auto& callback : it->second) {
                    callback();
                }
            }
        }
    }
    
    void RegisterReloadCallback(const std::string& file_name, std::function<void()> callback) {
        reload_callbacks_[file_name].push_back(callback);
    }

private:
    AnimationSystem& animation_system_;
    std::filesystem::path animation_directory_;
    std::unique_ptr<platform::FileWatcher> file_watcher_;
    std::unordered_map<std::string, std::vector<std::function<void()>>> reload_callbacks_;
};

} // namespace engine::animation
```

## Performance Requirements

### Target Specifications

- **Animated Entities**: Support 1000+ entities with active animations at 60+ FPS
- **Skeletal Animation**: Process 100+ skeletal characters with 50+ bones each without frame drops
- **Sprite Animation**: Handle 5000+ sprite animations with minimal CPU overhead
- **State Transitions**: Animation state changes complete within 16ms for responsive feel
- **Memory Usage**: Animation system peak memory usage under 100MB for typical game scenarios
- **LOD Performance**: Automatic LOD scaling maintains performance as animated entity count increases

### Cross-Platform Optimization

- **WebAssembly**: SIMD optimization for bone calculations where available, efficient memory layout
- **Native Platforms**: Full SIMD utilization for matrix operations and bone transformations
- **Threading**: Parallel bone processing across multiple CPU cores for complex skeletal animations
- **Memory Layout**: Cache-friendly component organization for optimal memory access patterns

## Integration Points

### ECS Core Integration

- Component-based animation architecture with efficient batch processing
- Animation components designed for cache-friendly memory layout
- System dependencies properly declared for threading model integration

### Rendering System Integration

- Direct integration with GPU skinning for skeletal animation
- Automatic sprite UV coordinate updates for texture atlas rendering
- Transform hierarchy updates propagated to rendering components

### Scripting System Integration

- Complete animation API exposed to Lua, Python, and AngelScript
- State machine logic scriptable for complex animation behaviors
- Animation events triggerable from scripts for gameplay integration

### Asset System Integration

- Animation assets loaded through unified asset pipeline
- Hot-reload capabilities for rapid development iteration
- Asset dependency tracking for animation state machines and references

### Scene Management Integration

- Transform hierarchy updates respect scene management parent-child relationships
- Spatial queries available for distance-based animation LOD calculations
- Bone attachment system integrates with scene transform hierarchies

This animation system provides comprehensive 2D and 3D animation capabilities while maintaining the engine's performance
targets and cross-platform compatibility. The scripting integration ensures that animation logic can be rapidly
prototyped and iterated, while the ECS architecture provides the performance foundation needed for large-scale entity
animation.
