# Roadmap

## Main roadmap

- Documentation update and create new documentation

- Runtime dispatch for support for things like certain registers, instructions,
hardware support (e.g. no. hardware threads), etc.

- Memory Allocator
  - Arenas allocator
  - general allocator
    - small   <= 8 KiB   per-thread bins over 64 KiB pages, remote free lists
    - medium  <= 512 KiB TLSF over boundary tags, one pool per segment
    - large   > 512 KiB  page granular spans, multi segment runs above 2 MiB
  - Debug layer
  - Migrate heap allocation that is not tied to a data type I will later replace
    - `Engine`'s main `Thread`, `ParkingLot` and `TaskSystem`
    - `TaskPayload`'s heap stored callables
    - `HeapArray`'s storage

- Expand assertion and assume macros

- RW Lock (see about using for helping moves, e.g. custom vector)

- Proper test and benchmarking framework
  - Benchmark allocator, probably against mimalloc and jemalloc
  - Sanitiser presets alongside `debug-tsan`: UBSan (cheap, broadly useful) and
  ASan (needs poisoning annotations in the engine allocators to see inside
  them)

- First party parallel running tasks

- Proper automatic handle release, mark a flag on destruction (i.e. not some
reference counted form)
  - Add `TRIVIAL_LIFETIMEBOUND` to `Task<T>::getResult()`
  - Add `[[clang::trivial_abi]]` to `Task<T>`

- Clang thread safety analysis (`-Wthread-safety`)
  - Mark the lock types (`capability`, `scoped_lockable` on LockGuard)
  - Mark lock and unlock functions (`acquire`/`release`/`try_acquire`)
  - Mark shared members (`guarded_by`) and functions that need a lock held or
  not held (`requires`/`excludes`)
  - Opt out with `no_thread_safety_analysis` where it can't follow the
  locking (locks crossing functions, ParkingLot bucket callbacks, atomics),
  or reshape those functions so it can

- Fibre backing to task system with context switching
  - Context switching at defined points not from outside the running thread
    - Keeps it clean and consistent, and switching from outside would be more of
    an anti-pattern
  - Yield directly to the waiting thread from the custom wait primitives rather
  than a plain OS yield, on macOS by passing the target thread instead of
  `MACH_PORT_NULL` to `thread_switch` with the option unchanged
  - Own TLS and handling fibre local storage

- Async support

- Review what is exposed as public for the task system and what can moved to
internal when they ought not be touched, or put in a detail namespace

- Custom versions of stl vector, string, maps, stack, queue, set, list, and
other data structures like heaps, trees, graphs, and any others deemed relevant
  - Consider `-Wpadded` for a pass over struct layout
  - Consider `-Wunsafe-buffer-usage`
  - `TaskSystem::m_parkedWorkerIndices` as the first user of the custom stack,
  with capacity fixed at the worker count so parking never allocates

- Custom deque for the `TaskPriorityQueue` ready queue

- Extend maths to give support for custom basic algorithms like min, max, clamp

- Add some sorting functions, along with a general one (SIMD where appropriate)

- Get `TaskSlot` to 128 bytes if not already

- Overhaul of ECS -> chunked archtype unless a better solution can be found
  - Replace the `typeid`/`std::type_index` component store lookup with engine
  type ids so RTTI can be turned off (`-fno-rtti`, `/GR-`), noting that games
  deriving engine classes (e.g. `Layer`) would then need it off too
    - Keep the ids stable across builds so they can double as serialisation
    tags

- Fixed timestep simulation with an integer nanosecond accumulator and the
layer phase hooks (fixed pre/post physics, frame, present with interpolation
alpha), with input buffering once an input system exists
  - Precise sleep for frame pacing: sleep until a per-platform margin before the
  deadline then spin with the CPU pause hint, fixed margins first and adaptive
  from measured overshoot only if needed

