// cs_caverun.c
// Written by <Danny Sun> <z5691331> on <13/03/2025>
//
// Description: <A C recreation of the classic game Boulder Run>

/*
Overview: The game has two phases, a setup phase and a gameplay phase. In the 
setup phase, the player enters the starting position of their character, as 
well as the position of other entities on the map such as boulders, gems
and exits. In the gameplay phase, the player manoeuvers their character to 
collect gems and open the exits to win the game. Boulders and lava will try to
kill the player, and if the player is hit, they lose a life and are respawned 
at their original position. Other features include printing the player's 
current score, map statistics, switching the direction of gravity for boulders
and illumination/shadow mode which hides sections of the map to simulate a 
realistic cave experience. 
*/

// Provided Libraries
#include <stdio.h>
#include <stdlib.h>

// Add your own #include statements below this line

#include <ctype.h>
#include <math.h>

// Provided constants

#define COLS 10
#define ROWS 10
#define INVALID_ROW -1
#define INVALID_COL -1
#define INITIAL_LIVES 3

// Add your own #defines constants below this line

#define FALSE                 0
#define TRUE                  1

#define UP_SINGLE            'w'
#define DOWN_SINGLE          's'
#define LEFT_SINGLE          'a'
#define RIGHT_SINGLE         'd'
#define UP_DASH              'W'
#define DOWN_DASH            'S'
#define LEFT_DASH            'A'
#define RIGHT_DASH           'D'

#define ILLUMINATE           'i'
#define SHADOW               'u'
#define GRAVITY              'g'
#define QUIT                 'q'
#define PRINT_SCORE          'p'
#define PRINT_MAP_STATS      'm'
#define LAVA_TRIGGER         'L'

#define START                's'
#define PLACE_WALL           'w'
#define PLACE_BOULDER        'b'
#define PLACE_GEM            'g'
#define PLACE_LAVA           'l'
#define PLACE_EXIT           'e'
#define PLACE_GROUPED_WALLS  'W'

#define POINTS_DIRT_NORMAL    1
#define POINTS_DIRT_LAVA      10
#define POINTS_GEM_NORMAL     20
#define POINTS_GEM_LAVA       200

#define GRAVITY_UP           'w'
#define GRAVITY_DOWN         's'
#define GRAVITY_LEFT         'a'
#define GRAVITY_RIGHT        'd'

#define ASCII_LIMIT           256
#define CMD_HISTORY_LENGTH    5
#define LAVA_GAME_BIRTH_COUNT 3
#define LAVA_SURVIVE_MIN      2
#define LAVA_SURVIVE_MAX      3
#define LAVA_SEED_BIRTH_COUNT 2

#define EPSILON               0.001
#define SHADOW_RAY_STEP       0.0001

const int D_ROW[ASCII_LIMIT] = {
    [UP_SINGLE] = -1, [DOWN_SINGLE] = 1, [LEFT_SINGLE] = 0, [RIGHT_SINGLE] = 0,  
    [UP_DASH] = -1, [DOWN_DASH] = 1, [LEFT_DASH] = 0, [RIGHT_DASH] = 0   
};
const int D_COL[ASCII_LIMIT] = {
    [UP_SINGLE] = 0, [DOWN_SINGLE] = 0, [LEFT_SINGLE] = -1, [RIGHT_SINGLE] = 1,
    [UP_DASH] = 0, [DOWN_DASH] = 0, [LEFT_DASH] = -1, [RIGHT_DASH] = 1
};

// Provided Enums
// Enum for features on the game board
enum entity {
    EMPTY,
    DIRT,
    WALL,
    BOULDER,
    GEM,
    EXIT_LOCKED,
    EXIT_UNLOCKED,
    HIDDEN,
    PLAYER
};

// Add your own enums below this line
enum lava_mode {
    LAVA_NONE,
    GAME_OF_LAVA,
    LAVA_SEEDS
};

// Represents a tile/cell on the game board
struct tile {
    enum entity entity;
    int has_lava;
    int next_turn_lava;
};

// Add your own structs below this line
struct constants {
    int start_row;
    int start_col;
    int init_dirt;
    int init_gem;
};

struct game_status {
    int player_row;
    int player_col;
    int score;
    int lives;
    int can_dash;
    int boulder_hit;
    int lava_hit;
    int illumination;
    int illumination_radius;
    int shadowed;
    int shadow_entire_board;
    char gravity;
    enum lava_mode lava_mode;
    char cmd_history[CMD_HISTORY_LENGTH];
};

// Provided Function Prototypes
void initialise_board(struct tile board[ROWS][COLS]);
void print_board(struct tile board[ROWS][COLS], int lives_remaining);
void print_board_line(void);
void print_board_header(int lives);
void print_map_statistics(
    int number_of_dirt_tiles,
    int number_of_gem_tiles,
    int number_of_boulder_tiles,
    double completion_percentage,
    int maximum_points_remaining
);

// Add your function prototypes below this line

//Setup function prototypes
void initialise_player_pos(struct tile board[ROWS][COLS], 
    struct constants *constants);
void add_features(struct tile board[ROWS][COLS]);
void add_single_tile_features(struct tile board[ROWS][COLS], char instruction);
void add_exits(struct tile board[ROWS][COLS]);
void add_grouped_walls(struct tile board[ROWS][COLS], 
    int start_row, int start_col, int end_row, int end_col);

