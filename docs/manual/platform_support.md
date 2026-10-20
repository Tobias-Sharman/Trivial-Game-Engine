# Platform Support

## Targets

Only 64-bit devices are supported. 32-bit devices are not common enough to merit
setting up another set of test target (an additional nice benefit is that it
makes implementation of anything that is pointer based need no extra
consideration for how different sizes would fit wrt to packing, alignment, cache
line sharing issues, etc.).

Testing is done on the below listed platforms and architectures before pushing
to main. It is expected the untested platforms would function, but it is simply
worth noting that they are untested before engine use. One notable possible
issue may be assumed memory sizes taken at compile time on windows arm and
emulated platforms like Rosetta.

|                  | x86-64   | ARM64    |
| ---------------- | -------- | -------- |
| Windows 10 1803+ | Tested   | Untested |
| macOS 14.4+      | Untested | Tested   |
| Linux            | Tested   | Tested   |

Linux testing has not been done on older libc versions, but no linker issues are
expected for older Linux versions - provided they are above the floor mentioned
below.

Any other operating system (including Apple platforms other than macOS - these
apple platforms will be extended in the future but adding another test target
would harm early development so they are neglected) or CPU architecture fails at
compile time.

Large page versions have also not been tested.

## Minimum versions

The minimum platform support and the reasons behind. Full historic platform
support is neglected to not open more test targets and allow for code to have a
cleaner implementation.

The below listed will not have an entire list out of all OS API usage. If there
is something not noted that lowers the floor please mention it.

- Windows 10 1803: `VirtualAlloc2`, `CREATE_WAITABLE_TIMER_HIGH_RESOLUTION`
  - Also used: `SetThreadDescription` (1607), `WaitOnAddress` (Windows 8)
- macOS 14.4: `os_sync_wait_on_address`
- Linux 4.5: `MADV_FREE`, only needed with lazy decommit (opt-in, checked at
start-up in debug builds)
  - Without lazy decommit the next highest are `MADV_HUGEPAGE` (2.6.38, large
  pages only) and private futexes (2.6.22)
  - glibc: `pthread_setname_np` (2.12)

The Windows and macOS floors are enforced at compile time: CMake sets
`_WIN32_WINNT`, `NTDDI_VERSION` and the macOS deployment target, and the build
fails if the target is below them. Windows also links `Synchronization` and
`onecore`.

## Compilers

Clang, clang-cl, GCC and MSVC are supported. No compiler version is enforced, as
the engine is meant to be portable across toolchains.

No issues were found with recent compiler versions, but it is worth a note that
these are not something that are readily tested so it is possible some issues
might arise and if so mention it as maybe it is something that can be cleanly
addressed. Main engine testing is just using clang and MSVC.

## Graphics and windowing

- Vulkan, through MoltenVK on macOS
- GLFW for windowing

Support requirements will be fleshed out with the graphics overhaul and the
creation of the window manager.

## Platform differences

### Memory

- Page size: the smallest unit memory is committed, freed or protected in (thus
also the size of the guard page)
  - Windows: 4 KiB
  - Linux: 4 KiB on x86-64, not known at compile time on ARM64, as kernels can
  use 4, 16 or 64 KiB pages, so it is queried at start-up
  - macOS: 16 KiB on ARM64, so small commits and guards cost four times what
  they do with 4 KiB pages, and 4 KiB on x86-64
- Allocation granularity: the step address space is reserved in
  - Windows: 64 KiB, so reservations are rounded and aligned to 64 KiB rather
  than to a page
  - Linux and macOS: the page size, so reservations only need page alignment
- Large pages
  - Windows: only available when the user has been granted the "Lock pages in
  memory" privilege, otherwise left off, and large page memory is locked into
  RAM rather than pageable
  - Linux: a hint (transparent huge pages) the kernel may or may not act on
  - macOS: not available
- Decommit: memory handed back to the OS stops counting towards the process
straight away and reads as zero when used again, and on every platform it is
made inaccessible until recommitted, so a stray use faults
  - Windows: a true decommit, the physical memory and its commit charge are
  released, so it no longer counts against the system commit limit
  - Linux: by default memory is only backed once touched, with no up front
  reservation to give back, so decommitting drops the physical pages, lowering
  the resident size
  - macOS: the usual Linux approach does not reliably release pages, so a
  macOS specific one is used that also removes them from the memory footprint
  shown in Activity Monitor, with recommitting adding them back
- Lazy decommit (opt-in): the OS only takes the memory back under memory
pressure, so reusing it soon after skips the cost of faulting it back in
  - Until reclaimed it still counts towards the process in OS tools, so they
  report more than the engine's own counters
  - Contents may be stale or zero when reused
  - It is notable this is opt in as tracked values is weighed as more important,
  especially in conjunction with the delayed decommit enforced by the allocator
  itself (holds committed memory for a period of time before decommitting to
  reduce needless OS calls when that memory is going to be wanted again soon),
  so there is a pseudo lazy decommit enforced by the engine that would represent
  a more consistent lazy decommit that is reflected nicely in monitoring
  software
  - Windows: the memory stays committed, so the commit charge is kept until the
  OS reclaims it, and a normal decommit is used if the reset fails
  - Linux: needs kernel 4.5 or newer
  - macOS: the same behaviour as Linux

### CPU

- Cache line size: 128 bytes on Apple silicon, 64 bytes elsewhere, so padding
for hot data, and the size of types padded to a cache line, differ between
platforms
- False sharing alignment: 128 bytes everywhere, covering x86 prefetching pairs
of 64 byte lines, so data padded to stop threads contending is the same size on
every platform

For development only false sharing is really taken into consideration.

### Threads

- Priority: each platform has its own model, so `ThreadConfig` has per-platform
fields rather than one shared priority
  - Windows: fixed priority levels
  - Linux: scheduling policies, where the real-time ones usually need extra
  privileges and normal threads share one level
  - macOS: QoS classes, which also steer threads between performance and
  efficiency cores
- CPU affinity: Windows and Linux can pin threads to cores, Windows in processor
groups of up to 64 cores; macOS cannot, so there are no affinity settings there
- Yield: macOS depresses the yielding thread's priority for 1 ms (rather than
the ~10 ms of a plain `sched_yield`), and Windows uses `Sleep(0)` by default
or `SwitchToThread` if configured

### Time

- Sleeping until a deadline: Linux and macOS wait on the deadline directly,
while Windows sleeps for the remaining time and re-checks until the deadline
passes
- Sleep precision: Windows relies on the high resolution waitable timers from
1803, without which it would be limited to the ~15.6 ms system timer
