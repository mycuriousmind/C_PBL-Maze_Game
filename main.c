#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include "maze.h"
#include "astar.h"

//SECTION 1: Terminal handling for raw mode

#ifdef _WIN32
    #include<conio.h>
#else
    #include<termios.h>
    #include<unistd.h>
    static struct termios original_termios;
#endif

void enable_raw_mode(void) {
#ifndef _WIN32
    struct termios raw;
    tcgetattr(STDIN_FILENO, &original_termios);
    raw = original_termios;
    raw.c_lflag &= ~(ICANON| ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

void restore_terminal(void) {
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
#endif
}

//SECTION 2: Game state struct

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

//SECTION 3: Game Logic

char get_player_input(void) {
    char ch;
#ifdef _WIN32
    ch = (char)_getch();
#else
    ch = (char)getchar();
#endif
    ch = (char)tolower((unsigned char)ch);
    if(ch=='w'||ch=='a'||ch=='s'||ch=='d') {
        return ch;
    }
    return 0;
}

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
    printf(" Turn: %-4d Player: (%d,%d) Bot: (%d,%d)\n",state->turn, state->player_row, state->player_col, state->bot_row, state->bot_col);
    printf("Controls: W=Up S=Down A=Left D=Right\n");
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
    printf("\033[H\033[J");
    printf("\n\n");
    if(result==1) {
        printf("You successfully escaped the maze!\n");
    } 
    else {
        printf("You failed!\n");
    }
}

//SECTION 4: Main game loop

int main(void) {
    GameState state;
    state.player_row = 1;
    state.player_col = 1;
    state.bot_row = MAZE_ROWS-2;
    state.bot_col = MAZE_COLS-2;
    state.exit_row = 7;
    state.exit_col = 13;
    state.turn = 1;
    state.game_over = 0;

    atexit(restore_terminal);
    enable_raw_mode();

    while(!state.game_over) {
        draw_maze(state.player_row, state.player_col, state.bot_row, state.bot_col, state.exit_row, state.exit_col);
        print_hud(&state);
        char input = get_player_input();
        if(input=='q') {
            printf("Quitting game. Thanks for playing!\n");
            break;
        }
        if(input==0) {
            continue;
        }
        move_player(&state, input);

        {
            int result = check_game_over(&state);
            if(result!=0) {
                draw_maze(state.player_row, state.player_col, state.bot_row, state.bot_col, state.exit_row, state.exit_col);
                print_game_over_screen(result);
                state.game_over = 1;
                break;
            }
        }

        state.turn++;
    }

    return 0;
}