//Gameplay function prototypes
void gameplay(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void static_instructions(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, char instruction);
void move_player_single(struct tile board[ROWS][COLS], char instruction, 
    struct game_status *status);
void move_player_dash(struct tile board[ROWS][COLS], 
    char instruction, char instruction2, struct game_status *status);

void entities_turns(struct tile game_board[ROWS][COLS],
    struct tile true_board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_turn(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_up(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_down(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_left(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_right(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status);
void boulder_ud_spawn_check(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, int i, int j);
void boulder_lr_spawn_check(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, int i, int j);
void lava_turn(struct tile board[ROWS][COLS], struct game_status *status);
void game_of_lava(struct tile board[ROWS][COLS]);
void lava_seeds(struct tile board[ROWS][COLS]);

void player_hit(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct constants constants, 
    struct game_status *status);
void illuminate_toggle(struct game_status *status);
void shadow_toggle(struct game_status *status);
void illuminate(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status);
void shadow(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status);
int check_hidden(struct tile board[ROWS][COLS], 
    struct game_status status, int i, int j);

//Helper functions
void initialise_constants_and_game_status(struct tile true_board[ROWS][COLS],
    struct constants *constants, struct game_status *status);
int check_valid_placement(struct tile board[ROWS][COLS], int row, int col);
int validate_grouped_walls(struct tile board[ROWS][COLS], 
    int start_row, int start_col, int end_row, int end_col);
int valid_move(struct tile board[ROWS][COLS], int new_row, int new_col);
int entity_counter(struct tile board[ROWS][COLS], enum entity entity_type);
int update_score(struct tile board[ROWS][COLS], 
    struct game_status status, int row, int col);
void check_exit_condition(struct tile board[ROWS][COLS], 
    struct game_status status);
void open_exits(struct tile board[ROWS][COLS]);
void board_update(struct tile value_board[ROWS][COLS], 
    struct tile target_board[ROWS][COLS]);
void print_correct_board(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], 
    struct game_status status, struct constants constants);
void print_gravity_direction(int gravity);
void update_command_history(struct game_status *status, char new_command);
void check_lava_code(struct game_status *status);
int count_adjacent_lava(struct tile board[ROWS][COLS], int i, int j);
int above_corner_check(struct tile board[ROWS][COLS], 
    double row, double col, int gradient_x, int gradient_y);
int below_corner_check(struct tile board[ROWS][COLS], 
    double row, double col, int gradient_x, int gradient_y);
void shadow_entire_board(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status);

/*
==============================================================================
================================= START MAIN =================================
==============================================================================
*/
int main(void) {
    printf("Welcome to CS Caverun!\n\n");
    printf("--- Game Setup Phase ---\n");

    //set up game and true boards (necessary for illumination) 
    //by making every tile dirt
    struct tile game_board[ROWS][COLS];
    struct tile true_board[ROWS][COLS];
    initialise_board(game_board);
    initialise_board(true_board);

    //declare necessary structs
    struct constants constants;
    struct game_status status;

    initialise_player_pos(true_board, &constants);
    add_features(true_board);
    gameplay(game_board, true_board, constants, &status);

    return 0;
}
/*
==============================================================================
================================== END MAIN ==================================
==============================================================================
*/

// Add your function definitions below this line

/*
==============================================================================
============================= START SETUP SECTION ============================
==============================================================================
*/

//places the player in a valid starting position 
void initialise_player_pos(struct tile board[ROWS][COLS], 
    struct constants *constants) {
    int row, col;
    char valid_starting_pos = 'f';

    while (valid_starting_pos != 't') {
        printf("Enter the player's starting position: ");
        scanf("%d %d", &row, &col);

        if (row >= ROWS || row < 0 || 
            col >= COLS || col < 0) {
            printf("Position %d %d is invalid!\n", row, col);
        } else {
            valid_starting_pos = 't';
        }
    }
    board[row][col].entity = PLAYER;
    //stores this position as initial player position
    constants->start_row = row;
    constants->start_col = col;
    print_board(board, INITIAL_LIVES);
}

//adds every possible feature to the game map
void add_features(struct tile board[ROWS][COLS]) {
    char instruction;
    printf("Enter map features:\n");

    while (scanf(" %c", &instruction) == 1) {
        if (instruction == START) {
            break;
        }
        if (instruction == PLACE_WALL || instruction == PLACE_BOULDER || 
            instruction == PLACE_GEM || instruction == PLACE_LAVA) {
            add_single_tile_features(board, instruction);
        } else if (instruction == PLACE_EXIT) {
            add_exits(board);
        } else if (instruction == PLACE_GROUPED_WALLS) {
            int start_row, start_col, end_row, end_col;
            scanf("%d %d %d %d", &start_row, &start_col, &end_row, &end_col);
            add_grouped_walls(board, start_row, start_col, end_row, end_col);
        }
    }

    //opens all exits if there are no gems
    int gem_count = entity_counter(board, GEM);
    if (gem_count == 0) {
        open_exits(board);
    }
    print_board(board, INITIAL_LIVES);
}

//adds non-group wall features to the map
void add_single_tile_features(struct tile board[ROWS][COLS], char instruction) {
    int row = 0;
    int col = 0;

    //read 2 integers for single-tile features
    scanf("%d %d", &row, &col);

    if (check_valid_placement(board, row, col)) {
        if (instruction == PLACE_WALL) {
            board[row][col].entity = WALL;
        } else if (instruction == PLACE_BOULDER) {
            board[row][col].entity = BOULDER;
        } else if (instruction == PLACE_GEM) {
            board[row][col].entity = GEM;
        } else if (instruction == PLACE_LAVA) {
            board[row][col].has_lava = 1;
        }
    }
}

//adds exits to the map
void add_exits(struct tile board[ROWS][COLS]) {
    int row = 0;
    int col = 0;
    scanf("%d %d", &row, &col);

    if (check_valid_placement(board, row, col)) {
        board[row][col].entity = EXIT_LOCKED;
    }
}

void add_grouped_walls(struct tile board[ROWS][COLS], 
    int start_row, int start_col, int end_row, int end_col) {
    
    if (validate_grouped_walls(board, start_row, start_col, end_row, end_col)) {
        for (int i = start_row; i <= end_row; i++) {
            for (int j = start_col; j <= end_col; j++) {
                board[i][j].entity = WALL;
            }
        }
    }
}

/*
==============================================================================
============================ END SETUP SECTION ===============================
==============================================================================
*/

/*
==============================================================================
========================= START GAMEPLAY SECTION =============================
==============================================================================
*/

//controls player single and dash movements
void gameplay(struct tile game_board[ROWS][COLS], struct tile true_board[ROWS]
    [COLS], struct constants constants, struct game_status *status) {

    initialise_constants_and_game_status(true_board, &constants, status); 
    char instruction, instruction2;

    while (scanf(" %c", &instruction) == 1) {
        update_command_history(status, instruction);
        check_lava_code(status);
        
        if (instruction == LAVA_TRIGGER) {
        } else if (!isupper(instruction)) {
            if (instruction == ILLUMINATE) {
                scanf("%d", &status->illumination_radius);
                illuminate_toggle(status);
                print_correct_board(game_board, true_board, *status, constants);
            } else if (instruction == SHADOW) {
                shadow_toggle(status);
                print_correct_board(game_board, true_board, *status, constants);
            } else if (instruction == GRAVITY) {
                scanf(" %c", &status->gravity);
                print_gravity_direction(status->gravity);
                entities_turns(game_board, true_board, constants, status);
            } else if (instruction == QUIT || instruction == PRINT_SCORE || 
                instruction == PRINT_MAP_STATS) {
                static_instructions(true_board, 
                    constants, *status, instruction);
            } else {
                move_player_single(true_board, instruction, status);
                entities_turns(game_board, true_board, constants, status);
            }
        } else {
            scanf(" %c", &instruction2);
            if (status->can_dash) {
                move_player_dash(true_board, instruction, instruction2, status);
                status->can_dash = FALSE; 
                entities_turns(game_board, true_board, constants, status);
            } else {
                printf("You're out of breath! Skipping dash move...\n");
                status->can_dash = TRUE; 
                print_correct_board(game_board, true_board, *status, constants);
            }
        }
    }
}

//handles non-movement instructions
void static_instructions(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, char instruction) {
    if (instruction == QUIT) {
        printf("--- Quitting Game ---\n");
        exit(0);
    } else if (instruction == PRINT_SCORE) {
        printf("You have %d point(s)!\n", status.score);
    } else if (instruction == PRINT_MAP_STATS) {
        int maximum_points_remaining;
        if (status.lava_mode != LAVA_NONE) {
            maximum_points_remaining = (entity_counter(board, DIRT) * 
            POINTS_DIRT_LAVA) + (entity_counter(board, GEM) * POINTS_GEM_LAVA);
        } else {
            maximum_points_remaining = entity_counter(board, DIRT) * 
            POINTS_DIRT_NORMAL + 
            (entity_counter(board, GEM) * POINTS_GEM_NORMAL);
        }
        print_map_statistics(entity_counter(board, DIRT), 
        entity_counter(board, GEM), entity_counter(board, BOULDER), 
        100.0 * (1.0 - (double)(entity_counter(board, DIRT) + 
        entity_counter(board, GEM)) / 
        (constants.init_dirt + constants.init_gem)), 
        maximum_points_remaining
        );
    }
}

//moves player by a single tile
void move_player_single(struct tile board[ROWS][COLS], char instruction, 
    struct game_status *status) {
    int new_row = status->player_row + D_ROW[(int) instruction];
    int new_col = status->player_col + D_COL[(int) instruction];

    if (valid_move(board, new_row, new_col)) {
        status->score += update_score(board, *status, new_row, new_col);
        //makes the current tile empty
        board[status->player_row][status->player_col].entity = EMPTY; 
        status->player_row = new_row;
        status->player_col = new_col;
        //if player is on exit tile, exits the game
        check_exit_condition(board, *status);
        //makes the new tile the player
        board[status->player_row][status->player_col].entity = PLAYER;
    }
    status->can_dash = TRUE;
}

//moves player by dash 
void move_player_dash(struct tile board[ROWS][COLS], 
    char instruction, char instruction2, struct game_status *status) {
    int new_row1 = status->player_row + D_ROW[(int) instruction];
    int new_col1 = status->player_col + D_COL[(int) instruction];

    //first move validation
    if (!valid_move(board, new_row1, new_col1)) {
        return;  
    }
    //apply first move
    status->score += update_score(board, *status, new_row1, new_col1);
    board[status->player_row][status->player_col].entity = EMPTY; 
    status->player_row = new_row1;
    status->player_col = new_col1;

    check_exit_condition(board, *status);

    //maps second movement instruction to new board location
    int new_row2 = status->player_row + D_ROW[(int) instruction2];
    int new_col2 = status->player_col + D_COL[(int) instruction2];

    //second move validation
    if (!valid_move(board, new_row2, new_col2)) {
        board[status->player_row][status->player_col].entity = PLAYER; 
        return;
    }
    //apply second move
    status->score += update_score(board, *status, new_row2, new_col2);
    board[status->player_row][status->player_col].entity = EMPTY;
    status->player_row = new_row2;
    status->player_col = new_col2;

    check_exit_condition(board, *status);
    board[status->player_row][status->player_col].entity = PLAYER;
}

//control movement and logic of all non-player entities
void entities_turns(struct tile game_board[ROWS][COLS],
    struct tile true_board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {

    boulder_turn(true_board, constants, status);
    if (status->boulder_hit) {
        player_hit(game_board, true_board, constants, status);
    }

    lava_turn(true_board, status);
    if (status->lava_hit) {
        player_hit(game_board, true_board, constants, status);
    } else {
        print_correct_board(game_board, true_board, *status, constants);
    }
}

//boulder movement
void boulder_turn(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {

    if (status->gravity == GRAVITY_UP) {
        boulder_up(board, constants, status);
    } else if (status->gravity == GRAVITY_DOWN) {
        boulder_down(board, constants, status);
    } else if (status->gravity == GRAVITY_LEFT) {
        boulder_left(board, constants, status);
    } else if (status->gravity == GRAVITY_RIGHT) {
        boulder_right(board, constants, status);
    }
}

void boulder_up(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {

    for (int i = 0; i < ROWS - 1; i++) {
        for (int j = 0; j < COLS; j++) {
            ///boulder hits player and spawn is currently occupied
            if (board[i][j].entity == PLAYER && 
                board[i + 1][j].entity == BOULDER && board[constants.start_row]
                [constants.start_col].entity != EMPTY) {
                boulder_ud_spawn_check(board, constants, *status, i, j);
                status->boulder_hit = TRUE;
            }
            //boulder moves down into space
            else if (board[i][j].entity == EMPTY && 
                board[i + 1][j].entity == BOULDER) {
                board[i][j].entity = BOULDER;
                board[i + 1][j].entity = EMPTY;
            }
            //boulder hits player on 1 life
            if (board[i][j].entity == PLAYER && 
                board[i + 1][j].entity == BOULDER && status->lives == 1) {
                board[i + 1][j].entity = EMPTY;
                status->boulder_hit = TRUE;
            } 
            //boulder hits player on 2+ lives
            else if (board[i][j].entity == PLAYER && 
                board[i + 1][j].entity == BOULDER && status->lives > 1) {
                board[i][j].entity = BOULDER;
                board[i + 1][j].entity = EMPTY;
                status->boulder_hit = TRUE;
            }  
        }
    } 
}

void boulder_down(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {

    for (int i = ROWS - 1; i > 0; i--) {
        for (int j = 0; j < COLS; j++) {
            ///boulder hits player and spawn is currently occupied
            if (board[i][j].entity == PLAYER && 
                board[i - 1][j].entity == BOULDER && board[constants.start_row]
                [constants.start_col].entity != EMPTY) {
                boulder_ud_spawn_check(board, constants, *status, i, j);
                status->boulder_hit = TRUE;
            } 
            //boulder moves down into space
            else if (board[i][j].entity == EMPTY && 
                board[i - 1][j].entity == BOULDER) {
                board[i][j].entity = BOULDER;
                board[i - 1][j].entity = EMPTY;
            }
            //boulder hits player on 1 life
            if (board[i][j].entity == PLAYER && 
                board[i - 1][j].entity == BOULDER && status->lives == 1) {
                board[i - 1][j].entity = EMPTY;
                status->boulder_hit = TRUE;
            } 
            //boulder hits player on 2+ lives
            else if (board[i][j].entity == PLAYER && 
                board[i - 1][j].entity == BOULDER && status->lives > 1) {
                board[i][j].entity = BOULDER;
                board[i - 1][j].entity = EMPTY;
                status->boulder_hit = TRUE;
            }  
        }
    } 
}

void boulder_left(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {
    
    for (int j = 0; j < COLS - 1; j++) {
        for (int i = 0; i < ROWS; i++) {
            ///boulder hits player and spawn is currently occupied
            if (board[i][j].entity == PLAYER && 
                board[i][j + 1].entity == BOULDER && board[constants.start_row]
                [constants.start_col].entity != EMPTY) {
                boulder_lr_spawn_check(board, constants, *status, i, j);
                status->boulder_hit = TRUE;
            }
            //boulder moves down into space
            else if (board[i][j].entity == EMPTY && 
                board[i][j + 1].entity == BOULDER) {
                board[i][j].entity = BOULDER;
                board[i][j + 1].entity = EMPTY;
            }
            //boulder hits player on 1 life
            if (board[i][j].entity == PLAYER && 
                board[i][j + 1].entity == BOULDER && status->lives == 1) {
                board[i][j + 1].entity = EMPTY;
                status->boulder_hit = TRUE;
            } 
            //boulder hits player on 2+ lives
            else if (board[i][j].entity == PLAYER && 
                board[i][j + 1].entity == BOULDER && status->lives > 1) {
                board[i][j].entity = BOULDER;
                board[i][j + 1].entity = EMPTY;
                status->boulder_hit = TRUE;
            }  
        }
    } 
}

void boulder_right(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status *status) {

    for (int j = COLS - 1; j > 0; j--) {
        for (int i = 0; i < ROWS; i++) {
            ///boulder hits player and spawn is currently occupied
            if (board[i][j].entity == PLAYER && 
                board[i][j - 1].entity == BOULDER && board[constants.start_row]
                [constants.start_col].entity != EMPTY) {
                boulder_lr_spawn_check(board, constants, *status, i, j);
                status->boulder_hit = TRUE;
            }
            //boulder moves down into space
            else if (board[i][j].entity == EMPTY && 
                board[i][j - 1].entity == BOULDER) {
                board[i][j].entity = BOULDER;
                board[i][j - 1].entity = EMPTY;
            }
            //boulder hits player on 1 life
            if (board[i][j].entity == PLAYER && 
                board[i][j - 1].entity == BOULDER && status->lives == 1) {
                board[i][j - 1].entity = EMPTY;
                status->boulder_hit = TRUE;
            } 
            //boulder hits player on 2+ lives
            else if (board[i][j].entity == PLAYER && 
                board[i][j - 1].entity == BOULDER && status->lives > 1) {
                board[i][j].entity = BOULDER;
                board[i][j - 1].entity = EMPTY;
                status->boulder_hit = TRUE;
            }  
        }
    } 
}

//checks whether that the boulder that hits the player will still be at spawn
//after hit
void boulder_ud_spawn_check(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, int i, int j) {

    if (status.gravity == GRAVITY_UP) {
        //is spawn is occuppied by a boulder?
        if (board[constants.start_row][constants.start_col].entity == BOULDER) {
            //if so, is it the same boulder that is currently below the player?
            if (i + 1 == constants.start_row && j == constants.start_col) {
                board[i + 1][j].entity = EMPTY;
                board[i][j].entity = BOULDER;
            } else {
                board[i + 1][j].entity = EMPTY;
            }
        } else {
            board[i + 1][j].entity = EMPTY;
        }
    } else if (status.gravity == GRAVITY_DOWN) {
        //is spawn is occuppied by a boulder?
        if (board[constants.start_row][constants.start_col].entity == BOULDER) {
            //if so, is it the same boulder that is currently above the player?
            if (i - 1 == constants.start_row && j == constants.start_col) {
                board[i - 1][j].entity = EMPTY;
                board[i][j].entity = BOULDER;
            } else {
                board[i - 1][j].entity = EMPTY;
            }
        } else {
            board[i - 1][j].entity = EMPTY;
        }
    }
}

void boulder_lr_spawn_check(struct tile board[ROWS][COLS], 
    struct constants constants, struct game_status status, int i, int j) {

    if (status.gravity == GRAVITY_LEFT) {
        //is spawn is occuppied by a boulder?
        if (board[constants.start_row][constants.start_col].entity == BOULDER) {
            //if so, is it the same boulder that is currently below the player?
            if (i == constants.start_row && j + 1 == constants.start_col) {
                board[i][j + 1].entity = EMPTY;
                board[i][j].entity = BOULDER;
            } else {
                board[i][j + 1].entity = EMPTY;
            }
        } else {
            board[i][j + 1].entity = EMPTY;
        }
    } else if (status.gravity == GRAVITY_RIGHT) {
        //is spawn is occuppied by a boulder?
        if (board[constants.start_row][constants.start_col].entity == BOULDER) {
            //if so, is it the same boulder that is currently below the player?
            if (i == constants.start_row && j - 1 == constants.start_col) {
                board[i][j - 1].entity = EMPTY;
                board[i][j].entity = BOULDER;
            } else {
                board[i][j - 1].entity = EMPTY;
            }
        } else {
            board[i][j - 1].entity = EMPTY;
        }
    }
}

void lava_turn(struct tile board[ROWS][COLS], struct game_status *status) {
    //lava movement
    if (status->lava_mode == GAME_OF_LAVA) {
        game_of_lava(board);
    } else if (status->lava_mode == LAVA_SEEDS) {
        lava_seeds(board);
    }
    //lava damage
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j].entity == PLAYER && board[i][j].has_lava) {
                board[i][j].entity = EMPTY;
                status->lava_hit = TRUE;
            }
        }
    }
}

void game_of_lava(struct tile board[ROWS][COLS]) {

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int adjacent_lava_count = count_adjacent_lava(board, i, j);
            if (!board[i][j].has_lava && adjacent_lava_count == 
                LAVA_GAME_BIRTH_COUNT) {
                board[i][j].next_turn_lava = TRUE;
            } else if (board[i][j].has_lava && 
                (adjacent_lava_count == LAVA_SURVIVE_MIN || 
                adjacent_lava_count == LAVA_SURVIVE_MAX)) {
                board[i][j].next_turn_lava = TRUE;
            } else if (board[i][j].has_lava && 
                (adjacent_lava_count < LAVA_SURVIVE_MIN || 
                adjacent_lava_count > LAVA_SURVIVE_MAX)) {
                board[i][j].next_turn_lava = FALSE;
            }
        }
    }

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j].has_lava = board[i][j].next_turn_lava;
            board[i][j].next_turn_lava = FALSE;
        }
    }
}

void lava_seeds(struct tile board[ROWS][COLS]) {

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int adjacent_lava_count = count_adjacent_lava(board, i , j);
            if (!board[i][j].has_lava && adjacent_lava_count == 
                LAVA_SEED_BIRTH_COUNT) {
                board[i][j].next_turn_lava = TRUE;
            } else {
                board[i][j].next_turn_lava = FALSE;
            }
        }
    }

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j].has_lava = board[i][j].next_turn_lava;
            board[i][j].next_turn_lava = FALSE;
        }
    }
}

