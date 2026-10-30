# Getting Started

The sandbox (`sandbox/src/main.cpp`) is a plain example of basic engine use and
can be read alongside this. It is a "naive" boid simulation meant to be of use
for stress testing - it is therefore not optimised but completes computations
exhaustively.

The entry point style and layer usage is subject to change when adding the
physics subsystem, as that will change the suitable hooks.

## Adding the engine to a game

- Bring it in one of two ways, then link against `trivial` (needs C++ 23)
  - As a git submodule (or a plain clone) inside the game's repository, giving a
  pinned copy that can be browsed and patched:

    ```bash
    git submodule add \
        https://github.com/Tobias-Sharman/Trivial-Game-Engine.git <path>
    ```

    ```cmake
    add_subdirectory(<path>/engine)
    target_link_libraries(game PRIVATE trivial)
    ```

  - With `FetchContent`, where CMake downloads it at configure time, pinned to a
  commit as there are no release tags yet:

    ```cmake
    include(FetchContent)
    FetchContent_Declare(
        trivial
        GIT_REPOSITORY https://github.com/Tobias-Sharman/Trivial-Game-Engine.git
        GIT_TAG <commit>
        SOURCE_SUBDIR engine
    )
    FetchContent_MakeAvailable(trivial)
    target_link_libraries(game PRIVATE trivial)
    ```

- If doing any advanced compile time configuration, the engine must be built
with the same config file - see [configuration](configuration.md)
- The engine is compiled with function and data sections, so a game can drop
unused engine code at link time by adding the appropriate linker flags. The
easiest way to do this is by just adding to your projects `CMakeLists.txt`:

    ```cmake
    target_link_options(game PRIVATE
        $<$<PLATFORM_ID:Darwin>:LINKER:-dead_strip>
        $<$<PLATFORM_ID:Linux>:LINKER:--gc-sections>
        $<$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>:LINKER:/OPT:REF>
    )
    ```

## Entry point

An example of this is shown in the sandbox so using that as a reference is
recommended.

- Fill in an `EngineConfig` (application name, window size and title, task
system, and graphics API settings as you want or leave to defaults)
- Construct the `Engine` first, which handles the setup of engine wide services
- Create an `Application` with the game layer
- Optionally attach a debug layer with `TRIVIAL_ATTACH_DEBUG_LAYER`, which
compiles away outside of debug builds
- `engine.run(app)` runs until the window closes

## Layers

- `onStart(gpu::Context*)`, `onUpdate(const FrameContext&)` and `onEnd()`
- `collectDrawables()` must be implemented
- `FrameContext` holds the delta time and the frame number
- Each frame: poll events, update the game layer, update the debug layer, then
render

## Engine features from game code

- Tasks: launching, waiting and getting results, with handles currently needing
an explicit release
  - Multithreading implementation with tasks constructed as a DAG
- Logging and assert macros
- Profiling scopes with Tracy
- Maths types

## Current limits

- One game layer plus one debug layer
- No input system beyond the window's event polling
- World/ECS is not connected to the engine
- Rendering is basic shapes only

## Attribution

- `NOTICE.md` goes through external code used for the engine and goes through
the attribution requirements needed to be followed
