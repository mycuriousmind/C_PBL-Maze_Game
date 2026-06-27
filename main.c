/*
 * main.c — Maze Escape Game
 *
 * State machine architecture compatible with Emscripten's
 * emscripten_set_main_loop (no blocking while loops).
 *
 * States: LOGIN → GAMEPLAY → LEVEL_TRANSITION → GAMEPLAY → ... → GAMEOVER
 *
 * Requirements:
 *   1. Timestamp (real-time clock on HUD)
 *   2. Login page (name input)
 *   3. Name display on HUD
 *   4. 3 levels of increasing complexity (15x15, 21x21, 31x31)
 *   5. Audio (win.wav on level beat, lose.wav on bot catch)
 *   6-7. Procedural maze generation (new layout every time)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "raylib.h"
#include "maze.h"
#include "astar.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* ------------------------------------------------------------------ */
/*  Constants                                                         */
/* ------------------------------------------------------------------ */

/* Window dimensions — computed at runtime as 2/3 of monitor size */
static int WINDOW_WIDTH  = 650;  /* fallback */
static int WINDOW_HEIGHT = 700;  /* fallback */

#define MAX_NAME_LEN  31
#define TRANSITION_FRAMES 120  /* ~2 seconds at 60 FPS */
#define NUM_LEVELS 3

static const int LEVEL_ROWS[NUM_LEVELS] = {15, 21, 31};
static const int LEVEL_COLS[NUM_LEVELS] = {15, 21, 31};

/* ------------------------------------------------------------------ */
/*  Game State Machine                                                */
/* ------------------------------------------------------------------ */

typedef enum {
    LOGIN,
    GAMEPLAY,
    GAMEOVER,
    LEVEL_TRANSITION
} GameScreen;

/* ------------------------------------------------------------------ */
/*  Game State (file-scope globals for UpdateDrawFrame access)        */
/* ------------------------------------------------------------------ */

static GameScreen currentScreen = LOGIN;

/* Player name input */
static char playerName[MAX_NAME_LEN + 1] = {0};
static int  nameLen = 0;

/* Level tracking */
static int currentLevel = 0;  /* 0-indexed: 0, 1, 2 */

/* Gameplay state */
static int player_row, player_col;
static int bot_row, bot_col;
static int exit_row, exit_col;
static int turn_count;
static int gameResult;  /* 1 = win (escaped), -1 = lose (caught) */
static int bot_move_interval;  /* bot moves once every N player turns */

/* Transition timer */
static int transitionTimer = 0;

/* Game elapsed timer — records GetTime() when gameplay starts */
static double gameStartTime = 0.0;

/* Audio */
static Sound winSound;
static Sound loseSound;
static int audioLoaded = 0;

/* ------------------------------------------------------------------ */
/*  Helper: Initialize a level                                        */
/* ------------------------------------------------------------------ */

static void init_level(int level) {
    int rows = LEVEL_ROWS[level];
    int cols = LEVEL_COLS[level];

    /*
     * Wall removal percentage — controls how many extra walls are
     * knocked out after the base maze is carved, creating loops
     * (multiple paths).
     *   Level 1 (15x15): 35% → very open, many alternate routes
     *   Level 2 (21x21): 25% → moderate branching
     *   Level 3 (31x31): 15% → tight corridors, fewer shortcuts
     */
    int removal_pct;
    if (level == 0)      removal_pct = 35;
    else if (level == 1) removal_pct = 25;
    else                 removal_pct = 15;

    generate_maze(rows, cols, removal_pct);

    /* Player starts top-left, exit is bottom-right */
    player_row = 1;
    player_col = 1;
    exit_row   = rows - 2;
    exit_col   = cols - 2;

    /* Bot starts near the center of the maze — NOT on the exit */
    bot_row = (rows / 2) | 1;  /* ensure odd index (valid path cell) */
    bot_col = (cols / 2) | 1;

    /* If the bot's starting cell is a wall, nudge it to the nearest path cell */
    if (is_wall(bot_row, bot_col)) {
        int r, c, found = 0;
        for (r = bot_row; r < rows - 1 && !found; r += 2) {
            for (c = 1; c < cols - 1 && !found; c += 2) {
                if (!is_wall(r, c) && !(r == 1 && c == 1)) {
                    bot_row = r;
                    bot_col = c;
                    found = 1;
                }
            }
        }
    }

    turn_count = 1;
    gameResult = 0;

    /*
     * Bot speed scales with difficulty:
     *   Level 1 (15x15): bot moves every 3rd turn  (slow — forgiving)
     *   Level 2 (21x21): bot moves every 2nd turn  (medium)
     *   Level 3 (31x31): bot moves every 1nd turn  (hard — big maze compensates)
     */
    if (level == 0)
        bot_move_interval = 3;
    else if(level==1) 
        bot_move_interval = 2;
    else
        bot_move_interval = 1;
}

