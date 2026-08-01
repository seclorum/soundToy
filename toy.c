/*
 * soundtoy - A Sound Exploration Toy for Oric-1 Atmos
 *
 * Version: 1.1.2-fix (2025 – fixed compilation, preserved original behaviour)
 * Author:  Original by Jay Vaughan – cleaned step-fn system
 *
 * This version:
 *  • Fixes name collision (kp instead of key)
 *  • Proper forward declaration + consistent struct pointer types
 *  • Keeps original UI, sound preset logic, blast, cursor, etc. unchanged
 *  • Introduces cleaner step_fn pattern (still mostly dancers for now)
 *  • Ready for adding dropper/eater/spark later without big breakage
 */

#include <lib.h>
#include "gencode.h"

#define NULL 0

/* ============================================================================
 * Constants and Macros      (kept almost identical to original)
 * ============================================================================ */

#define UI_Y_BASE          8
#define ROW_WIDTH          40
#define COL_HEIGHT         22
#define BLOCK_HEIGHT       8

#define SOURCE_BUFFER_BASE 0xA000
#define DEST_BUFFER_BASE   (0xA000 + (320 * 3))

#define POS_INITIAL        0xFAA7
#define POS_KEYCLICK       0xFB32

#define CGEN_STATE_INIT    0x11
#define CGEN_STATE_FORWARD 1
#define CGEN_STATE_BACKWARD -1

/* Keyboard Controls – original */
#define KEY_SOUND_TOGGLE   'H'
#define KEY_POS_INCREMENT  'J'
#define KEY_POS_DECREMENT  'K'
#define KEY_RESET_PRESET   'R'
#define KEY_RESET_PING     '.'
#define KEY_SET_POSITION   'P'
#define KEY_HELP           '?'
#define KEY_HIRES_MODE     'G'
#define KEY_QUIT           'Q'
#define KEY_CAPTURE_CELL   'C'
#define KEY_TRIGGER_SPACE  ' '
#define KEY_CURSOR_LEFT    'A'
#define KEY_CURSOR_RIGHT   'D'
#define KEY_CURSOR_UP      'W'
#define KEY_CURSOR_DOWN    'S'
#define KEY_BLAST          'B'

/* ============================================================================
 * Types & forward declarations
 * ============================================================================ */

struct CellState;   // forward declaration – fixes pointer-to-incomplete error

typedef void (*CellStepFn)(struct CellState *);

typedef struct CellState {
    int source_x;
    int source_y;
    int x;
    int y;
    int state;              // was direction, now more general
    CellStepFn step_fn;
} CellState;

/* ============================================================================
 * Globals – very close to original
 * ============================================================================ */

extern unsigned char currentSound[];

static int cursor_x = 2;
static int cursor_y = 2;
static int input_x  = 1;
static int input_y  = 1;
static int anim_x   = 12;
static int anim_y   = 12;
static int stoy_x   = 12;
static int stoy_y   = 12;

static unsigned char kp;                // <--- kept original name to avoid collision

static unsigned char should_play = 1;

static int g_state = CGEN_STATE_INIT;



/* ============================================================================
 * Function prototypes
 * ============================================================================ */

static int clamp(int value, int min_val, int max_val);
static void load_sound_table(unsigned int address);
static void trigger_current_sound(unsigned int position);

static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y);

static void cell_blast(unsigned int ix, unsigned int iy);
static void cell_ui_update(unsigned char key);
static void cell_anim_update(unsigned char key);
static void cell_hack_update(unsigned char key);

static void step_dancer(struct CellState *self);
static void step_null(struct CellState *self);

static void validate_ui_bounds(unsigned char key);
static void print_help_screen(void);
static void gen_random_colors(void);


