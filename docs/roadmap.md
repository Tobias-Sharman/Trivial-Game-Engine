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
  - Ensure support for shadow stacks on fibre switching when enabled

- Async support

- File system abstraction

- Linux side of CPU probing is in and then a full review of the CPU probing
subsystem to really clean up the currently at the very least mildly horrendous
logic. Current implementation may well be functional but is painful to work
through so will need adjustment
  - The dropped feature warnings list each feature with its missing
  prerequisites, which is a bit imperfect, so as part of the overhaul list out
  all of the missing features arising from each prerequisite instead

- Review what is exposed as public for the task system and what can moved to
internal when they ought not be touched, or put in a detail namespace

- Custom versions of stl vector, string, maps, stack, queue, set, list, and
other data structures like heaps, trees, graphs, and any others deemed relevant
  - Inline string and inline vector
  - Consider `-Wpadded` for a pass over struct layout
  - Consider `-Wunsafe-buffer-usage`
  - Try CRC32 (SSE4.2, ARM CRC32) for hashing keys in the custom maps
  - `TaskSystem::m_parkedWorkerIndices` as the first user of the custom stack,
  with capacity fixed at the worker count so parking never allocates
  - Enum utilities for the enums to make the loops cleaner and not have so many
  static casts
    - Add a note to style about how count should be split from enum values with
    a newline

- Custom deque for the `TaskPriorityQueue` ready queue

- Extend maths to give support for custom basic algorithms like min, max, clamp,
round -> keeping to constexpr where possible with possible branching on a
version for runtime that exploits CPU capabilities

- Reduction in casts around promotion and demotion by providing functions that
give the desired type returns with the maths helpers

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
  - Pop up a dialogue if it is seen that the system does not meet the
  requirements to run the application
  - Pretty formatting for the system info logs once proper helpers exist (e.g.
  boxes for CPU topology diagrams instead of the plain indented lines)
  - Move the system info logs off `std::string` to the custom small string so
  they avoid needless early heap allocation
  - Change up the dependency ties: `log.cpp` pulls in the CPU module for the
  system info logs while the CPU module logs through `log.h`, so the two depend
  on each other
    - Probably move to something builds up a string that log then just logs and
    handles line lengths

- Custom window manager to replace GLFW

- Update and settle architecture shape (in particular the design image)

- Code generation flags
  - Try `o2` vs `o3`, LTO, and PGO
  - Target architecture: x86-64 version (`-march=x86-64-v2`/`v3`, `/arch:AVX2`)
  as a hardware floor, and `-mcpu=apple-m1` for macOS (safe for the 14.4 floor)
  - x86 v1 support based on compile time branching
  - ARM v8.2 extended version, if profiling shows an issue
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
- change the thread concurrency default from
`std::thread::hardware_concurrency()` to the CPU module's counts (available
and quota). With that change also consider making `Thread::id()` a
`std::size_t`, as it no longer has to match `hardware_concurrency()` and is used
as a `std::size_t` subscript
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
- Pointer arithmetic functions so there are not so many NOLINTs (about 80 for
`pro-bounds-pointer-arithmetic`, `pro-type-reinterpret-cast` and
`no-int-to-ptr`), each holding its one suppression:
  - Byte offsets (`static_cast<char*>(p) + n` and `- n`): `SegmentAllocator`,
  the virtual memory and segment metadata files, `ThreadStackAllocator`
  - Pointer to address (`reinterpret_cast<std::uintptr_t>`) for alignment
  checks and masking: the virtual memory files, `SegmentAllocator`,
  `ParkingLot`'s key, `EscalatingLock`
  - Address back to pointer: `EscalatingLock`'s queue head
  - Aligning a pointer down to its segment: `segment_allocator.h`
  - Typed views over raw memory (`reinterpret_cast<T*>(cursor)`):
  `SegmentAllocator`'s bitmaps and records, `TaskSlot` and `TaskPayload`
  storage
  - Indexing raw pointer arrays, which could become spans instead:
  `SegmentAllocator`'s bitmaps and records
- SIMD the cached run search once runtime dispatch exists and a workload needs
it
- Verify the Linux large page size calculation in `virtual_memory.h`
- Plain reserve function in `virtual_memory.h` for page aligned requests (such
as the CPU topology tables), so they skip `reserveAligned`'s over reserve and
trim
- Get rid of the `MADV_FREE` probe, have it error, or decide on keeping it and
lowering the Linux floor
- Expand the memory probe
- Group engine specific allocations for memory efficiency, especially what is
meant to be read only (made to error on write), consuming the metadata and the
like for engine components such as the CPU topology and the allocators' memory
related data

