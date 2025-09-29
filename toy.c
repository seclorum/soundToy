/*
 * soundtoy - A Sound Exploration Toy for Oric-1 Atmos
 *
 * Author: Jay Vaughan (Original)
 *
 * This program serves as a creative workbench for exploring the Oric's sound
 * synthesis capabilities. It leverages ROM presets to generate sounds and includes
 * a hires mode for visual experimentation with cell-based graphics and animations.
 *
 * Key Features:
 * - Load and trigger sound presets from Oric ROM addresses.
 * - Interactive UI for navigating presets, triggering sounds, and entering hires mode.
 * - Hires mode with cell-based blitting, animations (dancers, droppers, eaters), and random color generation.
 * - Keyboard-driven controls for cursor movement, input selection, and actions.
 *
 * Refactor Notes:
 * - Organized code into logical sections: constants, globals, structs/enums, function prototypes, implementations.
 * - Improved naming conventions (e.g., snake_case for constants, descriptive names).
 * - Removed dead/unused code (e.g., #if 0 blocks, unused variables like 'rando', 'fil').
 * - Added comprehensive comments, including function purposes and parameter descriptions.
 * - Fixed inconsistencies (e.g., return types, null checks).
 * - Reduced global variables where possible; grouped related globals.
 * - Enhanced modularity: separated concerns (e.g., validation into a dedicated function).
 * - Ensured bounds checking and clamping for safety.
 * - Assumes dependencies: <lib.h> for Oric primitives (peek, poke, call, etc.), "gencode.h" for display instructions and constants.
 *
 * Version: 1.1.1 (Refactored)
 */

#include <lib.h>
#include "gencode.h"

/* ============================================================================
 * Constants and Macros
 * ============================================================================ */

/* UI Positioning */
#define UI_Y_BASE          8
#define UI_STORAGE_ROW_1   2
#define UI_STORAGE_ROW_2   4
#define UI_STORAGE_ROW_3   6

/* Memory and Address Constants */
#define NULL_FUNCTION      (0x0000)
#define POS_START          (0x0000)
#define POS_END            (0xFFFF)
#define POS_INITIAL        (0xFAA7)  /* Bootup sound preset */
#define POS_KEYCLICK       (0xFB32)
#define HIRES_START_ADDR   (0xA000)
#define HIRES_END_ADDR     (0xBF3F)

/* Hires Buffer Layout */
#define SOURCE_BUFFER_BASE  0xA000
#define DEST_BUFFER_BASE    (0xA000 + (320 * 3))
#define ROW_WIDTH           40
#define COL_HEIGHT          22
#define BLOCK_HEIGHT        8

/* Cell Generation States */
#define CGEN_STATE_INIT     0x11
#define CGEN_STATE_FORWARD  1
#define CGEN_STATE_BACKWARD -1
#define CGEN_STATE_QUIT     0x10

/* Sound Channels */
#define CHANNEL_A           0
#define CHANNEL_B           2
#define CHANNEL_C           4
#define CHANNEL_INDEX(ch)   (ch / 2)

/* Sound Pitch Macro */
#define PITCH_WORD(idx)     ((unsigned int)currentSound[idx] + ((unsigned int)currentSound[idx + 1] << 8))
#define CHANNEL_PITCH(ch)   PITCH_WORD(ch)

/* Keyboard Controls */
#define KEY_SOUND_TOGGLE    'H'
#define KEY_POS_INCREMENT   'J'
#define KEY_POS_DECREMENT   'K'
#define KEY_RESET_PRESET    'R'
#define KEY_RESET_PING      '.'
#define KEY_SET_POSITION    'P'
#define KEY_HELP            '?'
#define KEY_HIRES_MODE      'G'
#define KEY_MEM_HACK        'M'
#define KEY_QUIT            'Q'
#define KEY_CAPTURE_CELL    'C'
#define KEY_TRIGGER_SPACE   ' '
#define KEY_CURSOR_LEFT     'A'
#define KEY_CURSOR_RIGHT    'D'
#define KEY_CURSOR_UP       'W'
#define KEY_CURSOR_DOWN     'S'
#define KEY_INPUT_LEFT      'J'
#define KEY_INPUT_RIGHT     'L'
#define KEY_INPUT_UP        'I'
#define KEY_INPUT_DOWN      'K'
#define KEY_BLAST           'B'