static CellState cellStates[] = {
    {0, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 0 /* DIAG_FORWARD */,   step_null},
    {1, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 1 /* DIAG_BACKWARD */, step_null},
    {2, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2 /* DIAG_LEFT */,     step_null},
    {3, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3 /* DIAG_RIGHT */,    step_null},
    {4, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2 /* DIAG_LEFT */,     step_null},
    {5, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3 /* DIAG_RIGHT */,    step_null}
};

 
/* ============================================================================
 * Step functions
 * ============================================================================ */

static void step_null(struct CellState *self) {
    self;
}

static void step_dancer(struct CellState *self) {
    if (!self) return;

    switch (self->state) {
        case 0: self->x++; self->y++; break;
        case 1: self->x--; self->y--; break;
        case 2: self->x--; self->y++; break;
        case 3: self->x++; self->y--; break;
    }

    // Bounce & direction flip (original logic preserved)
    if (self->x > 30) { self->x = 4;  self->state = (self->state + 1) % 4; }
    if (self->y > 20) { self->y = 6;  self->state = (self->state + 3) % 4; }
    if (self->x < 4)  { self->x = 30; self->state = (self->state + 1) % 4; }
    if (self->y < 6)  { self->y = 20; self->state = (self->state + 3) % 4; }
}

/* ============================================================================
 * Core functions (mostly original)
 * ============================================================================ */

static int clamp(int value, int min_val, int max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static void load_sound_table(unsigned int address) {
    int i;
    for (i = 0; i < 14; i++) {
        currentSound[i] = peek(address + i);
    }
}

static void trigger_current_sound(unsigned int position) {
    load_sound_table(position);
    Synth();
}

static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y) {
    unsigned int src_base = SOURCE_BUFFER_BASE + (src_y * ROW_WIDTH * BLOCK_HEIGHT) + (src_x * 1);
    unsigned int dst_base = DEST_BUFFER_BASE   + (dst_y * ROW_WIDTH * BLOCK_HEIGHT) + (dst_x * 1);
    int row;

    if (cell_idx >= MAX_CB_DISPLAY_INSTRUCTIONS) return 1;

    for (row = 0; row < BLOCK_HEIGHT; row++) {
        unsigned int s = src_base + (row * ROW_WIDTH);
        unsigned int d = dst_base + (row * ROW_WIDTH);

        displayInstructions[cell_idx][3 + row * 6] = s & 0xFF;
        displayInstructions[cell_idx][4 + row * 6] = s >> 8;
        displayInstructions[cell_idx][6 + row * 6] = d & 0xFF;
        displayInstructions[cell_idx][7 + row * 6] = d >> 8;
    }
    return 0;
}

static void update_cell_sources(void) {
    cellStates[0].source_y = 1;
    cellStates[1].source_y = 2;
    cellStates[2].source_y = 3;
    cellStates[3].source_y = 4;
    cellStates[4].source_y = 5;
    cellStates[5].source_y = 6;
}

static void cell_blast(unsigned int input_x, unsigned int input_y) {
    int col;
    for (col = 0; col < ROW_WIDTH;  col++) set_display_instruction(col, col, 0, col, 4);
    for (col = 0; col < COL_HEIGHT; col++) set_display_instruction(col, col, 1, 4, col);
}

static void cell_ui_update(unsigned char key) {
    key=0;//unused for now
    set_display_instruction(CURS1, input_x, input_y, cursor_x, cursor_y);
    set_display_instruction(CURS2, input_x, input_y, cursor_x + 1, cursor_y + 1);
    set_display_instruction(CURS1, input_x + 1, input_y, cursor_x, cursor_y + 1);
    set_display_instruction(CURS2, input_x, input_y + 1, cursor_x + 1, cursor_y);
}

static void cell_anim_update(unsigned char key) {
    int i;
    key=0;//unused for now
    for (i = 0; i < 6; i++) {
        CellState *c = &cellStates[i];
        if (c->step_fn) c->step_fn(c);
        set_display_instruction(ANIM1 + i, c->source_x, c->source_y, c->x, c->y);
    }
}

