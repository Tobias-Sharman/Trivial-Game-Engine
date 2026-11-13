# Testing

## Overview

The tests use googletest and build into a single `trivial_tests` binary from
`tests/`. They are run through `build.py -t` (and as part of `--all`), see
[building](../manual/building.md) for the options and where reports go.

## What to test

Tests act on the API and check that it works as intended, whether that is at
compile time or at runtime. Properties of a type itself (size, alignment, being
trivially copyable, what it can and cannot be constructed or converted from)
belong as `static_assert`s in the type's header, so they hold on every compiler
whether or not tests are built. Tests check behaviour: runtime `EXPECT_*`
results, and `static_assert`s on `constexpr` results such as
`milliseconds(5).count() == 5'000'000`.

Internal types are tested through the public type built on them rather than on
their own, so `Parker` and `ParkingLot` are covered by the `Mutex`,
`ConditionVariable` and other synchronisation tests. This is to keep testing
coverage expansive to all points of failure, whilst not needing tedious tests
on internal private implementation that cannot be interacted with anyway. Some
internal implementation may be deemed worth direct testing and thus may still
have tests. This is an exception to the
standard style and upon creation of a public consumer there would be an argument
to removal for consistency, but there is no real benefit to drop those tests
that were used to confirm intermediary steps of development.

## Test build

The tests are built with the user config header in
`tests/config/trivial_user_config.h`, set by the root `CMakeLists.txt` through
`TRIVIAL_USER_CONFIG`. It currently sets nothing, so the user config path is
still built while the engine, tests and sandbox match an engine used without
one.

The test `main` initialises the memory, CPU and time capabilities itself, in the
same order as the `Engine` constructor, as tests do not create an `Engine`. It
also logs the system info at start-up (outside release) and frees the CPU
capabilities once the tests finish.

## Isolated tests

Tests that create threads or a `ParkingLot` change global state (the thread
index counter and the active parking lot), so each must run in its own process.
`build.py` runs every test named in its `ISOLATED_TESTS` list separately, and
everything else together in one process.

A new test that creates threads or a parking lot must be added to
`ISOLATED_TESTS`. `requireIsolatedProcess()` in `tests/support/helpers.h` aborts
if such a test runs alongside others, which is what happens when the test binary
is run directly. `build.py` also fails a name in the list that matches no test,
so renamed or deleted tests are caught.

### Support helpers

`tests/support/helpers.h` provides what threaded tests need:

- `ScopedParkingLot`: installs a parking lot for the test and removes it after
- `runOnAllThreads`: runs a body on several threads (to be removed when first
party parallel for is added - see [roadmap](../roadmap.md)
- `requireIsolatedProcess`: aborts unless the test runs on its own

A blocking call (`wait`, `acquire`, `waitFor`) made from the test's own thread
needs that thread adopted as a `Thread` first, or it crashes rather than
failing, so use something like:

```cpp
trivial::thread::Thread mainThread;
mainThread.adoptCurrentThread({
    .name = "Test Main",
    .type = trivial::thread::ThreadType::Main,
});
```

Threads the test spawns do not need this as they will be created through the
custom thread type anyway, and thus cover this case.

## Config dependent tests

A test that depends on a compile time feature stays defined in every build,
with the `#if` inside its body and `GTEST_SKIP()` explaining why in the `#else`
branch. Wrapping the whole test in `#if` would make it disappear in some builds,
which `build.py` cannot tell apart from a stale name in `ISOLATED_TESTS`.

```cpp
TEST_F(ExampleTest, DecommitReturnsMemory) {
#if TRIVIAL_MEMORY_ENABLE_DECOMMIT
	// ...
#else
	GTEST_SKIP() << "Decommit is off in this configuration";
#endif // TRIVIAL_MEMORY_ENABLE_DECOMMIT
}
```

Expected skips:

- `TaskSystemMultiThreadTest.TasksDistributeAcrossWorkers` on a single core
machine
- `CpuTest.CountsAreConsistent` on Linux, until its topology is probed

Skipped tests count towards the passed total, and `build.py` shows how many
were skipped.

## Untested paths

The presets cover each option being on and off somewhere, but some options and
combinations are not built by any preset, so they have never been compiled or
tested:

- Lazy decommit (`TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT`)
- SIMD off engine wide (`TRIVIAL_ENABLE_SIMD=OFF`), as opposed to only the maths
backend
- `SwitchToThread` as the Windows yield
(`TRIVIAL_THREAD_WINDOWS_YIELD_USE_SLEEP_ZERO=0`)
- Decommit off (`TRIVIAL_MEMORY_DECOMMIT=OFF`), as every preset keeps it on
- Tunable values (purge delay and generations, spin counts, shifts and so on)
other than their defaults
- Tracy or memory debugging off in an optimised build

Platform specific code is only covered on that platform's run, and
ThreadSanitizer does not exist on MSVC, so the Windows `debug-tsan` build only
checks FMA off. As clang-tidy only checks the active `#if`
branches, see [tooling](tooling.md), building one of these for the first time
may also raise new warnings - if so please do mention this.