void player_hit(struct tile game_board[ROWS][COLS],  
    struct tile true_board[ROWS][COLS], struct constants constants, 
    struct game_status *status) {

    (status->lives)--;
    if (status->lives == 0) {
        true_board[status->player_row][status->player_col].entity = PLAYER;
        printf("Game Lost! You scored %d points!\n", status->score);
        print_correct_board(game_board, true_board, *status, constants);
        exit(0);
    } else {
        //respawn point is clear
        if (true_board[constants.start_row]
            [constants.start_col].entity == EMPTY &&
            true_board[constants.start_row]
            [constants.start_col].has_lava == FALSE) {
            printf("Respawning!\n");

            true_board[constants.start_row]
            [constants.start_col].entity = PLAYER;
            status->player_row = constants.start_row;
            status->player_col = constants.start_col;

            if (status->boulder_hit) {
                status->boulder_hit = FALSE;
            } else if (status->lava_hit) {
                status->lava_hit = FALSE;
                print_correct_board(game_board, true_board, *status, constants);
            }
        } 
        else if (status->lava_mode == LAVA_NONE) {
            status->shadow_entire_board = TRUE;
            true_board[status->player_row][status->player_col].entity = PLAYER;

            printf("Respawn blocked! Game over. Final score: %d points.\n", 
                status->score);
            print_correct_board(game_board, true_board, *status, constants);
            exit(0);
        } else {
            status->shadow_entire_board = TRUE;
            true_board[status->player_row][status->player_col].entity = PLAYER;

            printf("Respawn blocked! You're toast! Final score: %d points.\n", 
                status->score);
            print_correct_board(game_board, true_board, *status, constants);
            exit(0);
        }
    }
}