static void cell_hack_update(unsigned char key) {
    // original stub – kept for compatibility
    int rval;
    key=0;//unused for now
    stoy_x += g_state;
    stoy_y += g_state;
    if (stoy_x > ROW_WIDTH)  stoy_x = 0;
    if (stoy_y > COL_HEIGHT) stoy_y = 0;

    rval = qrandomJ(peek(0x276)) % COL_HEIGHT;
    set_display_instruction(STOY1, 1, rval, stoy_x, stoy_y + 1);
    set_display_instruction(STOY2, 2, rval, stoy_x, stoy_y + 2);
}

static void validate_ui_bounds(unsigned char key) {
    if (key == KEY_CURSOR_LEFT)  cursor_x--;
    if (key == KEY_CURSOR_RIGHT) cursor_x++;
    if (key == KEY_CURSOR_UP)    cursor_y--;
    if (key == KEY_CURSOR_DOWN)  cursor_y--;

    if (key == KEY_BLAST) {
        cell_blast(input_x, input_y);
    }

    cursor_x = clamp(cursor_x, 2, 37);
    cursor_y = clamp(cursor_y, 2, 19);
}

static void print_help_screen(void) {
    cls();
    paper(0);
    ink(5);
    printf("\n soundtoy 1.1.2-fix\n\n");
    printf(" ?  help\n G  hires mode\n Q  quit\n");
    printf(" J/K  next/prev preset   R  boot   .  ping\n");
    printf(" P  set preset address\n SPACE  play current\n\n");
    printf("In hires:\n A D W S  move cursor\n B  blast\n");
}

static void gen_random_colors(void) {
    unsigned int j = 0xA000;
    unsigned int end = j + 320*3;
    unsigned char rg;
    do {
#ifdef USE_SAFE_TABLE
	rg = GetSafeColour();
#else
        do {
            rg = qrandomJ(peek(0x276)) & 255;
        } while ((rg & 0xF8) == 0x08 || (rg & 0xF8) == 0x18 ||
                 (rg & 0xF8) == 0x88 || (rg & 0xF8) == 0x98);
#endif
        poke(j++, rg);
    } while (j < end);
}

/* ============================================================================
 * Main
 * ============================================================================ */

void main(void) {
    unsigned int current_pos = POS_INITIAL;
    char edit_mode = 0;

    int i;
    for (i = 0; i < 6; i++) {
        cellStates[i].step_fn = step_dancer;   // all dancers for now – original behaviour
    }

    update_cell_sources();

    setflags(SCREEN + NOKEYCLICK);
    poke(0x24E, 5);
    poke(0x24F, 2);
    poke(0x26A, 10);

    print_help_screen();
    Synth();

    while ((kp = key()) != KEY_QUIT) {
        if (!edit_mode) {
            if (kp == KEY_HIRES_MODE) {
                edit_mode = 1;
                hires();
                poke(0x26A, 10);
                gen_random_colors();
                load_sound_table(current_pos);
                continue;
            }

            if (kp == KEY_POS_INCREMENT) current_pos++;
            if (kp == KEY_POS_DECREMENT) current_pos--;
            if (kp == KEY_RESET_PRESET) current_pos = POS_INITIAL;
            if (kp == KEY_RESET_PING)   current_pos = POS_KEYCLICK;
            if (kp == KEY_SET_POSITION) {
                unsigned int val;
                printf("Pos: 0x"); scanf("%x", &val);
                current_pos = val;
            }
            if (kp == KEY_HELP) print_help_screen();
            if (kp == KEY_TRIGGER_SPACE || kp > 32) {
                trigger_current_sound(current_pos);
            }

            continue;
        }

        // hires mode
        if (kp == KEY_QUIT || kp == KEY_HIRES_MODE) {
            edit_mode = 0;
            text();
            continue;
        }

        validate_ui_bounds(kp);
        cell_ui_update(kp);
        cell_anim_update(kp);
        //cell_hack_update(kp);

        call((unsigned int)&displayInstructions[0]);
    }

    text();
    load_sound_table(POS_INITIAL);
    Synth();
}
