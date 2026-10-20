# Tooling

## Overview

Compiler and clang tools are used for enforcing consistent formatting and
the identification of errors and bad practices in the C++ code. The versions of
these are pinned (except the compilers as errors should persist across versions;
maybe later the compiler versions will be enforced but no yet relevant cause of
concern has arisen) due to the divergence in raised warnings/errors and having
to address either all versions or consistently update would be to no gain and
only be an exercise in tedium. The raise in versions would be followed by
checking what NOLINTs would be obsolete and addressing newly raised issues,
which would be a notable undertaking, so a pinned version is desired.

Enforcement on style of the engine code is not limited to just clang tools and
tooling is provided for header guard style enforcement is provided via the
python build script.

Other languages are not as expansive in amounts of code so no version pinning is
done. Tooling configuration is still provided (i.e. ruff and basedpyright) that
is meant to be used for consistency and good style, but not enforced so
rigorously as these are just tools to aid in development so are not to be
consumed by an engine user, and thus by leaving them flexible usage has less
friction.

Tool configuration is at the repo root so they extend to all subdirectories.

Before a push to main, it is expected to have confirmed that no issues are
raised via any tools and all tests pass on the platforms shown in
[platform_support.md](../manual/platform_support.md).

If the compiler warnings and code generation flags are of much interest refer
directly to the engine's CMakeLists, but a brief account can be read below.

## Tool versions

- clang-format: 23.1.2
- clang-tidy: 23.1.2

## Compiler warnings

### Where they are set

Warnings are set in the block at the end of `engine/CMakeLists.txt` and are
applied to the engine only (`PRIVATE`), so tests, the sandbox and games using
the engine are not held to them. Third party code is compiled as `SYSTEM`, so
its headers do not raise warnings; on MSVC,
`/external:env:INCLUDE /external:W0` does the same for the SDK and standard
library headers.

### Policy

There is some variation across compilers, but what is mainly followed is every
flag that can be added (and was known of at the time of writing) is turned on,
provided it does not raise issues that would need widespread suppression.

### Deliberately off

- `-Wfloat-equal`: every exact float comparison in the engine is intended (zero
checks, value type equality), see [correctness](conventions/correctness.md);
turning it on with targeted suppressions is planned with the maths helpers in
the [roadmap](../roadmap.md)
- `-Wswitch-default`: enum switches have no `default` so `-Wswitch-enum` can
check them
- `-Wdocumentation`: Doxygen is not used
- `-Wcast-function-type-strict`: flags the Vulkan loader's `void(*)()` function
pointer idiom (`-Wcast-function-type-mismatch` is used instead, as
`-Wcast-function-type` implies strict on Clang)

Deferred warnings (`-Wpadded`, `-Wunsafe-buffer-usage`, `-Wthread-safety`) are
in the [roadmap](../roadmap.md).

### Warnings as errors

Warnings are not errors in normal builds, so work in progress does not have
annoying friction. They can be made errors with `TRIVIAL_WARNINGS_AS_ERRORS` in
CMake or `build.py -w`, and always are in `build.py --all` (the pre-push check).

### Suppressing a compiler warning

A warning is only suppressed when it is wrong for that spot, never to avoid
fixing the code. The macros in `compiler.h` go around the smallest possible
piece of code:

```cpp
TRIVIAL_DIAGNOSTIC_PUSH
TRIVIAL_DIAGNOSTIC_IGNORE_CLANG("-Wswitch-enum")
TRIVIAL_DIAGNOSTIC_IGNORE_GCC("-Wswitch-enum")
TRIVIAL_DIAGNOSTIC_IGNORE_MSVC(4061)
// ...
TRIVIAL_DIAGNOSTIC_POP
```

Each compiler's macro expands to nothing on the others, so the warning is named
for every compiler that raises it. If the suppression should be removed later,
it gets a roadmap entry.

## Code generation flags

