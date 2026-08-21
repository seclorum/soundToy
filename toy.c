/*
 * soundtoy - A Sound Exploration Toy for Oric-1 Atmos
 *
 * Version: 1.2.0-dense (integrated dense-cell + exchange-rule system)
 * Author:  Original by Jay Vaughan – dense blob exchange by Grok
 *
 * Changes from 1.1.2-fix:
 *  • Added dense CELL representation and colour-exchange rules
 *  • Two 8-cell “blobs” (6×8 pixel regions) placed at left/right ends
 *    of the random colour field
 *  • Each frame the blobs move toward each other while exchanging
 *    sub-pixels according to the rule system
 *  • Existing UI / sound / dancer logic preserved and cleaned
 */

#include <lib.h>
#include "gencode.h"

/* ------------------------------------------------------------------ */
/* Dense-cell + exchange-rule system (inlined for single-file build)  */
/* ------------------------------------------------------------------ */

#define DENSE_PIXEL_MASK  0x3F
#define DENSE_FORCE_BIT6  0x40
#define DENSE_INVERSE     0x80

static unsigned char dense_pixel(unsigned char bits6, int inverse)
{
    return (bits6 & DENSE_PIXEL_MASK) | DENSE_FORCE_BIT6 | (inverse ? DENSE_INVERSE : 0);
}

typedef struct {
    unsigned char paper;   /* 0..7 */
    unsigned char ink;     /* 0..7 */
    unsigned char bits;    /* 6-bit pattern */
    unsigned char inv;     /* 0/1 */
} Cell;

#define CELL_BIT_COLOUR(c, bit) \
    ( ((c).inv) ? (((bit) ? (c).ink : (c).paper) ^ 7) : ((bit) ? (c).ink : (c).paper) )

static int cell_transfer_impl(Cell *src, Cell *dst,
                                     unsigned char colour,
                                     unsigned char leftover)
{
    int s, d;

    for (s = 0; s < 6; ++s)
        if (CELL_BIT_COLOUR(*src, s) == colour) break;
    if (s == 6) return 0;

    for (d = 0; d < 6; ++d)
        if (CELL_BIT_COLOUR(*dst, d) != colour) break;
    if (d == 6) return 0;

    /* vacate source bit → leftover */
    if (src->inv) {
        if ((leftover ^ 7) == src->paper)      src->bits &= ~(1 << s);
        else if ((leftover ^ 7) == src->ink)   src->bits |=  (1 << s);
        else { src->paper = leftover ^ 7; src->bits &= ~(1 << s); }
    } else {
        if (leftover == src->paper)            src->bits &= ~(1 << s);
        else if (leftover == src->ink)         src->bits |=  (1 << s);
        else { src->paper = leftover; src->bits &= ~(1 << s); }
    }

    /* plant colour in destination */
    if (dst->inv) {
        if ((colour ^ 7) == dst->paper)        dst->bits &= ~(1 << d);
        else if ((colour ^ 7) == dst->ink)     dst->bits |=  (1 << d);
        else { dst->ink = colour ^ 7; dst->bits |= (1 << d); }
    } else {
        if (colour == dst->paper)              dst->bits &= ~(1 << d);
        else if (colour == dst->ink)           dst->bits |=  (1 << d);
        else { dst->ink = colour; dst->bits |= (1 << d); }
    }
    return 1;
}

#define CELL_TRANSFER(src, dst, colour, leftover) \
    cell_transfer_impl(&(src), &(dst), (colour), (leftover))

#define RULE_BALANCE_TO_BLACK(src, dst, colour) \
    CELL_TRANSFER(src, dst, colour, 0)

#define RULE_GREEN_BALANCE(src, dst) \
    RULE_BALANCE_TO_BLACK(src, dst, 2)

#define RULE_RGB_BALANCE(src, dst) ( \
    RULE_BALANCE_TO_BLACK(src, dst, 1) | \
    RULE_BALANCE_TO_BLACK(src, dst, 2) | \
    RULE_BALANCE_TO_BLACK(src, dst, 4) )

