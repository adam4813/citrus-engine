# SYS_SCENE_v1

> **System-Level Design Document for Scene Management and Spatial Organization**

## Executive Summary

This document defines a comprehensive scene management system for the modern C++20 game engine, designed to provide
efficient spatial organization, transform hierarchies, and culling optimization for 10,000+ entities at 60+ FPS. The
system integrates seamlessly with the established ECS architecture, threading model, and rendering pipeline while
supporting both 2D and 3D spatial queries. The design emphasizes cache-friendly data structures, lock-free spatial
partitioning, and declarative scene configuration to support the engine's performance targets and cross-platform
requirements.

## Scope and Objectives

### In Scope

- [ ] Hierarchical transform system with parent-child relationships and world matrix caching
- [ ] Spatial partitioning using adaptive grid and octree structures for efficient queries
- [ ] Frustum culling integration with the rendering system for optimal draw call reduction
- [ ] ECS integration through spatial components and specialized spatial query systems
- [ ] Multi-threaded spatial updates using the established threading model and job system
- [ ] Scene graph serialization and deserialization for asset pipeline integration
- [ ] Dynamic scene loading and unloading for large world streaming support
- [ ] Spatial query API for proximity-based gameplay systems (AI, physics, audio)
- [ ] Level-of-detail (LOD) management based on distance and screen space projection
- [ ] Debug visualization tools for spatial partitioning and transform hierarchies

### Out of Scope

- [ ] Advanced spatial acceleration structures (BVH, R-trees) - Phase 2 consideration
- [ ] Real-time global illumination or lighting probe systems
- [ ] Procedural scene generation (handled by separate generation systems)
- [ ] Scene editing tools (external editor integration approach)
- [ ] Network-synchronized scene state (future networking extension)
- [ ] Complex animation blending or skeletal hierarchy systems

### Primary Objectives

1. **Performance**: Support efficient spatial queries for 10,000+ entities with sub-millisecond response times
2. **Scalability**: Maintain 60+ FPS performance as scene complexity grows through intelligent culling
3. **ECS Integration**: Seamless integration with component queries and system dependencies
4. **Threading Safety**: Lock-free spatial operations compatible with parallel ECS system execution
5. **Memory Efficiency**: Cache-friendly data layouts optimized for frequent spatial traversals

### Secondary Objectives

- Flexible spatial partitioning that adapts to different world sizes and entity distributions
- Comprehensive debug visualization for development workflow optimization
- Integration with asset streaming for large open-world scenarios
- Future extensibility for advanced rendering techniques (instancing, GPU culling)
- Hot-reload support for scene configuration changes during development

## Architecture/Design

### High-Level Overview

```
Scene Management Architecture:

┌─────────────────────────────────────────────────────────────────┐
│                    Scene Manager                                │
├─────────────────────────────────────────────────────────────────┤
│  Scene Loading  │ Transform     │ Spatial        │ LOD          │
│  & Streaming    │ Hierarchies   │ Partitioning   │ Management   │
│                 │               │                │              │
│  Asset          │ Parent-Child  │ Adaptive Grid  │ Distance     │
│  Integration    │ Relationships │ Octree Hybrid  │ Based LOD    │
└─────────────────┴───────────────┴────────────────┴──────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Spatial Query Engine                          │
├─────────────────────────────────────────────────────────────────┤
│  Frustum        │ Proximity     │ Raycast        │ Overlap      │
│  Culling        │ Queries       │ Operations     │ Detection    │
│                 │               │                │              │
│  View-Dependent │ Radius-Based  │ Line/Sphere    │ AABB/Sphere  │
│  Visibility     │ Neighborhood  │ Intersection   │ Intersection │
└─────────────────┴───────────────┴────────────────┴──────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                 ECS Spatial Components                          │
├─────────────────────────────────────────────────────────────────┤
│  TransformComponent │ SpatialComponent │ LODComponent │ Bounds  │
│                     │                  │              │         │
│  World Matrices     │ Spatial Handles  │ LOD Levels   │ AABB    │
│  Local Matrices     │ Query Caching    │ Transitions  │ Spheres │
│  Dirty Flags        │ Partition Info   │ Thresholds   │ OBB     │
└─────────────────────┴──────────────────┴──────────────┴─────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Integration with Engine Systems                    │
│                                                                 │
│  Rendering System   │   Physics System   │   Audio System      │
│  (Culling & LOD)    │   (Broad Phase)    │   (3D Positioning)  │
│                     │                    │                     │
│  ┌─────────────┐    │  ┌─────────────┐   │  ┌─────────────┐     │
│  │ Visible     │    │  │ Collision   │   │  │ Audio       │     │
│  │ Entity      │    │  │ Candidate   │   │  │ Source      │     │
│  │ Collection  │    │  │ Pairs       │   │  │ Culling     │     │
│  └─────────────┘    │  └─────────────┘   │  └─────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

### Core Components

#### Component 1: Transform Hierarchy System

- **Purpose**: Efficient parent-child transform relationships with world matrix computation
- **Responsibilities**: Transform propagation, hierarchy management, matrix caching, dirty propagation
- **Key Classes/Interfaces**: `TransformHierarchy`, `TransformComponent`, `HierarchyNode`, `MatrixCache`
- **Data Flow**: Local transforms → Hierarchy traversal → World matrix computation → Component updates

#### Component 2: Spatial Partitioning Engine

- **Purpose**: Dynamic spatial organization for efficient proximity and visibility queries
- **Responsibilities**: Partition management, entity insertion/removal, query optimization, load balancing
- **Key Classes/Interfaces**: `SpatialPartition`, `AdaptiveGrid`, `Octree`, `SpatialHandle`
- **Data Flow**: Entity positions → Partition assignment → Query processing → Result collection

#### Component 3: Culling and Visibility System

- **Purpose**: View-dependent entity filtering for rendering and simulation optimization
- **Responsibilities**: Frustum culling, LOD selection, occlusion queries, visibility caching
- **Key Classes/Interfaces**: `CullingSystem`, `Frustum`, `LODManager`, `VisibilityCache`
- **Data Flow**: Camera data → Frustum extraction → Spatial queries → Visibility determination

#### Component 4: Scene Graph Management

- **Purpose**: High-level scene organization, loading, and asset integration
- **Responsibilities**: Scene loading/unloading, asset coordination, configuration management
- **Key Classes/Interfaces**: `SceneManager`, `Scene`, `SceneNode`, `SceneAsset`
- **Data Flow**: Scene assets → Parsing → Entity creation → Spatial registration

### Transform Hierarchy Implementation

#### Hierarchical Transform Components

```cpp
// Enhanced transform component with hierarchy support
struct TransformComponent {
    // Local transform data
    glm::vec3 local_position{0.0f};
    glm::quat local_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 local_scale{1.0f};
    
