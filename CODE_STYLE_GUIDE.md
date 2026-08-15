# Citrus Engine — Code Style Guide

> Coding standards for Citrus Engine. Docs are the single source of truth — this file is an index.

The style guide is split into three focused documents for easy navigation:

| Guide | What it covers |
|-------|---------------|
| **[Code Style](docs/code-style.md)** | Naming conventions, formatting, comments, type safety, containers/ranges |
| **[C++20 Modules](docs/cpp20-modules.md)** | Module interface/implementation structure, import rules, MSVC `constexpr` gotcha |
| **[Design Principles](docs/design-principles.md)** | GoF patterns, composition, RAII, DRY, options objects, smaller files, declarative code |

---

## Quick Reference

| Convention | Rule |
|------------|------|
| Functions / Methods | `PascalCase` |
| Variables / Parameters | `snake_case` |
| Classes / Structs | `PascalCase` |
| Private members | `snake_case_` (trailing underscore) |
| Namespaces | `snake_case` |
| Constants (`constexpr`, namespace scope) | `UPPER_CASE` |
| Constant parameters / local `const` | `snake_case` |
| Macros | `UPPER_CASE` |
| Files | `snake_case.cppm` / `snake_case.cpp` |

See [Code Style](docs/code-style.md) for the full conventions table with examples.

---

## Key Rules at a Glance

- **Indentation**: Tabs (width 4) for C++ files; 2-space spaces for YAML and CMake (enforced by `.editorconfig`)
- **Braces**: Opening brace on same line; `else` and `catch` go on their own line (not K&R)
- **Line length**: 120 characters max; binary operators break to the start of the next line
- **Parameters**: >3 params → options struct; obvious conventions like `(r, g, b, a)` or `(x, y, z)` are exempt
- **Modules**: Implementation files must repeat imports — they don't inherit from interface files
- **MSVC**: Use `constexpr`, never `inline constexpr` in module interfaces (4GB object file bug)
- **Composition**: Build features with member objects; use inheritance only for polymorphism
- **DRY**: Extract shared logic when it appears 3+ times; don't extract when it hurts readability
