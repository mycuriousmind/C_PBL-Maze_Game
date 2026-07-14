#include "maze.h"
#include "raylib.h"
#include <stdlib.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Globals                                                           */
/* ------------------------------------------------------------------ */

int g_maze_rows = 15;
int g_maze_cols = 15;
int maze[MAX_MAZE_SIZE][MAX_MAZE_SIZE];

/* ------------------------------------------------------------------ */
/*  is_wall — bounds-checked wall query                               */
/* ------------------------------------------------------------------ */

int is_wall(int row, int col) {
  if (row < 0 || row >= g_maze_rows || col < 0 || col >= g_maze_cols) {
    return 1;
  }
  return maze[row][col] == TILE_WALL;
}

/* ------------------------------------------------------------------ */
/*  remove_extra_walls — Knock out random interior walls to create     */
/*  loops (multiple paths). Only removes walls that separate two       */
/*  already-carved PATH cells, so connectivity is guaranteed.          */
/* ------------------------------------------------------------------ */

static void remove_extra_walls(int rows, int cols, int removal_percent) {
  /*
   * Eligible walls are interior cells (not on the border) where
   * removing the wall connects two existing path cells.
   * These occur at positions where one coordinate is even and
   * the other is odd (the "walls between" carved cells).
   */
  typedef struct {
    int r;
    int c;
  } WallPos;

  /* Upper bound on eligible walls */
  int max_eligible = rows * cols;
  WallPos *eligible = (WallPos *)malloc(sizeof(WallPos) * max_eligible);
  int count = 0;
  int r, c, i, j;

  for (r = 1; r < rows - 1; r++) {
    for (c = 1; c < cols - 1; c++) {
      if (maze[r][c] != TILE_WALL)
        continue;

      /*
       * Check if this wall separates two path cells:
       *   - Vertical wall (even row, odd col): check left/right
       *   - Horizontal wall (odd row, even col): check up/down
       */
      int connects = 0;
      if ((r % 2 == 0) && (c % 2 == 1)) {
        /* Horizontal wall between (r-1,c) and (r+1,c) */
        if (r - 1 >= 0 && r + 1 < rows && maze[r - 1][c] == TILE_PATH &&
            maze[r + 1][c] == TILE_PATH) {
          connects = 1;
        }
      } else if ((r % 2 == 1) && (c % 2 == 0)) {
        /* Vertical wall between (r,c-1) and (r,c+1) */
        if (c - 1 >= 0 && c + 1 < cols && maze[r][c - 1] == TILE_PATH &&
            maze[r][c + 1] == TILE_PATH) {
          connects = 1;
        }
      }

      if (connects) {
        eligible[count].r = r;
        eligible[count].c = c;
        count++;
      }
    }
  }

  /* Fisher-Yates shuffle the eligible walls */
  for (i = count - 1; i > 0; i--) {
    j = GetRandomValue(0, i);
    WallPos tmp = eligible[i];
    eligible[i] = eligible[j];
    eligible[j] = tmp;
  }

  /* Remove the first removal_percent% of eligible walls */
  int to_remove = (count * removal_percent) / 100;
  for (i = 0; i < to_remove; i++) {
    maze[eligible[i].r][eligible[i].c] = TILE_PATH;
  }

  free(eligible);
}

/* ------------------------------------------------------------------ */
/*  Procedural Maze Generation — Recursive Backtracker (iterative)    */
/*  with post-generation wall removal for multiple paths.             */
/* ------------------------------------------------------------------ */

/* A small struct for the carving stack */
typedef struct {
  int row;
  int col;
} Cell;