    // Cached world transform
    glm::mat4 world_matrix{1.0f};
    glm::vec3 world_position{0.0f};
    glm::quat world_rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 world_scale{1.0f};
    
    // Hierarchy information
    EntityId parent{INVALID_ENTITY};
    std::vector<EntityId> children;
    
    // Update tracking
    bool local_dirty = true;
    bool world_dirty = true;
    uint32_t hierarchy_version = 0;
};

// Component traits for ECS integration
template<>
struct ComponentTraits<TransformComponent> {
    static constexpr bool is_trivially_copyable = false; // Contains std::vector
    static constexpr size_t max_instances = 50000;
    static constexpr size_t alignment = alignof(glm::mat4);
};

// Spatial component for spatial partitioning integration
struct SpatialComponent {
    // Bounding volume information
    glm::vec3 aabb_min{-1.0f};
    glm::vec3 aabb_max{1.0f};
    float bounding_radius = 1.0f;
    
    // Spatial partitioning data
    SpatialHandle spatial_handle{SpatialHandle::Invalid};
    uint32_t partition_version = 0;
    
    // Query optimization
    bool static_object = false;
    bool culling_enabled = true;
    uint32_t query_mask = 0xFFFFFFFF;
};

// LOD component for level-of-detail management
struct LODComponent {
    struct LODLevel {
        float distance_threshold;
        float screen_size_threshold;
        AssetHandle<MeshAsset> mesh;
        AssetHandle<MaterialAsset> material;
        bool cast_shadows = true;
        bool visible = true;
    };
    
    std::vector<LODLevel> lod_levels;
    uint32_t current_lod = 0;
    float bias = 0.0f;
    bool force_lod = false;
    uint32_t forced_lod_level = 0;
};
```

#### Transform Hierarchy System

```cpp
// Efficient transform hierarchy management
class TransformHierarchy {
public:
    // Parent-child relationship management
    void SetParent(EntityId child, EntityId parent) {
        std::unique_lock lock(hierarchy_mutex_);
        
        // Remove from previous parent
        if (auto it = child_to_parent_.find(child); it != child_to_parent_.end()) {
            RemoveChildInternal(it->second, child);
        }
        
        // Add to new parent
        if (parent != INVALID_ENTITY) {
            child_to_parent_[child] = parent;
            parent_to_children_[parent].push_back(child);
            
            // Mark hierarchy dirty for update propagation
            MarkHierarchyDirty(child);
        } else {
            child_to_parent_.erase(child);
        }
    }
    
    void RemoveEntity(EntityId entity) {
        std::unique_lock lock(hierarchy_mutex_);
        
        // Remove as child
        if (auto it = child_to_parent_.find(entity); it != child_to_parent_.end()) {
            RemoveChildInternal(it->second, entity);
            child_to_parent_.erase(it);
        }
        
        // Reparent children to this entity's parent
        if (auto it = parent_to_children_.find(entity); it != parent_to_children_.end()) {
            EntityId grandparent = GetParent(entity);
            for (EntityId child : it->second) {
                SetParent(child, grandparent);
            }
            parent_to_children_.erase(it);
        }
    }
    
    [[nodiscard]] auto GetParent(EntityId entity) const -> EntityId {
        std::shared_lock lock(hierarchy_mutex_);
        
        auto it = child_to_parent_.find(entity);
        return it != child_to_parent_.end() ? it->second : INVALID_ENTITY;
    }
    
    [[nodiscard]] auto GetChildren(EntityId entity) const -> std::vector<EntityId> {
        std::shared_lock lock(hierarchy_mutex_);
        
        auto it = parent_to_children_.find(entity);
        return it != parent_to_children_.end() ? it->second : std::vector<EntityId>{};
    }
    
    // Transform propagation with dependency tracking
    void UpdateTransforms(World& world) {
        // Collect all entities that need transform updates
        std::vector<EntityId> entities_to_update;
        CollectDirtyTransforms(world, entities_to_update);
        
        // Sort by hierarchy depth for dependency-order processing
        SortByHierarchyDepth(entities_to_update);
        
        // Update transforms in batches where possible
        UpdateTransformBatches(world, entities_to_update);
    }
    
private:
    mutable std::shared_mutex hierarchy_mutex_;
    std::unordered_map<EntityId, EntityId> child_to_parent_;
    std::unordered_map<EntityId, std::vector<EntityId>> parent_to_children_;
    std::unordered_set<EntityId> dirty_hierarchy_entities_;
    
    void RemoveChildInternal(EntityId parent, EntityId child) {
        if (auto it = parent_to_children_.find(parent); it != parent_to_children_.end()) {
            auto& children = it->second;
            children.erase(std::remove(children.begin(), children.end(), child), children.end());
        }
    }
    
    void MarkHierarchyDirty(EntityId entity) {
        dirty_hierarchy_entities_.insert(entity);
        
        // Propagate dirty flag to all children
        if (auto it = parent_to_children_.find(entity); it != parent_to_children_.end()) {
            for (EntityId child : it->second) {
                MarkHierarchyDirty(child);
            }
        }
    }
    
    void CollectDirtyTransforms(World& world, std::vector<EntityId>& entities_to_update) {
        world.ForEachComponent<TransformComponent>([&](EntityId entity, const TransformComponent& transform) {
            if (transform.local_dirty || transform.world_dirty || 
                dirty_hierarchy_entities_.contains(entity)) {
                entities_to_update.push_back(entity);
            }
        });
    }
    
    void SortByHierarchyDepth(std::vector<EntityId>& entities) {
        // Sort entities by hierarchy depth to ensure parents are processed before children
        std::ranges::sort(entities, [this](EntityId a, EntityId b) {
            return GetHierarchyDepth(a) < GetHierarchyDepth(b);
        });
    }
    
    [[nodiscard]] auto GetHierarchyDepth(EntityId entity) const -> uint32_t {
        uint32_t depth = 0;
        EntityId current = entity;
        
        while (auto parent = GetParent(current); parent != INVALID_ENTITY) {
            current = parent;
            ++depth;
            
            // Prevent infinite loops in case of circular references
            if (depth > 1000) {
                break;
            }
        }
        
        return depth;
    }
    
