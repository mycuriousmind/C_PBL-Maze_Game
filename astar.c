#include<stdio.h>
#include<stdlib.h>
#include "astar.h"

int heuristic(int row_a, int col_a, int row_b, int col_b) {
    return abs(row_a - row_b) + abs(col_a - col_b);
}

int astar_find_path(int start_row, int start_col, int end_row, int end_col, int *next_row, int *next_col) {

    Node all_nodes[MAZE_ROWS][MAZE_COLS];
    Node* open_list[OPEN_LIST];
    int open_count = 0;
    int list_state[MAZE_ROWS][MAZE_COLS];
    int r,c;
    int i;
    int best_idx;
    Node* current;
    int found_goal;
    int dr[4] = {-1,1,0,0};
    int dc[4] = {0,0,-1,1};

    for(r=0;r<MAZE_ROWS;r++) {
        for(c=0;c<MAZE_COLS;c++) {
            all_nodes[r][c].row    = r;
            all_nodes[r][c].col    = c;
            all_nodes[r][c].g = 999999;  /* "Infinity" */
            all_nodes[r][c].h = 0;
            all_nodes[r][c].f = 999999;
            all_nodes[r][c].parent = NULL;    /* No path found yet */
            list_state[r][c]       = ASTAR_NOT_IN_LIST;
        }
    }

    all_nodes[start_row][start_col].g = 0;
    all_nodes[start_row][start_col].h = heuristic(start_row, start_col, end_row, end_col);
    open_list[open_count] = &all_nodes[start_row][start_col];
    open_count++;
    list_state[start_row][start_col] = ASTAR_IN_OPEN;
    found_goal = 0;

    while(open_count>0 && !found_goal) {
        best_idx = 0;
        for(i=1;i<open_count;i++) {
            if(open_list[i]->f<open_list[best_idx]->f) {
                best_idx = i;
            }
        }
        current = open_list[best_idx];
        open_list[best_idx] = open_list[open_count-1];
        open_count--;
        list_state[current->row][current->col] = ASTAR_IN_CLOSED;
        if(current->row==end_row && current->col==end_col) {
            found_goal = 1;
            break;
        }
        for(i=0;i<4;i++) {
            int n_row = current->row + dr[i];
            int n_col = current->col + dc[i];
            if(is_wall(n_row,n_col)) {
                continue;
            }
            int tentative_g = current->g + 1;
            if(tentative_g<all_nodes[n_row][n_col].g) {
                all_nodes[n_row][n_col].parent = current;
                all_nodes[n_row][n_col].g = tentative_g;
                all_nodes[n_row][n_col].h = heuristic(n_row, n_col, end_row, end_col);
                all_nodes[n_row][n_col].f = all_nodes[n_row][n_col].g + all_nodes[n_row][n_col].h;
                if(list_state[n_row][n_col] != ASTAR_IN_OPEN) {
                    open_list[open_count] = &all_nodes[n_row][n_col];
                    open_count++;
                    list_state[n_row][n_col] = ASTAR_IN_OPEN;
                }
            }
        }
    }
    
    if(!found_goal) {
        *next_row = start_row;
        *next_col = start_col;
        return 0;
    }
    {
        Node* tracer = &all_nodes[end_row][end_col];
        while(tracer->parent!=NULL && !(tracer->parent->row==start_row && tracer->parent->col==start_col)) {
            tracer = tracer->parent;
        }
        if(tracer->parent==NULL) {
            *next_row = start_row;
            *next_col = start_col;
        }
        else {
            *next_row = tracer->row;
            *next_col = tracer->col;
        }
    }
    return 1;
}
