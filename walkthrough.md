# Maze Game Refactor — Walkthrough

## Overview

Refactored all 5 source files to satisfy the 7 teacher requirements while maintaining Emscripten compatibility. The game now uses a state machine, procedural maze generation, 3 difficulty levels, a login screen, audio, and an enhanced HUD.

---

## Files Changed

### [maze.h](file:///c:/Users/swoja/C_PBL/maze.h) — Dynamic Grid Support

- Replaced fixed `MAZE_ROWS`/`MAZE_COLS` (15) with `MAX_MAZE_SIZE` (31)
- Added runtime globals `g_maze_rows` / `g_maze_cols` (set by `generate_maze()`)
- Added `generate_maze(int rows, int cols)` prototype
- Renamed `TITLE_PATH`/`TITLE_WALL` → `TILE_PATH`/`TILE_WALL` (typo fix)

### [maze.c](file:///c:/Users/swoja/C_PBL/maze.c) — Procedural Generation

- **Removed** the entire hardcoded 15×15 `const int maze[][]` array
- **Added** `generate_maze()` implementing the **Recursive Backtracker** algorithm:
  - Iterative stack-based (no deep recursion risk on 31×31 grids)
  - Uses Raylib's `GetRandomValue()` for random neighbor selection
  - Guarantees connectivity from `(1,1)` to `(rows-2, cols-2)`
- **Updated** `is_wall()` to use `g_maze_rows`/`g_maze_cols` for bounds
- **Updated** `draw_maze()` with dynamic cell sizing:
  - Computes `cell_size` to fit any grid (15/21/31) within the 650×700 window
  - Scales text and padding proportionally

### [astar.h](file:///c:/Users/swoja/C_PBL/astar.h) — Fixed & Updated

- **Fixed** syntax bug: `struct *Node parent` → `struct Node *parent`
- Changed `OPEN_LIST` → `OPEN_LIST_MAX` = `MAX_MAZE_SIZE * MAX_MAZE_SIZE`

### [astar.c](file:///c:/Users/swoja/C_PBL/astar.c) — Dynamic Grid Bounds

- Arrays declared with `[MAX_MAZE_SIZE][MAX_MAZE_SIZE]`
- Loops iterate up to `g_maze_rows`/`g_maze_cols` (current level dims)
- Added `ASTAR_IN_CLOSED` skip to avoid re-expanding closed nodes (performance fix)
- Properly initializes `f = g + h` for the start node

### [main.c](file:///c:/Users/swoja/C_PBL/main.c) — State Machine & Features

**Architecture**: State machine enum `GameScreen { LOGIN, GAMEPLAY, GAMEOVER, LEVEL_TRANSITION }` inside a single `UpdateDrawFrame()` callback.

| Requirement | Implementation |
|---|---|
| **1. Timestamp** | `time()` + `localtime()` → `HH:MM:SS` displayed top-right of HUD |
| **2. Login Page** | `LOGIN` state with `GetCharPressed()` text input, ENTER to start |
| **3. Name Display** | `playerName` shown top-left of gameplay HUD |
| **4. 3 Levels** | `LEVEL_ROWS/COLS = {15, 21, 31}`, advance on reaching exit |
| **5. Audio** | `InitAudioDevice()`, `LoadSound("win.wav"/"lose.wav")`, played on win/lose events |
| **6-7. Procedural Gen** | `generate_maze()` called at each level init, new layout every time |

**Emscripten compatibility**:
```c
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) { UpdateDrawFrame(); }
#endif
```

**Game flow**: LOGIN → GAMEPLAY (Level 1) → LEVEL_TRANSITION → GAMEPLAY (Level 2) → LEVEL_TRANSITION → GAMEPLAY (Level 3) → LEVEL_TRANSITION → GAMEOVER (win). If caught at any point: GAMEPLAY → GAMEOVER (lose). Press R at GAMEOVER → back to LOGIN.

---

## Audio Files Required

> [!IMPORTANT]
> You need to provide `win.wav` and `lose.wav` in the project root. The code gracefully handles missing files (checks `FileExists()` before loading). For Emscripten builds, use `--preload-file win.wav --preload-file lose.wav`.

---

## Build Commands

**Native (gcc + Raylib)**:
```bash
gcc -Wall -Wextra -std=c99 -o maze_game main.c maze.c astar.c -lraylib -lm
```

**Emscripten**:
```bash
emcc -o index.html main.c maze.c astar.c -I./raylib/include -L./raylib/lib -lraylib -s USE_GLFW=3 --preload-file win.wav --preload-file lose.wav
```