- Wall clock (`GetSystemTimePreciseAsFileTime`/`CLOCK_REALTIME`) for log
timestamps and save dates, as its own `SystemTime` type that never mixes with
`Instant`
  - Store as `timespec`-style `int64` seconds plus nanoseconds so every time an
  OS can be set to is representable (Windows allows 1601 to 30827), rather than
  `int64` nanoseconds clamped to 1677 to 2262
  - UTC calendar conversion with Joffe's fast 64-bit date algorithm
  (benjoffe.com/fast-date-64, BSL-1.0), which needs the high half of a 64x64
  multiply (`__umulh` on MSVC is not `constexpr`, so needs a constant evaluation
  path)
  - Some 128 bit maths probably

- System sleep detection to flag time jumps rather than relying on the max delta
clamp

- Basic physics system -> profile and test stuff

- More fleshed out physics system with parallel operation and SIMD backing

- Extend graphics support
  - Load Vulkan at runtime (volk or `vkGetInstanceProcAddr`) instead of linking
  the loader, so a missing loader can fall back to another backend (e.g. Metal
  under `GraphicsApi::Auto`) rather than failing to launch, and a bundled
  loader/MoltenVK built for the macOS floor can ship in the app
  - Complete the `VkResult` name switch so `-Wswitch-enum` (C4061 on MSVC) can
  check it, with values newer than the oldest supported SDK wrapped in
  `#if VK_HEADER_VERSION >= N` guards (or a minimum version on
  `find_package(Vulkan)`), then remove the `TRIVIAL_DIAGNOSTIC_*` suppression
  around it
  - Textures
  - Text display
  - Collect drawables from the world/ECS rather than the layers
  - Separate render passes for the game and debug layers so they profile
  separately
  - Select the graphics API from the system once there is more than one
  - Physical device selection, best device by default and user switchable
  - Large asset uploads without a hard crash
  (`VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT`)

- Logging system safe under multithreading
  - Route assert output through it

- Custom window manager to replace GLFW

- Update and settle architecture shape (in particular the design image)

- Code generation flags
  - Try `o2` vs `o3`, LTO, and PGO
  - Target architecture: x86-64 level (`-march=x86-64-v2`/`v3`, `/arch:AVX2`)
  as a hardware floor, and `-mcpu=apple-m1` for macOS (safe for the 14.4 floor)
  - Standard library hardening (`_LIBCPP_HARDENING_MODE`,
  `_GLIBCXX_ASSERTIONS`, `_MSVC_STL_HARDENING`), debug mode in debug builds and
  possibly fast mode in release
  - Stack and control flow protection (`-fstack-protector-strong`,
  `-D_FORTIFY_SOURCE=3`, `-fcf-protection`/`-mbranch-protection=standard`,
  `/guard:cf`, `/sdl`), mainly worthwhile with networking or mods

- Considerations for bit identical maths
  - Floating point: `-ffp-contract=off` for identical results across ARM, x86
  and compilers (ARM fuses `a*b+c` by default, baseline x86 does not), and
  `-fno-math-errno` (Apple Clang default, GCC is not, so `sqrt` can become a
  library call on Linux), never `-ffast-math`

## Subsystem TODOs

### General

Considerations rather than planned work:

- Settle what `Engine` owns once its form is explicit
- Support more layers in `Application`, possibly as a stack -> maybe how
granting mod support
- Screen out `FrameTimer`'s raw delta in release builds

### Task system

Fibre backing may change the shape of the task system significantly, so these
may change or disappear with it.

- Profile and adjust `TRIVIAL_TASK_PAYLOAD_INLINE_STORAGE_SIZE`
- Measure `TaskState` size, padding and alignment across platforms (with custom
types ought to be same across all of them)
- Test the task system's types for cache optimisation
- Avoid the `uint32_t` casts for shifts in `TaskState`
- Dedicated worker threads (main, render, etc.) with config for them
- Thread affinities beyond main
- More non-general task paths -> check difference in compiled results with span
vs not span styles for if even needed/wanted
- More even load balancing in `tryStealTask` whilst staying lightweight, maybe
a custom hash
- Thread local completion dependant vectors to reduce allocations
- Consider reducing the scope handle size in `TaskLaunchOptions`
- `TaskSystem::m_workers` from `std::deque` to `HeapArray`, as the workers are
fixed at construction and never move