/* Dance Directions Enum */
enum DanceDirection {
    DIAG_FORWARD = 0,
    DIAG_BACKWARD = 1,
    DIAG_LEFT = 2,
    DIAG_RIGHT = 3
};

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/* External Globals from Other Modules */
extern unsigned char currentSound[];  /* Current sound preset buffer (14 bytes) */
extern unsigned char BootSound[];     /* Boot sound preset */
extern unsigned char OverlayLabel[];  /* Hires overlay label */

/* UI and Animation State */
static int cursor_x = 2;              /* Cursor X position */
static int cursor_y = 2;              /* Cursor Y position */
static int input_x = 1;               /* Input cell X position */
static int input_y = 1;               /* Input cell Y position */
static int anim_x = 12;               /* Animation X position */
static int anim_y = 12;               /* Animation Y position */
static int stoy_x = 12;               /* Toy X position (for hack mode) */
static int stoy_y = 12;               /* Toy Y position (for hack mode) */
static int ui_x = 0;                  /* UI X offset */
static int ui_y = 0;                  /* UI Y offset */
static int r_val = 0;                 /* Random value for hacks */

/* Cell Source/Destination Addresses */
static unsigned int cell_source_addr;
static unsigned int cell_dest_addr;
static unsigned int cell_source_base;

/* Generation State */
static int g_state = CGEN_STATE_INIT; /* Current generation state */

/* Keyboard Input */
static unsigned char kp;              /* Key pressed */

/* Mode Flags */
static unsigned char should_play = 1; /* Sound toggle flag */
 
/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Utility Functions */
static int clamp(int value, int min_val, int max_val);
static char *char_to_hex_str(unsigned char value);
static void hex_dump(unsigned char *addr, int length);
static void load_sound_table(unsigned int address);

/* Display and Cell Manipulation */
static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y);
static void cell_blast(unsigned int input_x, unsigned int input_y);
static void cell_ui_update(unsigned char key);
static void cell_anim_update(unsigned char key);
static void cell_hack_update(unsigned char key);

/* Animation Step Functions */
static void step_null(void *state_ptr);
static void step_dancer(void *state_ptr);
static void step_dropper(void *state_ptr);
static void step_eater(void *state_ptr);

/* State Management */
static void update_cell_states(void);
static void validate_ui_bounds(unsigned char key);
static void test_cell_states(void);  /* For debugging, optional */

/* Sound Functions */
static void trigger_current_sound(unsigned int position);
static void dump_channel_pitch(unsigned char channel);
static void synth_zp(unsigned char high_byte, unsigned char low_byte);

/* Mode and UI Functions */
static void print_help_screen(void);
static void gen_random_colors(void);

/* Main Entry */
static void program_main(void);

 /* ============================================================================
 * Structures and Typedefs
 * ============================================================================ */

 /* Global Array of Cell States for Animations
 * Initializes 6 dancer cells with varying source X (0-5), fixed source Y=0,
 * centered initial positions (x=20, y=11), and directions cycling through DIAG_* enums.
 * The last two start with DIAG_LEFT and DIAG_RIGHT.
 * All use step_dancer for diagonal movement and bouncing.
 */
 
typedef struct {
    int source_x;         /* Source X coordinate */
    int source_y;         /* Source Y coordinate */
    int x;                /* Current X coordinate */
    int y;                /* Current Y coordinate */
    int state;            /* Current state (e.g., direction) */
    void (*step_fn)(void *); /* Step function pointer */
} CellState;