    void UpdateTransformBatches(World& world, const std::vector<EntityId>& entities) {
        // Process entities in dependency order
        for (EntityId entity : entities) {
            UpdateEntityTransform(world, entity);
        }
        
        // Clear dirty flags after successful update
        dirty_hierarchy_entities_.clear();
    }
    
    void UpdateEntityTransform(World& world, EntityId entity) {
        auto* transform = world.GetComponent<TransformComponent>(entity);
        if (!transform) {
            return;
        }
        
        // Compute local matrix if needed
        if (transform->local_dirty) {
            glm::mat4 local_matrix = glm::translate(glm::mat4(1.0f), transform->local_position);
            local_matrix *= glm::mat4_cast(transform->local_rotation);
            local_matrix = glm::scale(local_matrix, transform->local_scale);
            
            transform->local_dirty = false;
            transform->world_dirty = true;
        }
        
        // Compute world matrix if needed
        if (transform->world_dirty) {
            EntityId parent = GetParent(entity);
            if (parent != INVALID_ENTITY) {
                auto* parent_transform = world.GetComponent<TransformComponent>(parent);
                if (parent_transform) {
                    // Ensure parent is up to date
                    if (parent_transform->world_dirty) {
                        UpdateEntityTransform(world, parent);
                    }
                    
                    // Combine with parent world matrix
                    transform->world_matrix = parent_transform->world_matrix * 
                                            ComputeLocalMatrix(*transform);
                } else {
                    transform->world_matrix = ComputeLocalMatrix(*transform);
                }
            } else {
                transform->world_matrix = ComputeLocalMatrix(*transform);
            }
            
            // Extract world position, rotation, scale from matrix
            ExtractTransformFromMatrix(transform->world_matrix, 
                                     transform->world_position,
                                     transform->world_rotation,
                                     transform->world_scale);
            
            transform->world_dirty = false;
            transform->hierarchy_version++;
        }
    }
    
    [[nodiscard]] auto ComputeLocalMatrix(const TransformComponent& transform) const -> glm::mat4 {
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), transform.local_position);
        matrix *= glm::mat4_cast(transform.local_rotation);
        return glm::scale(matrix, transform.local_scale);
    }
    
    void ExtractTransformFromMatrix(const glm::mat4& matrix,
                                  glm::vec3& position,
                                  glm::quat& rotation,
                                  glm::vec3& scale) {
        // Extract position (translation)
        position = glm::vec3(matrix[3]);
        
        // Extract scale
        scale.x = glm::length(glm::vec3(matrix[0]));
        scale.y = glm::length(glm::vec3(matrix[1]));
        scale.z = glm::length(glm::vec3(matrix[2]));
        
        // Extract rotation (normalize to remove scale)
        glm::mat3 rotation_matrix;
        rotation_matrix[0] = glm::vec3(matrix[0]) / scale.x;
        rotation_matrix[1] = glm::vec3(matrix[1]) / scale.y;
        rotation_matrix[2] = glm::vec3(matrix[2]) / scale.z;
        
        rotation = glm::quat_cast(rotation_matrix);
    }
};
```

### Spatial Partitioning System

#### Adaptive Spatial Partitioning

```cpp
// Abstract interface for spatial partitioning strategies
class SpatialPartition {
public:
    virtual ~SpatialPartition() = default;
    
    // Entity management
    virtual auto InsertEntity(EntityId entity, const glm::vec3& position, 
                             const glm::vec3& size) -> SpatialHandle = 0;
    virtual void UpdateEntity(SpatialHandle handle, const glm::vec3& position, 
                             const glm::vec3& size) = 0;
    virtual void RemoveEntity(SpatialHandle handle) = 0;
    
    // Spatial queries
    virtual auto QuerySphere(const glm::vec3& center, float radius) 
        -> std::vector<EntityId> = 0;
    virtual auto QueryAABB(const glm::vec3& min, const glm::vec3& max) 
        -> std::vector<EntityId> = 0;
    virtual auto QueryFrustum(const Frustum& frustum) 
        -> std::vector<EntityId> = 0;
    virtual auto QueryRay(const glm::vec3& origin, const glm::vec3& direction, 
                         float max_distance) -> std::vector<EntityId> = 0;
    
    // Performance optimization
    virtual void Optimize() = 0;
    virtual auto GetStatistics() const -> SpatialStatistics = 0;
};

// Hybrid adaptive grid + octree implementation
class AdaptiveHybridPartition final : public SpatialPartition {
public:
    struct Configuration {
        float grid_cell_size = 50.0f;
        uint32_t max_entities_per_cell = 64;
        uint32_t max_octree_depth = 8;
        float octree_threshold_ratio = 0.75f;
        bool enable_dynamic_adaptation = true;
    };
    
    explicit AdaptiveHybridPartition(const Configuration& config = {}) 
        : config_(config) {
        InitializeGrid();
    }
    
    auto InsertEntity(EntityId entity, const glm::vec3& position, 
                     const glm::vec3& size) -> SpatialHandle override {
        std::unique_lock lock(partition_mutex_);
        
        // Create new spatial handle
        SpatialHandle handle = GenerateHandle();
        SpatialEntry entry{
            .entity = entity,
            .position = position,
            .aabb_min = position - size * 0.5f,
            .aabb_max = position + size * 0.5f,
            .handle = handle
        };
        
        // Determine which partitioning strategy to use
        if (ShouldUseOctree(entry)) {
            InsertIntoOctree(entry);
        } else {
            InsertIntoGrid(entry);
        }
        
        spatial_entries_[handle] = std::move(entry);
        entity_to_handle_[entity] = handle;
        
        return handle;
    }
    
    void UpdateEntity(SpatialHandle handle, const glm::vec3& position, 
                     const glm::vec3& size) override {
        std::unique_lock lock(partition_mutex_);
        
        auto it = spatial_entries_.find(handle);
        if (it == spatial_entries_.end()) {
            return;
        }
        
        auto& entry = it->second;
        
        // Remove from current partition
        if (entry.grid_cell != GridCell::Invalid) {
            RemoveFromGrid(entry);
        } else if (entry.octree_node != OctreeNode::Invalid) {
            RemoveFromOctree(entry);
        }
        
        // Update position and bounds
        entry.position = position;
        entry.aabb_min = position - size * 0.5f;
        entry.aabb_max = position + size * 0.5f;
        
        // Reinsert with new position
        if (ShouldUseOctree(entry)) {
            InsertIntoOctree(entry);
        } else {
            InsertIntoGrid(entry);
        }
    }
    
