# Naming and Formatting

## Language

British English is used everywhere that is freely chosen: identifiers, macros,
comments and docs (`colour`, `normalised`, `TRIVIAL_REINITIALISES`). Names fixed
by an external API or the standard library keep their own spelling. It is
understood that this is atypical and may clash with some users, but this is
taken as a personal preference from how it follows more directly from thought to
typing. Again I stress this is not what would be considered a typical good
widespread practice, but since the engine is being designed mainly for personal
use and consumption such idiosyncrasy is deemed acceptable.

## Identifiers

Naming is enforced by clang-tidy's `readability-identifier-naming`, configured
in `.clang-tidy`, so a wrong name fails the tidy check.

| Kind | Case | Example |
| --- | --- | --- |
| Namespaces | `lower_case` | `trivial::sync` |
| Types, concepts, template parameters | `CamelCase` | `TaskSystem` |
| Enum constants | `CamelCase`, no prefix | `ThreadType::Main` |
| Functions and methods | `camelBack` | `activeParkingLot()` |
| Variables and parameters | `camelBack` | `workerId` |
| Macros | `UPPER_CASE` | `TRIVIAL_ASSERT()` |

Variables get a prefix for their scope, then `k` if they are const or
`constexpr`:

| Scope | Prefix | Const |
| --- | --- | --- |
| Namespace scope | `g_` | `g_k` |
| Class static members and function local statics | `s_` | `s_k` |
| Private and protected members | `m_` | `k` |
| Locals, parameters and public members | none | `k` |

Namespace scope includes file `static` variables and anonymous namespaces.
The prefixes follow how clang-tidy classifies each variable, with no
exceptions, so `thread_local` variables take `g_` or `s_` by where they are
declared and there is no separate thread local prefix. A const member is a
plain `kName` at any access, as a private runtime const member usually wants to
be `constexpr` or `static` instead. It is highly likely that these const member
variables would be better addressed as poor code style so that would be the
first point of call for them.

It is worth noting that the imperfection of this naming for scope related
naming ties to the limitation of tools rather than a perfect desired form. The
conventions may therefore later change with an update to later clang tool
versions upon release, more precisely when such better optionality becomes no
longer an experimental feature.

## Macros

The main point to note is names are always distinct, so two macros could never
meet in one build. This is done through scoping. The rules below are meant to be
followed but since there is no tool to enforce them some drift may occur. If
some drift from the below patterns are identified please mention.

Every engine macro starts with `TRIVIAL_`, followed by the module it belongs to
(`TRIVIAL_MEMORY_SEGMENT_SIZE`, `TRIVIAL_TASK_PAYLOAD_INLINE_STORAGE_SIZE`,
`TRIVIAL_PLATFORM_PAGE_SIZE`). Internal macros go further and name the type too,
`TRIVIAL_<MODULE>_<TYPE>_<NAME>` (for example
`TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK`), so they read correctly on their own.

The exception is the core macros used throughout the engine, which have no
module scope, for example:

- Assertions: `TRIVIAL_ASSERT`, `TRIVIAL_ASSUME`, `TRIVIAL_VERIFY`,
`TRIVIAL_UNREACHABLE`
- Logging and profiling: `TRIVIAL_LOG_*`, `TRIVIAL_PROFILE_*`
- Compiler attributes and hints: `TRIVIAL_FORCE_INLINE`, `TRIVIAL_HOT`,
`TRIVIAL_LIFETIMEBOUND` and so on, and the `TRIVIAL_DIAGNOSTIC_*` macros
- Build configuration: `TRIVIAL_CONFIG_*` and the `TRIVIAL_ENABLE_*` options

A macro used only by its own header is `#undef`'d at the end of that header,
and one used only by a `.cpp` is defined in that `.cpp`. Fixed size values are
typed, such as `(std::uint8_t{0b01})`, so a mask cannot silently become 32
bits; tunables in `_config.h` files stay untyped so they can be overridden
freely.

## Files and Directories

File names are `lower_case`, and where appropriate will have a
module/subsystem scoping for clarity when searching for a file.

Folders/directories are `kebab-case`. This helps with quickly differentiating
files and directories, but is mainly a personal preference aesthetically.

Exceptions to these rules lie primarily in tools that require a certain style
and may in the future also include some style from standard conventions.

## Formatting

Formatting is entirely clang-format's job, see [tooling](../tooling.md): tabs,
120 columns, attached braces, and includes grouped and sorted by category. Code
is not hand-aligned around it.

The one rule clang-format needs help with: in nested designated initialisers it
does not auto format both with the trailing comma. It therefore requires
formatting by hand by seeing how both would individually format and then
combining the results.

## Comments

Code carries as few comments as possible. Names and structure should explain
the code, and where something is non-obvious the first choice is to change the
code's shape so it shows. A comment is kept only for a hidden, surprising or
dangerous fact that the code cannot show.

Future work is not left as `TODO` comments; it goes in the
[roadmap](../../roadmap.md), with a `ROADMAP:` comment in the code only where a
marker at that spot is genuinely needed, so they can be found by searching.
Explanation of design and behaviour belongs in the docs rather than the code.

The prior style of `TODO` comments littered about the code was neglected due to
how easy it was to lose track of them and when grepping/fuzzy finding for them
it became a friction by seeing to many spread about and not knowing where to
start.

Suppression comments (`NOLINT`) follow the rules in [tooling](../tooling.md).
