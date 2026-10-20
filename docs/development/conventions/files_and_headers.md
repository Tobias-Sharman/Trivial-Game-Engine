# Files and Headers

## Layout

Public headers live in `engine/include/trivial/<module>/` and are included as
`<trivial/...>`. Private headers and sources live in `engine/src/<module>/` and
are included as `"<module>/..."`. Anything a game should not see belongs in
`engine/src/`.

If something is not scoped to a subsystem it will be placed under the `core`
directory.

Each header is paired with at most one source file of the same name, and that
source only defines what its own header declares. The source includes its own
header first, which clang-format relies on to keep it at the top. A header
whose internal macros are also needed by its source `#undef`s them unless
`TRIVIAL_<...>_IMPLEMENTATION` is defined, which the source does before
including it.

## Include guards

Every header uses an include guard rather than `#pragma once`, named from its
path: `TRIVIAL_<path>` for public headers and `TRIVIAL_SRC_<path>` for private
ones. The `#endif` repeats the name as a comment. This is done more out of
personal preference in style than out of portability concerns.

```cpp
#ifndef TRIVIAL_CORE_MATH_VEC2_H
#define TRIVIAL_CORE_MATH_VEC2_H

// ...

#endif // TRIVIAL_CORE_MATH_VEC2_H
```

`build.py` checks every guard alongside formatting, see
[tooling](../tooling.md).

## Where code goes

Code stays in the file that needs it, including helpers and OS includes. It
only moves into a shared header once a second file actually needs it, rather
than adding a layer for a single user. Another reason for splitting up a file is
if a file gets so large it becomes hard to easily navigate.

Runtime config structs live in the header of the type that consumes them.
Compile time tunables live in the module's `_config.h`, see
[configuration](../../manual/configuration.md).

Properties of a type itself (size, alignment, being trivially copyable, what it
can and cannot be constructed or converted from) are `static_assert`ed in its
header, straight after the type, so they hold whether or not tests are built.
Checks of behaviour belong in the tests, see [testing](../testing.md).

## Dependencies between headers

Header layering problems are never solved with forward declarations or circular
includes. When several layers need the same type, it moves into a small
standalone `_types.h` header that they can all include (`window_types.h`,
`mesh_types.h`), or a similarly small header such as `application_info.h`. This
is done to avoid hiding issues that might slip through compilation. It is fully
understood that forward declarations and maybe some other techniques can reduce
compilation time, but they are deemed to not be worth the potential
consequences.

## System and third party headers

It is heavily preferred that OS and third party headers are kept private so as
to not impact engine consumers. Some cases may arise where this is impossible,
due to some headers needing to remain public, whether for project structure or
for their utility to the engine consumer. If profiling reveals private aspects
that would benefit from being public that are not addressed by compiler
optimisation techniques like LTO and PGO then they may move to public.

## File local code

Helpers local to a source file go in an anonymous namespace at file scope,
before the named namespace opens, not nested inside it. References back into
the named namespace are then fully qualified.

```cpp
namespace {

thread_local trivial::thread::Thread* g_currentThread = nullptr;

} // namespace

namespace trivial::thread {

// ...

} // namespace trivial::thread
```

Nesting it inside the named namespace has caused a real bug, where a local
helper was silently chosen over one with the same name from a shared header. It
is worth noting that this is as much a sign to have better naming, but the clear
isolated split is still heavily preferred.