### Memory

- Tune the shifts in `memory_config.h`
- Plan migration of segment kind to operate on a higher level (i.e. the
different higher level allocators with some kind of debug layer)
- Named structs for safety in `SegmentAllocator`'s internal functions
- SIMD the cached run search once runtime dispatch exists and a workload needs
it
- Verify the Linux large page size calculation in `virtual_memory.h`

### Multithreading

- `Mutex` fairness without timers
- Move `Thread`'s stack allocator member to a global
- Surface errors from thread entry routines
- Compile time fixed thread count option, allowing inline forms such as a static
`ParkingLot`
  - A lazy initialisation singleton is not pursued, since the gain from one less
  pointer indirection is insignificant compared to the rest of the function and
  it would make the behaviour less clean and consistent, and the global will
  likely be in a lower cache level so fetching is quick anyway

### Maths

- Perpendicular functions once a direction convention is settled
- Custom function for `Vec4::length` to avoid the duplicate operations of the
nested `std::hypot` calls
- Evaluate explicit FMA usage in hot paths
- Fixed point type for bit consistent maths, wrapping numeric traits like
epsilon, infinity and max when doing so
- More SIMD backing to functions, and extend it beyond float
- More matrix functions, in particular inverse
- Review all classes for missing support (maybe more maybe less)
  - `distance` and `distanceSquared` between points
  - `reflect`, `project` and `reject` for vectors, for collision response
  - `angleBetween` for vectors, and wrapping an `Angle` into a single turn
  - `clampLength` and `moveTowards` for capping and stepping vectors
  - Component-wise versions of the scalar helpers (`abs`, `min`, `max`,
  `clamp`, `floor` etc.)
- `Mat3` class
- Maths helpers (maybe more maybe less)
  - `nearlyEqual` on the maths classes for approximate comparison
    - Turn on `-Wfloat-equal` and deal with any bits that would want
     suppression
  - `min`, `max`, `clamp` and `minMax` (`<algorithm>`)
  - `abs`, `sign`, `floor`, `ceil`, `round` and `sqrt` (`<cmath>`), as
  `constexpr` where the standard ones are not
  - `lerp` and `midpoint` (`<cmath>`, `<numeric>`), plus `inverseLerp`, `remap`
  and `smoothstep`
  - `gcd` and `lcm` (`<numeric>`)
  - Power of two checks and rounding, and alignment helpers (`<bit>`)
- Graphics related classes
- Maybe some geometry classes
- Test against glm
- Maybe batch functions as a general case for use in simulation

### ECS

- Benchmark the best entity index and generation bit split (24/8 now)

### Rendering

- Expand the physical device feature options as support grows
- Rework pipeline shader loading once async exists, possibly embedding SPIR-V
in the binary
- Toggle for the Vulkan address binding report
- Sensible defaults for the basic shapes
- Queue family selection, for things like parallel upload alongside
graphics/compute (transfer queue etc.)
- Compute only mode for headless uses of the engine
- Swapchain image count always requests `minImageCount + 1` (typically triple
buffering), expose as an explicit double/triple buffering or present mode choice
once a settings/config system can surface it
- Expose swapchain format preferences rather than hardcoding them
- Expose present style as configuration rather than the hardcoded vsync
- Depth attachment
- Shaders
- Non-primitive shapes
- Mesh handles with generations, maybe folded into the ECS once that is in place
- Consider timeline semaphores if sync complexity grows (multiple queues, async
compute, more complex frame dependencies), see
[Vulkan timeline semaphores](https://www.khronos.org/blog/vulkan-timeline-semaphores)
- Multi-threaded command recording, needing one pool per (thread x swapchain
image) pair
- Mesh handles on a GPU API switch, needing tracking through the context or
similar
- For future reference: [How to Vulkan](https://www.howtovulkan.com/)

### Platform

Likely never done, as the custom window manager replaces this code:

- GLFW init and terminate in a runtime object for multiple windows
- Event polling in a window system rather than `Window::pollEvents`
- Window hints for graphics APIs other than Vulkan