    void RemoveEntity(SpatialHandle handle) override {
        std::unique_lock lock(partition_mutex_);
        
        auto it = spatial_entries_.find(handle);
        if (it == spatial_entries_.end()) {
            return;
        }
        
        const auto& entry = it->second;
        
        // Remove from partition
        if (entry.grid_cell != GridCell::Invalid) {
            RemoveFromGrid(entry);
        } else if (entry.octree_node != OctreeNode::Invalid) {
            RemoveFromOctree(entry);
        }
        
        // Remove from maps
        entity_to_handle_.erase(entry.entity);
        spatial_entries_.erase(it);
        
        // Recycle handle
        RecycleHandle(handle);
    }
    
    auto QuerySphere(const glm::vec3& center, float radius) -> std::vector<EntityId> override {
        std::shared_lock lock(partition_mutex_);
        
        std::vector<EntityId> results;
        
        // Query grid cells
        QueryGridSphere(center, radius, results);
        
        // Query octree nodes
        QueryOctreeSphere(center, radius, results);
        
        // Remove duplicates and return
        std::ranges::sort(results);
        results.erase(std::unique(results.begin(), results.end()), results.end());
        
        return results;
    }
    
    auto QueryFrustum(const Frustum& frustum) -> std::vector<EntityId> override {
        std::shared_lock lock(partition_mutex_);
        
        std::vector<EntityId> results;
        
        // Query grid cells intersecting frustum
        QueryGridFrustum(frustum, results);
        
        // Query octree nodes intersecting frustum
        QueryOctreeFrustum(frustum, results);
        
        // Remove duplicates
        std::ranges::sort(results);
        results.erase(std::unique(results.begin(), results.end()), results.end());
        
        return results;
    }
    
    void Optimize() override {
        std::unique_lock lock(partition_mutex_);
        
        if (config_.enable_dynamic_adaptation) {
            AnalyzeAndAdapt();
        }
        
        CompactOctree();
        RebalanceGrid();
    }
    
    [[nodiscard]] auto GetStatistics() const -> SpatialStatistics override {
        std::shared_lock lock(partition_mutex_);
        
        return SpatialStatistics{
            .total_entities = spatial_entries_.size(),
            .grid_cells_used = CountUsedGridCells(),
            .octree_nodes_used = CountUsedOctreeNodes(),
            .average_entities_per_cell = CalculateAverageEntitiesPerCell(),
            .max_entities_per_cell = FindMaxEntitiesPerCell(),
            .memory_usage_bytes = CalculateMemoryUsage()
        };
    }
    
private:
    using GridCell = uint32_t;
    using OctreeNode = uint32_t;
    
    struct SpatialEntry {
        EntityId entity;
        glm::vec3 position;
        glm::vec3 aabb_min;
        glm::vec3 aabb_max;
        SpatialHandle handle;
        
        // Partition assignment
        GridCell grid_cell{GridCell::Invalid};
        OctreeNode octree_node{OctreeNode::Invalid};
        
        static constexpr GridCell Invalid = std::numeric_limits<GridCell>::max();
    };
    
    Configuration config_;
    mutable std::shared_mutex partition_mutex_;
    
    // Spatial data structures
    std::unordered_map<SpatialHandle, SpatialEntry> spatial_entries_;
    std::unordered_map<EntityId, SpatialHandle> entity_to_handle_;
    
    // Grid partition
    std::unordered_map<GridCell, std::vector<SpatialHandle>> grid_cells_;
    glm::vec3 grid_origin_{0.0f};
    
    // Octree partition
    struct OctreeNodeData {
        glm::vec3 center;
        float half_size;
        std::vector<SpatialHandle> entities;
        std::array<OctreeNode, 8> children{OctreeNode::Invalid};
        OctreeNode parent{OctreeNode::Invalid};
        uint8_t depth = 0;
    };
    std::unordered_map<OctreeNode, OctreeNodeData> octree_nodes_;
    OctreeNode octree_root_{OctreeNode::Invalid};
    
    // Handle management
    std::atomic<uint32_t> next_handle_{1};
    std::vector<SpatialHandle> recycled_handles_;
    
    void InitializeGrid() {
        // Initialize with reasonable grid bounds
        // This will be adapted based on actual entity distribution
    }
    
    [[nodiscard]] auto ShouldUseOctree(const SpatialEntry& entry) const -> bool {
        // Use octree for large objects or high-density areas
        glm::vec3 size = entry.aabb_max - entry.aabb_min;
        float max_dimension = std::max({size.x, size.y, size.z});
        
        return max_dimension > config_.grid_cell_size * config_.octree_threshold_ratio;
    }
    
    void InsertIntoGrid(SpatialEntry& entry) {
        GridCell cell = ComputeGridCell(entry.position);
        grid_cells_[cell].push_back(entry.handle);
        entry.grid_cell = cell;
    }
    
    void InsertIntoOctree(SpatialEntry& entry) {
        if (octree_root_ == OctreeNode::Invalid) {
            CreateOctreeRoot();
        }
        
        OctreeNode node = FindBestOctreeNode(entry);
        octree_nodes_[node].entities.push_back(entry.handle);
        entry.octree_node = node;
        
        // Split node if it becomes too dense
        if (octree_nodes_[node].entities.size() > config_.max_entities_per_cell &&
            octree_nodes_[node].depth < config_.max_octree_depth) {
            SplitOctreeNode(node);
        }
    }
    
    [[nodiscard]] auto ComputeGridCell(const glm::vec3& position) const -> GridCell {
        int32_t x = static_cast<int32_t>((position.x - grid_origin_.x) / config_.grid_cell_size);
        int32_t y = static_cast<int32_t>((position.y - grid_origin_.y) / config_.grid_cell_size);
        int32_t z = static_cast<int32_t>((position.z - grid_origin_.z) / config_.grid_cell_size);
        
        // Hash 3D coordinates to single cell ID
        return static_cast<GridCell>(
            std::hash<int32_t>{}(x) ^ 
            (std::hash<int32_t>{}(y) << 1) ^ 
            (std::hash<int32_t>{}(z) << 2)
        );
    }
    
