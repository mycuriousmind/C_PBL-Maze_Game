#include <stdio.h>
#include "maze.h"

const int maze[MAZE_ROWS][MAZE_COLS] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
    { 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 },
    { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
    { 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

int is_wall(int rows, int cols) {
    if(rows<0||rows>=MAZE_ROWS||cols<0||cols>=MAZE_COLS) {
        return 1;
    }
    return maze[rows][cols] == TITLE_WALL;
}

void draw_maze(int player_row, int player_col, int bot_row, int bot_col, int exit_row, int exit_col) {
    for (int i = 0; i < MAZE_ROWS; i++) {
        for (int j = 0; j < MAZE_COLS; j++) {
            if (i == player_row && j == player_col) {
                printf("P ");
            } else if (i == bot_row && j == bot_col) {
                printf("B ");
            } else if (i == exit_row && j == exit_col) {
                printf("E ");
            } else if (maze[i][j] == TITLE_WALL) {
                printf("# ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }printf("\n");
}