/* ------------------------------------------------------------------ */
/*  Input handling                                                     */
/* ------------------------------------------------------------------ */

static char get_player_input(void) {
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    return 'w';
    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))   return 'a';
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))   return 's';
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))  return 'd';
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Game Logic                                                         */
/* ------------------------------------------------------------------ */

static void move_player(char direction) {
    int new_row = player_row;
    int new_col = player_col;

    if (direction == 'w') new_row--;
    else if (direction == 's') new_row++;
    else if (direction == 'a') new_col--;
    else if (direction == 'd') new_col++;

    if (!is_wall(new_row, new_col)) {
        player_row = new_row;
        player_col = new_col;
    }
}

static void move_bot(void) {
    int next_row, next_col;
    if (astar_find_path(bot_row, bot_col, player_row, player_col,
                        &next_row, &next_col)) {
        bot_row = next_row;
        bot_col = next_col;
    }
}

/*
 * Returns:  1 = player reached exit (win)
 *          -1 = bot caught player (lose)
 *           0 = game continues
 */
static int check_game_over(void) {
    if (bot_row == player_row && bot_col == player_col) {
        return -1;
    }
    if (player_row == exit_row && player_col == exit_col) {
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  HUD Drawing                                                        */
/* ------------------------------------------------------------------ */

static void draw_hud(void) {
    char buf[128];
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    /* Elapsed game timer (resets each new game) */
    double elapsed = GetTime() - gameStartTime;
    int total_sec = (int)elapsed;
    int mins = total_sec / 60;
    int secs = total_sec % 60;
    snprintf(buf, sizeof(buf), "Time: %02d:%02d", mins, secs);
    DrawText(buf, sw - 140, 15, 20, DARKGRAY);

    /* Player name */
    snprintf(buf, sizeof(buf), "Player: %s", playerName);
    DrawText(buf, 20, 15, 20, DARKBLUE);

    /* Level and Turn */
    snprintf(buf, sizeof(buf), "Level: %d/%d   Turn: %d",
             currentLevel + 1, NUM_LEVELS, turn_count);
    DrawText(buf, 20, 45, 18, BLACK);

    /* Grid size indicator */
    snprintf(buf, sizeof(buf), "Grid: %dx%d", g_maze_rows, g_maze_cols);
    DrawText(buf, sw - 130, 45, 18, GRAY);

    /* Controls */
    DrawText("WASD / Arrow Keys = Move    Q = Quit", 20, sh - 25, 14, GRAY);
}

/* ------------------------------------------------------------------ */
/*  Screen Drawing Functions                                          */
/* ------------------------------------------------------------------ */

static void draw_login_screen(void) {
    ClearBackground(RAYWHITE);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;  /* horizontal center */

    /* Title */
    const char *title = "MAZE ESCAPE";
    DrawText(title, cx - MeasureText(title, 50) / 2, sh / 6, 50, DARKBLUE);
    const char *sub = "The Bot Is Watching...";
    DrawText(sub, cx - MeasureText(sub, 20) / 2, sh / 6 + 60, 20, GRAY);

    /* Name input box */
    int boxW = 300;
    int boxX = cx - boxW / 2;
    int boxY = sh * 2 / 5;
    const char *prompt = "Enter your name:";
    DrawText(prompt, cx - MeasureText(prompt, 22) / 2, boxY - 40, 22, BLACK);

    Rectangle inputBox = {(float)boxX, (float)boxY, (float)boxW, 45};
    DrawRectangleRec(inputBox, WHITE);
    DrawRectangleLinesEx(inputBox, 2, DARKBLUE);
    DrawText(playerName, boxX + 10, boxY + 12, 24, BLACK);

    /* Blinking cursor */
    if (((int)(GetTime() * 2) % 2) == 0) {
        int textW = MeasureText(playerName, 24);
        DrawText("_", boxX + 10 + textW, boxY + 12, 24, DARKBLUE);
    }

    /* Instructions */
    if (nameLen > 0) {
        const char *go = "Press ENTER to start!";
        DrawText(go, cx - MeasureText(go, 20) / 2, boxY + 80, 20, DARKGREEN);
    } else {
        const char *hint = "Type your name to begin";
        DrawText(hint, cx - MeasureText(hint, 20) / 2, boxY + 80, 20, GRAY);
    }

    const char *desc1 = "Escape the maze before the bot catches you!";
    const char *desc2 = "3 levels of increasing difficulty await...";
    DrawText(desc1, cx - MeasureText(desc1, 18) / 2, sh * 3 / 4, 18, GRAY);
    DrawText(desc2, cx - MeasureText(desc2, 18) / 2, sh * 3 / 4 + 30, 18, GRAY);
}

static void draw_gameplay_screen(void) {
    ClearBackground(LIGHTGRAY);
    draw_maze(player_row, player_col, bot_row, bot_col, exit_row, exit_col);
    draw_hud();
}

static void draw_transition_screen(void) {
    ClearBackground(RAYWHITE);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    char buf[128];

    if (currentLevel + 1 < NUM_LEVELS) {
        snprintf(buf, sizeof(buf), "Level %d Complete!", currentLevel + 1);
        DrawText(buf, cx - MeasureText(buf, 40) / 2, sh / 3, 40, DARKGREEN);

        snprintf(buf, sizeof(buf), "Get ready for Level %d...", currentLevel + 2);
        DrawText(buf, cx - MeasureText(buf, 24) / 2, sh / 3 + 70, 24, GRAY);

        snprintf(buf, sizeof(buf), "Grid: %dx%d",
                 LEVEL_ROWS[currentLevel + 1], LEVEL_COLS[currentLevel + 1]);
        DrawText(buf, cx - MeasureText(buf, 22) / 2, sh / 3 + 120, 22, DARKBLUE);
    } else {
        /* Beat all 3 levels */
        DrawText("YOU WIN!", cx - MeasureText("YOU WIN!", 50) / 2, sh / 3, 50, GOLD);

        snprintf(buf, sizeof(buf), "Congratulations, %s!", playerName);
        DrawText(buf, cx - MeasureText(buf, 28) / 2, sh / 3 + 80, 28, DARKGREEN);
        const char *esc = "You escaped all 3 mazes!";
        DrawText(esc, cx - MeasureText(esc, 22) / 2, sh / 3 + 130, 22, GRAY);
    }

    /* Progress bar */
    int barW = 300;
    int bar_elapsed = TRANSITION_FRAMES - transitionTimer;
    int filled = (bar_elapsed * barW) / TRANSITION_FRAMES;
    DrawRectangle(cx - barW / 2, sh * 2 / 3, barW, 12, LIGHTGRAY);
    DrawRectangle(cx - barW / 2, sh * 2 / 3, filled, 12, DARKGREEN);
}

static void draw_gameover_screen(void) {
    ClearBackground((Color){30, 30, 30, 255});
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int cx = sw / 2;

    char buf[128];

    if (gameResult == 1) {
        /* Won all levels */
        DrawText("VICTORY!", cx - MeasureText("VICTORY!", 50) / 2, sh / 4, 50, GOLD);
        snprintf(buf, sizeof(buf), "Well done, %s!", playerName);
        DrawText(buf, cx - MeasureText(buf, 28) / 2, sh / 4 + 80, 28, GREEN);
        const char *msg = "You conquered all 3 mazes!";
        DrawText(msg, cx - MeasureText(msg, 22) / 2, sh / 4 + 130, 22, WHITE);
    } else {
        /* Caught by bot */
        DrawText("GAME OVER", cx - MeasureText("GAME OVER", 50) / 2, sh / 4, 50, RED);
        snprintf(buf, sizeof(buf), "The bot caught you, %s!", playerName);
        DrawText(buf, cx - MeasureText(buf, 24) / 2, sh / 4 + 80, 24, (Color){255, 100, 100, 255});

        snprintf(buf, sizeof(buf), "Caught on Level %d, Turn %d",
                 currentLevel + 1, turn_count);
        DrawText(buf, cx - MeasureText(buf, 20) / 2, sh / 4 + 130, 20, GRAY);
    }

    const char *restart = "Press R to Restart";
    DrawText(restart, cx - MeasureText(restart, 22) / 2, sh * 2 / 3, 22, WHITE);
    const char *quit = "Press ESC to Exit";
    DrawText(quit, cx - MeasureText(quit, 20) / 2, sh * 2 / 3 + 40, 20, GRAY);
}

/* ------------------------------------------------------------------ */
/*  UpdateDrawFrame — called every frame (Emscripten-compatible)      */
/* ------------------------------------------------------------------ */

void UpdateDrawFrame(void) {
    /* ---- Global quit ---- */
    if (currentScreen != LOGIN && IsKeyPressed(KEY_Q)) {
        currentScreen = GAMEOVER;
        if (gameResult == 0) gameResult = -1;
    }

    /* ---- State machine ---- */
    switch (currentScreen) {

    case LOGIN: {
        /* Handle text input */
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 125 && nameLen < MAX_NAME_LEN) {
                playerName[nameLen] = (char)key;
                nameLen++;
                playerName[nameLen] = '\0';
            }
            key = GetCharPressed();
        }
        /* Backspace */
        if (IsKeyPressed(KEY_BACKSPACE) && nameLen > 0) {
            nameLen--;
            playerName[nameLen] = '\0';
        }
        /* Enter to start */
        if (IsKeyPressed(KEY_ENTER) && nameLen > 0) {
            currentLevel = 0;
            init_level(currentLevel);
            gameStartTime = GetTime();  /* reset timer for new game */
            currentScreen = GAMEPLAY;
        }

        BeginDrawing();
        draw_login_screen();
        EndDrawing();
        break;
    }

    case GAMEPLAY: {
        char input = get_player_input();
        if (input != 0) {
            move_player(input);

            int result = check_game_over();
            if (result == 1) {
                /* Player reached exit — level beaten */
                if (audioLoaded) PlaySound(winSound);
                gameResult = 1;
                transitionTimer = TRANSITION_FRAMES;
                currentScreen = LEVEL_TRANSITION;
            } else if (result == -1) {
                /* Bot already on player (shouldn't happen after player move, but safe) */
                if (audioLoaded) PlaySound(loseSound);
                gameResult = -1;
                currentScreen = GAMEOVER;
            } else {
                /* Bot moves only on certain turns */
                if (turn_count % bot_move_interval == 0) {
                    move_bot();
                    result = check_game_over();
                    if (result == -1) {
                        if (audioLoaded) PlaySound(loseSound);
                        gameResult = -1;
                        currentScreen = GAMEOVER;
                    }
                }
                turn_count++;
            }
        }

        BeginDrawing();
        draw_gameplay_screen();
        EndDrawing();
        break;
    }

    case LEVEL_TRANSITION: {
        transitionTimer--;
        if (transitionTimer <= 0) {
            if (currentLevel + 1 < NUM_LEVELS) {
                /* Advance to next level */
                currentLevel++;
                init_level(currentLevel);
                currentScreen = GAMEPLAY;
            } else {
                /* All levels complete — final win */
                gameResult = 1;
                currentScreen = GAMEOVER;
            }
        }

        BeginDrawing();
        draw_transition_screen();
        EndDrawing();
        break;
    }

    case GAMEOVER: {
        /* R to restart */
        if (IsKeyPressed(KEY_R)) {
            /* Reset everything back to login */
            playerName[0] = '\0';
            nameLen = 0;
            currentLevel = 0;
            gameResult = 0;
            gameStartTime = 0.0;  /* will be set fresh on next ENTER */
            currentScreen = LOGIN;
        }

        BeginDrawing();
        draw_gameover_screen();
        EndDrawing();
        break;
    }

    } /* end switch */
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int main(void) {
    /* --- Determine 2/3 of the monitor size for the window --- */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(650, 700, "Maze Escape Game");  /* temp size to query monitor */

    int monitor = GetCurrentMonitor();
    int monW = GetMonitorWidth(monitor);
    int monH = GetMonitorHeight(monitor);
    WINDOW_WIDTH  = (monW * 2) / 3;
    WINDOW_HEIGHT = (monH * 2) / 3;
    SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetWindowPosition((monW - WINDOW_WIDTH) / 2, (monH - WINDOW_HEIGHT) / 2);

    InitAudioDevice();

    /* Load audio — gracefully handle missing files */
    if (FileExists("win.wav")) {
        winSound = LoadSound("win.wav");
        audioLoaded = 1;
    }
    if (FileExists("lose.wav")) {
        loseSound = LoadSound("lose.wav");
        audioLoaded = 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    /* Cleanup */
    if (audioLoaded) {
        if (IsSoundValid(winSound))  UnloadSound(winSound);
        if (IsSoundValid(loseSound)) UnloadSound(loseSound);
    }
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