    // Additional implementation methods...
    void RemoveFromGrid(const SpatialEntry& entry);
    void RemoveFromOctree(const SpatialEntry& entry);
    void QueryGridSphere(const glm::vec3& center, float radius, std::vector<EntityId>& results);
    void QueryOctreeSphere(const glm::vec3& center, float radius, std::vector<EntityId>& results);
    void QueryGridFrustum(const Frustum& frustum, std::vector<EntityId>& results);
    void QueryOctreeFrustum(const Frustum& frustum, std::vector<EntityId>& results);
    void AnalyzeAndAdapt();
    void CompactOctree();
    void RebalanceGrid();
    
    // Helper methods for statistics
    [[nodiscard]] auto CountUsedGridCells() const -> uint32_t;
    [[nodiscard]] auto CountUsedOctreeNodes() const -> uint32_t;
    [[nodiscard]] auto CalculateAverageEntitiesPerCell() const -> float;
    [[nodiscard]] auto FindMaxEntitiesPerCell() const -> uint32_t;
    [[nodiscard]] auto CalculateMemoryUsage() const -> size_t;
    
    // Handle management
    [[nodiscard]] auto GenerateHandle() -> SpatialHandle {
        if (!recycled_handles_.empty()) {
            SpatialHandle handle = recycled_handles_.back();
            recycled_handles_.pop_back();
            return handle;
        }
        return SpatialHandle{next_handle_.fetch_add(1, std::memory_order_relaxed)};
    }
    
    void RecycleHandle(SpatialHandle handle) {
        recycled_handles_.push_back(handle);
    }
};
```

### Culling and Visibility System

#### Frustum Culling Integration

```cpp
// Frustum representation for culling operations
class Frustum {
public:
    // Frustum planes in world space
    std::array<glm::vec4, 6> planes;
    
    // Construct frustum from view-projection matrix
    explicit Frustum(const glm::mat4& view_projection_matrix) {
        ExtractPlanesFromMatrix(view_projection_matrix);
    }
    
    // Test intersection with axis-aligned bounding box
    [[nodiscard]] auto IntersectsAABB(const glm::vec3& min, const glm::vec3& max) const -> bool {
        for (const auto& plane : planes) {
            // Find the positive vertex (farthest from plane)
            glm::vec3 positive_vertex;
            positive_vertex.x = plane.x >= 0.0f ? max.x : min.x;
            positive_vertex.y = plane.y >= 0.0f ? max.y : min.y;
            positive_vertex.z = plane.z >= 0.0f ? max.z : min.z;
            
            // If positive vertex is behind plane, AABB is outside frustum
            if (glm::dot(glm::vec3(plane), positive_vertex) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }
    
    // Test intersection with sphere
    [[nodiscard]] auto IntersectsSphere(const glm::vec3& center, float radius) const -> bool {
        for (const auto& plane : planes) {
            if (glm::dot(glm::vec3(plane), center) + plane.w < -radius) {
                return false;
            }
        }
        return true;
    }
    
private:
    void ExtractPlanesFromMatrix(const glm::mat4& matrix) {
        // Extract frustum planes from view-projection matrix
        // Left plane
        planes[0] = glm::vec4(
            matrix[0][3] + matrix[0][0],
            matrix[1][3] + matrix[1][0],
            matrix[2][3] + matrix[2][0],
            matrix[3][3] + matrix[3][0]
        );
        
        // Right plane
        planes[1] = glm::vec4(
            matrix[0][3] - matrix[0][0],
            matrix[1][3] - matrix[1][0],
            matrix[2][3] - matrix[2][0],
            matrix[3][3] - matrix[3][0]
        );
        
        // Bottom plane
        planes[2] = glm::vec4(
            matrix[0][3] + matrix[0][1],
            matrix[1][3] + matrix[1][1],
            matrix[2][3] + matrix[2][1],
            matrix[3][3] + matrix[3][1]
        );
        
        // Top plane
        planes[3] = glm::vec4(
            matrix[0][3] - matrix[0][1],
            matrix[1][3] - matrix[1][1],
            matrix[2][3] - matrix[2][1],
            matrix[3][3] - matrix[3][1]
        );
        
        // Near plane
        planes[4] = glm::vec4(
            matrix[0][3] + matrix[0][2],
            matrix[1][3] + matrix[1][2],
            matrix[2][3] + matrix[2][2],
            matrix[3][3] + matrix[3][2]
        );
        
        // Far plane
        planes[5] = glm::vec4(
            matrix[0][3] - matrix[0][2],
            matrix[1][3] - matrix[1][2],
            matrix[2][3] - matrix[2][2],
            matrix[3][3] - matrix[3][2]
        );
        
        // Normalize planes
        for (auto& plane : planes) {
            float length = glm::length(glm::vec3(plane));
            if (length > 0.0f) {
                plane /= length;
            }
        }
    }
};

// Culling system for rendering optimization
class CullingSystem final : public System {
public:
    explicit CullingSystem(SpatialPartition& spatial_partition) 
        : spatial_partition_(spatial_partition) {}
    
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                GetComponentTypeId<TransformComponent>(),
                GetComponentTypeId<SpatialComponent>(),
                GetComponentTypeId<CameraComponent>()
            },
            .write_components = {
                GetComponentTypeId<RenderableComponent>(),
                GetComponentTypeId<LODComponent>()
            },
            .execution_phase = ExecutionPhase::PreRender,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Find all active cameras
        std::vector<CameraData> active_cameras;
        CollectActiveCameras(world, active_cameras);
        
        // Perform culling for each camera
        for (const auto& camera_data : active_cameras) {
            PerformCulling(world, camera_data);
        }
        
        // Update LOD based on distance to primary camera
        if (!active_cameras.empty()) {
            UpdateLOD(world, active_cameras[0]);
        }
    }
    
private:
    struct CameraData {
        EntityId entity;
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;
        glm::vec3 position;
        Frustum frustum;
    };
    
    SpatialPartition& spatial_partition_;
    
    void CollectActiveCameras(World& world, std::vector<CameraData>& cameras) {
        world.ForEachComponent<CameraComponent, TransformComponent>(
            [&cameras](EntityId entity, const CameraComponent& camera, const TransformComponent& transform) {
                if (!camera.active) {
                    return;
                }
                
                CameraData camera_data;
                camera_data.entity = entity;
                camera_data.position = transform.world_position;
                camera_data.view_matrix = ComputeViewMatrix(transform);
                camera_data.projection_matrix = ComputeProjectionMatrix(camera);
                camera_data.frustum = Frustum(camera_data.projection_matrix * camera_data.view_matrix);
                
                cameras.push_back(camera_data);
            });
    }
    
