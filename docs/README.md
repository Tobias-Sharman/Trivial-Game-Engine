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

Some mockup notes are made for the core section for some probing of system
capabilities. These are likely to change in the future once documentation style
is settled. Time and memory should make suitable references, but CPU is not to
used as a reliable reference until finalising it with the review that will come
after integrating Linux probing.

- Core
  - [CPU](subsystems/core/cpu.md): CPU information for compile time and runtime
  dispatch for different CPU features and topologies
  - [Memory](subsystems/core/memory.md): memory information for use in
  allocators. Not of notable usage for engine users
  - [Time](subsystems/core/time.md): time information for accurate and easy
  time management

The rest are currently deferred and will be written over time as subsystems
become closer to completed form. Until then, the old task system design notes
are in [design/task_system.md](design/task_system.md). These notes are still
fairly accurate, but some changes have been made that were not reflected in
the docs from the migration to custom multi-threading primitives.
