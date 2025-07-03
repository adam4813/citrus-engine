# Game Engine Planning Process Instructions

## Overview

This document defines the systematic approach for planning the extraction and development of a modular, powerful game
engine from the existing colony game codebase. The planning follows a hierarchical approach from high-level architecture
down to implementation details, with each level building upon the previous.

## Core Planning Principles

### 1. Systems and Process First

- Every repeatable task must have a documented process
- Automation should be considered for any recurring activities
- Document decisions and rationale for future reference
- Create reusable templates and patterns

### 2. Modular Design Philosophy

- Design for composability and reusability
- Minimize coupling between systems
- Maximize cohesion within systems
- Enable selective feature inclusion

### 3. Documentation Standards

- All planning documents must follow the provided template
- Include rationale for major decisions
- Document trade-offs and alternatives considered
- Provide clear success criteria

### 4. C++20 Modern Standards (IMPERATIVE)

- **Language Features**: Leverage C++20 features (ranges, concepts, coroutines, structured bindings, `std::optional`,
  `std::variant`) where they improve readability, maintainability, or performance
- **Attributes**: Use `[[nodiscard]]` for return values that shouldn't be ignored, `[[maybe_unused]]` for intentional
  unused parameters
- **Const Correctness**: Mark variables, parameters, and member functions as `const` wherever possible
- **Constexpr**: Prefer `constexpr` for compile-time constants and functions; eliminate magic numbers
- **Memory Management**: Prefer stack allocation and smart pointers over raw pointers
- **Thread Safety**: Design for multithreaded access with appropriate synchronization primitives
- **Error Handling**: Use `std::optional`, `std::variant`, or exceptions; avoid new error handling dependencies

### 5. Declarative Over Imperative Design

- **Configuration-Driven**: Systems should be configurable through data rather than code changes
- **Data-Driven Architecture**: Game logic should be expressible through configuration files
- **Composition Over Inheritance**: Favor component composition and data-driven behavior
- **Functional Patterns**: Prefer pure functions and immutable data structures where possible

### 6. Scripting Interface Requirements

- **External Language Support**: All major engine systems must expose clean interfaces for external scripting
- **API Consistency**: Scripting APIs should follow consistent patterns across all systems
- **Type Safety**: Scripting bindings must maintain type safety and error reporting
- **Performance Isolation**: Script execution should not block critical engine systems

### 7. Entity Component System (ECS) Architecture (IMPERATIVE)

- **Component-Based Design**: All game objects are entities with associated components containing data
- **System Separation**: Logic is separated into systems that operate on components, not entities directly
- **Data-Oriented Design**: Components store only data; systems contain all logic
- **Component Queries**: Systems query for entities with specific component combinations
- **Inter-System Communication**: Use events, messages, or shared components for system communication
- **Component Lifetime**: Clear ownership and lifecycle management for components

### 8. Multi-Threading Architecture (IMPERATIVE)

- **System Parallelization**: Design systems to run in parallel batches based on data dependencies
- **Frame Pipelining**: Graphics rendering can use previous frame data while logic systems update current frame
- **Lock-Free Design**: Prefer lock-free data structures and atomic operations where possible
- **Thread-Safe Systems**: Each system must be designed for concurrent execution with appropriate synchronization
- **Work Stealing**: Enable work distribution across available CPU cores
- **Memory Consistency**: Ensure proper memory ordering for cross-thread data access

### 9. Build System and Platform Integration (IMPERATIVE)

- **CMake Integration**: All engine components must integrate cleanly with the existing CMake build system
- **Native and WASM Builds**: Engine must support both native and WebAssembly compilation targets
- **Module System**: Leverage C++20 modules where supported, with fallback to traditional headers
- **vcpkg Dependencies**: New dependencies must be added through vcpkg.json for reproducible builds
- **Cross-Platform Compatibility**: Design for Windows, Linux, and Web (WASM) targets

### 10. Asset Pipeline and Data Management (IMPERATIVE)

- **JSON Configuration**: Follow existing JSON structure patterns for assets (items.json, structures.json, etc.)
- **Asset Loading**: Design for both filesystem and embedded asset loading (for WASM)
- **Resource Registry Pattern**: Follow existing resource registry patterns for type-safe asset management
- **Data Format Consistency**: Maintain consistent data structure patterns across new engine systems

## Planning Hierarchy

### Phase 1: High-Level Architecture (ARCH_*)

**Scope**: Overall engine structure, major subsystems, and their relationships
**Deliverables**:

- System boundary definitions
- Inter-system communication patterns
- Technology stack decisions
- Platform compatibility strategy

### Phase 2: System-Level Design (SYS_*)

**Scope**: Individual system architecture and interfaces
**Deliverables**:

- API definitions
- Data flow diagrams
- Performance requirements
- Testing strategies

### Phase 3: Module-Level Design (MOD_*)