    void PerformCulling(World& world, const CameraData& camera_data) {
        // Query spatial partition for potentially visible entities
        auto candidate_entities = spatial_partition_.QueryFrustum(camera_data.frustum);
        
        // Perform detailed culling on candidates
        for (EntityId entity : candidate_entities) {
            auto* renderable = world.GetComponent<RenderableComponent>(entity);
            auto* spatial = world.GetComponent<SpatialComponent>(entity);
            
            if (!renderable || !spatial || !spatial->culling_enabled) {
                continue;
            }
            
            // Test against frustum with bounding volume
            bool visible = camera_data.frustum.IntersectsAABB(spatial->aabb_min, spatial->aabb_max);
            
            // Apply additional culling tests (occlusion, etc.) here if needed
            
            renderable->visible = visible;
        }
    }
    
    void UpdateLOD(World& world, const CameraData& camera_data) {
        world.ForEachComponent<LODComponent, TransformComponent, RenderableComponent>(
            [&camera_data](EntityId entity, LODComponent& lod, 
                          const TransformComponent& transform, RenderableComponent& renderable) {
                
                if (lod.force_lod) {
                    lod.current_lod = lod.forced_lod_level;
                    return;
                }
                
                float distance = glm::distance(camera_data.position, transform.world_position);
                
                // Find appropriate LOD level based on distance
                uint32_t best_lod = 0;
                for (size_t i = 0; i < lod.lod_levels.size(); ++i) {
                    if (distance >= lod.lod_levels[i].distance_threshold) {
                        best_lod = static_cast<uint32_t>(i);
                    } else {
                        break;
                    }
                }
                
                // Apply LOD bias
                if (lod.bias > 0.0f) {
                    distance *= (1.0f + lod.bias);
                } else if (lod.bias < 0.0f) {
                    distance *= (1.0f + lod.bias);
                }
                
                lod.current_lod = std::min(best_lod, static_cast<uint32_t>(lod.lod_levels.size() - 1));
                
                // Update renderable with LOD-specific settings
                if (lod.current_lod < lod.lod_levels.size()) {
                    const auto& current_level = lod.lod_levels[lod.current_lod];
                    renderable.visible = current_level.visible && renderable.visible;
                    renderable.cast_shadows = current_level.cast_shadows;
                    // LOD-specific mesh and material would be handled by rendering system
                }
            });
    }
    
    [[nodiscard]] auto ComputeViewMatrix(const TransformComponent& transform) const -> glm::mat4 {
        // Compute view matrix from transform (inverse of world matrix)
        return glm::inverse(transform.world_matrix);
    }
    
    [[nodiscard]] auto ComputeProjectionMatrix(const CameraComponent& camera) const -> glm::mat4 {
        if (camera.is_orthographic) {
            return glm::ortho(camera.left, camera.right, camera.bottom, camera.top, 
                            camera.near_plane, camera.far_plane);
        } else {
            return glm::perspective(camera.field_of_view, camera.aspect_ratio, 
                                  camera.near_plane, camera.far_plane);
        }
    }
};
```

### ECS Integration Systems

#### Spatial Update System

```cpp
// System for maintaining spatial partitioning data
class SpatialUpdateSystem final : public System {
public:
    explicit SpatialUpdateSystem(SpatialPartition& spatial_partition) 
        : spatial_partition_(spatial_partition) {}
    
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {
                GetComponentTypeId<TransformComponent>()
            },
            .write_components = {
                GetComponentTypeId<SpatialComponent>()
            },
            .execution_phase = ExecutionPhase::Update,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // Update spatial partitioning for moved entities
        UpdateMovedEntities(world);
        
        // Register new entities with spatial components
        RegisterNewEntities(world);
        
        // Remove destroyed entities from spatial partition
        CleanupDestroyedEntities(world);
        
        // Periodically optimize spatial structures
        frame_count_++;
        if (frame_count_ % optimization_interval_ == 0) {
            spatial_partition_.Optimize();
        }
    }
    
private:
    SpatialPartition& spatial_partition_;
    uint64_t frame_count_ = 0;
    uint64_t optimization_interval_ = 300; // Optimize every 5 seconds at 60 FPS
    
    void UpdateMovedEntities(World& world) {
        world.ForEachComponent<TransformComponent, SpatialComponent>(
            [this](EntityId entity, const TransformComponent& transform, SpatialComponent& spatial) {
                
                // Check if entity has moved since last update
                if (transform.hierarchy_version <= spatial.partition_version) {
                    return; // No update needed
                }
                
                // Update spatial partition
                if (spatial.spatial_handle != SpatialHandle::Invalid) {
                    glm::vec3 size = spatial.aabb_max - spatial.aabb_min;
                    spatial_partition_.UpdateEntity(spatial.spatial_handle, 
                                                  transform.world_position, size);
                }
                
                // Update component version
                spatial.partition_version = transform.hierarchy_version;
            });
    }
    
    void RegisterNewEntities(World& world) {
        world.ForEachComponent<TransformComponent, SpatialComponent>(
            [this](EntityId entity, const TransformComponent& transform, SpatialComponent& spatial) {
                
                // Check if entity needs to be registered
                if (spatial.spatial_handle != SpatialHandle::Invalid) {
                    return; // Already registered
                }
                
                // Register with spatial partition
                glm::vec3 size = spatial.aabb_max - spatial.aabb_min;
                spatial.spatial_handle = spatial_partition_.InsertEntity(
                    entity, transform.world_position, size);
                
                spatial.partition_version = transform.hierarchy_version;
            });
    }
    
    void CleanupDestroyedEntities(World& world) {
        // This would need integration with entity destruction callbacks
        // For now, entities are removed when their components are destroyed
    }
};

// System for spatial queries used by gameplay systems
class SpatialQuerySystem final : public System {
public:
    explicit SpatialQuerySystem(SpatialPartition& spatial_partition) 
        : spatial_partition_(spatial_partition) {}
    
    // Proximity-based queries for AI and gameplay
    [[nodiscard]] auto FindEntitiesInRadius(const glm::vec3& center, float radius) 
        -> std::vector<EntityId> {
        return spatial_partition_.QuerySphere(center, radius);
    }
    
