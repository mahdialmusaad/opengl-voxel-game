## Overview
A voxel game built for performance and efficiency.

<img src="markdown/main.png"></img><br>
<sup>In-game screenshot during sunrise</sup>

## Features
This game offers a wide variety of features to create a vast gameplay experience while allowing for new features to be trivially added.

Examples of features include:
- **64-bit** generation to allow worlds to span _trillions_ of blocks
- [Dynamic skybox](src/World/Sky.cpp) with clouds, stars and the sun and moon
- Custom [text rendering system](src/Rendering/TextRenderer.cpp) that supports different colours, sizes and other features
- Chatting and [commands](tab=readme-ov-file#commands)

## Goal
The purpose of this game is to achieve extreme performance using OpenGL in C++ while still offering many features and possibly serving as inspiration for others attempting to create 3D software using graphics libraries.

### Possible additions
There are still many things that could be improved about the game. The most likely additions include:
- Main menu with world select and saving
- Biomes and more varied terrain (e.g. caves)
- Controller support
- Player model
- Lighting

## Commands
Commands have been [implemented](src/Application/Game.hpp) in a way that makes creating new ones extremely trivial. The defaults allow for easy exploration and manipulation of the game and the world.

### Syntax
To write a command, use the chat with a forward slash as the first character, immediately followed by the command name and then any (or no) arguments separated with spaces: 

`/name arg1 arg2...`

A message written without a forward slash at the start else will be treated as a chat message instead.

Any command argument marked with an **asterik (*)** is optional. If the command has only one argument and it is optional, entering the command _without_ any arguments acts as a query for the value it changes<sup>1</sup>.

>Example: <code>/time&nbsp;256</code> will change the current game time to 256 seconds whereas <code>/time</code> will display the current time in the chat.

Using a **tilde (~)** as an argument will be treated as the current value. Any number after the tilde will be **added** to the value.

>Example: <code>/tp&nbsp;~&nbsp;<b>~10</b>&nbsp;~</code> will move you up 10 blocks in the Y axis whereas <code>/tp&nbsp;~&nbsp;~<b>–10</b>&nbsp;~</code> will move you downwards by 10 blocks.

A **negative symbol (–)** can also be added _before_ the tilde to negate the resulting value.

>Example: <code>/tp&nbsp;~&nbsp;<b>–~10</b>&nbsp;~</code> will first calculate the +10 of your Y position then negate it. If the Y position was 50, it would be changed to –60 as 50 + 10 = 60 and then 60 * –1 = –60.

The command '**help**' can also be used in-game to display information on how commands are formatted. It also provides a list of all available* commands and descriptions of their purpose.

#### Notable examples include:
- /**tp** x y z *pitch *yaw Teleport to the specified coordinates with optional arguments to set camera pitch and yaw.
- /**set** x y z id - Replaces the block at the specified coordinates to the given block ID<sup>2</sup>.
- /**speed** *n - Change the player's current speed to the specified value.
- /**tick** *n - Change the tick speed to the specified value, which affects the speed at which in-game time passes.
- /**time** *n - Change the current in-game time to the specified value. The day-night cycle is respected.
- /**fov** *n - Change the camera's field of view to the specified value.
- /**clear** - Clears the chat
- And more!


Writing any command with the only argument as `?` or with an invalid number of arguments will display help for that command<sup>3</sup>.
<hr>

<sub><sup>1</sup>Some other commands with multiple arguments have this property. An example is the /tp command (see use above) which will print out both your position and camera direction when run with no arguments.</sub>

<sub><sup>2</sup>A full list of all the blocks with their associated IDs and properties can be found in the [following file](src/World/Generation/Settings.hpp).</sub>

<sub><sup>3</sup>Debug commands are not included within the list of commands. They can instead be seen in the commands implementation in the same file linked above with accompanying descriptions as well.</sub>

<img src="markdown/cmd.png"></img>
<sup>In-game screenshot with GUI enabled</sup>

## Controls
The implementations of controls can also be viewed and easily edited in [this file](src/Application/Callbacks.cpp).<br>
Default controls are as follows:
- Movement: **WASD**
- Write command: **/** (forward slash)
- Write chat message **T**
- Exit game/close chat: **ESC**

### Toggle inputs:
- Toggle vertical sync: **X**
- Toggle inventory: **E**
- Toggle fog: **F**
- Toggle gravity: **C**
- Toggle noclip: **N**
- Toggle chunk generation: **V**

### Value inputs*
- Change speed _(repeatable)_: **COMMA** and **PERIOD**
- Change FOV _(repeatable)_: **I** and **O**
- Change render distance: **[** and **]** (square brackets)

### Function inputs
- Toggle all GUI: **F1**
- Take screenshot: **F2** (Saved into the `/Screenshots` directory)
- Free cursor: **F3**
- Toggle debug text: **F4**

### Debug inputs
- Wireframe: **Z**
- Toggle chunk borders: **J**

*<sub>The first input increases the value whilst the other decreases it. A '_repeatable_' input means either input can be held down to repeatedly change the value. Some values are limited to certain bounds, which also applies when editing them with commands.</sub>
## Build
To build and run this game, [CMake](https://cmake.org/) and [git](https://git-scm.com/) is required. You can then run the following commands on a terminal:

```bash
git clone https://github.com/mahdialmusaad/opengl-voxel-game
cd opengl-voxel-game
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
```
Alternatively, you can use the CMake GUI.<br>
The resulting executable can be found in the `/game` directory in the root folder.
## Libraries
This game makes use of a few libraries to work. Make sure to support them as this game would not be possible without them!

[lodepng](https://github.com/lvandeve/lodepng) - PNG encoder and decoder<br>
[GLFW](https://github.com/glfw/glfw) - Window and input library<br>
[glad](https://github.com/Dav1dde/glad) - OpenGL loader/generator<br>