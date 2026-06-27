# Maze Game Refactor — 7 Requirements Implementation Plan

## Summary

Refactor the existing turn-based C/Raylib maze game to satisfy all 7 teacher requirements, while remaining compatible with Emscripten's `emscripten_set_main_loop` (no blocking `while` loops — everything funnels through a single `UpdateDrawFrame` callback).

---

## Current State

| File | Role | Lines |
|------|------|-------|
| [main.c](file:///c:/Users/swoja/C_PBL/main.c) | Game loop, input, HUD, game-over screen | 170 |
| [maze.h](file:///c:/Users/swoja/C_PBL/maze.h) | Constants (`MAZE_ROWS`/`MAZE_COLS` = 15), extern maze array | 16 |
| [maze.c](file:///c:/Users/swoja/C_PBL/maze.c) | Hardcoded 15×15 maze, `is_wall()`, `draw_maze()` | 74 |
| [astar.h](file:///c:/Users/swoja/C_PBL/astar.h) | A* node struct, function prototypes | 26 |
| [astar.c](file:///c:/Users/swoja/C_PBL/astar.c) | A* pathfinding with fixed `MAZE_ROWS`×`MAZE_COLS` arrays | 99 |

> [!NOTE]
> The existing `astar.h` has a syntax bug on line 18: `struct *Node parent;` should be `struct Node *parent;`. This will be fixed as part of the refactor.

---

## Proposed Changes

### maze.h — Dynamic Grid Support

#### [MODIFY] [maze.h](file:///c:/Users/swoja/C_PBL/maze.h)

Replace the fixed `MAZE_ROWS`/`MAZE_COLS` constants with a **maximum size** approach:

```c
#define MAX_MAZE_SIZE 31   // largest level is 31×31

#define TILE_PATH 0
#define TILE_WALL 1

// Current level dimensions (set at runtime)
extern int g_maze_rows;
extern int g_maze_cols;

// Fixed max-size 2D array, partially used
extern int maze[MAX_MAZE_SIZE][MAX_MAZE_SIZE];

void generate_maze(int rows, int cols);         // Procedural generation
void draw_maze(int player_row, int player_col,
               int bot_row, int bot_col,
               int exit_row, int exit_col);
int  is_wall(int row, int col);
```

**Rationale**: Using a fixed `MAX_MAZE_SIZE` array avoids `malloc`/`free` complexity while still supporting the 3 level sizes (15, 21, 31). The runtime variables `g_maze_rows`/`g_maze_cols` tell all code the *active* portion of the array.

---

### maze.c — Procedural Generation (Recursive Backtracker)

#### [MODIFY] [maze.c](file:///c:/Users/swoja/C_PBL/maze.c)

Major changes:
1. **Remove** the hardcoded `const int maze[15][15]` array.
2. **Add** `int g_maze_rows`, `int g_maze_cols` globals.
3. **Add** `generate_maze(int rows, int cols)` — implements the **Recursive Backtracker** (randomized DFS) algorithm:
   - Fill entire grid with walls.
   - Start carving from cell `(1,1)`.
   - Use an iterative stack (to avoid deep recursion for 31×31 grids).
   - Guarantee connectivity from `(1,1)` to `(rows-2, cols-2)`.
   - Force-open the start `(1,1)` and exit `(rows-2, cols-2)` cells after generation.
4. **Update** `is_wall()` to use `g_maze_rows`/`g_maze_cols` instead of the old constants.
5. **Update** `draw_maze()` to:
   - Dynamically compute `CELL_SIZE` based on the current level dimensions so the maze always fits within the window (e.g., `cell_size = 600 / g_maze_cols`).
   - Loop up to `g_maze_rows`/`g_maze_cols`.

**Algorithm detail — Recursive Backtracker**:
```
1. Fill grid with WALL
2. Mark cell (1,1) as PATH, push onto stack
3. While stack is not empty:
   a. current = top of stack
   b. Find all unvisited neighbors 2 cells away (in the 4 cardinal dirs)
   c. If any exist: pick one at random, carve the wall between, push new cell
   d. If none: pop the stack (backtrack)
4. Force-open start (1,1) and exit (rows-2, cols-2)
```

> [!IMPORTANT]
> The algorithm works on **odd-dimensioned** grids where walls sit on even indices and paths on odd indices. All three level sizes (15, 21, 31) are odd, so this works correctly.

---

### astar.h — Dynamic Grid Support

#### [MODIFY] [astar.h](file:///c:/Users/swoja/C_PBL/astar.h)

- Replace `MAZE_ROWS`/`MAZE_COLS` references with `MAX_MAZE_SIZE`.
- Fix the syntax error in the `Node` struct (`struct *Node parent` → `struct Node *parent`).
- Update `OPEN_LIST` to `(MAX_MAZE_SIZE * MAX_MAZE_SIZE)`.

---

### astar.c — Dynamic Grid Limits

#### [MODIFY] [astar.c](file:///c:/Users/swoja/C_PBL/astar.c)

- Replace all `MAZE_ROWS`/`MAZE_COLS` loop bounds with `MAX_MAZE_SIZE` for array declarations, and `g_maze_rows`/`g_maze_cols` for loop iteration.
- The `is_wall()` function (from maze.c) already handles bounds checking, so neighbor expansion is safe.
- Node arrays use `[MAX_MAZE_SIZE][MAX_MAZE_SIZE]` (stack-allocated, ~30 KB — fine for stack).

---

### main.c — State Machine, Login, Levels, Audio, HUD

#### [MODIFY] [main.c](file:///c:/Users/swoja/C_PBL/main.c)

This is the largest change. The new structure:

#### State Machine
```c
typedef enum { LOGIN, GAMEPLAY, GAMEOVER, LEVEL_TRANSITION } GameScreen;
```

The `UpdateDrawFrame()` function (called by `emscripten_set_main_loop`) uses a `switch` on `currentScreen` to handle each state — no blocking loops.

#### Login Screen (`LOGIN`)
- Draws a title, a text-input box, and a "Press ENTER to start" prompt.
- Captures keyboard input character-by-character using Raylib's `GetCharPressed()` and `IsKeyPressed(KEY_BACKSPACE)`.
- Stores the name in a `char playerName[32]` buffer.
- On ENTER: transitions to `GAMEPLAY`, generates Level 1 maze.

#### Gameplay (`GAMEPLAY`)
- Same turn-based logic as before (player WASD → bot A* move).
- **HUD additions**:
  - **Timestamp**: Uses `time()` + `localtime()` to get system clock, displayed as `HH:MM:SS`.
  - **Player name**: Draws `playerName` on the HUD.
  - **Level indicator**: Draws `Level: X/3`.
- On player reaching exit → play `win.wav`, transition to `LEVEL_TRANSITION`.
- On bot catching player → play `lose.wav`, transition to `GAMEOVER`.

#### Level Transition (`LEVEL_TRANSITION`)
- Displays "Level X Complete!" for ~2 seconds (frame counter at 60 FPS = 120 frames).
- On timer expiry:
  - If level < 3: increment level, call `generate_maze()` with new dimensions, reset positions, transition to `GAMEPLAY`.
  - If level == 3: show "You Win!" and transition to `GAMEOVER` with a win result.

#### Game Over (`GAMEOVER`)
- Displays win or lose message.
- "Press R to restart" → resets to `LOGIN`.

#### Level Dimensions
```c
const int LEVEL_ROWS[3] = {15, 21, 31};
const int LEVEL_COLS[3] = {15, 21, 31};
```

#### Audio
```c
InitAudioDevice();
Sound winSound = LoadSound("win.wav");
Sound loseSound = LoadSound("lose.wav");
// Play on appropriate events
// CloseAudioDevice() on shutdown
```

#### Emscripten Compatibility
```c
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// In main():
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) { UpdateDrawFrame(); }
#endif
```

All game state (player position, bot position, level, screen, name, sounds) will be stored in file-scope globals so `UpdateDrawFrame` can access them without parameters.

---

## Audio Files

> [!IMPORTANT]
> You will need to provide `win.wav` and `lose.wav` files in the project root directory. For Emscripten builds, these must be preloaded with `--preload-file`. I will **not** generate audio files — you should source or create these yourself. The code will call `LoadSound("win.wav")` and `LoadSound("lose.wav")`.

---

## File Change Summary

| File | Type | Key Changes |
|------|------|-------------|
| [maze.h](file:///c:/Users/swoja/C_PBL/maze.h) | MODIFY | `MAX_MAZE_SIZE`, runtime `g_maze_rows`/`g_maze_cols`, `generate_maze()` prototype |
| [maze.c](file:///c:/Users/swoja/C_PBL/maze.c) | MODIFY | Remove hardcoded maze, add Recursive Backtracker, dynamic cell sizing |
| [astar.h](file:///c:/Users/swoja/C_PBL/astar.h) | MODIFY | Fix `Node` struct syntax, use `MAX_MAZE_SIZE` |
| [astar.c](file:///c:/Users/swoja/C_PBL/astar.c) | MODIFY | Use `MAX_MAZE_SIZE` arrays, `g_maze_rows`/`g_maze_cols` bounds |
| [main.c](file:///c:/Users/swoja/C_PBL/main.c) | MODIFY | State machine, login page, 3 levels, audio, timestamp HUD, name display, `UpdateDrawFrame` |

---

## Verification Plan

### Build Test
```bash
# Native build (gcc + raylib)
gcc -Wall -Wextra -std=c99 -o maze_game main.c maze.c astar.c -lraylib -lm

# Emscripten build
emcc -o index.html main.c maze.c astar.c -I./raylib/include -L./raylib/lib -lraylib -s USE_GLFW=3 --preload-file win.wav --preload-file lose.wav
```

### Functional Checks
- [ ] Login screen renders, accepts keyboard input, name stored correctly
- [ ] Name and real-time clock displayed on gameplay HUD
- [ ] Level 1: 15×15 maze, Level 2: 21×21, Level 3: 31×31
- [ ] Maze layout is different every play and every level
- [ ] Bot pathfinds correctly on all 3 grid sizes
- [ ] win.wav plays on reaching exit, lose.wav on bot catch
- [ ] Level transition screen shows between levels
- [ ] Game over screen shows win/lose, R to restart works
- [ ] No blocking loops — Emscripten `UpdateDrawFrame` compatible
