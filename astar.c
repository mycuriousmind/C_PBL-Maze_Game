#include <stdio.h>
#include <stdlib.h>
#include "astar.h"

int heuristic(int row_a, int col_a, int row_b, int col_b) {
    return abs(row_a - row_b) + abs(col_a - col_b);
}

int astar_find_path(int start_row, int start_col,
                    int end_row, int end_col,
                    int *next_row, int *next_col) {

    /*
     * Arrays sized to MAX_MAZE_SIZE but loops only iterate
     * up to g_maze_rows / g_maze_cols (the current level dims).
     */
    Node all_nodes[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
    Node *open_list[OPEN_LIST_MAX];
    int open_count = 0;
    int list_state[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
    int r, c, i, best_idx;
    Node *current;
    int found_goal;

    int dr[4] = {-1, 1, 0, 0};
    int dc[4] = { 0, 0,-1, 1};

    /* Initialize only the active portion of the grid */
    for (r = 0; r < g_maze_rows; r++) {
        for (c = 0; c < g_maze_cols; c++) {
            all_nodes[r][c].row    = r;
            all_nodes[r][c].col    = c;
            all_nodes[r][c].g      = 999999;
            all_nodes[r][c].h      = 0;
            all_nodes[r][c].f      = 999999;
            all_nodes[r][c].parent = NULL;
            list_state[r][c]       = ASTAR_NOT_IN_LIST;
        }
    }

    all_nodes[start_row][start_col].g = 0;
    all_nodes[start_row][start_col].h = heuristic(start_row, start_col, end_row, end_col);
    all_nodes[start_row][start_col].f = all_nodes[start_row][start_col].h;
    open_list[open_count] = &all_nodes[start_row][start_col];
    open_count++;
    list_state[start_row][start_col] = ASTAR_IN_OPEN;
    found_goal = 0;

    while (open_count > 0 && !found_goal) {
        /* Find node with lowest f in open list */
        best_idx = 0;
        for (i = 1; i < open_count; i++) {
            if (open_list[i]->f < open_list[best_idx]->f) {
                best_idx = i;
            }
        }

        current = open_list[best_idx];
        /* Remove from open list (swap with last) */
        open_list[best_idx] = open_list[open_count - 1];
        open_count--;
        list_state[current->row][current->col] = ASTAR_IN_CLOSED;

        if (current->row == end_row && current->col == end_col) {
            found_goal = 1;
            break;
        }

        /* Expand neighbors */
        for (i = 0; i < 4; i++) {
            int n_row = current->row + dr[i];
            int n_col = current->col + dc[i];

            /* is_wall() handles bounds checking */
            if (is_wall(n_row, n_col)) {
                continue;
            }
            if (list_state[n_row][n_col] == ASTAR_IN_CLOSED) {
                continue;
            }

            int tentative_g = current->g + 1;
            if (tentative_g < all_nodes[n_row][n_col].g) {
                all_nodes[n_row][n_col].parent = current;
                all_nodes[n_row][n_col].g = tentative_g;
                all_nodes[n_row][n_col].h = heuristic(n_row, n_col, end_row, end_col);
                all_nodes[n_row][n_col].f = tentative_g + all_nodes[n_row][n_col].h;

                if (list_state[n_row][n_col] != ASTAR_IN_OPEN) {
                    open_list[open_count] = &all_nodes[n_row][n_col];
                    open_count++;
                    list_state[n_row][n_col] = ASTAR_IN_OPEN;
                }
            }
        }
    }

    if (!found_goal) {
        *next_row = start_row;
        *next_col = start_col;
        return 0;
    }

    /* Trace back from goal to find the first step after start */
    {
        Node *tracer = &all_nodes[end_row][end_col];
        while (tracer->parent != NULL &&
               !(tracer->parent->row == start_row &&
                 tracer->parent->col == start_col)) {
            tracer = tracer->parent;
        }
        if (tracer->parent == NULL) {
            *next_row = start_row;
            *next_col = start_col;
        } else {
            *next_row = tracer->row;
            *next_col = tracer->col;
        }
    }
    return 1;
}