### CPU

The below is mostly ai generated and summarised notes, which is why it is so
dense. This will not really be touched until Linux support is filled in.

- Possibly probe the SVE and SME vector lengths in the future as needed
- Clean up the prerequisite closure `static_assert`s in `cpu_probe_arm64.h` and
`cpu_probe_x86_64.cpp` to only visit each feature's prerequisites rather than
every pair of features
- Clean up the form of the CPU detected capabilities struct
- Address the CPU vendor and brand string sizing (x86 has spec sizes of 13 and
48, ARM has no spec and the longest name seen is 60 characters)
- Replace the macos name creation with an inline string
- Update macos logs to say the efficiency class in the error logs
- Could reduce some macOS startup by limiting the features to what is in the mac
SDK (the cases in `sysctlSource` with no SDK capability bit each read a named
sysctl)
- Linux CPU topology probe: as it is opening and reading sysfs files, do it as
part of the section making the file platform abstract interface
- Linux cgroup CPU quota (`cpu.max`, and v1 `cfs_quota_us`/`cfs_period_us`)
for `quotaLogicalProcessorCount`, also file reads so with the file interface
- Linux available count: read `sched_getaffinity` once at start-up, before any
thread is pinned, as it is per thread and so wrong from a pinned thread. With
the file interface also read `cpuset.cpus.effective` as the hard limit, which
can be read again after pinning and sees containers changed at runtime
- Linux ARM64 brand string, which the kernel only exposes through files
- Linux running translated: there is no OS report, so detect it from CPUID in
the x86 probe. FEX returns `FEXIFEXIEMU` from hypervisor leaf 4000_0000h and the
host architecture in leaf 4000_0001h EAX[3:0] (2 is AArch64), and Rosetta
reports a `VirtualApple` brand (seen under macOS, unverified for Linux VMs).
Box64 and qemu-user have no known signature. qemu-user also runs ARM64 builds
on x86-64 hosts (Docker multi-architecture builds), which the ARM64 probe would
need its own way to spot
- Windows ARM64 features without a `PF_ARM_*` value: the registry holds per
processor copies of the ID registers next to `CP 4000` (`CP 4020` PFR0 to
`CP 403A` MMFR2, plus ZFR0 and ISAR2 on some machines, as zig and FEX read).
They are raw hardware values, not what the OS enabled, so they can only fill in
features with no register state (RDM, FHM, FCMA, FlagM, SB, SM3/SM4, RNG, LRCPC2
and 3, LSE128, CSSC, HBC, LUT, FAMINMAX and similar), intersected across every
processor. Check which values exist on real machines first
- Windows ARM64 FP8: FPMR and the Advanced SIMD FP8 features have no
`PF_ARM_*` value, so FPMR is only inferred from the SME FP8 features and plain
FP8 cannot be reported yet
- Verify the Windows registry processor numbering matches the processor group
order on machines with more than 64 logical processors
- Windows topology `assignTopologyId` checks every logical processor against
every package, NUMA node and cache record (about 3P² checks for P logical
processors), which could be made linear with a lookup from processor number to
logical processor position, walking each record's mask bits instead
- Add AMX support (Xeon only, Linux needs `ARCH_REQ_XCOMP_PERM` before first
use, which grows every signal frame by the tile state)
- AVX-512 warm up at startup on macOS (one EVEX instruction then `VZEROUPPER`
in the CPU init) so threads start promoted instead of each taking one trap on
first use, possibly with a compile time gate on whether it is warmed up
- AVX-512/AVX10 code paths for batch work where profiling shows a benefit
(culling, bitmap scans, compaction), keeping AVX2 as the main path since
consoles are Zen 2
- Tests that execute an instruction from each detected CPU feature
- AVX10 discrete feature bits (CPUID leaf 24H subleaf 1): the AVX10.2 spec names
`AVX10_V1_AUX` at ECX bit 2, LLVM reads bit 3 as `avx10v2aux` and the SDM lists
none yet, add once settled
- Some gatings could be put in to reduce compiled size in non checking builds
(with the floor check off, the target feature names and check code are unused)
- Detect whether CET shadow stacks (x86-64) or GCS (ARM64) are on, for the
fibre shadow stack handling: a per OS query (Linux `arch_prctl`, Windows'
process mitigation policy), as it is per thread state, not a feature
- Features left out for having no use in the engine yet (listed in the CPU
docs): AMX, Xeon Phi AVX-512 and ring 3 MWAIT, Key Locker, LWP, FSGSBASE, SGX,
ENQCMD, PTWRITE, RDPRU, the ARM PAuth variants, CPA, the Spectre properties,
MTE4 and its tag modes, DGH and LS64. Add any once a use comes up
- Windows CPU quota in nested jobs: only the process's own job is readable, so
a CPU rate set on a parent job is missed and the quota can be too high. There is
no API to reach the parent; measuring CPU time at startup is the only fallback
- Windows 10 with more than 64 logical processors: threads start in the
process's one processor group, so thread placement has to spread them across
groups with `SetThreadGroupAffinity` to use the whole available count
- Verify the Windows available count on a machine with more than 64 logical
processors, inside and outside a job with group affinity, and with groups of
unequal size (LLVM issue 56618 saw spurious process masks there)
- Replace the `std::vector`s in the available count (the Windows job groups
and the Linux affinity mask) with the small vector, so the common case does not
heap allocate outside the engine allocators on every call
- Graceful handling of VMs and emulators that report CPUID features they cannot
run: execute each detected feature once at startup under a fault handler
(SIGILL handler, SEH on Windows) and drop any that fault, as currently a false
report crashes on first use of that feature
- Own CPUID inline assembly, only if a toolchain lacks `<cpuid.h>`/`__cpuidex`
or clang's `cpuid.h` clashes with MSVC's `intrin.h` under clang-cl. Not needed
now as `__cpuid_count` already passes the subleaf in ECX (mimalloc's own
assembly works around `__get_cpuid`, which takes no subleaf)

