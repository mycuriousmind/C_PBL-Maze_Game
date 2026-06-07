#include "raylib.h"
#include "maze.h"

#define CELL_SIZE 40

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
    // Draw grid
    for (int i = 0; i < MAZE_ROWS; i++) {
        for (int j = 0; j < MAZE_COLS; j++) {
            int x = j * CELL_SIZE + 10;
            int y = i * CELL_SIZE + 80;
            
            if (maze[i][j] == TITLE_WALL) {
                // Draw wall
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, DARKGRAY);
                DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, BLACK);
            } else {
                // Draw path
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, WHITE);
                DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, LIGHTGRAY);
            }
        }
    }
    
    // Draw exit
    if (exit_row >= 0 && exit_col >= 0) {
        int x = exit_col * CELL_SIZE + 10;
        int y = exit_row * CELL_SIZE + 80;
        DrawRectangle(x + 5, y + 5, CELL_SIZE - 10, CELL_SIZE - 10, GOLD);
        DrawText("E", x + 15, y + 12, 16, BLACK);
    }
    
    // Draw bot
    if (bot_row >= 0 && bot_col >= 0) {
        int x = bot_col * CELL_SIZE + 10;
        int y = bot_row * CELL_SIZE + 80;
        DrawRectangle(x + 5, y + 5, CELL_SIZE - 10, CELL_SIZE - 10, RED);
        DrawText("B", x + 15, y + 12, 16, WHITE);
    }
    
    // Draw player
    if (player_row >= 0 && player_col >= 0) {
        int x = player_col * CELL_SIZE + 10;
        int y = player_row * CELL_SIZE + 80;
        DrawRectangle(x + 5, y + 5, CELL_SIZE - 10, CELL_SIZE - 10, BLUE);
        DrawText("P", x + 15, y + 12, 16, WHITE);
    }
}
