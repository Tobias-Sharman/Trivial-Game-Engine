# Documentation

## Manual

For making a game with the engine.

- [Getting started](manual/getting_started.md): adding the engine to a game,
the entry point, layers, and current limits
- [Building](manual/building.md): requirements, presets, and the build script
- [Configuration](manual/configuration.md): build options, compile time
tunables, and runtime configuration
- [Platform support](manual/platform_support.md): supported platforms, minimum
versions, and how platforms differ

## Development

For working on the engine itself.

- [Tooling](development/tooling.md): compiler warnings, clang-tidy,
clang-format, and the other checks
- [Testing](development/testing.md): what to test, isolated tests, and what no
preset covers
- Conventions
  - [Naming and formatting](development/conventions/naming_and_formatting.md)
  - [Types and APIs](development/conventions/types_and_apis.md)
  - [Files and headers](development/conventions/files_and_headers.md)
  - [Correctness](development/conventions/correctness.md)

## Overview

- [Roadmap](roadmap.md): planned work, and the outstanding work for each
subsystem
- [Architecture](architecture.md): deferred until the window manager exists

## Subsystems

Deferred, and written over time. Until then, the task system's old design notes
are in [design/task_system.md](design/task_system.md). These notes are still
fairly accurate, but some changes have been made that were not reflected in
the docs from the migration to custom multi-threading primitives.