/* Write a Cell as three dense bytes at HIRES address */
static void cell_write(Cell *c, unsigned char *addr)
{
    *addr++ = 0x10 | (c->paper & 7);
    *addr++ = 0x00 | (c->ink   & 7);
    *addr   = dense_pixel(c->bits, c->inv);
}

/* ------------------------------------------------------------------ */
/* Original constants / macros                                        */
/* ------------------------------------------------------------------ */

#define NULL 0

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

/* ------------------------------------------------------------------ */
/* Types                                                              */
/* ------------------------------------------------------------------ */

struct CellState;
typedef void (*CellStepFn)(struct CellState *);

typedef struct CellState {
    int source_x;
    int source_y;
    int x;
    int y;
    int state;
    CellStepFn step_fn;
} CellState;

/* ------------------------------------------------------------------ */
/* Blob system – 8 dense cells each, forming a 6×8 pixel region       */
/* ------------------------------------------------------------------ */

#define BLOB_CELLS 8
#define BLOB_ROWS  8          /* 8 scan-lines high */
#define BLOB_SPACING 3        /* columns between cells (3 bytes = dense cell) */

typedef struct {
    Cell cells[BLOB_CELLS];
    int  base_x;              /* column (0-39) of first cell */
    int  base_y;              /* scan-line (0-199) */
    int  dir;                 /* +1 = move right, -1 = move left */
} Blob;

static Blob blobL, blobR;

/* Initialise a blob with a strong colour bias */
static void blob_init(Blob *b, int x, int y, int dir, unsigned char bias)
{
    int i;
    b->base_x = x;
    b->base_y = 0;   /* start at top of screen */
    b->dir    = dir;
    for (i = 0; i < BLOB_CELLS; i++) {
        b->cells[i].paper = 0;               /* black paper */
        b->cells[i].ink   = bias;            /* strong bias colour */
        b->cells[i].bits  = 0x3F;            /* all six pixels on */
        b->cells[i].inv   = 0;
    }
}

/* Write the whole blob into HIRES memory */
static void blob_draw(Blob *b)
{
    int i, row;
    unsigned char *base = (unsigned char *)(0xA000 + b->base_y * ROW_WIDTH + b->base_x);

    for (row = 0; row < BLOB_ROWS; row++) {
        unsigned char *line = base + row * ROW_WIDTH;
        for (i = 0; i < BLOB_CELLS; i++) {
            cell_write(&b->cells[i], line + i * BLOB_SPACING);
        }
    }
}

/* Move blob one step toward the centre; clamp so they meet but do not cross */
static void blob_move(Blob *b)
{
    b->base_x += b->dir;
    b->base_y += 1;

    /* Bounce vertically: drop from top, bounce back up */
    if (b->base_y > 18) {
        b->base_y = 0;
        b->dir = +1;   /* bounce back DOWN */
    } else if (b->base_y < 0) {
        b->base_y = 18;
        b->dir = -1;   /* bounce back UP */
    }

    /* Bounce horizontally: meet at centre but do not cross */
    if (b->dir > 0 && b->base_x > 18) {
        b->base_x = 18;
        b->dir = -1;
    } else if (b->dir < 0 && b->base_x < 19) {
        b->base_x = 19;
        b->dir = +1;
    }
}

/* Exchange sub-pixels between corresponding cells of the two blobs */
static void blob_exchange(Blob *a, Blob *b)
{
    int i;
    for (i = 0; i < BLOB_CELLS; i++) {
        /* alternate green-balance and full RGB balance for visual variety */
        if (i & 1)
            RULE_GREEN_BALANCE(a->cells[i], b->cells[i]);
        else
            RULE_RGB_BALANCE(a->cells[i], b->cells[i]);
    }
}

/* ------------------------------------------------------------------ */
/* Globals                                                            */
/* ------------------------------------------------------------------ */

extern unsigned char currentSound[];