void generate_maze(int rows, int cols, int wall_removal_percent) {
  int r, c;

  g_maze_rows = rows;
  g_maze_cols = cols;

  /* 1. Fill with walls */
  for (r = 0; r < rows; r++) {
    for (c = 0; c < cols; c++) {
      maze[r][c] = TILE_WALL;
    }
  }

  /* Stack for iterative backtracking (max possible cells) */
  int max_cells = ((rows - 1) / 2) * ((cols - 1) / 2);
  Cell *stack = (Cell *)malloc(sizeof(Cell) * max_cells);
  int stack_top = 0;

  /* 2. Start at (1,1) */
  maze[1][1] = TILE_PATH;
  stack[stack_top].row = 1;
  stack[stack_top].col = 1;
  stack_top++;

  /* Direction offsets: up, down, left, right (step of 2) */
  int dr[4] = {-2, 2, 0, 0};
  int dc[4] = {0, 0, -2, 2};

  /* 3. Main carving loop */
  while (stack_top > 0) {
    Cell current = stack[stack_top - 1];

    /* Collect unvisited neighbors */
    int neighbors[4];
    int n_count = 0;
    int i;

    for (i = 0; i < 4; i++) {
      int nr = current.row + dr[i];
      int nc = current.col + dc[i];
      /* Check bounds (must be inside the border walls) */
      if (nr > 0 && nr < rows - 1 && nc > 0 && nc < cols - 1) {
        if (maze[nr][nc] == TILE_WALL) {
          neighbors[n_count] = i;
          n_count++;
        }
      }
    }

    if (n_count > 0) {
      /* Pick a random unvisited neighbor */
      int pick = neighbors[GetRandomValue(0, n_count - 1)];
      int nr = current.row + dr[pick];
      int nc = current.col + dc[pick];

      /* Carve the wall between current and the chosen neighbor */
      int wall_r = current.row + dr[pick] / 2;
      int wall_c = current.col + dc[pick] / 2;
      maze[wall_r][wall_c] = TILE_PATH;
      maze[nr][nc] = TILE_PATH;

      /* Push neighbor onto stack */
      stack[stack_top].row = nr;
      stack[stack_top].col = nc;
      stack_top++;
    } else {
      /* Backtrack */
      stack_top--;
    }
  }

  free(stack);

  /* 4. Guarantee start and exit are open */
  maze[1][1] = TILE_PATH;
  maze[rows - 2][cols - 2] = TILE_PATH;

  /* 5. Remove extra walls to create multiple paths (loops) */
  if (wall_removal_percent > 0) {
    remove_extra_walls(rows, cols, wall_removal_percent);
  }
}

/* ------------------------------------------------------------------ */
/*  draw_maze — Renders the maze scaled to fit the window             */
/* ------------------------------------------------------------------ */

void draw_maze(int player_row, int player_col, int bot_row, int bot_col,
               int exit_row, int exit_col) {
  /*
   * Compute cell size so the maze fits the current window,
   * leaving room for HUD at top (~85 px) and bottom (~35 px).
   */
  int winW = GetScreenWidth();
  int winH = GetScreenHeight();
  int usable_w = winW - 30;      /* 15 px padding each side */
  int usable_h = winH - 85 - 35; /* top HUD + bottom bar   */
  int cell_w = usable_w / g_maze_cols;
  int cell_h = usable_h / g_maze_rows;
  int cell_size = (cell_w < cell_h) ? cell_w : cell_h;

  /* Centering offsets */
  int offset_x = (winW - g_maze_cols * cell_size) / 2;
  int offset_y = 85;

  int i, j;

  /* Draw grid */
  for (i = 0; i < g_maze_rows; i++) {
    for (j = 0; j < g_maze_cols; j++) {
      int x = j * cell_size + offset_x;
      int y = i * cell_size + offset_y;

      if (maze[i][j] == TILE_WALL) {
        DrawRectangle(x, y, cell_size, cell_size, DARKGRAY);
        DrawRectangleLines(x, y, cell_size, cell_size, BLACK);
      } else {
        DrawRectangle(x, y, cell_size, cell_size, WHITE);
        DrawRectangleLines(x, y, cell_size, cell_size, LIGHTGRAY);
      }
    }
  }

  /* Draw exit */
  if (exit_row >= 0 && exit_col >= 0) {
    int x = exit_col * cell_size + offset_x;
    int y = exit_row * cell_size + offset_y;
    int pad = cell_size / 8;
    DrawRectangle(x + pad, y + pad, cell_size - 2 * pad, cell_size - 2 * pad,
                  GOLD);
    int font = cell_size / 3;
    if (font < 8)
      font = 8;
    DrawText("E", x + cell_size / 3, y + cell_size / 4, font, BLACK);
  }

  /* Draw bot */
  if (bot_row >= 0 && bot_col >= 0) {
    int x = bot_col * cell_size + offset_x;
    int y = bot_row * cell_size + offset_y;
    int pad = cell_size / 8;
    DrawRectangle(x + pad, y + pad, cell_size - 2 * pad, cell_size - 2 * pad,
                  RED);
    int font = cell_size / 3;
    if (font < 8)
      font = 8;
    DrawText("B", x + cell_size / 3, y + cell_size / 4, font, WHITE);
  }

  /* Draw player */
  if (player_row >= 0 && player_col >= 0) {
    int x = player_col * cell_size + offset_x;
    int y = player_row * cell_size + offset_y;
    int pad = cell_size / 8;
    DrawRectangle(x + pad, y + pad, cell_size - 2 * pad, cell_size - 2 * pad,
                  BLUE);
    int font = cell_size / 3;
    if (font < 8)
      font = 8;
    DrawText("P", x + cell_size / 3, y + cell_size / 4, font, WHITE);
  }
}