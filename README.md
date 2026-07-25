# Trivial Engine

A game engine designed for 2.5D games. The design will be more specialised and directed towards the creation of game with myself and a friend. The creation of the engine is to allow for complete freedom in the development of the game.

Building is done through cmake. It will pull in the base dependencies, e.g. GLFW, and VMA, to a dependencies directory, except for the Vukan SDK, which you must get yourself. For the Vulkan SDK, it is normally recommended to do so through the official LunarG Vulkan SDK.

There is no formal licence and everything is free to use with no rights reserved.

Documentation for the parts of the engine as they are in an appropriate place to not have an overhaul to the api or the internals that would entirely change functionality. For instance the task system can be found with its initial documentation in there.

Current plan of action:
 - Custom allocator
 - Basic physics system to test and profile the task system
 - Overhaul of the ECS system to have a proper efficient storage rather than the current placeholder mockup
    - Will be chunked archtype unless I can narrow done how to implement a sparse set with "archtypes" as the set types and well handle when different archtypes would want the same attribute with good cache locality for all the systems that benefit from it
 - More fleshed out physics system with parallel operation and SIMD backing
 - Extend graphics support to be more general to then visualise some basic 2d physics

The engine architecture can be seen below:

![alt text](docs/design/Architecture.svg "Engine Architecture")
