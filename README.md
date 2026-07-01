# Trivial Engine

A game engine designed for 2.5D games. The design will be more specialised and directed towards the creation of game with myself and a friend. The creation of the engine is to allow for complete freedom in the development if the game.

There is no current build script or handling for getting dependencies provided since it is not yet in a state that will be useful and it is untested across different OSs.

As of the start of developing the engine some of the main design choices made that matter are: the freedom for runtime alteration of the graphics api, and the possible later support of multiple simultaneous gpu usage - the second being mainly for projects where the extra latency and stuttering between each step/frame of the program does not matter compared to the sheer compute power, e.g. a particle simulation program where particles are batched based on location and stepped in parallel and there is no significant asset duplication across gpus required; and the design of the (to come) built in types for ECS archtypes based on what will be useful and needed for our game, since that would allow for easier and more direct optimisation; and no support for switching the Window manager outside of compile time is to be provided - this is primarily to reduce complexity since it is not of notable use to have runtime switching of the window manager for a game. Support for queue usage from different families will possibly be supported as one of the last things, or earlier if profiling suggests it is worth doing so (for a 2.5D game this is incredibly unlikely so will not likely be done), since the synchronisation handling and existing guides to follow online will be much simpler. Support for multiple api instances may also be provided but again it is far from priority.

The thread split will be to have dedicated main and render thread (maybe more later if it becomes pertinent, i.e. profiling reveals something like audio or physics is taking an unproportionate amount of compute they may get a dedicated thread). Any thread that is in a waiting state will be free to take from the to do work of other threads, this will be done through a dequeue structure to reduce instances of two threads wanting to start the same job.

The job system is the next goal to complete, then completing the so called hello triangle, then an overhaul of the ECS system to have a proper efficient storage rather than the current placeholder mockup along with some SIMD bits as they are interesting to work on. Through this the Physics capabilities will be slowly built up. After those are complete the subsequent direction will be decided.

The engine architecture can be seen below:

![alt text](docs/Architecture.svg "Engine Architecture")
