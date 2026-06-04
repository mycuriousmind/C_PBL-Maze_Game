#ifndef ASTAR_H
#define ASTAR_H

#include "maze.h"

#define ASTAR_NOT_IN_LIST 0
#define ASTAR_IN_OPEN 1
#define ASTAR_IN_CLOSED 2

#define OPEN_LIST (MAZE_ROWS * MAZE_COLS)

typedef struct Node {
    int row;
    int col;
    int g;
    int h;
    int f;
    struct *Node parent;
} Node;

int astar_find_path(int start_row, int start_col, int end_row, int end_col, int *next_row, int *next_col);

int heuristic(int row_a, int col_a, int row_b, int col_b);

#endif
