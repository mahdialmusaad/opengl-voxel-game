# Badcraft
A voxel-type game inspired by Minecraft, designed to be performant (in terms of memory usage and FPS), cross-compatible and easy to change.
## Features
- 64-bit rendering and generation system, allowing for worlds to span **trillions** of blocks with no issues!
- Custom text rendering system that supports different sizes and colours
- Event logging system in console
- Dynamic skybox
- Commands (see ['Commands'](Commands))
- Screenshotting (see ['Controls'](Controls))
## Goal
Badcraft was developed with performance and cross-compatibility in mind.
The main code (see ['src'](https://github.com/mahdialmusaad/badcraft/src) folder) was written to work on all platforms (although they still need to be compiled for each), such as by avoiding platform/compiler specific features and using specific-sized variables in appropriate cases.
See the [chunk generation/calculator](https://github.com/mahdialmusaad/badcraft/src/World/Chunk.cpp) C++ file for examples.
### Possible additions
There are still many things that could be improved about the game, but implementing them could prove to be difficult or possibly involve major changes of existing parts of the source code:
- Greedy meshing (currently in development)
- A proper chat rather than just commands
- Structures
- More inventory features (dragging, counters, dropping)
- Crafting and other blocks with their own respective GUIs
However, they could still become a feature one day!
## Commands
Badcraft also features commands to allow you to explore and edit the world more easily! The implementations can be seen in the [application C++ file](https://github.com/mahdialmusaad/badcraft/src/Utility/Application.cpp) (ApplyChat function)
- /tp x y z - Teleport to specified x, y and z coordinates. Scientific notation and words such as 'inf' are accepted.
- /speed s - Change the player's current speed to the specified value.
- /tick t - Change the game tick speed to the specified value (affects how fast time passes in-game)
- /exit - Exits the game. What did you expect?
- /help - (TODO, just exits the game for now 😶)
- /dcmp s - (DEBUG) Outputs the assembly code of the specified shader ID into 'bin.txt'
- /fill x y z x y z b - Fills the blocks from 
## Controls
The implementations of controls can be viewed and easily edited in the [application header file](https://github.com/mahdialmusaad/badcraft/src/Utility/Application.hpp). Currently, they are:
- Movement: __WASD__
- Toggle VSYNC: __X__
- Wireframe: __Z__
- Write command: __/__
- Exit/exit commands (if currently typing one): __ESC__
- Reload shaders: __R__
- Toggle inventory: __E__
- (DEBUG) Toggle collision: __C__
- Change speed (increase and decrease respectively): __,__ and __.__
- Change FOV (increase and decrease respectively): __O__ and __I__
- Toggle GUI: __F1__
- Take screenshot: __F2__
- Free cursor: __F3__
## Libraries
[lodepng](https://github.com/lvandeve/lodepng) - PNG encoder and decoder
[GLM](https://github.com/icaven/glm) - OpenGL maths library
[GLFW](https://github.com/glfw/glfw) - Window and input library
[GLAD](https://github.com/Dav1dde/glad) - OpenGL loader/generator
