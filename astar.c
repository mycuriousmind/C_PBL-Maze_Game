#include <stdio.h>
#include "astar.h"

int heuristic(int row_a, int col_a, int row_b, int col_b) {
    return abs(row_a - row_b) + abs(col_a - col_b);
}

int astar_find_next_step(int start_row, int start_col,
                         int goal_row,  int goal_col,
                         int *next_row, int *next_col)
{
    Node all_nodes[MAZE_ROWS][MAZE_COLS];

    /* The open list: an array of POINTERS to nodes.
     * We store pointers (not copies) so modifying a node
     * in all_nodes is immediately reflected in the open list. */
    Node* open_list[OPEN_LIST_MAX];

    /* Tracks how many items are currently in the open list. */
    int open_count = 0;

    /* 2D array tracking each tile's list membership status.
     * Values: ASTAR_NOT_IN_LIST, ASTAR_IN_OPEN, ASTAR_IN_CLOSED */
    int list_state[MAZE_ROWS][MAZE_COLS];

    /* Loop variables — declared at top per C99 style */
    int r, c;               /* For initialisation loops          */
    int i;                  /* General purpose loop counter       */
    int best_idx;           /* Index of lowest-f node in open list*/
    Node* current;          /* The node we are currently expanding*/
    int found_goal;         /* Flag: did we reach the goal?       */

    /* The 4 possible movement directions: Up, Down, Left, Right.
     * Each pair {dr, dc} is a (row_delta, col_delta).
     * Adding these to a position gives us a neighbour tile.
     *   Up:    row-1, col+0
     *   Down:  row+1, col+0
     *   Left:  row+0, col-1
     *   Right: row+0, col+1 */
    int dr[4] = {-1,  1,  0,  0};
    int dc[4] = { 0,  0, -1,  1};

    /* --------------------------------------------------
     * STEP 2: INITIALISE THE NODE POOL AND LIST STATES
     *
     * Set every tile to a known default state.
     * g_cost = a very large number means "not yet reached".
     * -------------------------------------------------- */
    for (r = 0; r < MAZE_ROWS; r++) {
        for (c = 0; c < MAZE_COLS; c++) {
            all_nodes[r][c].row    = r;
            all_nodes[r][c].col    = c;
            all_nodes[r][c].g_cost = 999999;  /* "Infinity" */
            all_nodes[r][c].h_cost = 0;
            all_nodes[r][c].f_cost = 999999;
            all_nodes[r][c].parent = NULL;    /* No path found yet */
            list_state[r][c]       = ASTAR_NOT_IN_LIST;
        }
    }

    /* --------------------------------------------------
     * STEP 3: SEED THE OPEN LIST WITH THE START NODE
     *
     * The start node costs 0 to reach (g=0) and its f_cost
     * is purely its heuristic estimate to the goal.
     * -------------------------------------------------- */
    all_nodes[start_row][start_col].g_cost = 0;
    all_nodes[start_row][start_col].h_cost = heuristic(start_row, start_col,
                                                        goal_row,  goal_col);
    all_nodes[start_row][start_col].f_cost = all_nodes[start_row][start_col].h_cost;

    /* Add the start node pointer to the open list */
    open_list[open_count] = &all_nodes[start_row][start_col];
    open_count++;
    list_state[start_row][start_col] = ASTAR_IN_OPEN;

    found_goal = 0; /* We haven't found the goal yet */

    /* --------------------------------------------------
     * STEP 4: THE MAIN A* LOOP
     *
     * Keep processing until the open list is empty (no path
     * exists) or we reach the goal tile.
     * -------------------------------------------------- */
    while (open_count > 0 && !found_goal) {

        /* ---- 4a: Find the node with the LOWEST f_cost ----
         *
         * A priority queue would do this in O(log n), but a
         * linear scan is simple and fast enough for 15x15. */
        best_idx = 0;  /* Assume the first item is best */
        for (i = 1; i < open_count; i++) {
            if (open_list[i]->f_cost < open_list[best_idx]->f_cost) {
                best_idx = i;
            }
        }

        /* ---- 4b: Remove the best node from the open list ----
         *
         * We grab the pointer to the best node, then fill its
         * slot in the array with the last element (order doesn't
         * matter in the open list — only f_cost does).
         * Then we shrink the count by 1. */
        current = open_list[best_idx];
        open_list[best_idx] = open_list[open_count - 1];
        open_count--;

        /* Mark this node as CLOSED — we won't revisit it */
        list_state[current->row][current->col] = ASTAR_IN_CLOSED;

        /* ---- 4c: Check if we've reached the GOAL ---- */
        if (current->row == goal_row && current->col == goal_col) {
            found_goal = 1; /* Set flag and exit the while loop */
            break;
        }

        /* ---- 4d: Expand all 4 NEIGHBOURS of current ---- */
        for (i = 0; i < 4; i++) {
            int n_row = current->row + dr[i];  /* Neighbour row */
            int n_col = current->col + dc[i];  /* Neighbour col */

            /* Skip if this neighbour is a wall or out of bounds */
            if (is_wall(n_row, n_col)) {
                continue; /* 'continue' jumps to next loop iteration */
            }

            /* Skip if we've already fully processed this tile */
            if (list_state[n_row][n_col] == ASTAR_IN_CLOSED) {
                continue;
            }

            /* Calculate the tentative g_cost to reach this neighbour
             * via the current node. Each step costs 1. */
            int tentative_g = current->g_cost + 1;

            /* Is this a BETTER path to this neighbour than any
             * previously found path? */
            if (tentative_g < all_nodes[n_row][n_col].g) {

                all_nodes[n_row][n_col].parent = current; 
                all_nodes[n_row][n_col].g = tentative_g;
                all_nodes[n_row][n_col].h = heuristic(n_row, n_col,
                                                            goal_row, goal_col);
                all_nodes[n_row][n_col].f = tentative_g
                                               + all_nodes[n_row][n_col].h;

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

    {
        Node* tracer = &all_nodes[goal_row][goal_col];
        while (tracer->parent != NULL &&
               !(tracer->parent->row == start_row &&
                 tracer->parent->col == start_col))
        {
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
