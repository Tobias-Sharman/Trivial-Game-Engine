# Building

## Getting the source

```bash
git clone https://github.com/Tobias-Sharman/Trivial-Game-Engine.git
```

## Building

The engine can be built via CMake in standard style:

```bash
cmake --preset <preset>
cmake --build --preset <preset>
```

but, due to some consequences of the global state, some tests need to be ran in
their own processes, so a build script was made (explained below) which can also
be used to build the project.

## Requirements

- CMake 3.30 or newer
- Ninja, used by every preset
- A C++23 compiler, see [platform support](platform_support.md)
- The Vulkan SDK, found by CMake with `find_package(Vulkan)`
- Python 3.14 or newer for `scripts/build.py`, which has no other dependencies
- clang-format and clang-tidy 23.1.2, only needed for the format and tidy
checks, and required at exactly that version so results match

## Dependencies

Fetched automatically by CMake at configure time:

- GLFW 3.5.1
- VulkanMemoryAllocator 3.4.0
- Tracy 0.13.0
- googletest 1.18.0, for the tests only

## Presets

- `debug`: asserts, logging, Vulkan validation and Tracy
- `debug-slow`: `debug` plus slow asserts and the scalar maths path
- `debug-tsan`: `debug` with ThreadSanitizer, with Tracy, FMA and decommit off
- `release`: no asserts, logging, validation or Tracy
- `relwithdebinfo`: optimised with logging and Tracy, no asserts or validation
- `debug-deps`: `debug` with third party code in `compile_commands.json`, for
tooling only

## Build script

`scripts/build.py` configures, builds and runs the checks for a preset. Allows
for easy building, test running, and tooling checks (clang-format & tidy).

### Options

- `-p`, `--preset`: the preset to use (default `debug`)
- `-j`, `--jobs`: parallel compile and clang-tidy jobs (default all logical
cores)
- `-w`, `--warnings-as-errors`: build with `TRIVIAL_WARNINGS_AS_ERRORS=ON`
- `-t`, `--test`: run the tests after building
- `-n`, `--repeat`: run the tests this many times, for catching flaky tests
- `-r`, `--run-sandbox`: run the sandbox after building
- `-f`, `--format`: apply clang-format and check include guards before building
- `--format-check`: report unformatted files and wrong include guards without
changing anything
- `--tidy`: run clang-tidy over all sources after building
- `-a`, `--all`: the full check, see below

### Order of steps

- Format and include guard checks, if asked for
- Configure and build, stopping the run if the build fails
- Point the root `compile_commands.json` at the preset's build
- clang-tidy, tests and sandbox, if asked for, each reporting its own result

### The full check (`--all`)

- Format and include guard checks once
- Then for every preset except `debug-deps`: a fresh build with warnings as
errors, the tests, a 5 second sandbox run and clang-tidy
- Ends with a table of results per check and per preset
- Run before pushing to main

### Output

- `build/<preset>/`: the build itself
- `build/<preset>/clang-tidy.txt`: every clang-tidy diagnostic, with
duplicates from shared headers removed
- `build/<preset>/test-results/latest/`: the test reports from the last run
- `build/test-failures/<preset>/<timestamp>/`: reports kept from failed runs,
left until deleted by hand
- `build/all-presets/<preset>.log`: build and sandbox output from `--all`
- Coloured output in a terminal, turned off with `NO_COLOR` or forced with
`FORCE_COLOR`