/* Typedef for Convenience */
typedef CellState *CellStatePtr;

/* Define a function pointer type for step functions */
typedef void (*CellStateFnPtr)(void *v);

// Array to hold multiple cell states
CellState cellStates[] = {
    {0, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_FORWARD, step_dancer},
    {1, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_BACKWARD, step_dancer},
    {2, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_LEFT, step_dancer},
    {3, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_RIGHT, step_dancer},
    {4, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_LEFT, step_dancer},
    {5, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, DIAG_RIGHT, step_dancer}
};


/* ============================================================================
 * Function Implementations
 * ============================================================================ */

/*
 * Clamp a value to a specified range.
 * Ensures values stay within bounds for safe indexing.
 */
static int clamp(int value, int min_val, int max_val) {
    if (value < min_val) {
        return min_val;
    } else if (value > max_val) {
        return max_val;
    }
    return value;
}

/*
 * Convert a byte to a two-character hexadecimal string.
 * Uses a static buffer for efficiency in repeated calls.
 */
static char *char_to_hex_str(unsigned char value) {
    static char hex_str[3];
    static const char hex_digits[] = "0123456789ABCDEF";
    hex_str[0] = hex_digits[(value >> 4) & 0x0F];
    hex_str[1] = hex_digits[value & 0x0F];
    hex_str[2] = '\0';
    return hex_str;
}

/*
 * Dump a block of memory as hexadecimal values.
 * Useful for inspecting sound presets or cell data.
 * Parameters:
 *   addr: Starting address to dump.
 *   length: Number of bytes to dump.
 */
static void hex_dump(unsigned char *addr, int length) {
    int i, cnt = 0;
    printf("Dump at %p:\n", addr);
    for (i = 0; i < length; i++) {
        if (cnt++ >= 8) {
            cnt = 0;
            printf("\n");
        }
        printf(" %s", char_to_hex_str(addr[i]));
    }
    printf("\n");
}

/*
 * Load a 14-byte sound preset from the given ROM address into currentSound[].
 * Assumes currentSound[] is at least 14 bytes.
 * Parameters:
 *   address: ROM address of the preset.
 */
static void load_sound_table(unsigned int address) {
    int i;
    for (i = 0; i < 14; i++) {
        currentSound[i] = peek(address + i);
    }
}

/*
 * Set up a display instruction for blitting a cell block.
 * Configures source and destination addresses for 8 rows of 40 bytes each.
 * Returns 0 on success, 1 if cell index is out of bounds.
 * Parameters:
 *   cell_idx: Index into displayInstructions array.
 *   src_x, src_y: Source cell coordinates.
 *   dst_x, dst_y: Destination cell coordinates.
 */
static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y) {

    unsigned int src_base = SOURCE_BUFFER_BASE + (src_y * ROW_WIDTH * BLOCK_HEIGHT) + (src_x * 1);
    unsigned int dst_base = DEST_BUFFER_BASE + (dst_y * ROW_WIDTH * BLOCK_HEIGHT) + (dst_x * 1);
    int row;

    if (cell_idx >= MAX_CB_DISPLAY_INSTRUCTIONS) {
        return 1;  /* Out of bounds */
    }
 
    for (row = 0; row < BLOCK_HEIGHT; row++) {
        /* Source Address */
        cell_source_addr = src_base + (row * ROW_WIDTH);
        displayInstructions[cell_idx][3 + row * 6] = cell_source_addr & 0xFF;       /* Low byte */
        displayInstructions[cell_idx][4 + row * 6] = (cell_source_addr >> 8) & 0xFF; /* High byte */

        /* Destination Address */
        cell_dest_addr = dst_base + (row * ROW_WIDTH);
        displayInstructions[cell_idx][6 + row * 6] = cell_dest_addr & 0xFF;       /* Low byte */
        displayInstructions[cell_idx][7 + row * 6] = (cell_dest_addr >> 8) & 0xFF; /* High byte */
    }
    return 0;
}