    [[nodiscard]] auto FindEntitiesInAABB(const glm::vec3& min, const glm::vec3& max) 
        -> std::vector<EntityId> {
        return spatial_partition_.QueryAABB(min, max);
    }
    
    [[nodiscard]] auto Raycast(const glm::vec3& origin, const glm::vec3& direction, 
                             float max_distance) -> std::vector<EntityId> {
        return spatial_partition_.QueryRay(origin, direction, max_distance);
    }
    
    // Utility functions for common spatial queries
    [[nodiscard]] auto FindNearestEntity(const glm::vec3& position, float max_radius = 1000.0f) 
        -> std::optional<EntityId> {
        auto candidates = spatial_partition_.QuerySphere(position, max_radius);
        
        EntityId nearest = INVALID_ENTITY;
        float nearest_distance = std::numeric_limits<float>::max();
        
        for (EntityId entity : candidates) {
            // Would need access to transform component to compute actual distance
            // This is a simplified example
        }
        
        return nearest != INVALID_ENTITY ? std::make_optional(nearest) : std::nullopt;
    }
    
    auto GetThreadingRequirements() const -> ThreadingRequirements override {
        return ThreadingRequirements{
            .read_components = {},
            .write_components = {},
            .execution_phase = ExecutionPhase::Update,
            .thread_safety = ThreadSafety::ThreadSafe
        };
    }
    
    void Update(World& world, float delta_time) override {
        // This system primarily provides query functionality
        // Update method can be used for background optimizations
    }
    
private:
    SpatialPartition& spatial_partition_;
};
```

### Scene Graph and Asset Integration

#### Scene Management

```cpp
// High-level scene management
class SceneManager {
public:
    struct SceneConfiguration {
        std::string name;
        std::filesystem::path asset_path;
        glm::vec3 world_bounds_min{-1000.0f};
        glm::vec3 world_bounds_max{1000.0f};
        SpatialPartition::Configuration spatial_config;
        bool enable_streaming = false;
        float streaming_radius = 500.0f;
    };
    
    // Scene lifecycle management
    auto LoadScene(const SceneConfiguration& config) -> std::unique_ptr<Scene> {
        auto scene = std::make_unique<Scene>(config);
        
        // Initialize spatial partitioning for this scene
        scene->spatial_partition = std::make_unique<AdaptiveHybridPartition>(config.spatial_config);
        
        // Load scene assets
        if (!config.asset_path.empty()) {
            LoadSceneAssets(*scene, config.asset_path);
        }
        
        return scene;
    }
    
    void UnloadScene(std::unique_ptr<Scene> scene) {
        if (!scene) {
            return;
        }
        
        // Cleanup all entities in the scene
        CleanupSceneEntities(*scene);
        
        // Scene will be automatically destroyed
    }
    
    // Scene streaming for large worlds
    void UpdateSceneStreaming(Scene& scene, const glm::vec3& viewer_position) {
        if (!scene.config.enable_streaming) {
            return;
        }
        
        // Determine which scene chunks should be loaded/unloaded
        auto chunks_to_load = DetermineRequiredChunks(scene, viewer_position);
        auto chunks_to_unload = DetermineUnneededChunks(scene, viewer_position);
        
        // Asynchronously load new chunks
        for (const auto& chunk : chunks_to_load) {
            if (scene.loaded_chunks.find(chunk.id) == scene.loaded_chunks.end()) {
                LoadSceneChunkAsync(scene, chunk);
            }
        }
        
        // Unload distant chunks
        for (const auto& chunk_id : chunks_to_unload) {
            UnloadSceneChunk(scene, chunk_id);
        }
    }
    
private:
    struct SceneChunk {
        uint32_t id;
        glm::vec3 center;
        float radius;
        std::filesystem::path asset_path;
    };
    
    void LoadSceneAssets(Scene& scene, const std::filesystem::path& asset_path) {
        // Load scene description from JSON or other format
        auto scene_data = LoadSceneData(asset_path);
        
        // Create entities and components based on scene data
        CreateSceneEntities(scene, scene_data);
    }
    
    void CleanupSceneEntities(Scene& scene) {
        // Remove all entities that belong to this scene
        // This would integrate with the ECS World to destroy entities
    }
    
    auto LoadSceneData(const std::filesystem::path& path) -> nlohmann::json {
        // Load and parse scene configuration
        return nlohmann::json{}; // Placeholder
    }
    
    void CreateSceneEntities(Scene& scene, const nlohmann::json& scene_data) {
        // Parse scene data and create entities with appropriate components
        // This would integrate with the ECS World and asset system
    }
    
    auto DetermineRequiredChunks(const Scene& scene, const glm::vec3& position) 
        -> std::vector<SceneChunk> {
        // Determine which scene chunks should be loaded based on position
        return {};
    }
    
    auto DetermineUnneededChunks(const Scene& scene, const glm::vec3& position) 
        -> std::vector<uint32_t> {
        // Determine which chunks can be unloaded
        return {};
    }
    
    void LoadSceneChunkAsync(Scene& scene, const SceneChunk& chunk) {
        // Asynchronously load scene chunk using asset system
    }
    
    void UnloadSceneChunk(Scene& scene, uint32_t chunk_id) {
        // Unload scene chunk and cleanup entities
        scene.loaded_chunks.erase(chunk_id);
    }
};

// Scene representation
class Scene {
public:
    explicit Scene(const SceneManager::SceneConfiguration& config) 
        : config(config) {}
    
    SceneManager::SceneConfiguration config;
    std::unique_ptr<SpatialPartition> spatial_partition;
    std::unordered_set<uint32_t> loaded_chunks;
    
    // Scene-specific data
    std::vector<EntityId> scene_entities;
    std::unordered_map<std::string, EntityId> named_entities;
    
    // Scene bounds and metadata
    glm::vec3 bounds_min;
    glm::vec3 bounds_max;
    std::string metadata;
};
```

## Performance Optimization

### Cache-Friendly Data Structures

```cpp
// Optimized transform storage for cache efficiency
class OptimizedTransformStorage {
public:
    // Structure-of-arrays layout for better cache utilization
    struct TransformArrays {
        std::vector<glm::vec3> positions;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec3> scales;
        std::vector<glm::mat4> world_matrices;
        std::vector<bool> dirty_flags;
        std::vector<EntityId> entities;
    };
    