//toggles the state of the illumination flag
void illuminate_toggle(struct game_status *status) {
    if (status->illumination_radius <= 0) {
        status->illumination = FALSE;
        printf("Illumination Mode: Deactivated\n");
    } else {
        status->illumination = TRUE;
        printf("Illumination Mode: Activated\n");
    }
}

//toggles the state of the shadowed flag
void shadow_toggle(struct game_status *status) {
    if (status->shadowed == FALSE) {
        printf("Shadow Mode: Activated\n");
        status->shadowed = TRUE;
    } else {
        printf("Shadow Mode: Deactivated\n");
        status->shadowed = FALSE;
    }
}

//maps the true board to the game board, with hidden tiles based on radius
void illuminate(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status) {

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            game_board[i][j].has_lava = true_board[i][j].has_lava;
            double distance = 
                sqrt((i - status.player_row) * (i - status.player_row) + 
                (j - status.player_col) * (j - status.player_col));
            if (distance <= status.illumination_radius) {
                game_board[i][j].entity = true_board[i][j].entity;
            } else {
                game_board[i][j].entity = HIDDEN;
            }
        }
    }
}

//maps the true board to the game board, with hidden tiles based on shadows
void shadow(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status) {

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int hide = 0;
            game_board[i][j].has_lava = true_board[i][j].has_lava;
            if (true_board[i][j].entity != PLAYER) {
                hide = check_hidden(true_board, status, i, j);
            }
            if (!hide) {
                game_board[i][j].entity = true_board[i][j].entity;
            } else {
                game_board[i][j].entity = HIDDEN;
            }
        }
    }
}

