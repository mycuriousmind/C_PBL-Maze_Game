#ifndef MAZE_H
#define MAZE_H

/* Maximum grid size (largest level is 31x31) */
#define MAX_MAZE_SIZE 31

#define TILE_PATH 0
#define TILE_WALL 1

/* Current level dimensions — set at runtime by generate_maze() */
extern int g_maze_rows;
extern int g_maze_cols;

/* The maze grid. Only [0..g_maze_rows-1][0..g_maze_cols-1] is used. */
extern int maze[MAX_MAZE_SIZE][MAX_MAZE_SIZE];

/*
 * generate_maze  — Procedural maze generation (Recursive Backtracker)
 *                  with post-generation wall removal for multiple paths.
 *   rows, cols            — must be ODD numbers (15, 21, or 31).
 *   wall_removal_percent  — 0-100, percentage of eligible interior walls
 *                           to remove after generation, creating loops.
 *   Sets g_maze_rows/g_maze_cols and fills the maze[][] array.
 *   Guarantees a path from (1,1) to (rows-2, cols-2).
 */
void generate_maze(int rows, int cols, int wall_removal_percent);

/*
 * draw_maze — Renders the maze, player, bot, and exit using Raylib.
 *   Dynamically scales cell size to fit the current grid in the window.
 */
void draw_maze(int player_row, int player_col,
               int bot_row, int bot_col,
               int exit_row, int exit_col);

/*
 * is_wall — Returns 1 if (row, col) is a wall or out of bounds, 0 otherwise.
 */
int is_wall(int row, int col);

#endif