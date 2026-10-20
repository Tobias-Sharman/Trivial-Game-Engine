# Trivial Engine

A game engine designed for 2.5D games. This project in the initial stages, and
maybe later in its life cycle too, will be designed and specialised for personal
use cases. It is to be used for the creation of some games, so its direction
will be influenced by them. The creation of the engine is to allow for complete
freedom in the development of the game and for personal interest in low-level
optimisation and code writing.

It is a personal project, so it is developed without a great formality - as such
the documentation will grow when desired and may have divergence from the actual
code (in such a case when significant it will have a clear note). In addition,
ahead of a proper version 1.0.0, there is no guarantee on changes to API and
function contracts without going through a deprecated phase.

## Quick start

```bash
git clone https://github.com/Tobias-Sharman/Trivial-Game-Engine.git
cd Trivial-Game-Engine
python3 scripts/build.py -t -r
```

This builds the `debug` preset, runs the tests, and runs the sandbox. See
[building](docs/manual/building.md) for the requirements and the other
presets, and [getting started](docs/manual/getting_started.md) for using the
engine in a game.

> It is recommended to use tools like uv for python environment management here,
> and simply in general.

## Documentation

The [documentation](docs/README.md) is split into a manual for making games
with the engine, development docs for working on the engine itself, and the
[roadmap](docs/roadmap.md). Later the architecture and subsystem aspects will be
filled in.

## Platforms

Windows, Linux, and macOS on x86-64 and ARM64, with Windows on ARM64 and macOS
on x86-64 untested, see [platform support](docs/manual/platform_support.md).
Please report any issues found.

## Licence

Licensed under the [Apache License, Version 2.0](LICENSE). Games and other
software built with the engine can use any licence, but must keep the
[NOTICE.md](NOTICE.md) file's attribution somewhere a user could reasonably
find it (e.g. a credits screen, documentation, or an in-app NOTICE display).
NOTICE.md also lists the engine's third-party dependencies and their licence
terms, and credits design references where deemed appropriate. A note on this
is it need not be a title screen as that can often interfere with the feel of a
game, and as such implementation of accreditation is deferred to the user. It
would though be appreciated to be let know if the engine is used as it would be
interesting to see what would be made using it.