### Multithreading

- Get commit hash not dates read for parking lot
- `Mutex` fairness without timers
- Power saving spin waits with timed low power waits (`TPAUSE`/`UMWAIT` on
Intel, `MONITORX`/`MWAITX` on AMD, `WFET` on ARM), through runtime dispatch
- Uneven work splitting on hybrid CPUs, by measuring the relative speed of each
efficiency class (timing a fixed workload at startup or per class job timings)
rather than relying on an OS capacity value, as only Linux exposes one
- Use the inline vector for `ParkingLot::unparkAll`'s wake batch instead of the
fixed batch with extra waiters woken under the bucket lock
  - Template option for the inline vector's allocation to be direct OS calls
  for a page rather than the engine allocators (allocating through those under
  a bucket lock can park on the `ParkingLot` itself)
    - Or maybe change segment allocator lock not held to parking lot, e.g. spin
    lock
  - Still keep handling for failed allocation (wake under the lock)
  - It should be noted that this should only be reviewed properly once the fibre
  approach is implemented due to the impact that might have on locks
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
- Clean up the SIMD maths backing for a clearer split of determinism vs fast
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
  - Bit functions (`popcount`, `countrZero`, `countlZero`, `bitCeil`,
  `bitWidth`) returning desired types so call sites lose their `static_cast`s
  - Bit maths helpers that do as much as they can at compile time and are
  templated over the unsigned integer sizes (8, 16, 32 and 64 bit) so the same
  call works on any register or packed field:
    - `extractBits(value, highBit, lowBit)` and `insertBits(value, field,
    highBit, lowBit)` for reading and writing a packed field, e.g. the task
    state's packed status, priority, affinity and lifetime
    - `bitMask(width)` and `bitRangeMask(highBit, lowBit)`, valid up to the full
    width
    - `isBitSet`, `setBit`, `clearBit` and `toggleBit` on a value, plus bitmap
    versions over a span of words, replacing `SegmentAllocator`'s own bitmap
    functions, `FeatureSet`'s word masks, the CPUID bit tests in
    `cpu_probe_x86_64.cpp` and the affinity bit tests in `thread_linux.cpp` and
    `cpu_probe_windows.cpp`
    - `lowestSetBit` (`x & -x`) and `clearLowestSetBit` (`x & (x - 1)`), and
    iterating the set bits of a mask, e.g. walking affinity and group masks
    - `countlOne`, `countrOne`, `rotl`, `rotr` and `byteswap` (`<bit>`)
    - `signExtend(value, bits)` for signed packed fields
    - `reverseBits` and `parity`, if a use comes up
    - Morton encoding (interleaving the bits of two or three coordinates), for
    spatial hashing and cache friendly grid layouts
  - Ceiling integer division and rounding up to a multiple
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
