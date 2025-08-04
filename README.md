## Overview
A voxel game built for performance and efficiency.

<img width=48% src="markdown/main_day.png"/> <img width=48% src="markdown/main_night.png">
<br>
<sup>In-game screenshots during the day and night</sup>

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
- Biomes and more varied terrain (caves, structures, etc)
- Controller support
- Player model
- Lighting

## Commands
Commands have been [implemented](src/Application/Application.hpp) in a way that makes creating new ones extremely trivial. The defaults allow for easy exploration and manipulation of the game and the world.

### Syntax
To write a command, use the chat with a forward slash as the first character, immediately followed by the command name and then any (or no) arguments seperated with spaces: 

`/name arg1 arg2...`

A message written without a forward slash at the start else will be treated as a chat message instead.

Any command argument marked with an **asterik (*)** is optional. If the command has only one argument and it is optional, entering the command _without_ any arguments acts as a query for the value it changes. 

>Example: `/time 256` will change the current game time to 256 seconds whereas `/time` will display the current time in the chat.

Using a **tilde (~)** as an argument will be treated as the current value. Any number after the tilde will be **added** to the value.

>Example: `/tp ~ ~10 ~` will move you up 10 blocks in the Y axis whereas <code>/tp&nbsp;~&nbsp;~<b>–</b>10&nbsp;~</code> will move you downwards by 10 blocks.

A **negative symbol (–)** can also be added _before_ the tilde to negate the resulting value.

>Example: <code>/tp ~ **–**~10 ~</code> will first calculate the +10 of your Y position then negate it. If the Y position was 50, it would be changed to –60 as 50 + 10 = 60 and then 60 * –1 = –60.

The command '**help**' can also be used in-game to display information on how commands are formatted. It also provides a list of all available* commands and descriptions of their purpose.

#### Notable examples include:
- /**tp** x y z *pitch *yaw Teleport to the specified coordinates with optional arguments to set camera pitch and yaw.
- /**set** x y z id - Replaces the block at the specified coordinates to the given block ID.<sup>^</sup>
- /**speed** *n - Change the player's current speed to the specified value.
- /**tick** *n - Change the tick speed to the specified value, which affects the speed at which in-game time passes.
- /**time** *n - Change the current in-game time to the specified value. The day-night cycle is respected.
- /**fov** *n - Change the camera's field of view to the specified value.
- /**clear** - Clears the chat

Writing any command with the only argument as `?` or with an invalid number of arguments will display help for that command.
<hr>

*<sub>Debug commands are not included within the list of commands. They can, however, be seen in the commands implementation in the same file linked below.</sub>

^<sub>A full list of all the blocks and their associated IDs and properties can be found in the [following file](src/World/Generation/Settings.hpp).</sub>

<img src="markdown/cmd.png"></img>
<sup>In-game screenshot with GUI enabled</sup>

## Controls
The implementations of controls can also be viewed and easily edited in [this file](src/Application/Callbacks.cpp). 

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
- Change speed: **COMMA** and **PERIOD** (repeatable)
- Change FOV: **I** and **O** (repeatable)
- Change render distance: **[** and **]** (square brackets)

### Function inputs
- Toggle all GUI: **F1**
- Take screenshot: **F2**
- Free cursor: **F3**
- Toggle debug text: **F4**
- Toggle fullscreen: **F11**

### Debug inputs
- Wireframe: **Z**
- Reload shaders: **R**
- Toggle chunk borders: **J**
- Rebuild chunks: **U**

*<sub>The first input increases the value whilst the other decreases it. A 'repeatable' input means either input can be held down to repeatedly change the value. Some values are limited to certain bounds, which also applies when editing them with commands.</sub>
## Build
Since this game relies on a few libraries (see submodules in [this](libraries/) folder or the [Libraries](tab=readme-ov-file#libraries) section below), you will need to use the following clone command (requires [git](https://git-scm.com/)) to also clone them alongside the game:

```bash
$ git clone --recurse-submodules --j 5 https://github.com/mahdialmusaad/badcraft
```

To build and run the game, you can simply use [CMake](https://cmake.org/) and run the following (with your own directories and settings):

```bash
$ cmake -S [source-dir] -B [build-dir]
```

## Libraries
This game makes use of a few libraries to work. They can be seen in the submodules in the [libraries](libraries/) folder or below. Make sure to support them as this game would not be possible without them!

[lodepng](https://github.com/lvandeve/lodepng) - PNG encoder and decoder

[GLM](https://github.com/icaven/glm) - OpenGL maths library

[GLFW](https://github.com/glfw/glfw) - Window and input library

[glad](https://github.com/Dav1dde/glad) - OpenGL loader/generator

[fmt](https://github.com/fmtlib/fmt) - Formatting library