static int cursor_x = 12;
static int cursor_y = 12;
static int input_x  = 1;
static int input_y  = 1;
static int anim_x   = 12;
static int anim_y   = 12;
static int stoy_x   = 12;
static int stoy_y   = 12;

static unsigned char kp;
static unsigned char should_play = 1;
static int g_state = CGEN_STATE_INIT;

static CellState cellStates[] = {
    {0, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 0, 0},
    {1, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 1, 0},
    {2, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2, 0},
    {3, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3, 0},
    {4, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2, 0},
    {5, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3, 0}
};

/* ------------------------------------------------------------------ */
/* xorshift8                                                          */
/* ------------------------------------------------------------------ */

unsigned char xorshift8(unsigned char *xsy)
{
    *xsy ^= *xsy << 7;
    *xsy ^= *xsy >> 5;
    *xsy ^= *xsy << 3;
    return *xsy;
}

/* ------------------------------------------------------------------ */
/* Step functions                                                     */
/* ------------------------------------------------------------------ */

static void step_null(struct CellState *self) { self; }

static void step_dancer(struct CellState *self)
{
    if (!self) return;
    switch (self->state) {
        case 0: self->x++; self->y++; break;
        case 1: self->x--; self->y--; break;
        case 2: self->x--; self->y++; break;
        case 3: self->x++; self->y--; break;
    }
    if (self->x > 30) { self->x = 4;  self->state = (self->state + 1) % 4; }
    if (self->y > 20) { self->y = 6;  self->state = (self->state + 3) % 4; }
    if (self->x < 4)  { self->x = 30; self->state = (self->state + 1) % 4; }
    if (self->y < 6)  { self->y = 20; self->state = (self->state + 3) % 4; }
}

/* ------------------------------------------------------------------ */
/* Core helpers                                                       */
/* ------------------------------------------------------------------ */

