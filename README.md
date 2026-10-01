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
software built with the engine can use any licence, but must keep the
[NOTICE.md](NOTICE.md) file's attribution somewhere a user could reasonably
find it (e.g. a credits screen, documentation, or an in-app NOTICE display).
NOTICE.md also lists the engine's third-party dependencies and their licence
terms, and credits design references where deemed appropriate. A note on this
is it need not be a title screen as that can often interfere with the feel of a
game, and as such implementation of accreditation is deferred to the user.

Some documentation can be found in this repository, this documentation is not
complete, due to some bits with work ongoing and without a proper release
schedule keeping to having full documentation for each component added that may
likely change later. It is a personal project so a great formality of this is
neglected prior to a proper version 1.0.0.

## Current plan of action

- Windows basic testing
- Custom Chrono with suitable types
- Documentation update and create new documentation
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
- First party parallel running tasks
- Proper automatic handle release, mark a flag on destruction (i.e. not some
reference counted form)
  - `lifetimebound` on `Task<T>::getResult()`, the result then dies with the
  handle so `launch(work).getResult()` held past the statement dangles
  - `[[clang::trivial_abi]]` on `Task<T>` so the destructor does not force it
  out of registers, keep it move-only with a moved-from handle invalid so only
  one owner sets the flag
- Clang thread safety analysis (`-Wthread-safety`), macros expand to nothing on
other compilers
  - `capability` on Mutex/SpinLock/EscalatingLock, `scoped_lockable` on
  LockGuard
  - `acquire_capability`/`release_capability`/`try_acquire_capability` on the
  lock and unlock functions
  - `guarded_by`/`pt_guarded_by` on shared members, `requires_capability` and
  `excludes` on functions that expect a lock to be held or not held
  - Limitations: no tracking of locks taken in one function and released in
  another, locking via callbacks (ParkingLot buckets), lock ordering across
  objects, or atomics, such spots need `no_thread_safety_analysis` -> see if
  some functions want adjustment to employ this effectively
- Fibre backing to task system with context switching
  - Context switching from defined points and not called from outside of the
  running thread to keep the register handling simple, clean, and consistent
    - A need for context switching from outside would go in contrast to some of
    the intended principles of the task system with clean tasks run to
    completion independently, and without side effects
- Async support
- Overhaul of the ECS system to have a proper efficient storage rather than the
current placeholder mockup
  - Will be chunked archtype unless I can narrow done how to implement a sparse
  set with "archtypes" as the set types and well handle when different archtypes
  would want the same attribute with good cache locality for all the systems
  that benefit from it
- More fleshed out physics system with parallel operation and SIMD backing
- Extend graphics support

The engine architecture can be seen below:

![alt text](docs/design/Architecture.svg "Engine Architecture")