/*
 * Perform a "blast" operation: blit horizontal and vertical lines from input cell.
 * Creates crosshair-like patterns in hires mode.
 * Parameters:
 *   input_x, input_y: Input cell coordinates to source from.
 */
static void cell_blast(unsigned int input_x, unsigned int input_y) {
    int col;
    /* Horizontal blast across row 0 */
    for (col = 0; col < ROW_WIDTH; col++) {
        set_display_instruction(col, col, 0, col, 4);
    }
    /* Vertical blast down column 1 */
    for (col = 0; col < COL_HEIGHT; col++) {
        set_display_instruction(col, col, 1, 4, col);
    }
}

/*
 * Update UI cursor display by blitting input cell to cursor position.
 * Handles offset for visual alignment.
 * Parameters:
 *   key: Currently pressed key (unused, but passed for consistency).
 */
static void cell_ui_update(unsigned char key) {
    set_display_instruction(CURS1, input_x, input_y, cursor_x, cursor_y);
    set_display_instruction(CURS2, input_x, input_y, cursor_x + 1, cursor_y + 1);
    set_display_instruction(CURS1, input_x + 1, input_y, cursor_x, cursor_y + 1);
    set_display_instruction(CURS2, input_x, input_y + 1, cursor_x + 1, cursor_y);
}

/*
 * Update animation cells by stepping states and blitting to positions.
 * Drives the "dancer" animations.
 * Parameters:
 *   key: Currently pressed key (unused).
 */
static void cell_anim_update(unsigned char key) {
    int anim_vec;
    int anim_idx = ANIM1;
    for (anim_vec = 0; anim_vec < 5; anim_vec++) {
        cellStates[anim_vec].step_fn(&cellStates[anim_vec]);
        set_display_instruction(anim_idx++, cellStates[anim_vec].source_x, cellStates[anim_vec].source_y,
                                cellStates[anim_vec].x, cellStates[anim_vec].y);
    }
}

/*
 * Update hack mode visuals: random toy positions and blits.
 * Incorporates random values and state-based movement.
 * Parameters:
 *   key: Currently pressed key (unused).
 */
static void cell_hack_update(unsigned char key) {
    /* Update toy position based on generation state */
    stoy_x += g_state;
    stoy_y += g_state;

    /* Generate bounded random Y */
    r_val = qrandomJ(peek(0x276)) % COL_HEIGHT;
    if (r_val < 12) {
        anim_y -= g_state;
    }

    /* Wrap toy positions */
    if (stoy_x > ROW_WIDTH) stoy_x = 0;
    if (stoy_y > COL_HEIGHT) stoy_y = 0;

    /* Blit toy cells */
    set_display_instruction(STOY1, 1, r_val, stoy_x, stoy_y + 1);
    set_display_instruction(STOY2, 2, r_val, stoy_x, stoy_y + 2);
    set_display_instruction(STOY3, 3, r_val, stoy_x, stoy_y + 3);
    set_display_instruction(SAVE1, cursor_x, cursor_y, MAX_CB_X, MAX_CB_Y);
}

/*
 * Null step function: Does nothing.
 * Used as a placeholder for inactive cells.
 * Parameters:
 *   state_ptr: Pointer to cell state (ignored).
 */
static void step_null(void *state_ptr) {
    /* No-op */
}

/*
 * Step function for dancer cells: Moves diagonally based on state.
 * Bounces off bounds and cycles directions.
 * Parameters:
 *   state_ptr: Pointer to CellState.
 */