static int clamp(int value, int min_val, int max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static void load_sound_table(unsigned int address)
{
    int i;
    for (i = 0; i < 14; i++)
        currentSound[i] = peek(address + i);
}

static void trigger_current_sound(unsigned int position)
{
    load_sound_table(position);
    Synth();
}

static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y)
{
    unsigned int src_base = SOURCE_BUFFER_BASE + (src_y * ROW_WIDTH * BLOCK_HEIGHT) + src_x;
    unsigned int dst_base = DEST_BUFFER_BASE   + (dst_y * ROW_WIDTH * BLOCK_HEIGHT) + dst_x;
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

static void update_cell_sources(void)
{
    cellStates[0].source_y = 1;
    cellStates[1].source_y = 2;
    cellStates[2].source_y = 3;
    cellStates[3].source_y = 4;
    cellStates[4].source_y = 5;
    cellStates[5].source_y = 6;
}

static void cell_blast(unsigned int ix, unsigned int iy)
{
    int col;
    for (col = 0; col < ROW_WIDTH;  col++) set_display_instruction(col, col, 0, col, 4);
    for (col = 0; col < COL_HEIGHT; col++) set_display_instruction(col, col, 1, 4, col);
}

static void cell_ui_update(unsigned char key)
{
    key = 0;
    set_display_instruction(CURS1, input_x, input_y, cursor_x, cursor_y);
    set_display_instruction(CURS2, input_x, input_y, cursor_x + 1, cursor_y + 1);
//    set_display_instruction(CURS1, input_x + 1, input_y, cursor_x, cursor_y + 1);
//    set_display_instruction(CURS2, input_x, input_y + 1, cursor_x + 1, cursor_y);
}

static void cell_anim_update(unsigned char key)
{
    int i;
    key = 0;
    for (i = 0; i < 6; i++) {
        CellState *c = &cellStates[i];
        if (c->step_fn) c->step_fn(c);
        set_display_instruction(ANIM1 + i, c->source_x, c->source_y, c->x, c->y);
    }
}

static void cell_hack_update(unsigned char key)
{
    int rval;
    key = 0;
    stoy_x += g_state;
    stoy_y += g_state;
    if (stoy_x > ROW_WIDTH)  stoy_x = 0;
    if (stoy_y > COL_HEIGHT) stoy_y = 0;
    rval = qrandomJ(peek(0x276)) % COL_HEIGHT;
    set_display_instruction(STOY1, 1, rval, stoy_x, stoy_y + 1);
    set_display_instruction(STOY2, 2, rval, stoy_x, stoy_y + 2);
}

static void validate_ui_bounds(unsigned char key)
{
    if (key == KEY_CURSOR_LEFT)  cursor_x--;
    if (key == KEY_CURSOR_RIGHT) cursor_x++;
    if (key == KEY_CURSOR_UP)    cursor_y--;
    if (key == KEY_CURSOR_DOWN)  cursor_y++;
    if (key == KEY_BLAST)        cell_blast(input_x, input_y);

    cursor_x = clamp(cursor_x, 2, 22);
    /* Extended Y range to allow cursor to move up from row 2 */
    cursor_y = clamp(cursor_y, 0, 19);
}

static void print_help_screen(void)
{
    cls();
    paper(0);
    ink(5);
    printf("\n soundtoy 1.2.0-dense\n\n");
    printf(" ?  help\n G  hires mode\n Q  quit\n");
    printf(" J/K  next/prev preset   R  boot   .  ping\n");
    printf(" P  set preset address\n SPACE  play current\n\n");
    printf("In hires:\n A D W S  move cursor\n B  blast\n");
    printf(" Blobs exchange colours while approaching\n");
}

/* Random colour field (entire hires screen) */
static void gen_random_colors(void)
{
    unsigned int j = 0xA000;
    unsigned int end = j + (320 * 24);
    unsigned char rg;
	unsigned char xsy = 1;
    do {
        do {
            rg = xorshift8(&xsy);
        } while ((rg & 0xF8) == 0x08 || (rg & 0xF8) == 0x18 ||
                 (rg & 0xF8) == 0x88 || (rg & 0xF8) == 0x98);
        poke(j++, rg);
    } while (j < end);
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

void main(void)
{
    unsigned int current_pos = POS_INITIAL;
    char edit_mode = 0;
    int i;

    for (i = 0; i < 6; i++)
        cellStates[i].step_fn = step_dancer;

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

                /* place the two blobs at opposite ends of the random field */
                blob_init(&blobL,  2,  0, +1, 4);   /* left,  green-biased, moves right */
                blob_init(&blobR, 30,  0, -1, 6);   /* right, red-biased, moves left */
                blob_draw(&blobL);
                blob_draw(&blobR);

                load_sound_table(current_pos);
                continue;
            }

            if (kp == KEY_POS_INCREMENT) current_pos++;
            if (kp == KEY_POS_DECREMENT) current_pos--;
            if (kp == KEY_RESET_PRESET)  current_pos = POS_INITIAL;
            if (kp == KEY_RESET_PING)    current_pos = POS_KEYCLICK;
            if (kp == KEY_SET_POSITION) {
                unsigned int val;
                printf("Pos: 0x"); scanf("%x", &val);
                current_pos = val;
            }
            if (kp == KEY_HELP) print_help_screen();
            if (kp == KEY_TRIGGER_SPACE || kp > 32)
                trigger_current_sound(current_pos);
            continue;
        }

        /* ---- HIRES mode ---- */
        if (kp == KEY_QUIT || kp == KEY_HIRES_MODE) {
            edit_mode = 0;
            text();
            continue;
        }

        validate_ui_bounds(kp);
        cell_ui_update(kp);
        cell_anim_update(kp);

        /* dense-blob exchange + movement (once per frame) */
        blob_exchange(&blobL, &blobR);
        blob_move(&blobL);
        blob_move(&blobR);
        blob_draw(&blobL);
        blob_draw(&blobR);

        call((unsigned int)&displayInstructions[0]);
    }

    text();
    load_sound_table(POS_INITIAL);
    Synth();
}
