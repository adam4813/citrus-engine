# Contributing to Citrus Engine

Thank you for your interest in contributing to Citrus Engine! 🍊 Whether you're fixing a bug, adding a feature, or improving documentation, your help is appreciated.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [What We're Looking For](#what-were-looking-for)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)
- [Your First Contribution](#your-first-contribution)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Testing Requirements](#testing-requirements)
- [Commit Conventions](#commit-conventions)
- [Pull Request Process](#pull-request-process)
- [Code Style](#code-style)

---

## Code of Conduct

Be respectful and constructive. We want this to be a welcoming project for contributors of all experience levels.

---

## What We're Looking For

We welcome contributions in these areas:

- **Bug fixes** — Especially reproducible crashes, rendering artifacts, or incorrect behavior
- **Engine systems** — New components, systems, or platform abstractions
- **Performance improvements** — With profiling data to justify the change
- **Documentation** — Corrections, clarifications, new examples
- **Tests** — Integration and end-to-end tests following [TESTING.md](TESTING.md)
- **Editor features** — Improvements to the scene editor (`editor/`)

We are less likely to accept:
- Large refactors without prior discussion
- Speculative performance changes without profiling evidence
- Style-only changes that don't follow our formatting rules

When in doubt, [open an issue](https://github.com/adam4813/citrus-engine/issues) to discuss before investing significant time.

---

## Reporting Bugs

1. **Search existing issues** first — the bug may already be reported.
2. **Open a new issue** using the Bug Report template.
3. Include:
   - Platform and compiler (Windows/MSVC, Linux/Clang-18, etc.)
   - Steps to reproduce
   - Expected vs. actual behavior
   - Build preset used (`native`, `wasm`, etc.)
   - Any relevant log output or error messages

---

## Suggesting Features

1. **Search existing issues and discussions** first.
2. **Open a Feature Request issue** describing:
   - The problem you're trying to solve
   - Your proposed solution
   - Alternatives you've considered
3. Wait for feedback before starting implementation — this avoids duplicate work and misaligned designs.

---

## Your First Contribution

Looking for a good starting point? Search for issues labeled:

- `good first issue` — Small, well-scoped, documented tasks
- `help wanted` — Larger tasks the maintainers want help with

Not sure where to start? Read [Architecture Overview](docs/architecture.md) to understand the codebase structure.

---

## Development Setup

### Prerequisites

See [README.md Quick Start](README.md#-quick-start) for full platform-specific setup.

**Short version:**

- CMake 3.28+
- vcpkg (set `VCPKG_ROOT` environment variable)
- Windows: MSVC 2022 or Clang-18+
- Linux: Clang-18+

### Build and Test

```bash
# Configure (Windows — adjust triplet for your platform)
cmake --preset native -DVCPKG_TARGET_TRIPLET=x64-windows

# Build
cmake --build --preset native-debug

# Run tests
ctest --preset native-test-debug
```

### Editor Project

The scene editor is a separate CMake project in `editor/`. When modifying engine code, you must bump the engine version in `ports/citrus-engine/vcpkg.json` so the editor picks up the change. See [docs/contributing workflow](#editor-project) or the skill at `.github/skills/citrus-editor-workflow/SKILL.md`.

---

## Making Changes

### Branch naming

```
feature/short-description
fix/short-description
docs/short-description
refactor/short-description
```

### Adding a new engine system

1. **Plan** — Create a design document in `plan/systems/` describing the system interface and responsibilities
2. **Design** — Define the module interface (`.cppm`) before implementing
3. **Implement** — Add source in the appropriate `src/engine/` subdirectory
4. **Test** — Add integration tests in `tests/integration/` (see [TESTING.md](TESTING.md))
5. **Document** — Update relevant docs in `docs/` and add to mkdocs nav if significant

### Modifying existing systems

- Run existing tests before starting: `ctest --preset native-test-debug`
- Keep changes focused — separate refactors from behavior changes in different commits
- If changing a public API, update all call sites and the relevant docs

---

## Testing Requirements

All PRs must pass the full test suite:

```bash
ctest --preset native-test-debug
```

Test priority (from [TESTING.md](TESTING.md)):

1. **Integration tests** — Highest priority; verify component interactions
2. **End-to-end tests** — Complete workflows
3. **Unit tests** — Complex logic only; avoid testing trivial code

New features should include integration tests. Bug fixes should include a test that would have caught the bug.

---

## Commit Conventions

Use clear, imperative commit messages:

```
Add tilemap collision detection
Fix texture memory leak on reload
Refactor shader cache to use string_view keys
Update CONTRIBUTING.md with editor workflow
```

- **Capitalize** the first word
- **No period** at the end
- **Imperative mood**: "Add", "Fix", "Update", "Remove" — not "Added", "Fixes", "Updating"
- Keep the subject line under 72 characters
- Use the body for the *why*, not the *what* (the diff shows the what)

For co-authored commits, include trailers:

```
Co-authored-by: Name <email@example.com>
```

---

## Pull Request Process

1. **Fork** the repository and create your branch from `main`
2. **Make your changes** following the guidelines above
3. **Run the full test suite** — PRs with failing tests will not be reviewed
4. **Build both engine and editor** if you modified engine code
5. **Open a PR** with:
   - A clear description of what changed and why
   - Reference to any related issues (`Fixes #123`)
   - Screenshots or recordings for visual changes
6. **Respond to review feedback** — expect at least one review cycle
7. **Squash or tidy commits** if requested before merge

PRs are merged via squash merge to keep the main branch history clean.

---

## Code Style

We enforce consistent style across the codebase. Before committing, format your code:

```bash
# Format all changed files
clang-format -i <your-files>

# Or use the Makefile target if available
make format
```

Full standards are documented in three focused guides:

| Guide | Covers |
|-------|--------|
| [Code Style](docs/code-style.md) | Naming, formatting, comments, type safety, containers |
| [C++20 Modules](docs/cpp20-modules.md) | Module structure, import rules, MSVC gotchas |
| [Design Principles](docs/design-principles.md) | GoF patterns, composition, DRY, options objects, RAII |

The root [CODE_STYLE_GUIDE.md](CODE_STYLE_GUIDE.md) links to all three.
