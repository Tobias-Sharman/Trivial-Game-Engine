# Types and APIs

## Structs and classes

A type is a `struct` when it is used for the purpose of storage. This means it
should not involve inheritance, no functions are the to alter its internal
state, and no private members for such a reason as not allowing direct access.
There are exceptions to this style that should be used carefully as they
completely and directly contradict the above requirements (except the
inheritance): if the internal storage is bit-packed then access to the stored
values should only be done safely via specific functions to private storage;
operator overloads are an allowed exception to the internal state mutation as
they allow for nicer code without breaking the spirit of the rules; and finally
functions like `reset()` which would return all values to defaults are allowed
as they are a way to convey intention with greater clarity than `*this = T{}`
(it should be noted this is only for standard value setting not something that
would take an argument and set based on that value). The point is the spirit of
the name being that you can read something is a struct and know that it is only
meant as a clean way to wrap a storage type and it is this spirit that is still
followed by the exceptions, but with allowance for cleaner and more robust code.

Anything else makes it a `class`: inheritance or virtual functions, functions
that change state for other reasons, calls to the OS or the clock, locking, or
owning resources and object lifetimes. Being non-copyable or having an identity
is not enough on its own since such a property is based around the safe usage of
that underlying storage not the way such a type should be treated. It is
understood on writing this style of usage with allowing denial of copying is
weird, but it is important to understand that the type still remains to have
the purpose of storage not an object and it is the functions that rest on top of
that struct/class that makes it a class not a struct.

## Functions

Every function is `noexcept`, and every function that can genuinely run at
compile time is `constexpr`. Templated functions should be checked with
`static_assert`s for if they allow that expected compile time evaluation.

Functions returning a value are `[[nodiscard]]`, and functions returning a
reference into the object are `TRIVIAL_LIFETIMEBOUND`, so the compiler can catch
a reference outliving what it points into.

## Parameters

Required, read-only config and info structs are passed as `const T&`, as are
objects only used during the call, with no null checks since a reference cannot
be null (`Engine(const EngineConfig&)`). Pointers are kept for non-owning
handles that are stored for later, and for data whose address must outlive the
call. That data is owned by the caller and passed down as a pointer, never a
`const T&`, as a reference would bind to a temporary and dangle. Or more briefly
always prefer use of reference over pointers since that usage covers issues with
being `nullptr`.

Other than constant references parameters are not marked constant. This is for
two primary reasons: first, whether they are `const` or not has no bearing on
the caller, since either way functions that same for arguments taken by value,
and therefore marking `const` is of no real benefit for enforcement of not
changing the value; and second, marking them `const` would require then the `k`
prefix ([naming conventions](naming_and_formatting.md)) and make tools showing
the names for the function parameters at the point of use ugly.

## Variables

Usage of `auto` is avoided to reduce likelihood of a type changing in unexpected
ways. It is understood that there are some places like loops that benefit from
`auto` for nicer looking code, but for the consistency no exceptions are
allowed for this as it offers no benefit and in fact makes the code more opaque.

## Constants

Engine constants are macros, for consistency, following the naming in
[naming_and_formatting.md](naming_and_formatting.md). They are typed with a
suffix or a cast such as `(std::uint8_t{0b01})` when their width matters. An
`inline constexpr` variable is used only where a macro cannot work, such as the
templated maths constants.

Constants local to a function are plain `constexpr` locals, including structs
passed to an API by address, as building them on the stack each call costs
nothing next to the call itself. Only lookup tables indexed at runtime are
function local `static constexpr`.

## Index, id and count types

An index, id or count uses the type that matches its role and any real constraint,
not a blanket default. A count matches the API that produces it, so
`Thread::id()` is `std::uint32_t` like `hardware_concurrency()`, while a
subscript matches its container, so worker ids are `std::size_t`. A
narrower type is only chosen for a real reason, such as an external API, a
genuine packing saving (not one undone by alignment padding), or keeping an
atomic lock-free on a hot path.

## The `detail` namespace

`detail` holds things that must be visible for the code to compile, but that a
user would be wrong to touch directly, because doing so would bypass an assert,
initialisation order or an invariant. Examples are the `g_` globals behind
accessors, such as the active parking lot and task system, and the memory
`DetectedCapabilities`. It is possible some of these code be moved to private
files, but by a case by case basis if there is a notable benefit to being public
(e.g. linking related) then they may be placed public and hidden behind the
`detail` namespace. Helpers that are simply not meant for users, but are
harmless to use stay public, as while they are not meant for use they are safe
to use so there is no benefit to trying to deny direct usage. This convention is
to enforce this users are not to touch `detail` namespaces, since such an action
would have no benefit and *very* likely break the project in undefined ways.
This is a convention only and nothing enforces it, but making it only when
absolutely needed will make engine consumption cleaner across all users, since
nobody should be touching such implementation details.
