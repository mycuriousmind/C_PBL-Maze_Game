# C PBL Maze Game

## Overview

This project is a Maze Game developed in C using the raylib graphics library. It features procedural maze generation and incorporates the A* (A-star) pathfinding algorithm to solve the mazes. The project is designed to be cross-platform, capable of running as a standalone desktop executable or natively in a web browser via WebAssembly.

## Features

* **Maze Generation:** Dynamically generates random, solvable mazes.
* **A* Pathfinding:** Implements the A* search algorithm to compute and display the optimal path from the start to the end of the maze.
* **Cross-Platform Support:** Contains build configurations for desktop environments and pre-compiled files for the web (WASM).
* **Graphical Interface:** Utilizes raylib for smooth, hardware-accelerated 2D rendering.

## Project Structure

* `main.c`: The entry point and main game loop, handling graphics, timing, and user input.
* `maze.c` & `maze.h`: Core logic and data structures for creating, storing, and managing the maze grid.
* `astar.c` & `astar.h`: The implementation of the A* pathfinding algorithm.
* `MakeFile` & `Makefile.raylib`: Build scripts for compiling the C source code into an executable.
* `index.html`, `index.js`, `index.wasm`: Web-ready files generated using Emscripten to run the game in a browser.
* `maze_game.exe`: Pre-compiled Windows executable.

## Requirements

To compile this project from source, you will need:

* A C compiler (e.g., GCC, MinGW for Windows, or Clang)
* Make build tool
* [raylib](https://www.raylib.com/) installed and configured on your system
* [Emscripten](https://emscripten.org/) (only required if you wish to re-compile the WebAssembly version yourself)

## How to Build and Run

### Running the Pre-compiled Windows Version

If you are on Windows, you can simply run the provided executable:

1. Double-click `maze_game.exe`.

### Compiling for Desktop

1. Ensure raylib is properly installed and accessible to your compiler.
2. Open a terminal in the project directory.
3. Run the make command to build the executable. Depending on your environment, run:
```bash
make

```


*Or, if using the specific raylib makefile:*
```bash
make -f Makefile.raylib

```


4. Run the newly generated executable.

### Running the Web Version

The repository includes the compiled WebAssembly files. Because browsers restrict loading local files via `file://` protocols, you must use a local web server to play it.

1. Start a local web server in the project directory. If you have Python installed, you can run:
```bash
python -m http.server

```


2. Open your preferred web browser and navigate to `http://localhost:8000/index.html`.
