# Trivial Engine

A game engine designed for 2.5D games. The design will be more specialised and
directed towards the creation of game with myself and a friend. The creation of
the engine is to allow for complete freedom in the development of the game.

Build tooling (including grabbing dependencies) is handled via CMake, but built
on top is a python script that is recommended for usage for building and
especially for testing.

Tested on MacOS with Apple Silicon and remains needing testing on other
platforms - all architectures for Windows and Linux, and older x86 Mac. A later
check on support for mobile platforms may also be taken as an extension to the
engine.

Licensed under the [Apache License, Version 2.0](LICENSE). Games and other
software built with the engine can use any license, but must keep the
[NOTICE.md](NOTICE.md) file's attribution somewhere a user could reasonably
find it (e.g. a credits screen, documentation, or an in-app NOTICE display).
NOTICE.md also lists the engine's third-party dependencies and their license
terms, and credits design references where deemed appropriate. A note on this
is it need not be a title screen as that can often interfere with the feel of a
game, and as such implementation of accreditation is deferred to the user.

Some documentation can be found in this repository, this documentation is not
complete, due to some bits with work ongoing and without a proper release
schedule keeping to having full documentation for each component added that may
likely change later. It is a personal project so a great formality of this is
neglected prior to a proper version 1.0.0.

## Current plan of action

- Rework config for engine and task system to match style taken through other
engine components and move some such config to compile time over runtime.
- Custom Chrono with suitable types
- Function decoration with const args and compiler attributes where appropriate
- Allocator
  - Arenas allocator
  - general allocator
    - small   <= 8 KiB   per-thread bins over 64 KiB pages, remote free lists
    - medium  <= 512 KiB TLSF over boundary tags, one pool per segment
    - large   > 512 KiB  page granular spans, multi segment runs above 2 MiB
  - Debug layer
- Proper test and benchmarking framework
  - Benchmark allocator, probably against mimalloc and jemalloc
  - Full testing of allocator (segment allocator is briefly yet importantly not
  fully tested)
- Basic physics system to test and profile the task system
- Fibre backing to task system with context switching
  - Context switching from defined points and not called from outside of the
  running thread to keep the register handling simple, clean, and consistent
    - A need for context switching from outside would go in contrast to some of
    the intended principles of the task system with clean tasks run to
    completion independently, and without side effects
- First party parallel running tasks
- Proper automatic handle release, mark a flag on destruction (i.e. not some
reference counted form)
- Overhaul of the ECS system to have a proper efficient storage rather than the
current placeholder mockup
  - Will be chunked archtype unless I can narrow done how to implement a sparse
  set with "archtypes" as the set types and well handle when different archtypes
  would want the same attribute with good cache locality for all the systems
  that benefit from it
- More fleshed out physics system with parallel operation and SIMD backing
- Extend graphics support to be more general to then visualise some basic 2d physics

The engine architecture can be seen below:

![alt text](docs/design/Architecture.svg "Engine Architecture")
