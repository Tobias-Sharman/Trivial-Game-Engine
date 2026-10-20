# Configuration

There is some basic configuration of the engine as runtime values but most
configuration is compile time. The compile time configuration is mainly for more
advanced configuration of the engine and is probably not of much need to be
touched by a typical engine user. Take note of the below instructions on doing
so if deemed truly necessary for you. The different configurations are not
tested reliably beyond the default given values so more care is recommended,
since there has been no screening for bugs done in advance for you.

## Build options

- Set through CMake (`-D` on the command line, or in a preset)
- The full list, with descriptions, is the `option()` block at the top of
`CMakeLists.txt`
- Baked into the generated `build_config.h`, so they are fixed per build

## Compile time tunables

- All tunables are in file names ending in `_config.h`. Not listed here so as to
not have a divergence later
- Override them by defining the value in a user config header, either:
  - `trivial_user_config.h` on the include path, or
  - a header named by `TRIVIAL_USER_CONFIG`, quotes included, e.g.
  `-DTRIVIAL_USER_CONFIG="\"game/engine_config.h\""`, which wins over the
  default name (for this usage it is recommended to bake into a CMake preset)
- Build options cannot be overridden this way, as `build_config.h` defines them
first, so they have to be set through CMake
- Overrides go through the same checks as the defaults, so invalid values fail
the build (possible some bad values were not identified so recommended to take
care when using)
- The engine must be built with the same user config header as the game, as it
is a static library compiled separately, otherwise the two disagree on values
such as sizes and layouts

## Runtime configuration

- `EngineConfig`, passed to the `Engine` constructor
  - `graphicsApi`
  - `applicationInfo`
  - `window`
  - `tasks`
