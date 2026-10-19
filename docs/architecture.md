# Architecture

> **Deferred:** this doc and its diagram will be addressed upon completion of
> the window manager, when dependencies will be sorted out and all the low level
> layers in place for everything else to build on top of. As such the images of
> the architecture are out of date and may be erroneous to a minor of major
> degree.

## Changes to make

Form of the diagram will change to be more layered with usage of standard vs
dashed lines more consistent. Sections in no particular ordering and maybe
missing or not including relevant aspects to stress are listed below to serve as
a starting point in the future.

### Sections

- Overview
- Diagram
- Module layering
- Engine lifetime (start-up order and shutdown in reverse)
- Frame flow
- Threading model (main thread vs workers)
- Global services
- Compile time vs run time choices
- Not built yet (linking to the roadmap)
- Where next (getting started, subsystem docs)

### Missing from the diagram

- Memory: virtual memory, segment allocator, thread stack allocator,
capabilities
- Threading: `Thread`, ParkingLot, sync primitives
- Time: platform time, `Instant`/`Duration`
- Frame: `FrameTimer`, `FrameContext`
- Layers inside Application (game layer, debug layer)
- Engine-wide globals as their own tier (ParkingLot, TaskSystem, set up by
Engine)
- Diagnostics split into logging, asserts, profiling (Tracy) and the debug layer
- The window manager, once it replaces GLFW