The code generation flags sit in the same `engine/CMakeLists.txt` block as the
warnings. Exceptions are off (`-fno-exceptions`, `/EHs-c-`), and functions and
data are placed in their own sections (`-ffunction-sections -fdata-sections`,
`/Gy /Gw`) so games can strip unused engine code. Frame pointers are kept in
every build except Release, for profiling and stack traces, and debug builds
fill uninitialised locals with a pattern (`-ftrivial-auto-var-init=pattern`) so
reads of them show up. MSVC also gets its conformance flags: `/permissive-`,
`/Zc:__cplusplus`, `/Zc:preprocessor` and `/utf-8`.

Deferred flags (optimisation level, LTO, PGO, architecture, floating point,
hardening) are in the [roadmap](../roadmap.md).

## clang-tidy

### Configuration

`.clang-tidy` at the root enables whole check groups and turns off individual
checks where they do not fit the codebase. Naming rules are enforced through
`readability-identifier-naming`, see
[naming and formatting](conventions/naming_and_formatting.md).
`misc-include-cleaner` keeps includes exact, with a list of platform headers it
gets wrong, either by wanting the C header rather than the C++ one or by
pointing at private headers that should not be used.

### How it runs

`build.py --tidy` (and `--all`) runs clang-tidy over every engine, test and
sandbox source, plus every header on its own, using the preset's
`compile_commands.json` and failing if there is none. A diagnostic from a header
reached through several files is only reported once. Every diagnostic, warning
or error, fails the check, and the full list is written to
`build/<preset>/clang-tidy.txt`.

It is of course fine to run clang tidy in the normal fashion still, but the
build script allows better enforcement of currently active preset, which affects
some compile time branches, and enforces the tool version.

### Suppressing a clang-tidy diagnostic

Suppressions are to be explicit in why, and thus ought to mention explicitly the
name of the check being suppressed and be close to the bit being suppressed.
This is done via a preference to `// NOLINT(check-name)` on the same line when
it fits into the enforced 120 columns limit from clang-format, else
`// NOLINTNEXTLINE(check-name)` goes on the line above the offending one with
nothing else on that line. `NOLINTBEGIN`/`NOLINTEND` are therefore never used,
as suppression would not be directly clear.

### Coverage

clang-tidy only analyses the `#if` branches active for the preset being built,
so code for other platforms, or for options no preset turns on, gets no checks
at all, not even compile errors. Building a new platform or option for the first
time can therefore surface warnings never seen before; see [testing](testing.md)
for what no preset covers. This is one reason to prefer `if constexpr` over
`#if` where there is the option.

## clang-format

`.clang-format` at the root is LLVM based, with tabs, 120 columns and attached
braces. Includes are ordered by category: standard and other libraries, GLFW,
Vulkan, Tracy, googletest, Trivial public headers, then private `"..."`
headers. Unlike clang-tidy, it formats every `#if` branch. `build.py -f`
formats in place and `--format-check` only reports.

One known quirk: put a trailing comma only on the outermost initialiser list,
as an inner one makes clang-format skip the whole statement. This must be
therefore formatted by hand. Do so by trying each set of braces with the desired
trailing comma and format both to see the appropriate brace positions and number
of spaces before.

For full formatting rules refer to the clang-format file.

## clangd

`.clangd` at the root configures the editor, with strict missing and unused
include diagnostics to match `misc-include-cleaner`, and the same list of
ignored platform headers as `.clang-tidy`. It uses the root
`compile_commands.json`, which `build.py` points at the last preset built. The
`debug-deps` preset adds third party code to `compile_commands.json`, for
jumping into dependency sources.

## Include guards

Include guards are checked by `build.py` alongside formatting; the naming rule
is in [files and headers](conventions/files_and_headers.md).

## Python

`scripts/build.py` is the only Python, with no dependencies. ruff (lint and
format, 120 columns) and basedpyright (strict) settings are in `pyproject.toml`
for editors, but are not enforced by any check.

## Other

`.yamlfmt` formats YAML files, and the hidden `vs-analysis` preset turns on
clang-tidy in Visual Studio's code analysis.
