# Trivial Engine

A game engine designed for 2.5D games. The design will be more specialised and directed towards the creation of game with myself and a friend. The creation of the engine is to allow for complete freedom in the development if the game.

There is no current build script or handling for getting dependencies provided since it is not yet in a state that will be useful and it is untested across different OSs.

Similarly there is not a proper README since I do not yet see a point as the engine api is yet subject to too much flux so this is just a thought dump mostly that I can use as reference.

Likewise no LICENCE that will be addressed when I have the time and nothing better to do. Until then it is free use in whatever so form is desired though do inform me if using it I would be interested in your usage and use case.

As of the start of developing the engine some of the main design choices made that matter are: the freedom for runtime alteration of the graphics api, and the possible later support of multiple simultaneous gpu usage - the second being mainly for projects where the extra latency and stuttering between each step/frame of the program does not matter compared to the sheer compute power, e.g. a particle simulation program where particles are batched based on location and stepped in parallel and there is no significant asset duplication across gpus required; and the design of the (to come) built in types for ECS archtypes based on what will be useful and needed for our game, since that would allow for easier and more direct optimisation; and no support for switching the Window manager outside of compile time is to be provided - this is primarily to reduce complexity since it is not of notable use to have runtime switching of the window manager for a game. Support for queue usage from different families will possibly be supported as one of the last things, or earlier if profiling suggests it is worth doing so (for a 2.5D game this is incredibly unlikely so will not likely be done), since the synchronisation handling and existing guides to follow online will be much simpler. Support for multiple api instances may also be provided but again it is far from priority.

The task system is in and will have proper documentation added later but in short supports tasks in a DAG with proper prerequisite handling and multithreading with *very* basic further implementation for stuff like task stealing.

Current plan of action:
 - Basic physics system to test and profile the task system
 - Completing the so called hello triangle of graphics
 - Overhaul of the ECS system to have a proper efficient storage rather than the current placeholder mockup
    - Will be chunked archtype unless I can narrow done how to implement a sparse set with "archtypes" as the set types and well handle when different archtypes would want the same attribute with good cache locality for all the systems that benefit from it
 - More fleshed out physics system with parallel operation and SIMD backing
 - Extend graphics support to be more general to then visualise some basic 2d physics

The engine architecture can be seen below:

![alt text](docs/design/Architecture.svg "Engine Architecture")

As an aside to clear confusion on my, at times asinine, comment usage:

- TOTEST is something that needs testing and I could not at the time do so (will 99% of the time be for hardware I do not have with me at the time)
- TODO is to represent something for me to, as in the name, do that is not urgent but will want to be addressed before the engine is in a good state
- NOTE is for something to inform future changes of things I think may be relevant or inform me when looking at something I have thought this through and will come back to it when relevant it can be left be
- FIXME may or may not appear, and is to be used for stuff that gets broken pre first proper version and does not need fixing because it isn't in use - will likely come with a TODO marker too
- Regular comments that are just meant as comments that I could not care to find with a grep or equivalent tool

These use cases will blur when I am coding half awake so they are to be taken with a grain of salt.