    void UpdateTransformBatch(std::span<const EntityId> entities) {
        // Process transforms in batches for better cache utilization
        constexpr size_t batch_size = 64; // Cache line friendly
        
        for (size_t i = 0; i < entities.size(); i += batch_size) {
            size_t end = std::min(i + batch_size, entities.size());
            ProcessTransformBatch(entities.subspan(i, end - i));
        }
    }
    
private:
    TransformArrays transform_data_;
    
    void ProcessTransformBatch(std::span<const EntityId> batch) {
        // SIMD-friendly batch processing
        // Update multiple transforms simultaneously where possible
    }
};

// Memory pool for spatial partitioning
class SpatialMemoryPool {
public:
    explicit SpatialMemoryPool(size_t pool_size = 64 * 1024 * 1024) // 64MB default
        : pool_size_(pool_size) {
        memory_pool_ = std::make_unique<uint8_t[]>(pool_size_);
        current_offset_ = 0;
    }
    
    template<typename T>
    [[nodiscard]] auto Allocate(size_t count = 1) -> T* {
        size_t size = sizeof(T) * count;
        size_t aligned_size = AlignSize(size, alignof(T));
        
        if (current_offset_ + aligned_size > pool_size_) {
            throw std::bad_alloc(); // Pool exhausted
        }
        
        T* ptr = reinterpret_cast<T*>(memory_pool_.get() + current_offset_);
        current_offset_ += aligned_size;
        
        return ptr;
    }
    
    void Reset() {
        current_offset_ = 0;
    }
    
private:
    std::unique_ptr<uint8_t[]> memory_pool_;
    size_t pool_size_;
    std::atomic<size_t> current_offset_;
    
    [[nodiscard]] auto AlignSize(size_t size, size_t alignment) const -> size_t {
        return (size + alignment - 1) & ~(alignment - 1);
    }
};
```

## Success Criteria

### Performance Metrics

1. **Spatial Query Performance**: Sub-millisecond response for typical proximity queries with 10,000+ entities
2. **Transform Update Performance**: Process 50,000+ transform updates per frame at 60+ FPS
3. **Culling Efficiency**: Reduce rendered entities by 80%+ through frustum and occlusion culling
4. **Memory Efficiency**: Spatial data structures consume <100MB for typical game scenarios

### Functionality Requirements

1. **Transform Hierarchies**: Support 10+ levels of parent-child relationships without performance degradation
2. **Spatial Partitioning**: Automatic adaptation to entity distribution with optimal query performance
3. **Cross-Platform Compatibility**: Identical spatial behavior between native and WebAssembly builds
4. **ECS Integration**: Seamless component-based spatial queries and updates

### Threading Safety

1. **Concurrent Queries**: Multiple threads can perform spatial queries simultaneously without blocking
2. **Lock-Free Updates**: Transform updates use lock-free algorithms where possible
3. **Memory Ordering**: Correct memory ordering for all cross-thread spatial operations
4. **Deterministic Results**: Identical spatial query results across multiple runs

## Future Enhancements

### Phase 2: Advanced Spatial Acceleration

#### GPU-Accelerated Culling

```cpp
// GPU compute shader-based culling for massive entity counts
class GPUCullingSystem {
public:
    void PerformGPUCulling(const CameraData& camera, std::span<const EntityData> entities);
    auto GetVisibleEntities() -> std::span<const EntityId>;
    
private:
    // OpenGL compute shader integration for parallel culling
    std::unique_ptr<ComputeShader> culling_shader_;
    std::unique_ptr<StorageBuffer> entity_buffer_;
    std::unique_ptr<StorageBuffer> visibility_buffer_;
};
```

#### Advanced LOD Systems

```cpp
// Temporal and geometric LOD with hysteresis
class AdvancedLODSystem {
public:
    void UpdateTemporalLOD(float time_budget_ms);
    void UpdateGeometricLOD(const CameraData& camera);
    void ApplyLODHysteresis(float hysteresis_factor = 0.1f);
    
private:
    // Prevent LOD thrashing with temporal smoothing
    std::unordered_map<EntityId, LODTransition> lod_transitions_;
};
```

### Phase 3: Streaming and Virtualization

#### Virtual Scene Management

```cpp
// Virtualized scene management for massive worlds
class VirtualSceneManager {
public:
    void SetVirtualWorldSize(const glm::vec3& world_size);
    void UpdateVirtualization(const glm::vec3& viewer_position);
    auto GetVirtualEntities(const glm::vec3& region_min, const glm::vec3& region_max) 
        -> std::vector<VirtualEntity>;
    
private:
    // Stream entities in/out based on proximity
    std::unique_ptr<EntityStreamer> entity_streamer_;
    std::unique_ptr<LODStreamer> lod_streamer_;
};
```

#### Hierarchical Level-of-Detail

```cpp
// Hierarchical LOD with automatic mesh simplification
class HierarchicalLOD {
public:
    void BuildHLODHierarchy(const std::vector<EntityId>& entities);
    void UpdateHLODVisibility(const CameraData& camera);
    auto GetHLODRepresentation(const glm::vec3& region_center, float region_size) 
        -> HLODNode;
    
private:
    // Automatic mesh clustering and simplification
    std::unique_ptr<MeshSimplifier> mesh_simplifier_;
    std::unique_ptr<ClusterBuilder> cluster_builder_;
};
```

## Risk Mitigation

### Performance Risks

- **Risk**: Spatial partitioning overhead negating query performance benefits
- **Mitigation**: Adaptive partitioning strategies with performance monitoring
- **Fallback**: Simple grid-based partitioning for worst-case scenarios

### Threading Complexity Risks

- **Risk**: Race conditions in spatial data structure updates
- **Mitigation**: Lock-free design with extensive testing and validation
- **Fallback**: Simplified single-threaded updates for debugging

### Memory Usage Risks

- **Risk**: Spatial data structures consuming excessive memory
- **Mitigation**: Memory pools with configurable limits and monitoring
- **Fallback**: Disable spatial optimization features in memory-constrained environments

### Integration Risks

- **Risk**: Complex integration with existing rendering and physics systems
- **Mitigation**: Incremental integration with comprehensive testing
- **Fallback**: Maintain compatibility with existing spatial query patterns

This scene management system provides the spatial foundation required for optimal rendering performance and efficient
gameplay systems while integrating seamlessly with the established engine architecture. The design emphasizes
performance through intelligent culling and cache-friendly data structures while maintaining the flexibility needed for
diverse game scenarios.
