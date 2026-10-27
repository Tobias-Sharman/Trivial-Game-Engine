# Time

This covers the clock the engine reads and what is probed for it at start-up.
It is the platform clock only: engine and world time (fixed timesteps, pausing,
scaling) are a separate concern built on top of it, see the
[roadmap](../../roadmap.md).

## The clock

`time::now()` reads a monotonic clock and converts its ticks to nanoseconds:

- Windows: `QueryPerformanceCounter`, in ticks of the performance counter
frequency
- macOS: `mach_absolute_time`, in timebase units
- Linux: `clock_gettime(CLOCK_MONOTONIC)`, already in nanoseconds

Keeping to nanoseconds is for consistency and any usage is likely to be based
around wanting real world timings so such storage allows for faster access and
conversion. A technically marginally more accurate clock for mac and windows is
thus neglected here as any benefit would be heavily outweighed by the cost. If
needing accuracy to the tick level then more direct reads as explained below
ought be used.

## Probed at start-up

Time capabilities are probed at start-up and stored in
`trivial::time::detail::g_detectedCapabilities`:

- Windows: the performance counter frequency (`QueryPerformanceFrequency`), as
`ticksPerSecond()`
- macOS: the timebase numerator and denominator (`mach_timebase_info`), and the
ticks per second they give
- Linux: nothing, as the clock is in nanoseconds, so the global does not exist
and `ticksPerSecond()` is a constant

The frequency is fixed for the life of the machine, so it is stored rather than
queried on every call.

## Conversion

Converting ticks to nanoseconds is the hot part of every clock read, so the
common frequencies take a path with their ratio known at compile time, and
anything else falls back to a general scaling that cannot overflow:

- Windows: 10 MHz is the usual frequency on x86-64 and 24 MHz on ARM64, so
each architecture checks its own first
- macOS: Apple silicon's 24 MHz timebase, and Intel Macs' timebase of 1 (ticks
are already nanoseconds)

## CPU timestamp counters

The OS clocks above are built on counters the CPU itself provides, which a
profiler would read directly instead:

- x86-64: the timestamp counter, read with `RDTSC` or `RDTSCP`
- ARM64: the generic timer's virtual count, `CNTVCT_EL0`

The CPU module reports what makes a direct read sound:

- `X86_64Feature::InvariantTsc`: the counter ticks at a constant rate through
frequency changes and sleep states. Without it the counter is not usable as a
clock. Its rate is not reliably reported, so it has to be calibrated against an
OS clock
- `X86_64Feature::Rdtscp`: `RDTSCP` waits for earlier instructions to finish
before reading, so a measurement is not skewed by reordering
- ARM64's counter always runs at a constant rate, given by `CNTFRQ_EL0`, and the
OS decides whether user code may read it. A read can be reordered unless an
`ISB` comes before it; `Arm64Feature::Ecv` adds self synchronising reads
(`CNTVCTSS_EL0`) that need no barrier