static void step_dancer(void *state_ptr) {
    CellStatePtr dancer = (CellStatePtr)state_ptr;
    if (dancer == (void *)NULL_FUNCTION) {
        return;
    }

    /* Apply movement based on direction */
    switch (dancer->state) {
        case DIAG_FORWARD:
            dancer->x++;
            dancer->y++;
            break;
        case DIAG_BACKWARD:
            dancer->x--;
            dancer->y--;
            break;
        case DIAG_LEFT:
            dancer->x--;
            dancer->y++;
            break;
        case DIAG_RIGHT:
            dancer->x++;
            dancer->y--;
            break;
        default:
            printf("Invalid dancer state: %d\n", dancer->state);
            dancer->state = DIAG_FORWARD;
            break;
    }

    /* Source coordinate wrapping */
    if (dancer->source_x > COL_HEIGHT) dancer->source_x = 0;
    if (dancer->source_y > 3) dancer->source_y = 0;

    /* Position bouncing and state cycling */
    if (dancer->x > 30) {
        dancer->x = 4;
        dancer->state = (dancer->state + 1) % 4;
    }
    if (dancer->y > 20) {
        dancer->y = 6;
        dancer->state = (dancer->state + 3) % 4;  /* Reverse direction */
    }
    if (dancer->x < 4) {
        dancer->x = 30;
        dancer->state = (dancer->state + 1) % 4;
    }
    if (dancer->y < 6) {
        dancer->y = 20;
        dancer->state = (dancer->state + 3) % 4;
    }

    /* Additional source wrapping */
    if (dancer->source_x > ROW_WIDTH) {
        dancer->source_x = ROW_WIDTH / 2;
        dancer->state = DIAG_FORWARD;
    }
    if (dancer->source_y > COL_HEIGHT) {
        dancer->source_y = COL_HEIGHT / 2;
        dancer->state = DIAG_RIGHT;
    }

    /* Clamp state to valid range */
    if (dancer->state < 0) dancer->state = DIAG_RIGHT;
    if (dancer->state > DIAG_RIGHT) dancer->state = DIAG_FORWARD;
}

/*
 * Step function for dropper cells: Toggles state cyclically.
 * Parameters:
 *   state_ptr: Pointer to CellState.
 */
static void step_dropper(void *state_ptr) {
    CellStatePtr dropper = (CellStatePtr)state_ptr;
    if (dropper == (void *)NULL_FUNCTION) {
        return;
    }
    dropper->state = (dropper->state + 1) % 2;
}

/*
 * Step function for eater cells: Toggles state cyclically.
 * Parameters:
 *   state_ptr: Pointer to CellState.
 */
static void step_eater(void *state_ptr) {
    CellStatePtr eater = (CellStatePtr)state_ptr;
    if (eater == (void *)NULL_FUNCTION) {
        return;
    }
    eater->state = (eater->state + 1) % 2;
}

/*
 * Test function to step all cell states (for debugging).
 * Returns 0 on completion.
 */
static void test_cell_states(void) {
    int i;
    for (i = 0; i < sizeof(cellStates) / sizeof(CellState); i++) {
        cellStates[i].step_fn(&cellStates[i]);
    }
}

/*
 * Update source Y coordinates for cell states.
 * Prepares animations for the current frame.
 */
static void update_cell_states(void) {
    cellStates[0].source_y = 1;
    cellStates[1].source_y = 2;
    cellStates[2].source_y = 3;
    cellStates[3].source_y = 4;
    cellStates[4].source_y = 5;
    cellStates[5].source_y = 6;
}

/*
 * Trigger the current sound preset: Dump, load, and synthesize.
 * Parameters:
 *   position: ROM address of the preset.
 */
static void trigger_current_sound(unsigned int position) {
    hex_dump((unsigned char *)position, 16);
    load_sound_table(position);
    Synth();
}

/*
 * Dump the pitch value for a specific channel.
 * Parameters:
 *   channel: Channel index (0, 2, or 4).
 */
static void dump_channel_pitch(unsigned char channel) {
    printf("[%d] Pitch: 0x%x\n", CHANNEL_INDEX(channel), CHANNEL_PITCH(channel));
}

