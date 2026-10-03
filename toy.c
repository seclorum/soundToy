/*
   soundtoy - A Sound Exploration Toy for Oric-1 Atmos
  
   Version: 1.3.0-narrative (fixed attribute system + character-based narrative)
   Author:  Original by Jay Vaughan – dense blob exchange by Grok
  
   Changes from 1.2.0-dense:
    • Fixed Oric attribute system bugs: reduced blob width to 3 bytes (1 char)
    • Placed blobs at x=2 and x=37 to avoid scanline overflow (40-byte limit)
    • Replaced per-cell colour rules with *state-driven* rules (wall, living, affinity)
    • Added 40th byte as a *boundary communication channel* between blobs
    • Introduced Character abstraction with energy, age, affinity, rule_set
    • Vertical motion now independent of horizontal direction (dir_x, dir_y)
    • Blob interaction now based on *semantic states* (e.g., wall repels living)

   Notes:

	- The goal is to have as dense a colour environment as 
	possible, within the limits of the Orics' attribute system 
	while using that attribute system to add a dynamic nature 
	to the program. The rule set is intended to be able to 
	create 'characters' - 6x8 cells - which react to each other 
	based on the rule set.

	A "blob" is now a 3-byte-wide (1 character) × 8-scanline-tall region,
	placed at left/right ends of the random colour field.

	Each frame the blobs move toward each other while exchanging
	sub-pixels according to the rule system.

	Existing UI / sound / dancer logic preserved and cleaned.

   The Oric's attribute system is now *exploited*, not fought:

	- The first P/I pair on a scanline sets the colour for all subsequent bytes.
	- A 6×8 character is NOT self-contained — its visual identity depends on the preceding P/I pair.
	- This dependency is used to encode narrative: a "wall" (P=0,I=0) repels, a "living" cell (P=2,I=4) absorbs.

   The 40th byte (byte index 39) is used as a *boundary channel*:
	- Left blob writes its P/I state to byte 39 before its own [P][I][X].
	- Right blob reads byte 39 to detect the left blob's state.

   The Character abstraction adds:
		energy, age, affinity, rule_set
	- affinity determines which rules apply (e.g., 0=wall, 1=plant, 2=animal)
	- energy decays over time; if energy == 0, the character becomes a wall (P=0,I=0)

 . Bugs Fixed:

	- dir was used for both horizontal and vertical motion (now dir_x, dir_y)
	- Blob width was 24 bytes (overflowed 40-byte scanline) → now 3 bytes
	- Per-cell P/I pairs corrupted colour state → now single P/I pair per row
	- 40th byte ignored → now used as boundary channel

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
/* Character abstraction – 8 dense rows + narrative metadata          */
/* ------------------------------------------------------------------ */

typedef struct {
    Cell row[8];            // 8 scanlines, each with P/I/pixel
    unsigned char energy;   // 0–255 (decays over time)
    unsigned char age;      // 0–255
    unsigned char affinity; // 0=wall, 1=plant, 2=animal, etc.
    unsigned char rule_set; // index into rule table
} Character;

/* ------------------------------------------------------------------ */
/* Blob system – 1 character (3 bytes) wide, 8 scanlines tall         */
/* ------------------------------------------------------------------ */

#define BLOB_ROWS  8          /* 8 scan-lines high */
#define BLOB_WIDTH 3          /* 3 bytes = 1 character (P/I/X) */

typedef struct {
    Character character;
    int  x;                 /* column (0-37), must satisfy x+3 ≤ 40 */
    int  y;                 /* scan-line (0-192), must satisfy y+8 ≤ 200 */
    int  dir_x;             /* +1 = move right, -1 = move left */
    int  dir_y;             /* +1 = move down, -1 = move up */
} Blob;

static Blob blobL, blobR;

/* Initialise a blob with a strong colour bias */
static void blob_init(Blob *b, int x, int y, int dir_x, int dir_y, unsigned char affinity)
{
    int i;
    b->x = x;
    b->y = y;
    b->dir_x = dir_x;
    b->dir_y = dir_y;
    b->character.affinity = affinity;
    b->character.energy = 255;
    b->character.age = 0;
    b->character.rule_set = 0;

    for (i = 0; i < BLOB_ROWS; i++) {
        b->character.row[i].paper = 0;               /* black paper */
        b->character.row[i].ink   = (affinity == 0) ? 0 : (2 + (i % 6)); /* wall=black, others=varied */
        b->character.row[i].bits  = 0x3F;            /* all six pixels on */
        b->character.row[i].inv   = 0;
    }
}

/* Write the whole blob into HIRES memory */
static void blob_draw(Blob *b)
{
    int row;
    for (row = 0; row < BLOB_ROWS; row++) {
        unsigned char *line = (unsigned char *)(0xA000 + (b->y + row) * ROW_WIDTH + b->x);

        /* Write blob's own P/I/X (3 bytes) */
        cell_write(&b->character.row[row], line);

        /* If left blob, write its P/I state to byte 39 (boundary channel) */
        if (b->x == 2) {
            unsigned char *boundary = (unsigned char *)(0xA000 + (b->y + row) * ROW_WIDTH + 39);
            *boundary = 0x10 | (b->character.row[row].paper & 7); // P byte only (I follows in next line)
        }
    }
}

/* Move blob one step; clamp so they meet but do not cross */
static void blob_move(Blob *b)
{
    b->x += b->dir_x;
    b->y += b->dir_y;

    /* Bounce vertically: drop from top, bounce back up */
    if (b->y > 192) {
        b->y = 0;
        b->dir_y = +1;   /* bounce back DOWN */
    } else if (b->y < 0) {
        b->y = 192;
        b->dir_y = -1;   /* bounce back UP */
    }

    /* Bounce horizontally: meet at centre but do not cross */
    if (b->dir_x > 0 && b->x > 18) {
        b->x = 18;
        b->dir_x = -1;
    } else if (b->dir_x < 0 && b->x < 19) {
        b->x = 19;
        b->dir_x = +1;
    }

    /* Decay energy */
    if (b->character.energy > 0) b->character.energy--;
    b->character.age++;
}

/* Exchange sub-pixels between corresponding rows of the two blobs */
static void blob_exchange(Blob *a, Blob *b)
{
    /* Only interact if blobs are on the same scanline (y-coordinates match) */
    if (a->y != b->y) return;

    /* Check for horizontal overlap (x+3 > other_x && other_x+3 > a_x) */
    if (a->x + BLOB_WIDTH > b->x && b->x + BLOB_WIDTH > a->x) {
        for (int i = 0; i < BLOB_ROWS; i++) {
            /* Apply rule based on *semantic states* (affinity, energy) */
            if (a->character.affinity == 0 && a->character.energy > 0) {
                /* Left is a wall → repel right blob */
                b->dir_x = -b->dir_x;
            } else if (b->character.affinity == 0 && b->character.energy > 0) {
                /* Right is a wall → repel left blob */
                a->dir_x = -a->dir_x;
            } else {
                /* Living cells: exchange ink colours */
                unsigned char tmp = a->character.row[i].ink;
                a->character.row[i].ink = b->character.row[i].ink;
                b->character.row[i].ink = tmp;
            }
        }
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
    printf("\n soundtoy 1.3.0-narrative\n\n");
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
                /* Left blob: x=2, right blob: x=37 (3 bytes wide, 40-byte limit) */
                blob_init(&blobL,  2,  0, +1, +1, 1);   /* left,  living (affinity=1), moves right/down */
                blob_init(&blobR, 37,  0, -1, +1, 0);   /* right, wall (affinity=0), moves left/down */
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