**Scope**: Detailed component designs within systems
**Deliverables**:

- Class hierarchies
- Interface contracts
- Implementation patterns
- Integration points

### Phase 4: Implementation Plans (IMPL_*)

**Scope**: Concrete implementation roadmaps
**Deliverables**:

- File structure definitions
- Code organization patterns
- Build system integration
- Migration strategies from existing code

## Document Naming Convention

Use the following naming pattern for all planning documents:

```
{PHASE}_{SYSTEM}_{COMPONENT}_{VERSION}.md
```

Examples:

- `ARCH_ENGINE_CORE_v1.md` - High-level engine architecture
- `SYS_GRAPHICS_RENDERING_v1.md` - Graphics rendering system design
- `MOD_GRAPHICS_SHADER_v1.md` - Shader management module design
- `IMPL_GRAPHICS_OPENGL_v1.md` - OpenGL renderer implementation plan

## Planning Document Requirements

### Required Sections (see template)

1. **Executive Summary** - One-paragraph overview
2. **Scope and Objectives** - Clear boundaries and goals
3. **Architecture/Design** - Core technical content
4. **Dependencies** - Prerequisites and relationships
5. **Success Criteria** - Measurable outcomes
6. **Implementation Strategy** - How to achieve the goals
7. **Risk Assessment** - Potential issues and mitigations
8. **Decision Rationale** - Why this approach was chosen

### Cross-Reference Requirements

- Reference existing codebase examples where relevant
- Link to related planning documents
- Maintain dependency graphs between systems
- Document integration points clearly

## Validation Process

### Before Implementation

1. **Dependency Check**: Verify all dependencies are satisfied
2. **Interface Validation**: Ensure APIs are consistent across systems
3. **Resource Review**: Confirm implementation scope is reasonable
4. **Conflict Resolution**: Address any contradictions with existing plans

### During Implementation

1. **Progress Tracking**: Document implementation status
2. **Issue Logging**: Record deviations from plan and resolutions
3. **Interface Changes**: Update plans when APIs evolve
4. **Testing Validation**: Confirm success criteria are met

## Task Granularity Guidelines

### PR-Sized Work Units

Each implementation task should be:

- Completable in 1-3 days by a senior engineer
- Independently reviewable and testable
- Focused on a single concern or feature
- Include appropriate unit tests
- Maintain backward compatibility where possible

### Task Independence

- Minimize dependencies between concurrent tasks
- Clearly define interface contracts upfront
- Enable parallel development streams
- Provide clear integration checkpoints

## Planning Best Practices

### 1. Start with Existing Code Analysis

- Inventory current engine-like components in `src/engine/`
- Identify reusable patterns and anti-patterns
- Document current architectural debt
- Map existing dependencies

### 2. Design for Modern C++20

- Leverage modules, concepts, and ranges where appropriate
- Use standard library features over custom implementations
- Follow the project's coding standards strictly
- Consider compile-time computation opportunities

### 3. Minimize Third-Party Dependencies

- Evaluate each dependency for value vs. maintenance cost
- Prefer header-only libraries when possible
- Document licensing and compatibility requirements
- Plan for dependency updates and security patches

### 4. Performance-First Design

- Consider cache-friendly data layouts
- Design for multithreading from the start
- Plan for SIMD optimization opportunities
- Include performance benchmarking in success criteria

### 5. Cross-Platform Considerations

- Design abstractions for platform-specific code
- Plan for both native and WASM builds
- Consider mobile platforms for future expansion
- Document platform-specific limitations

## Review and Iteration Process

### Planning Review Cycles

1. **Initial Draft**: Create first version of planning document
2. **Dependency Review**: Validate relationships with other systems
3. **Technical Review**: Deep dive on technical feasibility
4. **Resource Review**: Confirm scope and timeline estimates
5. **Final Approval**: Document is ready for implementation

### Version Control Integration

- All planning documents are version controlled
- Use semantic versioning for major revisions
- Link planning documents to implementation PRs
- Maintain change logs for significant updates

## Success Metrics

### Planning Quality Indicators

- Implementation matches planning predictions (>90% accuracy)
- Minimal architectural rework required during implementation
- Clear separation of concerns achieved
- Performance targets met or exceeded
- Third-party dependency count minimized

### Process Efficiency Indicators

- Planning documents are reusable across similar systems
- Development velocity increases over time
- Bug count decreases in engine components
- Code review time decreases due to clear plans
- New team members can contribute faster

## Templates and Tools

See `PLANNING_TEMPLATE.md` for the standard document template that should be used for all planning documents. This
template ensures consistency and completeness across all planning efforts.

## Continuous Improvement

This planning process itself should evolve based on:

- Lessons learned during implementation
- Feedback from development team
- Changes in project requirements
- New best practices discovered

Update this document as the process matures, maintaining backward compatibility with existing planning documents where
possible.