int check_hidden(struct tile board[ROWS][COLS], 
    struct game_status status, int i, int j) {

    int start_x = status.player_row;
    int start_y = status.player_col;
    int gradient_x = i - start_x;
    int gradient_y = j - start_y;
    int corner_blocked_above = FALSE;
    int corner_blocked_below = FALSE;

    for (double lambda = 0; lambda < 1; lambda += SHADOW_RAY_STEP) {
        double raw_row = start_x + lambda * gradient_x;
        double raw_col = start_y + lambda * gradient_y;
        int current_row = (int)(round(raw_row));
        int current_col = (int)(round(raw_col));

        if (raw_row >= i - 0.5 && raw_row <= i + 0.5 &&
            raw_col >= j - 0.5 && raw_col <= j + 0.5) {
            break;
        }
        //corner check
        if (fabs(fmod(raw_row, 1.0) - 0.5) < EPSILON &&
            fabs(fmod(raw_col, 1.0) - 0.5) < EPSILON) {
            corner_blocked_above |= above_corner_check(board, 
                raw_row, raw_col, gradient_x, gradient_y);
            corner_blocked_below |= below_corner_check(board, 
                raw_row, raw_col, gradient_x , gradient_y);
        } else {
            char type = board[current_row][current_col].entity;
            if (type == WALL || type == BOULDER || type == GEM) {
                return TRUE;
            }
        }
    }

    if (corner_blocked_above && corner_blocked_below) {
        return TRUE;
    } 
    return FALSE;   
}

