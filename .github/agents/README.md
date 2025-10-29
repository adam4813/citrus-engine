# Citrus Engine Custom Agents

This directory contains specialized GitHub Copilot custom agents for the Citrus Engine project. Each agent is an expert in a specific engine module or domain.

## What are Custom Agents?

Custom agents are specialized AI assistants that have deep knowledge of specific parts of the codebase. When you invoke a custom agent through GitHub Copilot, it provides expert guidance tailored to that domain.

## Available Agents

### Module Experts

| Agent | Module | Expertise |
|-------|--------|-----------|
| `assets-expert` | Assets (`src/engine/assets/`) | Asset loading, tileset management, resource handling |
| `components-expert` | Components (`src/engine/components/`) | ECS component design, data-oriented patterns |
| `ecs-expert` | ECS (`src/engine/ecs/`) | Flecs ECS framework, entity management, systems |
| `input-expert` | Input (`src/engine/input/`) | Keyboard, mouse, gamepad input handling |
| `os-expert` | OS (`src/engine/os/`) | Operating system abstraction layer |
| `platform-expert` | Platform (`src/engine/platform/`) | Cross-platform windowing, file system, timing |
| `rendering-expert` | Rendering (`src/engine/rendering/`) | OpenGL/WebGL, shaders, materials, meshes |
| `scene-expert` | Scene (`src/engine/scene/`) | Scene graphs, transform hierarchies |
| `ui-expert` | UI (`src/engine/ui/`) | ImGui immediate mode GUI, batch rendering |

### Documentation Expert

| Agent | Expertise |
|-------|-----------|
| `documentation-expert` | Creating user-facing documentation with Doxygen, ReadTheDocs, and Markdown |

## When to Use Custom Agents

### Use module-specific agents when:
- Adding new features to a module
- Fixing bugs in a module
- Optimizing module performance
- Understanding module architecture
- Writing tests for a module

### Use the documentation-expert when:
- Writing API documentation (Doxygen comments)
- Creating user guides and tutorials
- Setting up ReadTheDocs
- Organizing documentation structure
- Ensuring docs are user-friendly

## How to Use Custom Agents

In GitHub Copilot, you can invoke a custom agent by using the `@agent-name` syntax in your conversation. For example:

```
@rendering-expert How do I implement a new shader type?
```

```
@ecs-expert What's the best way to create a query for entities with Transform and Velocity?
```

```
@documentation-expert Write Doxygen comments for this texture loading function
```

## Agent Structure

Each agent follows the GitHub Copilot custom agent specification:

```markdown
---
name: agent-name
description: Brief description of agent expertise
---

Agent instructions and guidelines...
```

The YAML frontmatter defines the agent's identity, and the markdown body contains:
- Expertise areas
- Module structure and organization
- Guidelines and best practices
- Code examples and patterns
- Integration points with other modules
- Platform considerations
- References to relevant documentation
- Responsibilities

## Agent Philosophy

### Module Agents Know:
- ✅ Module API and implementation patterns
- ✅ How their module integrates with others
- ✅ Performance considerations for their module
- ✅ Platform-specific issues for their module
- ✅ Best practices and common pitfalls
- ✅ Testing strategies for their module

### Module Agents Follow:
- ✅ C++20 standards and modern practices
- ✅ Engine coding conventions (AGENTS.md)
- ✅ Cross-platform compatibility requirements
- ✅ Data-oriented design principles
- ✅ RAII and smart pointer usage

### Documentation Agent Knows:
- ✅ How to write clear API documentation
- ✅ Doxygen comment syntax and best practices
- ✅ ReadTheDocs configuration and setup
- ✅ Documentation organization and structure
- ✅ Writing for engine users (not developers)

## Maintaining Agents

When making significant changes to a module:

1. **Update the corresponding agent** if:
   - Module architecture changes
   - New patterns or best practices are introduced
   - Integration points with other modules change
   - Platform support changes

2. **Keep agents focused** on their module:
   - Don't add general engine knowledge
   - Reference other agents for cross-module concerns
   - Keep examples relevant to the module

3. **Test agent guidance**:
   - Ensure code examples compile and work
   - Verify guidelines match current codebase
   - Check references are up-to-date

## References

- [GitHub Copilot Custom Agents Documentation](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/create-custom-agents)
- [Citrus Engine AGENTS.md](../../AGENTS.md) - General AI agent instructions
- [Citrus Engine Documentation](../../docs/) - User-facing documentation

## Contributing

When adding a new module to the engine, create a corresponding custom agent:

1. Create `module-name-expert.md` in this directory
2. Follow the structure of existing agents
3. Include YAML frontmatter with `name` and `description`
4. Document module expertise, guidelines, and patterns
5. Add code examples and integration points
6. Reference relevant documentation
7. Update this README with the new agent

---

**Note**: These agents are tools to help developers work on the engine. They provide expertise and guidance but should always be combined with code review, testing, and validation.