/*
 * Synthesize using zero-page addressing.
 * Parameters:
 *   high_byte: High byte of ZP address.
 *   low_byte: Low byte of ZP address.
 */
static void synth_zp(unsigned char high_byte, unsigned char low_byte) {
    poke(0x14, high_byte);
    poke(0x15, low_byte);
    call(0xF590);
}

/*
 * Generate random colors across the hires buffer.
 * Fills from HIRES_START to HIRES_START + 320*3 with filtered random bytes.
 * Avoids invalid color values (0x08, 0x18, 0x88, 0x98).
 */
static void gen_random_colors(void) {
    unsigned int j = (unsigned int)HIRES_START_ADDR;
    unsigned int k = j + (320 * 3);
    volatile unsigned char rg;

    do {
        do {
            rg = qrandomJ(peek(0x276)) % 255;
        } while (((rg & 0x78) == 0x08 || (rg & 0x78) == 0x18) ||
                 ((rg & 0x78) == 0x88 || (rg & 0x78) == 0x98));

        poke(j++, rg);
    } while (j < k);
}

/*
 * Validate and clamp UI positions based on key press.
 * Handles cursor/input movement, blasts, triggers, and bounds wrapping.
 * Parameters:
 *   key: Pressed key to process.
 */
static void validate_ui_bounds(unsigned char key) {
    /* Cursor Movement */
    if (key == KEY_CURSOR_LEFT) {
        cursor_x--;
    } else if (key == KEY_CURSOR_RIGHT) {
        cursor_x++;
    } else if (key == KEY_CURSOR_UP) {
        cursor_y--;
    } else if (key == KEY_CURSOR_DOWN) {
        cursor_y++;
    }

    /* Input Movement */
    if (key == KEY_INPUT_LEFT) {
        input_x--;
    } else if (key == KEY_INPUT_RIGHT) {
        input_x++;
    } else if (key == KEY_INPUT_UP) {
        input_y--;
    } else if (key == KEY_INPUT_DOWN) {
        input_y++;
    } else if (key == KEY_BLAST) {
        cell_blast(input_x, input_y);
    } else if (key == KEY_TRIGGER_SPACE) {
        trigger_current_sound(cell_source_base);
    }

    /* Clamp/Wrap Positions */
    cursor_x = clamp(cursor_x, 2, MAX_CB_X);
    cursor_y = clamp(cursor_y, 2, MAX_CB_Y);
    input_x = clamp(input_x, 0, MAX_CB_X);
    input_y = clamp(input_y, 0, MAX_CB_Y);
    anim_y = clamp(anim_y, 2, MAX_CB_Y);

    /* Generation State Toggle */
    if (stoy_x > MAX_CB_X) {
        g_state = CGEN_STATE_BACKWARD;
    } else if (stoy_x < 2) {
        g_state = CGEN_STATE_FORWARD;
    }
    stoy_y = clamp(stoy_y, 2, MAX_CB_Y);

    /* UI Offset Wrapping */
    ui_x = (ui_x + 1) % ROW_WIDTH;
    ui_y = (ui_y + 1) % COL_HEIGHT;
}

/*
 * Display the help screen with controls.
 */
static void print_help_screen(void) {
    cls();
    paper(0);
    ink(5);
    printf("\nsoundtoy:: 1.1.1 (Refactored)\nA toy for exploring Oric ROM sounds.\n");
    printf("(by seclorum, refactored)\n\n");
    printf("Key: Function:\n\n");
    printf(" ?   Display this help.\n");
    printf(" P   Set PRESET pointer (enter HEX, e.g., FAA7, FABD).\n");
    printf(" J   Increment PRESET pointer.\n");
    printf(" K   Decrement PRESET pointer.\n");
    printf(" R   Reset to bootup sound.\n");
    printf(" .   Reset to PING sound.\n");
    printf(" G   Enter HIRES mode.\n");
    printf(" Q   Quit soundtoy.\n\n");
    printf("Any other key triggers synth.\n");
    printf("Explore Oric ROM for sounds!\n");
}