/*
==============================================================================
=========================== END GAMEPLAY SECTION =============================
==============================================================================
*/

/*
==============================================================================
=========================== START HELPER SECTION =============================
==============================================================================
*/

void initialise_constants_and_game_status(struct tile true_board[ROWS][COLS],
    struct constants *constants, struct game_status *status) {
    printf("--- Gameplay Phase ---\n"); 

    status->player_row = constants->start_row;
    status->player_col = constants->start_col; 
    constants->init_dirt = entity_counter(true_board, DIRT);
    constants->init_gem = entity_counter(true_board, GEM);

    status->score = 0;
    status->can_dash = TRUE;
    status->boulder_hit = FALSE;
    status->lava_hit = FALSE;
    status->lives = INITIAL_LIVES;

    status->illumination = FALSE;
    status->illumination_radius = 0;
    status->shadowed = FALSE;
    status->shadow_entire_board = FALSE;
    status->gravity = GRAVITY_DOWN;
    status->lava_mode = LAVA_NONE;

    for (int i = 0; i < CMD_HISTORY_LENGTH; i++) {
        status->cmd_history[i] = 0;
    }
}

int check_valid_placement(struct tile board[ROWS][COLS], int row, int col) {

    int valid_placement = TRUE;

    if (row < 0 || row >= ROWS || 
        col < 0 || col >= COLS) {
        printf("Invalid location: position is not on map!\n");
        valid_placement = FALSE;
    } else if (board[row][col].entity != DIRT) {
        printf("Invalid location: tile is occupied!\n");
        valid_placement = FALSE;
    } 
    return valid_placement;
}

