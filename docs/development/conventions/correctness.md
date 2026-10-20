# Correctness

## Assertion macros

The macros in `core/assert.h` behave differently depending on whether asserts
are enabled (`TRIVIAL_ENABLE_ASSERTS`, on in the debug presets):

| Macro | Asserts on | Asserts off |
| --- | --- | --- |
| `TRIVIAL_ASSERT(expr)` | checked, fatal on failure | removed |
| `TRIVIAL_SLOW_ASSERT(expr)` | checked only with slow asserts on | removed |
| `TRIVIAL_VERIFY(expr)` | checked, fatal on failure | evaluated, unchecked |
| `TRIVIAL_ASSUME(expr)` | checked, fatal on failure | optimiser hint |
| `TRIVIAL_UNREACHABLE()` | fatal | `std::unreachable()` |

`TRIVIAL_VERIFY` is for expressions that must run either way, such as an OS
call whose result is checked in debug. `TRIVIAL_SLOW_ASSERT` is for checks too
expensive for normal debug builds, turned on by the `debug-slow` preset.

## Choosing how to guard

How an edge case is handled depends on how it can be reached:

- Edge cases that can realistically happen at runtime (overflow, running out of
memory, external data, the system clock changing) get real handling that stays
in release builds
- A caller breaking a function's contract (an out of range index, a count of
zero, a zero divisor) is a programming bug, so it gets `TRIVIAL_ASSUME`, or
`TRIVIAL_UNREACHABLE` for a case that cannot happen, rather than a silent
fallback. It is caught in debug, and in release the compiler can use it. Assume
over assert since it may allow for some compiler optimisation
- Inputs only reachable by absurd values (a timeout of centuries) get a
`TRIVIAL_ASSERT` that is removed in release, maybe this is bad practice but
usage that would result in this should be an anti-pattern. Such cases should be
marked in comments with a `NOTE:` marker at the start of the comment for easy
searching
- A precondition the engine itself guarantees, such as the capabilities being
initialised before use, gets a `TRIVIAL_ASSERT`, so breaking it is caught in
debug while hot paths pay nothing in release

## Assumes

`TRIVIAL_ASSUME` tells the optimiser the condition holds, which also lets it
remove later checks on the same values. Its condition must not contain a
function call: Clang silently drops an `[[assume]]` containing one, so
`-Werror=assume` on the engine turns that into an error. A value needing a call
is computed first:

```cpp
constexpr T kMin = std::numeric_limits<T>::min();
TRIVIAL_ASSUME(!(x == kMin && rhs.x == T{-1}));
```

Assume can result in undefined behaviour so usage is recommended to be taken
with care and have expansive testing in debug mode and care for ensuring no
breach of function contract.

## Switches

- A switch over an engine enum covers every enumerator with no `default`, so
`-Wswitch-enum` reports a missing case, with `TRIVIAL_UNREACHABLE()` after the
switch
- A switch over an integer has `default: TRIVIAL_UNREACHABLE();`, inside the
`default` so clang-tidy sees the case handled
- A switch over an external enum that can hold values unknown at compile time
(such as `VkResult` from a newer driver) returns a fallback for them instead of
treating them as unreachable. `VkResult` currently does this with a `default`
and a suppressed `-Wswitch-enum`, until every value is listed (see the
[roadmap](../../roadmap.md)). This may later get a change in style upon
addressing the first consumer properly

## Floating point comparisons

Exact `==` and `!=` on floats are only used where exactness is the meaning:
checking for zero (divisor assumes, zero length vectors), sentinel values, and
equality of the maths value types, whose defaulted `operator==` means
identical. Values that are computed and compared as being the same use an
approximate comparison (`nearlyEqual` on the maths classes, planned in the
[roadmap](../../roadmap.md)). `-Wfloat-equal` is currently off, see
[tooling](../tooling.md).

## Errors

Exceptions are disabled, so failures are returned rather than thrown, using
results written for the purpose (such as `ThreadCreateResult`) rather than a
generic result type. A failure the engine cannot continue from, such as being
unable to allocate a core service at start-up, logs a fatal message and aborts.
