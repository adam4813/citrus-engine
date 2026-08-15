# AI Agent Instructions for Citrus Engine

## Project Structure (Non-Obvious)

This repo contains **two linked CMake projects**:

- **Engine** (root) — the core library, built with `cli-native` presets
- **Editor** (`editor/`) — a separate CMake project that consumes the engine as a vcpkg package via an overlay port
  in `ports/citrus-engine/`

**The editor does not directly reference engine source.** It depends on the engine being installed as a vcpkg package.
When engine code changes, the overlay port version must be bumped so vcpkg recognizes the package changed and
rebuilds it. See `.github/skills/citrus-editor-workflow/SKILL.md` for the full version-sync procedure.

## Build & Verify

Agent builds use **`cli-*` presets** — not `native`. The `native` presets omit agent-environment configuration.
Using the wrong preset silently produces incorrect builds.

```bash
cmake --build --preset cli-native-debug          # build engine
ctest --preset cli-native-test-debug             # run tests
```

**A task is not complete until both projects build and all tests pass.** Clean up any temporary investigation files
before marking done.

## Domain Knowledge (Progressive Disclosure)

Read the relevant guide *before* implementing — these encode design decisions that are not inferable from code alone:

| Domain | Reference |
|--------|-----------|
| UI / batch rendering | `UI_DEVELOPMENT_BIBLE.md` |
| Testing priorities and structure | `TESTING.md` |
| Naming, formatting, C++20 idioms | `docs/code-style.md` |
| C++20 module structure + MSVC gotchas | `docs/cpp20-modules.md` |
| Design patterns, DRY, options objects | `docs/design-principles.md` |
| Adding a dependency | `.github/skills/citrus-add-dependency/SKILL.md` |
| Editor version sync | `.github/skills/citrus-editor-workflow/SKILL.md` |

## Automation

Before writing a new script or workflow, check `Makefile` and `scripts/` for existing automation. When you do create
new automation, make it reusable and document it so the same steps don't have to be reconstructed manually.