int validate_grouped_walls(struct tile board[ROWS][COLS], 
    int start_row, int start_col, int end_row, int end_col) {

    //validate map rectangle bounds
    if (start_row < 0 || start_row >= ROWS || 
        start_col < 0 || start_col >= COLS ||
        end_row < 0 || end_row >= ROWS || 
        end_col < 0 || end_col >= COLS) {
        printf("Invalid location: feature cannot be placed here!\n");
        return FALSE;
    } 
    //validates tile occupation i.e. all tiles need to be free
    int is_occupied = FALSE;
    for (int i = start_row; i <= end_row; i++) {
        for (int j = start_col; j <= end_col; j++) {
            if (board[i][j].entity != DIRT) {
                is_occupied = TRUE;
                //saves unneccessary checking once one invalid tile is found
                break;
            }
        }
    }

    if (is_occupied) {
        printf("Invalid location: feature cannot be placed here!\n");
        return FALSE;
    } else {
        return TRUE;
    }            
}

//checks whether movement will arrive at a valid destination
int valid_move(struct tile board[ROWS][COLS], int new_row, int new_col) {
    return (new_row >= 0 && new_row < ROWS &&
            new_col >= 0 && new_col < COLS &&
            (board[new_row][new_col].entity == EMPTY ||
             board[new_row][new_col].entity == DIRT ||
             board[new_row][new_col].entity == GEM ||
             board[new_row][new_col].entity == EXIT_UNLOCKED));
}

//counts how many type of a certain entity are currently on the board
int entity_counter(struct tile board[ROWS][COLS], enum entity entity_type) {
    int counter = 0; 

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j].entity == entity_type) {
                counter++;
            }
        }
    }
    return counter;
}

//updates the score based on dirt and gem collection
int update_score(struct tile board[ROWS][COLS], 
    struct game_status status, int row, int col) {
    if (board[row][col].entity == DIRT) {
        if (status.lava_mode != LAVA_NONE) {
            return POINTS_DIRT_LAVA;
        } else {
            return POINTS_DIRT_NORMAL;
        }
    }
    else if (board[row][col].entity == GEM) {
        board[row][col].entity = EMPTY;
        if (status.lava_mode != LAVA_NONE) {
            return POINTS_GEM_LAVA;
        } else {
            return POINTS_GEM_NORMAL;
        }
    }
    return 0;
}

//determines whether to open the exits based on how many gems remaining 
void check_exit_condition(struct tile board[ROWS][COLS], 
    struct game_status status) {
    if (entity_counter(board, GEM) == 0) {
        open_exits(board);
    }

    if (board[status.player_row][status.player_col].entity == EXIT_UNLOCKED) {
        board[status.player_row][status.player_col].entity = PLAYER;
        print_board(board, status.lives);
        printf("You Win! Final Score: %d point(s)!\n", status.score);
        exit(0);
    }
}

//opens all exits on the map
void open_exits(struct tile board[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j].entity == EXIT_LOCKED) {
                board[i][j].entity = EXIT_UNLOCKED;
            }
        }
    }
}

void board_update(struct tile value_board[ROWS][COLS], 
    struct tile target_board[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            target_board[i][j].entity = value_board[i][j].entity;
        }
    }
}

//prints either the game or true board depending on illumination mode
void print_correct_board(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], 
    struct game_status status, struct constants constants) {
    
    if (status.shadow_entire_board && status.shadowed) {
        shadow_entire_board(game_board, true_board, status);
        print_board(game_board, status.lives);
    } else if (status.shadowed) {
        shadow(game_board, true_board, status);
        print_board(game_board, status.lives);
    } else if (status.illumination) {
        illuminate(game_board, true_board, status);
        print_board(game_board, status.lives);
    } else {
        print_board(true_board, status.lives);
    } 
}

void print_gravity_direction(int gravity) {

    if (gravity == GRAVITY_UP) {
        printf("Gravity now pulls UP!\n");
    } else if (gravity == GRAVITY_DOWN) {
        printf("Gravity now pulls DOWN!\n");
    } else if (gravity == GRAVITY_LEFT) {
        printf("Gravity now pulls LEFT!\n");
    } else if (gravity == GRAVITY_RIGHT) {
        printf("Gravity now pulls RIGHT!\n");
    }
}

void update_command_history(struct game_status *status, char new_command) {

    for (int i = 0; i < CMD_HISTORY_LENGTH - 1; i++) {
        status->cmd_history[i] = status->cmd_history[i + 1];
    }
    status->cmd_history[CMD_HISTORY_LENGTH - 1] = new_command;
}