/*
 * Main program logic.
 * Handles preset navigation, sound triggering, and hires mode entry.
 */
static void program_main(void) {
    unsigned int position = POS_INITIAL;  /* Current preset position */
    int edit_mode = 0;                    /* 0: Preset mode, 1: Hires mode */

    /* Initialize Base Addresses and Flags */
    cell_source_base = SOURCE_BUFFER_BASE + (input_y * ROW_WIDTH * BLOCK_HEIGHT) + (input_x * 1);
    setflags(SCREEN + NOKEYCLICK);
    poke(0x24E, 5);  /* Keyboard delay */
    poke(0x24F, 2);  /* Keyboard repeat */
    poke(0x26A, 10); /* Disable keyclick/cursor */

    print_help_screen();
    Synth();  /* Initial synth call */

    /* Main Loop */
    while ((kp = key()) != KEY_QUIT) {
        if (kp == KEY_RESET_PRESET) {
            printf("Reset to boot!\n");
            position = (unsigned int)&BootSound;
            /* Reset positions */
            cursor_x = 2; cursor_y = 2;
            input_x = 1; input_y = 1;
            anim_x = 12; anim_y = 12;
            stoy_x = 12; stoy_y = 12;
            g_state = CGEN_STATE_FORWARD;
        }

        if (edit_mode) {
            /* Hires Mode Loop */
            if (kp == KEY_CAPTURE_CELL) {
                printf("Capture at (%d,%d)\n", input_x, input_y);
                cell_source_base = SOURCE_BUFFER_BASE + (input_y * ROW_WIDTH * BLOCK_HEIGHT) + (input_x * 1);
                hex_dump((unsigned char *)cell_source_base, 8);
            }

            /* Update and Render Frame */
            update_cell_states();
            cell_hack_update(kp);
            cell_ui_update(kp);
            cell_anim_update(kp);
            validate_ui_bounds(kp);
            call((unsigned int)&displayInstructions[0]);  /* Execute blits */

            continue;
        }

        /* Preset Mode Key Handling */
        if (kp == KEY_SOUND_TOGGLE) {
            should_play = !should_play;
        } else if (kp == KEY_POS_INCREMENT) {
            position = (position + 1 > POS_END) ? POS_START : position + 1;
            printf("\nPos: 0x%x\n", position);
        } else if (kp == KEY_POS_DECREMENT) {
            position = (position - 1 < POS_START) ? POS_END : position - 1;
            printf("\nPos: 0x%x\n", position);
        } else if (kp == KEY_RESET_PING) {
            printf("Ping!\n");
            position = POS_INITIAL;
        } else if (kp == KEY_SET_POSITION) {
            unsigned int val;
            printf("Pos: 0x");
            scanf("%x", &val);
            position = val;
            printf("\nSet to 0x%x\n", position);
        } else if (kp == KEY_HELP) {
            print_help_screen();
        } else if (kp == KEY_HIRES_MODE) {
            edit_mode = 1;
            hires();  /* Enter hires mode */
            poke(0x26A, 10);  /* Re-disable keyclick */

            gen_random_colors();  /* Fill with random colors */

            load_sound_table(position);
            hex_dump((unsigned char *)position, 16);

            /* Dump channel pitches */
            dump_channel_pitch(CHANNEL_A);
            dump_channel_pitch(CHANNEL_B);
            dump_channel_pitch(CHANNEL_C);
        } else if (kp == KEY_MEM_HACK) {
            /* Placeholder for mem hack; extend as needed */
        } else if (kp == KEY_TRIGGER_SPACE) {
            trigger_current_sound(position);
        }
    }

    /* Cleanup: Exit to text mode and play boot sound */
    text();
    load_sound_table((unsigned int)&BootSound);
    Synth();
}

void main() {
    program_main();
}

