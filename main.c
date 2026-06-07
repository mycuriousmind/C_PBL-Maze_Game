/*$env:Path = "C:\emsdk;C:\emsdk\upstream\emscripten;" + $env:Path
emcc --version
This is for setting the path manually(for just the current session)*/

#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include "raylib.h"
#include "maze.h"
#include "astar.h"

#define WINDOW_WIDTH 650
#define WINDOW_HEIGHT 700

//SECTION 2: Input handling

char get_player_input(void) {
    if(IsKeyPressed(KEY_W)) return 'w';
    if(IsKeyPressed(KEY_A)) return 'a';
    if(IsKeyPressed(KEY_S)) return 's';
    if(IsKeyPressed(KEY_D)) return 'd';
    return 0;
}

//SECTION 3: Game state struct

typedef struct {
    int player_row;
    int player_col;
    int bot_row;
    int bot_col;
    int exit_row;
    int exit_col;
    int turn;
    int game_over;
}GameState;

//SECTION 4: Game Logic

void move_player(GameState *state, char direction) {
    int new_row = state->player_row;
    int new_col = state->player_col;
    if(direction=='w') new_row--;
    else if(direction=='s') new_row++;
    else if(direction=='a') new_col--;
    else if(direction=='d') new_col++;
    if(!is_wall(new_row,new_col)) {
        state->player_row = new_row;
        state->player_col = new_col;
    }
}

void move_bot(GameState *state) {
    int next_row, next_col;
    int path_found = astar_find_path(state->bot_row, state->bot_col, state->player_row, state->player_col, &next_row, &next_col);
    if(path_found) {
        state->bot_row = next_row;
        state->bot_col = next_col;
    }
}

void print_hud(GameState *state) {
    char turn_text[100];
    char player_text[100];
    char controls_text[200];
    
    snprintf(turn_text, sizeof(turn_text), "Turn: %-4d", state->turn);
    snprintf(player_text, sizeof(player_text), "Player: (%d,%d)  Bot: (%d,%d)", 
             state->player_row, state->player_col, state->bot_row, state->bot_col);
    snprintf(controls_text, sizeof(controls_text), "Controls: W=Up  S=Down  A=Left  D=Right  Q=Quit");
    
    DrawText(turn_text, 20, 20, 20, BLACK);
    DrawText(player_text, 20, 50, 16, DARKBLUE);
    DrawText(controls_text, 20, 670, 14, GRAY);
}

int check_game_over(GameState *state) {
    if(state->bot_row==state->player_row && state->bot_col==state->player_col) {
        printf("Game Over! The bot caught you in %d turns.\n", state->turn);
        return -1;
    }
    if(state->player_row==state->exit_row && state->player_col==state->exit_col) {
        printf("Congratulations! You escaped the maze in %d turns.\n", state->turn);
        return 1;
    }
    return 0;
}

void print_game_over_screen(int result) {
    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, (Color){200, 200, 200, 200});
    
    if(result == 1) {
        DrawText("Congratulations!", 150, 200, 40, DARKGREEN);
        DrawText("You escaped the maze!", 120, 280, 30, DARKGREEN);
    } else {
        DrawText("Game Over!", 180, 200, 40, RED);
        DrawText("The bot caught you!", 140, 280, 30, RED);
    }
    
    DrawText("Press ESC to exit", 180, 400, 20, GRAY);
}

//SECTION 5: Main game loop

int main(void) {
    // Initialize Raylib window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Maze Escape Game");
    SetTargetFPS(10);  // 10 FPS for game logic
    
    GameState state;
    state.player_row = 1;
    state.player_col = 1;
    state.bot_row = MAZE_ROWS - 2;
    state.bot_col = MAZE_COLS - 2;
    state.exit_row = 7;
    state.exit_col = 13;
    state.turn = 1;
    state.game_over = 0;

    // Main game loop
    while (!WindowShouldClose()) {
        // Check for quit key
        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        
        // Get player input
        char input = get_player_input();
        
        // Process input
        if (input != 0) {
            move_player(&state, input);
            
            // Check if game is over after player moves
            int result = check_game_over(&state);
            if (result != 0) {
                state.game_over = 1;
            } else {
                // Bot moves if game not over
                move_bot(&state);
                
                // Check if bot caught player
                result = check_game_over(&state);
                if (result != 0) {
                    state.game_over = 1;
                }
                
                state.turn++;
            }
        }
        
        // Draw
        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        
        if (state.game_over) {
            int result = (state.bot_row == state.player_row && state.bot_col == state.player_col) ? -1 : 1;
            print_game_over_screen(result);
        } else {
            draw_maze(state.player_row, state.player_col, state.bot_row, state.bot_col, state.exit_row, state.exit_col);
            print_hud(&state);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