void check_lava_code(struct game_status *status) {

    if (status->cmd_history[0] == UP_SINGLE && 
        status->cmd_history[1] == RIGHT_SINGLE &&
        status->cmd_history[2] == DOWN_SINGLE && 
        status->cmd_history[3] == LEFT_SINGLE &&
        status->cmd_history[4] == LAVA_TRIGGER) {
        if (status->lava_mode != GAME_OF_LAVA) {
            printf("Game Of Lava: Activated\n");
        }
        status->lava_mode = GAME_OF_LAVA;
    } else if (status->cmd_history[0] == UP_SINGLE && 
        status->cmd_history[1] == LEFT_SINGLE &&
        status->cmd_history[2] == DOWN_SINGLE && 
        status->cmd_history[3] == RIGHT_SINGLE &&
        status->cmd_history[4] == LAVA_TRIGGER) {
        if (status->lava_mode != LAVA_SEEDS) {
            printf("Lava Seeds: Activated\n");
        }
        status->lava_mode = LAVA_SEEDS;
    }
}

int count_adjacent_lava(struct tile board[ROWS][COLS], int i, int j) {

    int adjacent_lava_counter = 0;

    if (board[(10 + i - 1) % ROWS][(10 + j - 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[(10 + i - 1) % ROWS][j].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[(10 + i - 1) % ROWS][(j + 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[i][(10 + j - 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[i][(j + 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[(i + 1) % ROWS][(10 + j - 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[(i + 1) % ROWS][j].has_lava) {
        adjacent_lava_counter++;
    } 
    if (board[(i + 1) % ROWS][(j + 1) % COLS].has_lava) {
        adjacent_lava_counter++;
    } 
    return adjacent_lava_counter;
}

int above_corner_check(struct tile board[ROWS][COLS], 
    double row, double col, int gradient_x, int gradient_y) {

    int base_row = (int)(floor(row));
    int base_col;

    if (gradient_x * gradient_y > 0) {
        base_col = (int)(floor(col)) + 1;
    } else {
        base_col = (int)(floor(col));
    }

    char type = board[base_row][base_col].entity;
    if (type == WALL || type == BOULDER || type == GEM) {
        return TRUE; 
    }
    return FALSE;
}

int below_corner_check(struct tile board[ROWS][COLS], 
    double row, double col, int gradient_x, int gradient_y) {

    int base_row = (int)(floor(row)) + 1;
    int base_col;

    if (gradient_x * gradient_y > 0) {
        base_col = (int)(floor(col));
    } else {
        base_col = (int)(floor(col)) + 1;
    }
    
    char type = board[base_row][base_col].entity;
    if (type == WALL || type == BOULDER || type == GEM) {
        return TRUE; 
    }
    return FALSE;
}

void shadow_entire_board(struct tile game_board[ROWS][COLS], 
    struct tile true_board[ROWS][COLS], struct game_status status) {
    
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            game_board[i][j].has_lava = true_board[i][j].has_lava;
            if (i == status.player_row && j == status.player_col) {
                game_board[i][j].entity = PLAYER;
            } else {
                game_board[i][j].entity = HIDDEN;
            }
        }
    }
}

/*
==============================================================================
============================ END HELPER SECTION ==============================
==============================================================================
*/

// ===========================================================================
// Definitions of Provided Functions
// ===========================================================================

// Given a 2D board array, initialise all tile entities to DIRT.
void initialise_board(struct tile board[ROWS][COLS]) {

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            board[row][col].entity = DIRT;
            board[row][col].has_lava = FALSE;
            board[row][col].next_turn_lava = FALSE;
        }
    }
}

// Prints the game board, showing the player's position and lives remaining
void print_board(
    struct tile board[ROWS][COLS], int lives_remaining) {

    print_board_line();
    print_board_header(lives_remaining);
    print_board_line();

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            printf("|");
            if (board[row][col].entity == PLAYER) {
                printf("^_^");
            } else if (board[row][col].has_lava) {
                printf("^^^");
            } else if (board[row][col].entity == EMPTY) {
                printf("   ");
            } else if (board[row][col].entity == DIRT) {
                printf(" . ");
            } else if (board[row][col].entity == WALL) {
                printf("|||");
            } else if (board[row][col].entity == BOULDER) {
                printf("(O)");
            } else if (board[row][col].entity == GEM) {
                printf("*^*");
            } else if (board[row][col].entity == EXIT_LOCKED) {
                printf("[X]");
            } else if (board[row][col].entity == EXIT_UNLOCKED) {
                printf("[ ]");
            } else if (board[row][col].entity == HIDDEN) {
                printf(" X ");
            } else {
                printf("   ");
            }
        }
        printf("|\n");
        print_board_line();
    }
    printf("\n");
    return;
}

// Helper function for print_board(). You will not need to call this.
void print_board_header(int lives) {
    printf("| Lives: %d    C A V E R U N             |\n", lives);
}

// Helper function for print_board(). You will not need to call this.
void print_board_line(void) {
    printf("+");
    for (int col = 0; col < COLS; col++) {
        printf("---+");
    }
    printf("\n");
}

// Prints game statistics: tile types, completion %, and points remaining.
void print_map_statistics(
    int number_of_dirt_tiles,
    int number_of_gem_tiles,
    int number_of_boulder_tiles,
    double completion_percentage,
    int maximum_points_remaining
) {
    printf("========= Map Statistics =========\n");
    printf("Tiles Remaining by Type:\n");
    printf("  - DIRT:      %d\n", number_of_dirt_tiles);
    printf("  - GEMS:      %d\n", number_of_gem_tiles);
    printf("  - BOULDERS:  %d\n", number_of_boulder_tiles);
    printf("Completion Status:\n");
    printf("  - Collectible Completion: %.1f%%\n", completion_percentage);
    printf("  - Maximum Points Remaining: %d\n", maximum_points_remaining);
    printf("==================================\n");
}
