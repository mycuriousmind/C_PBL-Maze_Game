#ifndef MAZE_H
#define MAZE_H

#define MAZE_ROWS 15
#define MAZE_COLS 15

#define TITLE_PATH 0
#define TITLE_WALL 1

extern const int maze[MAZE_ROWS][MAZE_COLS];

void draw_maze(int player_row, int player_col, int bot_row, int bot_col, int exit_row, int exit_col);

int is_wall(int row, int col);

#endif