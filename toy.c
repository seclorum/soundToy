/*
   soundtoy - A Sound Exploration Toy for Oric-1 Atmos

   Version: 1.5.4-creatures (exploits the Oric serial-attribute system)
   Author:  Original by Jay Vaughan

   Goal: the most colour-dense graphics the Oric attribute system allows,
   and use the attribute system itself to animate / make cells interact.

   The Oric attribute rules this toy exploits:

    - A byte is an ATTRIBUTE iff (v & 0x60) == 0.  Everything else is
      6-pixel PIXEL DATA (bit 6 or bit 5 set, bit 7 = inverse).
    - The first P/I pair on a scanline sets the colour for all
      subsequent bytes on that line, until the next attribute.
      => ONE attribute byte write recolours the whole segment to its
         right (this is the wavefront / sweep mechanism).
    - A [P][I][X] triple is one clash-free dense cell (paper, ink,
      6 sub-pixels).  Bit 7 (inverse) SWAPS the paper/ink roles - the
      hardware does not complement the colour values (1.4.0 modelled
      it as (P^7,I^7), which is wrong).
    - Video-control attributes (0x18-0x1F / 0x98-0x9F) would switch
      TEXT/HIRES machine-wide, so the random field NEVER contains them.

   Exploits in this version:

    - SEED FIELD: the random colour field (which seeds the cells, the
      blobs and the osmosis context) is generated ONLY in the top 1/3
      of the screen (rows 0..66).  Below that the screen is void:
      INK-black attribute bytes that never draw pixels.
    - TWINKLE: shimmer_step() (shimmer.s) EORs bit 7 of random PIXEL
      bytes in a round-robin over 9 bands covering the seed field, so
      every field row animates evenly.  Bit 7 never turns a pixel into
      an attribute, so the field stays legal while cells shimmer
      between their two paper/ink pairings.
    - WAVEFRONT: shimmer inverts one random ATTRIBUTE byte per frame;
      inverting an INK/PAPER attribute recolours everything to its
      right with a single byte write.
    - SWEEPS: shimmer writes a [PAPER][INK] attribute PAIR (both
      colours; the old sweep fixed only the paper and left the ink
      stale) at a random spot - colour "weather".
    - OSMOSIS: touching blobs exchange sub-pixels of the primaries via
      the CELL_TRANSFER rules, plus the colour of the field they are
      standing on (environment osmosis, read back through the shadow);
      successful transfers feed the characters (energy).
    - LIFE CYCLE: energy decays; at zero a living cell becomes a black
      wall (P=0,I=0) and repels other blobs; age revives walls.
    - MAILBOX: row 199 (mostly under vertical overscan) is the blob
      boundary channel: states are published there BEFORE the exchange
      reads them (1.4.0 read them a frame late).  Every telemetry byte
      is ENCODED into a legal class (0x20|n pixel / 0x10|n paper): the
      gate array scans even the hidden rows machine-wide, and raw
      energy values 24-31/152-159 used to deposit live video-control
      attributes (0x18-0x1F / 0x98-0x9F) that flipped 50/60Hz.
    - INVARIANT (validate_field + shimmer guards): the screen NEVER
      contains a style (0x08-0x0F/0x88-0x8F) or video-control
      (0x18-0x1F/0x98-0x9F) attribute.  Only PAPER/INK (+inverse) and
      pixel bytes are legal writes; the style/video exclusion that
      1.4.0's generator guaranteed (IS_VIDEO_CTRL) is enforced at the
      twinkle/wavefront writes AND healed by validate_field().
    - DITHER: at contact rows the two blobs take turns showing the
      other's ink per frame, so the inks interleave into a perceived
      blend without corrupting cell state (1.4.0 XOR-accumulated the
      pixel bits and drifted forever).
    - SOUND COUPLING: blob energy/affinity modulate currentSound[] and
      the shimmer flip budget (shm_flips) every frame; bursts of
      successful transfers, deaths and revivals retrigger the sound.
    - FRAME DRIVEN: per-frame non-blocking poll (STOY_KBD_MODE):
        0 = ROM key() poll - the proven 1.4 path (the OSDK library
            key() is ALREADY non-blocking: grsimple.s jsr $023B; the
            buffer is filled by the ROM FIRQ).  Default.
        1 = 6502 matrix scan chained inside the IRQ (keyboard.s
            _KbdIrqInstall) - no sei window, no ROM race.
        3 = legacy main-loop scan (1.5.0) - kept for bisect only:
            the sei'd AY/VIA writes race the ROM FIRQ's keyboard
            buffer + sound replay and freeze the ROM player.
      frame_tick() also degrades to free-run if the tick dies.
    - BLIT CHAIN: the displayInstruction blocks run as a prefix chain
      ending in RTS at the highest used block (1.4.0 always executed
      all 40 blocks, ~3/4 of them copying zero page onto itself).
    - BLAST: uses dedicated blocks 12..37 so the cursor/ANIM blocks
      survive (1.4.0 blast reprogrammed exactly the blocks the per-
      frame UI update then overwrote, making it invisible).
    - LAYERING: the blit runs BEFORE the blob restore/draw, so blobs
      are the top layer and dancers no longer scribble over them (and
      blob_restore no longer erases dancers mid-blob).

   Engine fixes over 1.4.0-attribute:

    - init_safe_map() no longer reuses the class bits as colour bits:
      it produced ONLY paper=black / ink=1 / inv-paper=1 (a "random
      colour" field that was secretly monochrome).  It now spans all
      8 papers / 8 inks and also uses the legal PIX_BIT5 pixel class
      (0x20-0x3F / 0xA0-0xBF) for extra texture.
    - blob_exchange energy feed clamps at 255 (249+8 used to wrap to 1
      and starve healthy blobs).
    - RULE_MUTUAL counts transfers with '+' not '|'.
    - Wall repulsion keys off affinity (walls live at energy 0, so the
      old 'energy > 0' gate made dead walls harmless and the initial
      wall blob, spawned at energy 255, repel for the wrong reason).
    - Blobs bounce off each other symmetrically after both have moved
      (the old clamp compared against an already-moved position and
      locked both blobs moving right at equal x).
    - frame_tick waits one 50 Hz tick (it waited 18 = ~3 fps while
      claiming one tick).
    - 'P' reads hex manually (OSDK stdio has no %x).
    - CELL_BIT_COLOUR models inverse as a paper/ink SWAP (hardware)
      rather than a colour complement.
    - shimmer row picker uses real mod arithmetic by subtraction (the
      1.4 'and #191' was a bitmask and skipped rows 64..127 entirely).

   1.5.4-creatures (this release):

    - CREATURES FLESHED OUT: blobs are 2 dense cells (36 px) wide with a
      body/face half; dancers 0..3 got a second column via the formerly
      dead STOY1..3/SAVE1 blocks (chain end and blast span unchanged)
      plus source-column churn and a step_orbit step fn; the cursor is
      a 12 px two-column glyph and 'C' cycles its source bank cell.
    - rule_set is LIVE: plants osmose green-only (RULE_BALANCE_TO_BLACK
      both ways), animals run full RULE_MUTUAL across both halves.
    - VOID STATUS: energy bars + affinity pips (legal attrs/pixels only)
      at rows 176/188, refreshed every 2 frames.
    - H toggles sound (should_play finally read); help screen updated.

   1.5.3-arrows:

    - HIRES cursor moves with the ORIC cursor keys: the ROM key buffer
      codes are 0x1A/0x9A (left/right) and 0x1D/0x9D (up/down) - the
      bit7 carries the pair direction, so poll_key() must pass those
      four through unmasked.
    - TEXT-mode details restored from the 1.1 era: preset position echo
      on J/K/R/P, hexDump + per-channel pitch dump of the preset bytes
      when entering HIRES.
    - shadow_screen is FIELD_ROWS tall again; a 200-row shadow spilled
      into the screen ($A000+) and silently aliased it (blob structs
      were being DRAWN on screen).  CMake now hard-fails the link if
      the data segment reaches $A000.
*/

#include <lib.h>
#include "gencode.h"

/* ------------------------------------------------------------------ */
/* Debug / bisect toggles (1.5.1 keyboard-freeze investigation)       */
/*                                                                    */
/* The 1.5.0 release polled the keyboard by scanning the 6502 matrix  */
/* from main (sei + raw AY/VIA writes).  That races the ROM FIRQ,     */
/* which owns the same VIA/AY lines for the keyboard buffer and the   */
/* AY sound replay - and it hung the ROM sound player ($FA86, called  */
/* by Synth()) on the first keypress.  Modes below isolate the cause. */
/* ------------------------------------------------------------------ */

#ifndef STOY_KBD_MODE
#define STOY_KBD_MODE 1       /* 1 = ROM IRQ chain does the matrix scan */
                              /* 1 = matrix scan inside IRQ chain     */
                              /* 2 = keyboard disabled (bisect)       */
                              /* 3 = legacy main-loop scan (freezes!) */
#endif
#ifndef STOY_NO_SYNTH_ON_KEY
#define STOY_NO_SYNTH_ON_KEY 0 /* 1 = skip Synth() on text-mode keys */
#endif
#ifndef STOY_NO_FRAMEWAIT
#define STOY_NO_FRAMEWAIT 0    /* 1 = frame_tick returns immediately */
#endif
#ifndef STOY_NO_VIA_RNG
#define STOY_NO_VIA_RNG 1      /* 0 = shimmer RNG reads T1CL/T2CL    */
                              /* (acks ROM timer latches - unsafe)   */
#endif

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

/* Hardware inverse SWAPS the roles: bit set -> paper, bit clear -> ink */
#define CELL_BIT_COLOUR(c, bit) \
    ( ((c).inv) ? ((bit) ? (c).paper : (c).ink) : ((bit) ? (c).ink : (c).paper) )

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

    /* vacate source bit -> leftover */
    if (src->inv) {
        /* bit set displays paper, bit clear displays ink */
        if (leftover == src->ink)         src->bits &= ~(1 << s);
        else if (leftover == src->paper)  src->bits |=  (1 << s);
        else { src->ink = leftover; src->bits &= ~(1 << s); }
    } else {
        if (leftover == src->paper)       src->bits &= ~(1 << s);
        else if (leftover == src->ink)    src->bits |=  (1 << s);
        else { src->paper = leftover; src->bits &= ~(1 << s); }
    }

    /* plant colour in destination */
    if (dst->inv) {
        if (colour == dst->paper)         dst->bits |= (1 << d);
        else if (colour == dst->ink)      dst->bits &= ~(1 << d);
        else { dst->paper = colour; dst->bits |= (1 << d); }
    } else {
        if (colour == dst->paper)         dst->bits &= ~(1 << d);
        else if (colour == dst->ink)      dst->bits |= (1 << d);
        else { dst->ink = colour; dst->bits |= (1 << d); }
    }
    return 1;
}

#define CELL_TRANSFER(src, dst, colour, leftover) \
    cell_transfer_impl(&(src), &(dst), (colour), (leftover))

#define RULE_BALANCE_TO_BLACK(src, dst, colour) \
    CELL_TRANSFER(src, dst, colour, 0)

/* '+' not '|': count BOTH direction successes (1.4.0 saturated at 1) */
#define RULE_MUTUAL(src, dst, colourX, colourY, leftX, leftY) \
    ( CELL_TRANSFER(src, dst, colourX, leftX) + \
      CELL_TRANSFER(dst, src, colourY, leftY) )

/* Write a Cell as three dense bytes at addr */
static void cell_write(Cell *c, unsigned char *addr)
{
    *addr++ = 0x10 | (c->paper & 7);
    *addr++ = 0x00 | (c->ink   & 7);
    *addr   = dense_pixel(c->bits, c->inv);
}

/* ------------------------------------------------------------------ */
/* Original constants / macros                                        */
/* ------------------------------------------------------------------ */

#define UI_Y_BASE          8
#define ROW_WIDTH          40
#define COL_HEIGHT         22
#define BLOCK_HEIGHT       8

#define HIRES_BASE         0xA000
#define FIELD_ROWS         67    /* seed field = top 1/3: rows 0..66    */
#define MAILBOX_ROW        199   /* blob boundary channel (off-screen)  */
#define BLOB_BOTTOM        (FIELD_ROWS - BLOB_ROWS) /* blobs stay in the seed field */

#define POS_INITIAL        0xFAA7
#define POS_KEYCLICK       0xFB32

#define KEY_SOUND_TOGGLE   'H'
#define KEY_POS_INCREMENT  'J'
#define KEY_POS_DECREMENT  'K'
#define KEY_RESET_PRESET   'R'
#define KEY_RESET_PING     '.'
#define KEY_SET_POSITION   'P'
#define KEY_HELP           'V'   /* matrix scanner has no '?' mapping   */
#define KEY_HIRES_MODE     'G'

/* debug harness: auto-press G after N idle menu frames (0 = off) */
#ifndef STOY_AUTOG
#define STOY_AUTOG 0
#endif

/* debug harness: jump straight into HIRES on the first frame (no G key) */
#ifndef STOY_AUTOHIRES
#define STOY_AUTOHIRES 1
#endif

/* debug harness: RTI-mute the RAM interrupt vectors at hires entry */
#ifndef STOY_MUTE_IRQ
#define STOY_MUTE_IRQ 0
#endif

/* debug harness: sanity-check blob blit operands; each failure mode spins
 * in its own function so the emulator can log which one (exec bp on entry) */
#ifndef STOY_CHECK
#define STOY_CHECK 0
#endif
#if STOY_CHECK
static int spin_row(void)  { int z; z = 0; for (;;) { z = z + 1; } return z; }
static int spin_y(void)    { int z; z = 1; for (;;) { z = z + 1; } return z; }
static int spin_dst(void)  { int z; z = 2; for (;;) { z = z + 1; } return z; }
static int spin_py(void)   { int z; z = 3; for (;;) { z = z + 1; } return z; }
static int spin_dst2(void) { int z; z = 4; for (;;) { z = z + 1; } return z; }
static int spin_s1(void)   { int z; z = 5; for (;;) { z = z + 1; } return z; }
static int spin_s2(void)   { int z; z = 6; for (;;) { z = z + 1; } return z; }
static int spin_s3(void)   { int z; z = 7; for (;;) { z = z + 1; } return z; }
static int spin_s4(void)   { int z; z = 8; for (;;) { z = z + 1; } return z; }
static int spin_s5(void)   { int z; z = 9; for (;;) { z = z + 1; } return z; }
static int spin_s6(void)   { int z; z = 10; for (;;) { z = z + 1; } return z; }
static int spin_s7(void)   { int z; z = 11; for (;;) { z = z + 1; } return z; }
static int spin_s8(void)   { int z; z = 12; for (;;) { z = z + 1; } return z; }
static int spin_s9(void)   { int z; z = 13; for (;;) { z = z + 1; } return z; }
static int spin_kq(void)   { int z; z = 14; for (;;) { z = z + 1; } return z; }
static int spin_p8(void)   { int z; z = 15; for (;;) { z = z + 1; } return z; }
static int spin_p16(void)  { int z; z = 16; for (;;) { z = z + 1; } return z; }
static int spin_p32(void)  { int z; z = 17; for (;;) { z = z + 1; } return z; }
static int spin_p48(void)  { int z; z = 18; for (;;) { z = z + 1; } return z; }
#endif

/* the ROM SYNTH driver (JSR $FA86) walks the ROM sound queue at
 * $0534..$0570 - which our $550-based load has overwritten - so the ROM
 * 50Hz handler plays garbage and STAs through wild pointers.  The
 * vendored keyboard.s synth_safe writes AY reg0..15 directly instead. */
extern void synth_safe(void);
#define KEY_QUIT           'Q'
#define KEY_TRIGGER_SPACE  ' '
/* ROM key-buffer codes for the ORIC cursor keys.  Bit7 is MEANINGFUL
 * here (1A/9A = left/right, 1D/9D = up/down) - poll_key() must not
 * mask these four.  (kernel.rom loads #$1D/#$9D for the up/down pair.) */
#define KEY_CURSOR_LEFT    0x1A
#define KEY_CURSOR_RIGHT   0x9A
#define KEY_CURSOR_UP      0x1D
#define KEY_CURSOR_DOWN    0x9D
#define KEY_BLAST          'B'
#define KEY_CAPTURE        'C'   /* cycle the cursor source bank cell */

#define CB_UI_FIRST        CURS1            /* 4                       */
#define CB_UI_LAST         ANIM6            /* 11 - default chain end  */
#define CB_BLAST_FIRST     12               /* blast gets its own span */
#define CB_BLAST_LAST      37

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

typedef struct {
    Cell row[8];            /* body half: 8 scanlines, P/I/pixel  */
    Cell row2[8];           /* face half: second dense cell       */
    unsigned char energy;   /* decays; 0 -> becomes wall          */
    unsigned char age;
    unsigned char affinity; /* 0=wall, 1=plant, 2=animal          */
    unsigned char rule_set; /* 1=plant green-balance, 2=animal    */
} Character;

/* ------------------------------------------------------------------ */
/* Blob system - 2 dense cells (6 bytes = 36 px) wide, 8 scanlines    */
/* ------------------------------------------------------------------ */

#define BLOB_ROWS  8
#define BLOB_WIDTH 6
/* rightmost legal x so bytes x..x+5 stay inside the 40-byte row */
#define BLOB_RIGHT 34

typedef struct {
    Character character;
    unsigned char x, y;       /* current top-left byte position      */
    unsigned char px, py;     /* previous position (for trail erase) */
    unsigned char min_x, max_x;
    signed char dir_x, dir_y;
} Blob;

static Blob blobL, blobR;

/* ------------------------------------------------------------------ */
/* Globals                                                            */
/* ------------------------------------------------------------------ */

extern unsigned char currentSound[];

/* shared with shimmer.s (linker resolves _shadow_screen etc.) */
/* Authoritative random field, FIELD_ROWS rows only.  The void below the
 * seed field is all 0x00 (INK-black) and needs no storage - field_put
 * screens it, every shadow reader is field-bounded.  NEVER make this
 * 200 rows: 8000 B of BSS + program tail pushes it past $A000, where it
 * aliases the live screen (blob structs landed in screen memory once). */
unsigned char shadow_screen[ROW_WIDTH * FIELD_ROWS];
unsigned char row_off_lo[FIELD_ROWS];          /* (y*40) low  byte table     */
unsigned char row_off_hi[FIELD_ROWS];          /* (y*40) high byte table     */
extern unsigned char shm_flips;                /* twinkle budget (shimmer.s) */
extern void shm_init(void);
extern void disable_irq(void);                 /* SEI  (block-patch atomic) */
extern void enable_irq(void);                  /* CLI                       */                    /* pin ZP scratch inits       */
#if STOY_MUTE_IRQ
extern void mute_irq(void);                    /* RTI the RAM vectors        */
#endif

/* keyboard.s entry points (OSDK C char return: low byte in X) */
extern unsigned char PollKeyMain();   /* mode 3: scan from main       */
extern unsigned char PollKeyIrq();    /* mode 1: scan done in IRQ     */
extern void ScanKeyboard(void);       /* raw matrix scan (keyboard.s) */
extern unsigned char shm_via;         /* shimmer RNG entropy source   */
extern unsigned char sd_cell[], sd_sx[], sd_sy[], sd_dx[], sd_dy[];
extern unsigned char sd_dil[], sd_dih[], sd_pbl[], sd_pbh[];
extern void sdi_patch(void);          /* set_display_instruction asm  */
extern unsigned char gc_gpl[], gc_gph[];
extern unsigned char ar_rpb[], ar_rpb2[];
extern unsigned char gc_ar_asm(void); /* gc_affects_right asm         */
extern unsigned char gc_lok_asm(void);/* gc_left_ok asm               */

/* OSDK lib (irq.s): official RAM-vector IRQ chaining; the callback
   runs inside the IRQ chain with A/X/Y + C temporaries saved. */
extern void chain_irq_handler(void (*fn)(void));

#if STOY_KBD_MODE == 1
static void kbd_irq_cb(void)
{
    ScanKeyboard();
}
#endif

static unsigned char *row_ptr[200];            /* screen row pointers        */
static unsigned char  pattern_bank[320 * 8];   /* off-screen pattern bank    */
static unsigned char  safe_map[256];           /* random byte -> legal byte  */
static unsigned char  xsy = 0x6D;              /* xorshift8 seed (nonzero)   */
static unsigned char  frame_count = 0;
static unsigned char  last_g = 0;              /* transfers last exchange    */
static unsigned char  g_event = 0;             /* sound-retrigger event flag */

static unsigned char  cb_highest = CB_UI_LAST; /* chain terminator position  */
static int            blast_hold = 0;          /* frames left of blast state */
static int            blast_max_idx = CB_UI_LAST;

static int cursor_x = 12;
static int cursor_y = 12;
static int input_x  = 1;
static int input_y  = 1;

static unsigned char kp;
static unsigned char should_play = 1;

static CellState cellStates[] = {
    {0, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 0, 0},
    {1, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 1, 0},
    {2, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2, 0},
    {3, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3, 0},
    {4, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 2, 0},
    {5, 0, ROW_WIDTH / 2, COL_HEIGHT / 2, 3, 0}
};

/* ------------------------------------------------------------------ */
/* Random helpers                                                     */
/* ------------------------------------------------------------------ */

static unsigned char rnd8(void)
{
    xsy ^= xsy << 7;
    xsy ^= xsy >> 5;
    xsy ^= xsy << 3;
    return xsy;
}

/*
 * Map any 8-bit value onto a byte that is legal in the random field:
 *   12.5% PAPER, 12.5% INK, 12.5% inv-PAPER, 12.5% PIX_BIT5 pixels,
 *   50% plain pixels.  Style (0x08-0x0F) and video (0x18-0x1F)
 * attributes are NEVER produced, plain or inverse, so the field can
 * never flip TEXT/HIRES.
 *
 * 1.4.0 reused (i & 7) for the colour bits after using it to pick the
 * class, so the field could ONLY ever hold paper=black / ink=1 /
 * inv-paper=1.  Colours now come from (i >> 4) and the legal
 * PIX_BIT5 pixel class (0x20-0x3F, one pixel always lit) is used too.
 */
static void init_safe_map(void)
{
    int i;
    unsigned char v, cls, col;
    for (i = 0; i < 256; i++) {
        cls = (unsigned char)(i & 7);
        col = (unsigned char)((i >> 4) & 7);
        if (cls == 0)            v = (unsigned char)(0x10 | col);
        else if (cls == 1)       v = col;
        else if (cls == 2)       v = (unsigned char)(0x90 | col);
        else if (cls == 3)       v = (unsigned char)(0x20 | (i & 0x1F) | (i & 0x80));
        else                     v = (unsigned char)(0x40 | (i & 0x3F) | (i & 0x80));
        safe_map[i] = v;
    }
}

/* ------------------------------------------------------------------ */
/* Step functions                                                     */
/* ------------------------------------------------------------------ */

/* shared cell-grid bounce (cell units, 8 px per cell row) */
static void bounce_bounds(struct CellState *self)
{
    if (self->x > 30) { self->x = 4;  self->state = (self->state + 1) % 4; }
    if (self->y > 20) { self->y = 6;  self->state = (self->state + 3) % 4; }
    if (self->x < 4)  { self->x = 30; self->state = (self->state + 1) % 4; }
    if (self->y < 6)  { self->y = 20; self->state = (self->state + 3) % 4; }
}

static void step_dancer(struct CellState *self)
{
    if (!self) return;
    switch (self->state) {
        case 0: self->x++; self->y++; break;
        case 1: self->x--; self->y--; break;
        case 2: self->x--; self->y++; break;
        case 3: self->x++; self->y--; break;
    }
    bounce_bounds(self);
}

/* wider diagonals - breaks the six-clones look of 1.5.x */
static void step_orbit(struct CellState *self)
{
    if (!self) return;
    switch (self->state) {
        case 0: self->x += 2; self->y++; break;
        case 1: self->x -= 2; self->y--; break;
        case 2: self->x--;    self->y += 2; break;
        case 3: self->x++;    self->y -= 2; break;
    }
    bounce_bounds(self);
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

static void init_geometry(void)
{
    int y;
    int off;
    for (y = 0; y < 200; y++)
        row_ptr[y] = (unsigned char *)(HIRES_BASE + y * ROW_WIDTH);
    for (y = 0; y < FIELD_ROWS; y++) {
        off = y * ROW_WIDTH;
        row_off_lo[y] = (unsigned char)(off & 0xFF);
        row_off_hi[y] = (unsigned char)(off >> 8);
    }
}

#if STOY_CHECK
/* canary: verify row_ptr[] still holds its init_geometry() values */
static void chk_rp(int stage)
{
    int i;
    unsigned int p, want;
    want = (unsigned int)HIRES_BASE;
    for (i = 0; i < 200; i++, want += ROW_WIDTH) {
        p = (unsigned int)row_ptr[i];
        if (p != want) {
            switch (stage) {
            case 0: spin_s1();
            case 1: spin_s2();
            case 2: spin_s3();
            case 3: spin_s4();
            case 4: spin_s5();
            case 5: spin_s6();
            case 6: spin_s7();
            case 7: spin_s8();
            default: spin_s9();
            }
        }
    }
}
#endif

/* write a field byte into both the screen and the shadow copy */
static void field_put(int x, int y, unsigned char v)
{
    if (y < FIELD_ROWS)
        shadow_screen[y * ROW_WIDTH + x] = v;   /* shadow = seed field only */
    row_ptr[y][x] = v;
}

/*
 * The seed field (colours everything: cells, blobs, osmosis context,
 * pattern bank is drawn from the same safe_map) is generated ONLY in
 * the top 1/3 of the screen (rows 0..FIELD_ROWS-1).  The rest is void:
 * INK-black attribute bytes (0x00) which never draw pixels, so the
 * lower screen stays clean black even when shimmer touches it.
 */
/*
 * Blank the ENTIRE 8 KB video region $A000-$BFFF, including the $BF40+
 * tail and the $BB80 text-page rows that the gate array composites as the
 * bottom "system" line.  TEXT-mode printf() leaves glyphs (e.g. the
 * "[1][p:0000]" preset dump) there, and gen_random_colors only fills up to
 * MAILBOX_ROW, so without this the leftover text persists under HIRES.
 * Every byte is a legal blank (0x00 = INK-black attribute: draws no
 * pixels, never a style/video-control attribute), so nothing outside the
 * HIRES pixel area is corrupted.
 */
static void clear_screen_full(void)
{
    unsigned char *p = (unsigned char *)HIRES_BASE;
    int i;
    for (i = 0; i < 8192; i++)
        p[i] = 0;
}

static void gen_random_colors(void)
{
    int x, y;
    for (y = 0; y < FIELD_ROWS; y++)
        for (x = 0; x < ROW_WIDTH; x++)
            field_put(x, y, safe_map[rnd8()]);
    for (y = FIELD_ROWS; y < MAILBOX_ROW; y++)
        for (x = 0; x < ROW_WIDTH; x++)
            field_put(x, y, 0);
    for (x = 0; x < ROW_WIDTH; x++)
        field_put(x, MAILBOX_ROW, 0);
}

static void gen_pattern_bank(void)
{
    int i;
    for (i = 0; i < 320 * 8; i++)
        pattern_bank[i] = safe_map[rnd8()];
}

/*
 * INVARIANT: the screen must NEVER contain a style attribute
 * (0x08-0x0F / 0x88-0x8F) or a video-control attribute
 * (0x18-0x1F / 0x98-0x9F).  The latter flips 50/60 Hz and TEXT/HIRES
 * machine-wide the instant the raster scans them - ANYWHERE in screen
 * memory, including the hidden mailbox row.  Only PAPER/INK (and
 * their inverse forms) and pixel bytes are legal.
 *
 * (v & 0x78) == 0x08 -> style, (v & 0x78) == 0x18 -> video control.
 * This is the IS_VIDEO_CTRL/IS_STYLE check that 1.4.0's generator
 * comments guaranteed; 1.5.0's rewrite silently dropped it from the
 * shimmer paths and the mailbox never had it at all.
 */
static unsigned char is_illegal_attr(unsigned char v)
{
    unsigned char t;
    if ((v & 0x60) != 0) return 0;           /* pixel byte: always legal */
    t = (unsigned char)(v & 0x78);
    return (unsigned char)(t == 0x08 || t == 0x18);
}

/* Heal any byte that violates the invariant (belt AND braces) */
static void validate_field(void)
{
    int x, y;
    int base = 0;
    unsigned char v;
    for (y = 0; y < FIELD_ROWS; y++) {
        for (x = 0; x < ROW_WIDTH; x++, base++) {
            v = shadow_screen[base];
            if (is_illegal_attr(v))
                field_put(x, y, safe_map[v]);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Game cells: the seed field is the SOURCE of 6x8-px cells (1 byte   */
/* wide, 8 scanlines tall) that drop into the void below FIELD_ROWS.  */
/*                                                                  */
/* Oric attribute physics (what the rules below are really testing): */
/* rendering walks the screen linearly left-to-right; INK ($0x-$7)   */
/* and PAPER ($10-$17) bytes (bit7 = inverse) set the render state    */
/* that persists for every PIXEL byte to the right until re-set.     */
/* The void is $00 (INK-black), so ink bleed self-heals one byte     */
/* later, but PAPER/inverse bleed leaks rightward across the whole   */
/* screen: those cells must slide left until their left neighbour     */
/* hands them the state they were authored with (or the edge).       */
/* Pure-pixel cells (no attribute bytes at all) are "monochrome"     */
/* and drift right instead.                                          */
/*                                                                  */
/* CRITICAL: no function-local `static` vars anywhere - OSDK C       */
/* pools those at $4000, straight over program code.                 */
/* ------------------------------------------------------------------ */

#ifndef STOY_CELLS
#define STOY_CELLS   1     /* 0 = compile out the falling-cell game (profiling) */
#endif

#define GC_MAX       12
#define GC_BOTTOM    191   /* highest legal ytop (bytes ytop..ytop+7 <= 198) */
#define GC_SPAWN_P   8     /* one spawn attempt every N frames       */

typedef struct {
    unsigned char x, y;        /* left byte column / top byte row    */
    unsigned char live;        /* 0 free, 1 moving, 2 landed         */
    unsigned char snap[8];     /* the cell's 8 screen bytes          */
    unsigned char expect[8];   /* (paper<<4|ink) incoming at spawn   */
} GameCell;

static GameCell gcells[GC_MAX];
/* occupancy bitmap: only rows FIELD_ROWS..199 ever hold cells, so the
 * array is offset by FIELD_ROWS (a full 200-row map pushed osdk_end past
 * $A000 into the screen - see the build-time overlap guard). */
#define GC_ROWS 133                        /* rows 67..199 inclusive    */
static unsigned char gc_occ[GC_ROWS][5];
static unsigned char gc_spawn_ctr = 0;

static void gc_occ_set(int x, int y, int v)
{
    int r;
    if (y < FIELD_ROWS || y >= FIELD_ROWS + GC_ROWS) return;
    r = y - FIELD_ROWS;
    if (v) gc_occ[r][x >> 3] |= (unsigned char)(1 << (x & 7));
    else   gc_occ[r][x >> 3] &= (unsigned char)~(1 << (x & 7));
}

static int gc_occ_get(int x, int y)
{
    int r;
    if (y < FIELD_ROWS || y >= FIELD_ROWS + GC_ROWS) return 0;
    r = y - FIELD_ROWS;
    return (gc_occ[r][x >> 3] >> (x & 7)) & 1;
}

/* erase the 8 bytes a cell currently occupies */
static void gc_clear(GameCell *g)
{
    int r;
    unsigned char *rowp;
    for (r = 0; r < 8; r++) {
        rowp = row_ptr[g->y + r];
        rowp[g->x] = 0x00;
        gc_occ_set(g->x, g->y + r, 0);
    }
}

static void gc_put(GameCell *g)
{
    int r;
    unsigned char *rowp;
    for (r = 0; r < 8; r++) {
        rowp = row_ptr[g->y + r];
        rowp[g->x] = g->snap[r];
        gc_occ_set(g->x, g->y + r, 1);
    }
}

/* are all 8 bytes plain pixels? ($40-$7F: bits set, no attr, no inv) */
static int gc_monochrome(GameCell *g)
{
    int r;
    for (r = 0; r < 8; r++)
        if ((g->snap[r] & 0xC0) != 0x40) return 0;
    return 1;
}

/*
 * Render-state update INLINED as a macro: these loops run thousands of
 * times per frame (12 cells x 8 rows x row walks) and the OSDK function
 * call overhead dominated the whole game tick (~1M cycles/frame).
 * b with bits6/5 set is a pixel byte: it never touches (paper,ink,inv).
 * NOTE: no function-local statics anywhere (OSDK pools them at $4000).
 */
#define GC_APPLY(b, p, i, v)                       \
    if (((b) & 0x60) == 0) {                       \
        if ((b) & 0x80) (v) = 1; else (v) = 0;     \
        if ((b) & 0x10) (p) = (unsigned char)((b) & 7); \
        else            (i) = (unsigned char)((b) & 7); \
    }

/* render state entering byte (x,y), walking the row prefix.
 * Pointer walk, not row_ptr[y][k]: 16-bit indexing dominates on OSDK. */
static void gc_state_at(int y, int x, unsigned char *pap, unsigned char *ink)
{
    unsigned char p = 0, i = 0, v = 0, b;
    unsigned char *rowp = row_ptr[y];
    int k;
    for (k = 0; k < x; k++) {
        b = *rowp++;
        if (b == 0) {                    /* void byte: cheap identity   */
            if (p == 0 && i == 0 && v == 0) continue;
        }
        GC_APPLY(b, p, i, v);
    }
    *pap = p; *ink = i;
}

/*
 * Would placing g at (x,y) alter the rendered colour of anything to
 * its right?  Compare two walks over the rest of each of the 8 rows:
 * WITH the cell's byte vs WITHOUT it (the void's $00 baseline).  A
 * divergence that survives (different ink under lit pixels / different
 * paper under unlit pixels) means the cell would recolour the screen.
 *
 * FAST PATH: if the cell's byte is a pixel byte it cannot change the
 * render state, so both walks start identical and immediately converge
 * - the whole row is provably unaffected and is skipped.  Only rows
 * holding a real attribute byte pay for the double walk, and the walk
 * stops as soon as the two states re-converge.
 */
static int gc_affects_right(GameCell *g)
{
    unsigned int a = (unsigned int)g;
    gc_gpl[0] = (unsigned char)a;
    gc_gph[0] = (unsigned char)(a >> 8);
    return (int)gc_ar_asm();
}

/* does the column to the left hand the cell the state it expects? */
static int gc_left_ok(GameCell *g)
{
    unsigned int a = (unsigned int)g;
    gc_gpl[0] = (unsigned char)a;
    gc_gph[0] = (unsigned char)(a >> 8);
    return (int)gc_lok_asm();
}

/* is the destination column free for the cell at (x, y)?  Used for
 * slide-left / scroll-right, where the target column differs from the
 * cell's own, so no self-exclusion is needed. */
static int gc_col_free(int x, int y)
{
    int r;
    if (y + 7 > GC_BOTTOM + 7) return 0;
    for (r = 0; r < 8; r++)
        if (gc_occ_get(x, y + r)) return 0;
    return 1;
}

/* Can g fall one row?  The cell's own body already occupies (g->x,
 * g->y..g->y+7); the fall target (g->x, g->y+1..g->y+8) overlaps that
 * body on rows g->y+1..g->y+7, so those must NOT count as obstacles -
 * otherwise a cell sees itself and rests forever.  Only the one new
 * bottom row (g->y+8) is foreign.  Caller bounds g->y < GC_BOTTOM. */
static int gc_fall_free(GameCell *g)
{
    if (g->y + 8 > GC_BOTTOM + 7) return 0;   /* byte <= row 198 */
    return !gc_occ_get(g->x, g->y + 8);
}

static void gc_spawn(void)
{
    GameCell *g;
    int i, r, x;
    unsigned char p, i2;

    x = rnd8() & 31;                       /* stay off the last edge col */
    if (x >= 39) x = 38;
    if (!gc_col_free(x, FIELD_ROWS)) return;
    g = 0;
    for (i = 0; i < GC_MAX; i++) if (!gcells[i].live) { g = &gcells[i]; break; }
    if (!g) {                              /* pool full: recycle the    */
        int best = -1;                     /* deepest settled cell so   */
        for (i = 0; i < GC_MAX; i++)       /* the rain never dries up   */
            if (gcells[i].live == 2 &&
                (best < 0 || gcells[i].y > gcells[best].y)) best = i;
        if (best < 0) return;              /* all still moving: skip    */
        gc_clear(&gcells[best]);
        gcells[best].live = 0;
        g = &gcells[best];
    }

    g->x = (unsigned char)x;
    g->y = FIELD_ROWS;
    for (r = 0; r < 8; r++) {
        /* source: bottom of the seed field; the field itself stays */
        g->snap[r] = row_ptr[FIELD_ROWS - 8 + r][x];
        gc_state_at(FIELD_ROWS - 8 + r, x, &p, &i2);
        g->expect[r] = (unsigned char)(p << 4) | i2;
    }
    g->live = 1;
    gc_put(g);
}

/* one tick: apply the fall / slide-left / scroll-right rules */
static void gc_step(GameCell *g)
{
    if (g->live != 1) return;

    if (gc_monochrome(g)) {                /* rule: scroll right      */
        if (g->x < 39 && gc_col_free(g->x + 1, g->y)) {
            gc_clear(g); g->x++; gc_put(g);
            return;
        }
        g->live = 2;                       /* edge/wall: come to rest   */
        return;                            /* monochrome never falls  */
    }

    /* A cell falls when placing it leaves the screen to its right
     * unchanged, OR once it has slid far enough left that its left
     * neighbour hands it exactly the (paper,ink) it was authored with
     * (gc_left_ok also covers x==0, the screen edge).  Otherwise the
     * cell's attributes would recolour the screen, so it MUST slide
     * left until the edge or a compatible neighbour, then drop. */
    if (!gc_affects_right(g) || gc_left_ok(g)) {
        if (g->y < GC_BOTTOM && gc_fall_free(g)) {
            gc_clear(g); g->y++; gc_put(g);
            return;
        }
        g->live = 2;                       /* bottom reached: rest      */
        return;
    }

    /* would recolour the right: slide left first                     */
    if (g->x > 0 && gc_col_free(g->x - 1, g->y)) {
        gc_clear(g); g->x--; gc_put(g);
        return;
    }
    /* edge or boxed in with no compatible neighbour: settle anyway   */
    if (g->y < GC_BOTTOM && gc_fall_free(g)) {
        gc_clear(g); g->y++; gc_put(g);
        return;
    }
    g->live = 2;
}

void cells_init(void)
{
    int i, y;
    for (i = 0; i < GC_MAX; i++) gcells[i].live = 0;
    for (y = 0; y < GC_ROWS; y++) { gc_occ[y][0]=0; gc_occ[y][1]=0;
                                    gc_occ[y][2]=0; gc_occ[y][3]=0; gc_occ[y][4]=0; }
    gc_spawn_ctr = 0;
    for (i = 0; i < 6; i++) gc_spawn();    /* seed the rain           */
}

void cells_tick(void)
{
    int i;
    if (++gc_spawn_ctr >= GC_SPAWN_P) {
        gc_spawn_ctr = 0;
        gc_spawn();
    }
    for (i = 0; i < GC_MAX; i++) gc_step(&gcells[i]);
}

/*
 * Non-blocking key poll.  Mode 0 uses the OSDK library key() itself
 * (ROM keyboard buffer, maintained by the ROM FIRQ - the proven 1.4
 * path, zero VIA access from main).  bit7 handling is masked out so
 * ASCII comparisons are reliable regardless of the ROM's flag state -
 * EXCEPT for the four cursor codes (1A/9A/1D/9D), whose bit7 is what
 * distinguishes right from left and down from up: those pass through
 * raw, matching the KEY_CURSOR_* defines.
 */
static unsigned char poll_key(void)
{
    unsigned char k;
#if STOY_KBD_MODE == 0
    k = (unsigned char)key();
#elif STOY_KBD_MODE == 1
    k = PollKeyIrq();
#elif STOY_KBD_MODE == 2
    k = 0;
#else
    k = PollKeyMain();
#endif
    if (k == KEY_CURSOR_LEFT  || k == KEY_CURSOR_RIGHT ||
        k == KEY_CURSOR_UP    || k == KEY_CURSOR_DOWN)
        return k;
    return (unsigned char)(k & 0x7F);
}

/*
 * Wait for the next 50 Hz tick using the ROM frame counter ($A0).
 * Hardened: if the tick stops advancing (a killed ROM FIRQ - e.g. a
 * VIA-race bug), degrade to free-running after ~12 missed waits
 * instead of stalling ~0.25 s every frame ("looks frozen" failure).
 */
static unsigned char tick_dead = 0;

static void frame_tick(void)
{
    unsigned char t;
    unsigned int guard;
#if STOY_NO_FRAMEWAIT
    return;
#else
    if (tick_dead > 12) return;
    t = peek(0xA0);
    for (guard = 0; guard < 5000 && peek(0xA0) == t; guard++)
        ;
    if (guard >= 5000) {
        if (tick_dead < 255) tick_dead++;
    } else {
        tick_dead = 0;
    }
#endif
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
    synth_safe();
}

/*
 * TEXT-mode detail display (restored from the 1.1 era): hex dump of the
 * ROM preset bytes at the current position + per-channel pitch readout,
 * plus position feedback on every J/K/R/P edit.
 */
#define CHAN_A 0
#define CHAN_B 2
#define CHAN_C 4
#define CHAN_INDEX(x) ((x) / 2)
#define PITCH_WORD(x) \
    ((unsigned int)currentSound[x] + ((unsigned int)currentSound[x + 1] << 8))

/* CRITICAL: OSDK C allocates function-local `static` (non-const) vars in
   a pool at $4000 - which our program code occupies.  All such variables
   MUST live at file scope (BSS) or the compiler scribbles over code. */
static char hexString[3];
#if STOY_AUTOG
static char autog_left;
#endif
#if STOY_AUTOHIRES
static char first_auto_done;
#endif

static char *charHexStr(unsigned char value)
{
    static const char hexLookup[] = "0123456789ABCDEF";
    hexString[0] = hexLookup[(value >> 4) & 0x0F];
    hexString[1] = hexLookup[value & 0x0F];
    hexString[2] = '\0';
    return hexString;
}

static void hexDump(unsigned char *addr, int length)
{
    int cnt = 0;
    int i;
    printf("p:%x\n", (unsigned int)addr);
    for (i = 0; i < length; i++) {
        if (cnt++ > 7) {
            cnt = 0;
            printf("\n");
        }
        printf(" %s", charHexStr(addr[i]));
    }
    printf("\n");
}

static void dumpPitch(unsigned char channel)
{
    printf("[%d][p:%x]\n", CHAN_INDEX(channel), PITCH_WORD(channel));
}

static void dump_preset_details(unsigned int position)
{
    load_sound_table(position);
    printf("\n");
    hexDump((unsigned char *)position, 16);
    dumpPitch(CHAN_A);
    dumpPitch(CHAN_B);
    dumpPitch(CHAN_C);
}

/* manual hex entry (OSDK stdio has no %x) */
static unsigned int read_hex4(void)
{
    unsigned int v = 0;
    unsigned char k, i;
    for (i = 0; i < 4; i++) {
        k = 0;
        while (k == 0) {
            k = poll_key();
            frame_tick();
        }
        if (k >= '0' && k <= '9')      v = (v << 4) | (unsigned int)(k - '0');
        else if (k >= 'a' && k <= 'f') v = (v << 4) | (unsigned int)(k - 'a' + 10);
        else if (k >= 'A' && k <= 'F') v = (v << 4) | (unsigned int)(k - 'A' + 10);
        else continue;                 /* ignore non-hex noise */
        putchar(k);
    }
    return v;
}

/* ------------------------------------------------------------------ */
/* Blit engine (gencode.h blocks)                                     */
/* ------------------------------------------------------------------ */

/*
 * The 40 blocks fall through into each other; only blocks 0..cb_highest
 * need to execute.  Byte 50 of the last executed block is patched to
 * RTS, everything below it stays NOP - 1.4.0 always ran all 40 blocks,
 * ~3/4 of them copying zero page onto itself.
 */
static void cb_init(void)
{
    int i;
    disable_irq();
    for (i = 0; i < MAX_CB_DISPLAY_INSTRUCTIONS; i++)
        displayInstructions[i][50] = 0xEA;                   /* NOP  */
    displayInstructions[MAX_CB_DISPLAY_INSTRUCTIONS - 1][50] = 0x60; /* keep RTS */
    displayInstructions[CB_UI_LAST][50] = 0x60;              /* chain end */
    cb_highest = CB_UI_LAST;
    blast_hold = 0;
    blast_max_idx = CB_UI_LAST;
    enable_irq();
}

/*
 * Copy 8 scanlines from the OFF-SCREEN pattern bank to a screen cell.
 * (The bank lives in RAM, not on-screen rows 0-6, so the blit can
 * never alias against its own sources mid-frame.)
 */
static int set_display_instruction(int cell_idx, int src_x, int src_y, int dst_x, int dst_y)
{
    /* block 39 must keep its RTS terminator */
    if (cell_idx >= MAX_CB_DISPLAY_INSTRUCTIONS - 1) return 1;

    disable_irq();          /* ROM executes this chain from its 1ms IRQ */
    sd_cell[0] = (unsigned char)cell_idx;
    sd_sx[0]   = (unsigned char)src_x;
    sd_sy[0]   = (unsigned char)src_y;
    sd_dx[0]   = (unsigned char)dst_x;
    sd_dy[0]   = (unsigned char)dst_y;
    sdi_patch();            /* 8x(LDA/STA abs) immediate rewrite in asm */

    if (cell_idx > cb_highest) {
        displayInstructions[cb_highest][50] = 0xEA;          /* old end -> NOP */
        displayInstructions[cell_idx][50] = 0x60;            /* new end -> RTS */
        cb_highest = (unsigned char)cell_idx;
    }
    enable_irq();
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

void stepEater(void *v) {
	cellStateTP eater = (cellStateTP)v;

	if (eater == NULL_FN) {
		return;
	}

    eater->state = (eater->state + 1) % 2; // Toggle between states
}

// Example function to demonstrate usage
int testStates() {
	int i;
	for ( i = 0; i < sizeof(cellStates) / sizeof(cellStateT); ++i) {
		cellStates[i].stepFunction(&cellStates[i]);
	}
	return 0;
}

void updateStates() { 
	cellStates[0].sy = 1;
	cellStates[1].sy = 2;
	cellStates[2].sy = 3;
	cellStates[3].sy = 4;
	cellStates[4].sy = 5;
	cellStates[5].sy = 6;
}



char *charHexStr(unsigned char value)
{
	static char hexString[3];
	static const char hexLookup[] = "0123456789ABCDEF";
	hexString[0] = hexLookup[(value >> 4) & 0x0F];
	hexString[1] = hexLookup[value & 0x0F];
	hexString[2] = '\0';
	return hexString;
}

void hexDump(unsigned char *addr, int length)
{
	int cnt = 0;
	int i = 0;

	printf("p:%x\n", addr);

	for (i = 0; i < length; i++) {
		if (cnt++ > 7) {
			cnt = 0;
			printf("\n");
		}
		printf(" %s", charHexStr(addr[i]));
		// if (i>13) printf(":"); else printf(" ");
	}
	printf("\n");
}

loadTable(unsigned int address)
{
	currentSound[0] = peek(address);
	currentSound[1] = peek(address + 1);
	currentSound[2] = peek(address + 2);
	currentSound[3] = peek(address + 3);
	currentSound[4] = peek(address + 4);
	currentSound[5] = peek(address + 5);
	currentSound[6] = peek(address + 6);
	currentSound[7] = peek(address + 7);
	currentSound[8] = peek(address + 8);
	currentSound[9] = peek(address + 9);
	currentSound[10] = peek(address + 10);
	currentSound[11] = peek(address + 11);
	currentSound[12] = peek(address + 12);
	currentSound[13] = peek(address + 13);
}

// Set the Display Instruction at Cell
// X,Y values 40x20
int setDisplayInstruction(int cell, int sourceX, int sourceY, int destX, int destY)
{

	unsigned int buffer_base =
	SOURCE_BUFFER_START + (sourceY * ROW_LENGTH * BLOCK_HEIGHT) +
	(sourceX * 1);
	unsigned int dest_base =
	DESTINATION_BUFFER_START + (destY * ROW_LENGTH * BLOCK_HEIGHT) +
	(destX * 1);

	if (cell >= MAX_CB_DISPLAY_INSTRUCTIONS) {
	} else {
		// Modify the source and destination addresses for each row (8 rows)
		int pos;
		for (pos = 0; pos < BLOCK_HEIGHT; pos++) {
			// Source address modification
			cell_source_addr = buffer_base + (pos * ROW_LENGTH);
			displayInstructions[cell][3 + pos * 6] = cell_source_addr & 0xFF;	// Low byte
			displayInstructions[cell][4 + pos * 6] = (cell_source_addr >> 8) & 0xFF;	// High byte

			// Destination address modification
			cell_dest_addr = dest_base + (pos * ROW_LENGTH);
			displayInstructions[cell][6 + pos * 6] = cell_dest_addr & 0xFF;	// Low byte
			displayInstructions[cell][7 + pos * 6] = (cell_dest_addr >> 8) & 0xFF;	// High byte
		}

		return 0;
	}
	return 1;
}


#include <lib.h>

// =============================================
// Self-modifying arbitrary 7-byte bit shifter
// Targets Oric Atmos HIRES (screen at $A000)
// =============================================

#define MAX_SHIFT_CODE_SIZE   2048   // Generous - actual usage ~1.1KB for 56 bits
#define BLOB_BYTES            7
#define MAX_SHIFT_BITS        32

unsigned char shiftCode[MAX_SHIFT_CODE_SIZE];

// Pre-build the template once at startup
void initShiftCode(void)
{
    unsigned int pos = 0;
    int bit, b;

    // Initial setup - clear carry
    shiftCode[pos++] = 0x18;        // CLC

    // Unrolled shift sequence for up to 56 bits
    // For each bit: 7 × (LDA abs + ROL/ROR + STA abs)
    for (bit = 0; bit < MAX_SHIFT_BITS; bit++) {
        for (b = 0; b < BLOB_BYTES; b++) {
            // LDA $xxxx     (absolute - address will be patched)
            shiftCode[pos++] = 0xAD;
            shiftCode[pos++] = 0x00;   // Low byte placeholder
            shiftCode[pos++] = 0xA0;   // High byte placeholder (default $A000)

            // Rotate opcode placeholder: 0x2A = ROL, 0x6A = ROR
            shiftCode[pos++] = 0x2A;   // Default to ROL (left)

            // STA $xxxx
            shiftCode[pos++] = 0x8D;
            shiftCode[pos++] = 0x00;   // Low byte placeholder
            shiftCode[pos++] = 0xA0;   // High byte placeholder
        }
    }

    // End marker
    shiftCode[pos++] = 0x60;        // RTS

    // Fill remainder with NOPs
    while (pos < MAX_SHIFT_CODE_SIZE) {
        shiftCode[pos++] = 0xEA;
    }
}

// Main setup function - call this when shift amount/direction or position changes
// shift_amount: positive = left shift, negative = right shift
// base_addr:    starting address of the 7-byte blob (e.g. 0xA000)
void setBlobShift(signed char shift_amount, unsigned int base_addr)
{
    int effective_bits;
    unsigned char rotate_op;
    unsigned int pos = 1;   // Skip initial CLC (offset 0)
    int bit, b;
    unsigned int addr;

    // Normalize shift amount (mod 56)
    if (shift_amount < 0) {
        effective_bits = (-shift_amount) % MAX_SHIFT_BITS;
        rotate_op = 0x6A;               // ROR (right shift)
    } else {
        effective_bits = shift_amount % MAX_SHIFT_BITS;
        rotate_op = 0x2A;               // ROL (left shift)
    }

    if (effective_bits == 0) {
        // Special case: no shift - just RTS (we can patch first instruction to RTS if wanted, but for simplicity we leave CLC + many NOPs)
        // For zero we could optimize further, but this is fine.
    }

    // Patch base address into ALL LDA/STA instructions
    for (bit = 0; bit < MAX_SHIFT_BITS; bit++) {
        for (b = 0; b < BLOB_BYTES; b++) {
            addr = base_addr + b;

            // LDA address
            shiftCode[pos + 1] = addr & 0xFF;           // low
            shiftCode[pos + 2] = (addr >> 8) & 0xFF;    // high

            // Rotate opcode
            shiftCode[pos + 3] = (bit < effective_bits) ? rotate_op : 0xEA;  // NOP after desired bits

            // STA address
            shiftCode[pos + 5] = addr & 0xFF;
            shiftCode[pos + 6] = (addr >> 8) & 0xFF;

            pos += 7;   // 7 bytes per byte-operation (LDA + rot + STA)
        }
    }

    // The rest remains NOPs or the final RTS
}

// Convenience wrappers
void shiftBlobLeft(unsigned char bits, unsigned int base_addr)
{
    setBlobShift((signed char)bits, base_addr);
}

void shiftBlobRight(unsigned char bits, unsigned int base_addr)
{
    setBlobShift(-(signed char)bits, base_addr);
}

// Example shifter 
int do_shifter(void)
{
    unsigned int current_base = 0xA000;   // Start at top-left

   {

        // Example operations (arbitrary shifts on any 7-byte region)
        shiftBlobLeft(4, current_base);           // Left 4
        call((unsigned int)&shiftCode[0]);        // Execute the patched shifter

        shiftBlobRight(8, current_base);          // Right 8
        call((unsigned int)&shiftCode[0]);

        // Move to another blob elsewhere on screen
        current_base = 0xA028;                    // Example: next scanline or different X
        shiftBlobLeft(3, current_base);
        call((unsigned int)&shiftCode[0]);

        // ... other UI / data processing ...
    }

    return 0;
}




#if 0
// Initialize the table with some data (e.g., odd numbers)
void initTable()
{
	int i;

	for (i = 0; i < 256; i++) {
		table[i] = i * 2 + 1;	// Fill with odd numbers as an example
	}
}

int check_rantab()
{
	// Call the assembly function
	unsigned char randomValue;

	randomValue = FindRandomNumber(table);

	// Print the result
	printf("Random number not in the table: %u\n", randomValue);

	return 0;
}
#endif


// ix,iy: input x,y of the cell to Blast
void cellBlast(unsigned int ix, unsigned int iy)
{
	int c;
	int x;
	int y;

	c = 0;
	x = 0;

	for (c = 0; c < (ROW_LENGTH); c++) {
		setDisplayInstruction(c, c, 0, c, 4);
		// printf("1blast:%d,%d:%d,%d\n", ix, iy, x, c);
	}

	for (c=0; c < COL_LENGTH; c++) {
		setDisplayInstruction(c, c, 1, 4, c);
		// printf("1blast:%d,%d:%d,%d\n", ix, iy, x, c);
	}

    // if (0)
	// {
	// 	setDisplayInstruction(0, 0, 0, (ROW_LENGTH / 2), (COL_LENGTH / 2));
	// 	setDisplayInstruction(1, 1, 0, 10, 10);
	// 	setDisplayInstruction(2, 2, 0, 12, 10);
	// 	setDisplayInstruction(3, 3, 0, 14, 10);

	// 	setDisplayInstruction(4, 1, 1, 10, 12);
	// 	setDisplayInstruction(5, 2, 1, 12, 12);
	// 	setDisplayInstruction(6, 3, 1, 14, 12);

	// 	setDisplayInstruction(7, 1, 2, 10, 14);
	// 	setDisplayInstruction(8, 2, 2, 12, 14);
	// 	setDisplayInstruction(9, 3, 2, 14, 14);
	// }

	//call((unsigned int) &displayInstructions[0]);

	// for (c = 0; c < MAX_CB_DISPLAY_INSTRUCTIONS; c++) {
	// 	setDisplayInstruction(c, ix, c, c, 12);
	// 	// printf("2blast:%d,%d:%d,%d\n", ix, iy, x, c);
	// }
	// call((unsigned int) &displayInstructions[0]);

 	// for (c = 0; c < MAX_CB_DISPLAY_INSTRUCTIONS; c++) {
	// 	setDisplayInstruction(c, ix, c, c, 16);
	// 	// printf("2blast:%d,%d:%d,%d\n", ix, iy, x, c);
	// }
	// call((unsigned int) &displayInstructions[0]);
 
//  	for (c = 0; c < MAX_CB_DISPLAY_INSTRUCTIONS; c++) {
//		setDisplayInstruction(c, MAX_CB_DISPLAY_INSTRUCTIONS - c, c, c, 13);
//		//printf("blast:%d,%d:%d,%d\n", ix, iy, x, c);
//	}
//	call((unsigned int) &displayInstructions[0]);


}

void triggerCurrent(int position)
{
	hexDump((unsigned char *) position, 16);
	loadTable(position);
	Synth();
	//unsigned char H;
	//unsigned char L;
	//L = position << 8;
	//H = position & 0xFF00;
	//printf("\nZP:%x %x %x\n", position, H, L);
	//SynthZP(H, L);
}


void cellUI(unsigned char kp)
{

//	if (input_x % 2) {
	setDisplayInstruction(CURS1, input_x, input_y, cursor_x, cursor_y);
	setDisplayInstruction(CURS2, input_x, input_y, cursor_x + 1, cursor_y + 1);
//	} else {
	setDisplayInstruction(CURS1, input_x + 1, input_y, cursor_x, cursor_y + 1);
	setDisplayInstruction(CURS2, input_x, input_y + 1, cursor_x + 1, cursor_y);
//	}

}

void cellAnim(unsigned char kp)
{
	int av;		// animation vector
	int ai;		// animation instruction


	ai = ANIM1;
	for (av=0;av<5;av++) {
        cellStates[av].stepFunction(&cellStates[av]); // Call the step function for each cell
        setDisplayInstruction(ai++, cellStates[av].sx, cellStates[av].sy, cellStates[av].x, cellStates[av].y);
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

void cellHack(unsigned char kp)
{

//  initTable();  // Initialize the table with values
//  check_rantab(); 

//  printf("c:%d,%d,i:%d,%d p:%d,%d r:%d\r", cursor_x, cursor_y, input_x, input_y, anim_x, anim_y, r_val);

	{
		// the random stuff
		stoy_x += g_state;
		stoy_y += g_state;

		r_val = qrandomJ(peek(0x276)) % COL_LENGTH;

		if (r_val < 12) {
			anim_y -= g_state;
		};

		if (stoy_x > ROW_LENGTH) stoy_x = 0;
		if (stoy_y > COL_LENGTH) stoy_y = 0;

		setDisplayInstruction(STOY1, 1, r_val, stoy_x, stoy_y + 1);
		setDisplayInstruction(STOY2, 2, r_val, stoy_x, stoy_y + 2);
		setDisplayInstruction(STOY3, 3, r_val, stoy_x, stoy_y + 3);

		setDisplayInstruction(SAFE1, cursor_x, cursor_y, MAX_CB_X, MAX_CB_Y);
	}

#if 0
	setDisplayInstruction(ANIM1, input_x, input_y, cursor_x, cursor_y);

	setDisplayInstruction(CURS2, cursor_x, cursor_x, 1, 1);
	setDisplayInstruction(CURS1, input_x, input_y, 2, 1);
#endif 
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
static void update_cell_sources(void)
{
    cellStates[0].source_y = 1;
    cellStates[1].source_y = 2;
    cellStates[2].source_y = 3;
    cellStates[3].source_y = 4;
    cellStates[4].source_y = 5;
    cellStates[5].source_y = 6;
}

static void blast_revert(void)
{
    int idx, row;
    disable_irq();
    for (idx = CB_BLAST_FIRST; idx <= blast_max_idx; idx++)
        for (row = 0; row < BLOCK_HEIGHT; row++) {
            displayInstructions[idx][3 + row * 6] = 0;
            displayInstructions[idx][4 + row * 6] = 0;
            displayInstructions[idx][6 + row * 6] = 0;
            displayInstructions[idx][7 + row * 6] = 0;
        }
    if (blast_max_idx < MAX_CB_DISPLAY_INSTRUCTIONS - 1)
        displayInstructions[blast_max_idx][50] = 0xEA;
    displayInstructions[CB_UI_LAST][50] = 0x60;
    cb_highest = CB_UI_LAST;
    blast_max_idx = CB_UI_LAST;
    enable_irq();
}

/*
 * Blast: fill every other column and row with bank copies for a short
 * window.  Uses blocks CB_BLAST_FIRST..CB_BLAST_LAST only, so the
 * cursor/ANIM blocks keep working (1.4.0 blast wrote into the exact
 * blocks the per-frame UI update then overwrote, making it invisible).
 */
static void cell_blast(void)
{
    int idx = CB_BLAST_FIRST;
    int col, row;
    for (col = 0; col < ROW_WIDTH && idx <= CB_BLAST_LAST; col += 2)
        set_display_instruction(idx++, col, 0, col, 4);
    for (row = 0; row < COL_HEIGHT && idx <= CB_BLAST_LAST; row += 4)
        set_display_instruction(idx++, row, 1, 4, row);
    blast_max_idx = idx - 1;
    blast_hold = 75;
}

/* cursor = two adjacent byte-columns (12 px) on one band - recognisable
   glyph that slides under the arrow keys.  Source cell = (input_x,input_y)
   bank cell + its right neighbour; 'C' cycles the source (KEY_CAPTURE). */
static void cell_ui_update(void)
{
    set_display_instruction(CURS1, input_x, input_y, cursor_x, cursor_y);
    set_display_instruction(CURS2, (input_x + 1) % ROW_WIDTH, input_y,
                            cursor_x + 1, cursor_y);
}

/*
 * Dancers: ANIM1..6 (blocks 6..11) carry column x of each dancer; the
 * formerly-dead STOY1..3 + SAVE1 (blocks 0..3) carry column x+1 for
 * dancers 0..3, making them 12 px wide (dancers 4..5 stay single-column
 * for variation).  The chain already runs 0..11, so no chain-end or blast
 * bookkeeping changes.  source_x churns every 8 frames for texture.
 */
static void synth_zp(unsigned char high_byte, unsigned char low_byte) {
    poke(0x14, high_byte);
    poke(0x15, low_byte);
    call(0xF590);
}

void generate_color_particles()
{

	int j;
	int k;

	volatile char rg;
	char s;

	// s = peek(0x276);
	// seed_lfsr(s);

	// j .. memory .. k
	j = (unsigned int) HIRES_START;
	k = (unsigned int) HIRES_START + 320 * 3;
	//k = (unsigned int)HIRES_END;

	do {
		// rg = 16+((qrandomJ(peek(0x276)) % 255) & 7);
		// // Somewhat slow C-based implementation with assembly RNG - works
		do {
			rg = qrandomJ(peek(0x276)) % 255;
		}
		while (((rg & 0x78) == 0x08 || (rg & 0x78) == 0x18)
			|| ((rg & 0x78) == 0x88 || (rg & 0x78) == 0x98));

		// rg = qrandomJ(peek(0x276)) % 255;

		// Linear-feedback shift register method
		// rg = lfsr_random();

		// table-based with 
		// rg = fastbloop();

		// Chema's randgen:
		// rg == randgen();

			poke(j, rg);
		// printf("j: %x rg: %x\n", j, rg);

	}
	while (j++ < k);

}
 
void validateStates()
{
			if (kp == APP_CURSOR_L) {
				cursor_x -= 1;
			}
			else
			if (kp == APP_CURSOR_R) {
				cursor_x += 1;
			}
			else
			if (kp == APP_CURSOR_U) {
				cursor_y -= 1;
			}
            else
			if (kp == APP_CURSOR_D) {
				cursor_y += 1;
			};

			if (kp == APP_INPUT_L) {
				input_x -= 1;
			}
            else
			if (kp == APP_INPUT_R) {
				input_x += 1;
			}
            else
			if (kp == APP_INPUT_U) {
				input_y -= 1;
			}
			else
			if (kp == APP_INPUT_D) {
				input_y += 1;
			}
			else
 			if (kp == APP_BLAST) {
				cellBlast(input_x, input_y);
			}
            else
			if (kp == APP_SPACE) {
				triggerCurrent(cell_source_base);
			};

			if (anim_y > MAX_CB_Y) {
				anim_y = 2;
			}
            else
			if (anim_y < 2) {
				anim_y = MAX_CB_Y;
			};

			if (stoy_x > MAX_CB_X) {
				g_state = CGENSTATE_BACKWARD;
			}
            else
			if (stoy_x < 2) {
				g_state = CGENSTATE_FORWARD;
			};

			if (stoy_y > MAX_CB_Y) {
				stoy_y = 2;
			}
            else
			if (stoy_y < 2) {
				stoy_y = MAX_CB_Y;
			};

			if (cursor_y > MAX_CB_Y) {
				cursor_y = 2;
			}
            else
			if (cursor_y < 2) {
				cursor_y = MAX_CB_Y;
			};
            
			if (cursor_x > MAX_CB_X) {
				cursor_x = MAX_CB_X;
			}
			else
			if (cursor_x < 2) {
				g_state = CGENSTATE_FORWARD;
			};

			ui_x++;
			ui_y++;
			if (ui_x > ROW_LENGTH) {
				ui_x = 0;
			}
			else
			if (ui_y > COL_LENGTH) {
				ui_y = 0;
			};
}


void main()
{
	unsigned int position;		// position
	unsigned int i;				// input
	unsigned char r;
	unsigned char shouldPlay;
	int j;
	int m;
	int e_mode = 0;				// edit mode


	
	cell_source_base =
				SOURCE_BUFFER_START +
				(input_y * ROW_LENGTH * BLOCK_HEIGHT) + (input_x * 1);
	setflags(SCREEN + NOKEYCLICK);

	shouldPlay = 1;
	position = POS_INITIAL_POSITION;	// start of ROM

	poke(0x24E, 5);				// set keyb delay 5 at #24E
	poke(0x24F, 2);				// set keyb repeat
	poke(0x26A, 10);			// disble keyclick and cursor

	printHelp();
	Synth();

	while ((kp = key()) != APP_QUIT) {

		if (kp == APP_RESET) {
			printf("reset!\n");
			position = (unsigned int) &BootSound;
			//position=POS_KEYCLICK1;
			cursor_x = 2;
			cursor_y = 2;
			input_x = 1;
			input_y = 1;
			anim_x = 12;
			anim_y = 12;
			stoy_x = 12;
			stoy_y = 12;
			g_state = CGENSTATE_FORWARD;
		}

		if (e_mode > 0) {

			if (kp == APP_CAPTURE) {
				// capture the current cell
				printf("Cin: %d,%d", input_x, input_y);
				cell_source_base =
				SOURCE_BUFFER_START +
				(input_y * ROW_LENGTH * BLOCK_HEIGHT) + (input_x * 1);
				hexDump((unsigned char *) cell_source_base, 8);
			}


			// tick
			updateStates();

			cellHack(kp);
			cellUI(kp);
			cellAnim(kp);
			
			validateStates();

			call((unsigned int) &displayInstructions[0]);

			do_shifter();

			continue;
		}

		if (kp == APP_SOUND_ONOFF) {
			shouldPlay = !shouldPlay;
		} else if (kp == APP_POS_POSITIVE) {
			position++;
			if (position >= POS_END) {
				position = POS_START;
			}
			printf("\np:%x", position);
		} else if (kp == APP_POS_MINUS) {
			position--;
			if (position <= POS_START) {
				position = POS_END;
			}
			printf("\np:%x", position);
		} else if (kp == APP_RESET_POS) {
			printf(".\n");
			position = POS_INITIAL_POSITION;
		} else if (kp == APP_POSITION) {
			unsigned int v;
			printf("P:");
			scanf("%x", &v);
			position = v;
			printf("\nv:%x p:%x", v, position);
		} else if (kp == APP_HELP) {
			printHelp();
		} else if (kp == APP_HIRES) {

			e_mode = 1;

			hires();
			poke(0x26A, 10);	// disble keyclick and cursor

			// memcpy((unsigned char*)0xa000, OverlayLabel, 8000);

			// spamit();
			// randcolorgen();
			// randcogtab();

     		initShiftCode();                      // Build template once

			generate_color_particles();

			// printf("pos: mode:%x\n", position, e_mode);
			loadTable(position);
			hexDump((unsigned char *) position, 16);

			dumpPitch(CHAN_A);
			dumpPitch(CHAN_B);
			dumpPitch(CHAN_C);

		} else if (kp == APP_MEMHAK) {
			// static const int segSize = 0;
			// unsigned char *src, *dst;
			// src = HIRES_START;
			// dst = HIRES_START + 40;
			// dst = (void *)(HIRES_START + (8*40);
			// for (src = (void *)HIRES_START; 
			//      src < HIRES_END - (8*40) - 4; 
			//      src += segSize, dst += segSize) {
			//      memcpy(src+(1*40), dst+(1*40), segSize);
			//      memcpy(src+(2*40), dst+(2*40), segSize);
			//      memcpy(src+(3*40), dst+(3*40), segSize);
			//      memcpy(src+(4*40), dst+(4*40), segSize);
			//      memcpy(src+(5*40), dst+(5*40), segSize);
			//      memcpy(src+(6*40), dst+(6*40), segSize);
			//      memcpy(src+(7*40), dst+(7*40), segSize);
			//      memcpy(src+(8*40), dst+(8*40), segSize);
			// }
		} else if (kp == APP_SPACE) {
			triggerCurrent(position);
		}

	}

	text();

	loadTable((unsigned int) &BootSound);
	Synth();
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

static void cell_anim_update(void)
{
    int i;
    CellState *c;
    if ((frame_count & 7) == 0) {
        for (i = 0; i < 6; i++)
            cellStates[i].source_x = (cellStates[i].source_x + 1) & 31;
    }
    for (i = 0; i < 6; i++) {
        c = &cellStates[i];
        if (c->step_fn) c->step_fn(c);
        set_display_instruction(ANIM1 + i, c->source_x, c->source_y, c->x, c->y);
        if (i < 4)
            set_display_instruction(i, (c->source_x + 1) & 31, c->source_y,
                                    c->x + 1, c->y);
    }
}

static void validate_ui_bounds(unsigned char key)
{
    if (key == KEY_CURSOR_LEFT)  cursor_x--;
    if (key == KEY_CURSOR_RIGHT) cursor_x++;
    if (key == KEY_CURSOR_UP)    cursor_y--;
    if (key == KEY_CURSOR_DOWN)  cursor_y++;
    if (key == KEY_BLAST)        cell_blast();
    if (key == KEY_CAPTURE) {                 /* cycle cursor source cell */
        input_x++;
        if (input_x >= ROW_WIDTH) {
            input_x = 0;
            input_y = (input_y + 1) % 8;      /* pattern bank bands */
        }
    }

    cursor_x = clamp(cursor_x, 2, 22);
    cursor_y = clamp(cursor_y, 0, 19);
}

static void print_help_screen(void)
{
    cls();
    paper(0);
    ink(5);
    printf("\n soundtoy 1.5.4-creatures\n\n");
    printf(" V  help\n G  hires mode\n Q  quit\n");
    printf(" J/K  next/prev preset   R  boot   .  ping\n");
    printf(" P  set preset address   H  mute\n");
    printf(" SPACE  play current\n\n");
    printf("In hires:\n ARROWS  move cursor   B  blast\n");
    printf(" C  cycle cursor pattern\n");
    printf(" Energy bars + nature pip below the field\n");
    printf(" (green=plant, red=animal, dark=wall).\n");
}

/* ------------------------------------------------------------------ */
/* Blobs                                                              */
/* ------------------------------------------------------------------ */

/*
 * Body half (row[]): broad checker, ink steps through the palette.
 * Face half (row2[]): finer pattern, ink offset so the two cells read
 * as distinct halves.  Walls seed black-on-black.
 */
static void blob_seed_rows(Blob *b)
{
    int i;
    unsigned char alive = (unsigned char)(b->character.affinity != 0);
    for (i = 0; i < BLOB_ROWS; i++) {
        b->character.row[i].paper = 0;
        b->character.row[i].ink   = alive
                                  ? (unsigned char)(2 + (i % 6)) : 0;
        b->character.row[i].bits  = (i & 1) ? 0x3F : 0x2A;
        b->character.row[i].inv   = 0;
        b->character.row2[i].paper = 0;
        b->character.row2[i].ink   = alive
                                  ? (unsigned char)(1 + ((i + 2) % 6)) : 0;
        b->character.row2[i].bits  = (i & 1) ? 0x2A : 0x15;
        b->character.row2[i].inv   = (unsigned char)(alive && (i == 3));
    }
}

static void blob_init(Blob *b, int x, int min_x, int max_x, int dir_x, int dir_y, unsigned char affinity)
{
    b->x = (unsigned char)x;
    b->y = 0;
    b->px = (unsigned char)x;
    b->py = 0;
    b->min_x = (unsigned char)min_x;
    b->max_x = (unsigned char)max_x;
    b->dir_x = (signed char)dir_x;
    b->dir_y = (signed char)dir_y;
    b->character.affinity = affinity;
    b->character.energy = (affinity == 0) ? 0 : 255;  /* walls starve at 0 */
    b->character.age = 0;
    b->character.rule_set = (affinity == 1) ? 1 : 2;  /* plant vs animal */
    if (b->max_x > BLOB_RIGHT) b->max_x = BLOB_RIGHT;
    if (b->x > BLOB_RIGHT)     b->x = BLOB_RIGHT;
    blob_seed_rows(b);
}

/*
 * Colour of the segment the field is currently showing under x.
 * Walks the shadow left from (x,y) for the last INK/PAPER attribute -
 * the hardware's own "last attribute colours the run" rule, so the
 * field itself feeds the osmosis rules (environment osmosis).
 */
static unsigned char field_context_colour(int x, int y)
{
    int idx = y * ROW_WIDTH + x - 1;
    int i;
    unsigned char v;
    if (idx < 0 || y >= FIELD_ROWS) return 0;   /* void below the seed field */
    for (i = 0; i < ROW_WIDTH && idx >= 0; i++, idx--) {
        v = shadow_screen[idx];
        if ((v & 0x60) == 0) {
            v = (unsigned char)(v & 0x0F);
            if (v <= 7) return v;      /* INK or PAPER colour bits */
        }
    }
    return 0;
}

/* restore the blob's previous footprint from the shadow field */
static void blob_restore(Blob *b)
{
    int row, i;
    unsigned char *dst;
    int base;
    for (row = 0; row < BLOB_ROWS; row++) {
        dst  = row_ptr[b->py + row] + b->px;
#if STOY_CHECK
        if (b->py > 66) spin_py();
        if ((unsigned int)dst < (unsigned int)HIRES_BASE ||
            (unsigned int)dst >= (unsigned int)HIRES_BASE + 8000u) spin_dst2();
#endif
        base = (b->py + row) * ROW_WIDTH + b->px;
        for (i = 0; i < BLOB_WIDTH; i++)
            dst[i] = shadow_screen[base + i];
    }
}

static void blob_draw(Blob *b, Blob *other)
{
    int row;
    int contact;
    Cell tmp;
    unsigned char *dst;
    contact = (b->y == other->y &&
               b->x < other->x + BLOB_WIDTH &&
               other->x < b->x + BLOB_WIDTH);
    for (row = 0; row < BLOB_ROWS; row++) {
        dst = row_ptr[b->y + row] + b->x;
#if STOY_CHECK
        if (row > 7) spin_row();
        if (b->y > 66) spin_y();
        if ((unsigned int)dst < (unsigned int)HIRES_BASE ||
            (unsigned int)dst >= (unsigned int)HIRES_BASE + 8000u) spin_dst();
#endif
        tmp.paper = b->character.row[row].paper;
        tmp.ink   = b->character.row[row].ink;
        tmp.bits  = b->character.row[row].bits;
        tmp.inv   = b->character.row[row].inv;
        /* dither fringe on the BODY half only: alternating the
           partner's ink per frame interleaves the two inks into a
           perceived blend without mutating cell state (1.4.0
           XOR-accumulated the pixel bits).  The face stays stable. */
        if (contact && ((frame_count + row) & 1))
            tmp.ink = other->character.row[row].ink;
        cell_write(&tmp, dst);
        cell_write(&b->character.row2[row], dst + 3);
    }
}

static void blob_move(Blob *b)
{
    int nx, ny;

    nx = b->x + b->dir_x;
    ny = b->y + b->dir_y;

    /* vertical bounce inside the visible field */
    if (ny < 0)                { ny = 0;            b->dir_y = +1; }
    else if (ny > BLOB_BOTTOM) { ny = BLOB_BOTTOM;  b->dir_y = -1; }

    /* horizontal bounce inside own range (BLOB_RIGHT: 6-byte row fits) */
    if (nx < b->min_x)         { nx = b->min_x;     b->dir_x = +1; }
    else if (nx > b->max_x)    { nx = b->max_x;     b->dir_x = -1; }
    if (nx > BLOB_RIGHT)       { nx = BLOB_RIGHT;   b->dir_x = -1; }

    b->px = b->x;  b->x = (unsigned char)nx;
    b->py = b->y;  b->y = (unsigned char)ny;

    /* life cycle: living cells decay to walls, walls revive with age */
    if (b->character.affinity != 0) {
        if (b->character.energy > 0) b->character.energy--;
        if (b->character.energy == 0) {
            b->character.affinity = 0;
            blob_seed_rows(b);           /* black paper + black ink */
            g_event = 1;
        }
    } else {
        if (b->character.age != 0 && (b->character.age % 96) == 0) {
            b->character.affinity = 2;   /* revive as animal */
            b->character.energy = 120;
            blob_seed_rows(b);
            g_event = 1;
        }
    }
    b->character.age++;
}

/*
 * Symmetric anti-pass-through: applied AFTER both blobs have moved
 * (1.4.0 clamped blobR against blobL's already-moved x and forced both
 * dir_x = +1 at equal x, locking them into lockstep drift).
 */
static void blob_separate(void)
{
    if (blobL.x <= blobR.x) return;
    if (blobL.dir_x > 0) { blobL.x = blobL.px; blobL.dir_x = -1; }
    if (blobR.dir_x < 0) { blobR.x = blobR.px; blobR.dir_x = +1; }
}

/*
 * Boundary channel on the (mostly off-screen) mailbox row.
 * EVERY byte is encoded into a legal class - the row lives in screen
 * memory and the gate array scans it machine-wide, so raw telemetry
 * (energy 0..255 etc.) used to deposit video-control bytes 0x18-0x1F
 * / 0x98-0x9F there and flip 50/60Hz.  Now:
 *   affinity/position/frame -> 0x20|n  (PIX_BIT5 pixel byte, inert)
 *   energy                  -> 0x10|n  (PAPER attr: energy is literally
 *                                         colour-coded on the hidden row)
 * Readers mask with & 3 / & 7 / & 0x1F.
 */
static void mailbox_update(void)
{
    unsigned char *m = row_ptr[MAILBOX_ROW];
    m[0] = (unsigned char)(0x20 | (blobL.character.affinity & 3));
    m[1] = (unsigned char)(0x20 | (blobR.character.affinity & 3));
    m[2] = (unsigned char)(0x10 | (blobL.character.energy & 7));
    m[3] = (unsigned char)(0x10 | (blobR.character.energy & 7));
    m[4] = (unsigned char)(0x20 | (blobL.x & 0x1F));
    m[5] = (unsigned char)(0x20 | (blobR.x & 0x1F));
    m[6] = (unsigned char)(0x20 | (blobL.y & 0x1F));
    m[7] = (unsigned char)(0x20 | (blobR.y & 0x1F));
    m[8] = (unsigned char)(0x20 | (frame_count & 0x1F));
}

static void blob_exchange(Blob *a, Blob *b)
{
    int i, g, ox;
    unsigned char ls, fc;
    unsigned int e;

    last_g = 0;
    if (a->y != b->y) return;
    if (!(a->x + BLOB_WIDTH > b->x && b->x + BLOB_WIDTH > a->x)) return;

    /* read the left blob's published state through the screen mailbox
       (mailbox_update() already ran this frame - 1.4.0 read it stale).
       The byte is a 0x20|n pixel-class encoding; mask to the value. */
    ls = (unsigned char)(row_ptr[MAILBOX_ROW][0] & 3);

    /* wall repulsion - keys off affinity; walls LIVE at energy 0, so
       the old 'energy > 0' gate made dead walls harmless */
    if (ls == 0 || a->character.affinity == 0) {
        if (b->dir_x < 0) b->dir_x = +1;
        return;
    }
    if (b->character.affinity == 0) {
        if (a->dir_x > 0) a->dir_x = -1;
        return;
    }

    /* living cells: osmose primaries both ways, plus the colour the
       field itself is showing under the contact point */
    ox = (a->x < b->x) ? b->x : a->x;
    fc = field_context_colour(ox, a->y + 3);
    g = 0;
    if (a->character.rule_set == 1) {
        /* plants: green-only balance, both halves, both ways
           (the RULE_GREEN_BALANCE the 1.4 rules table promised) */
        for (i = 0; i < BLOB_ROWS; i++) {
            g += RULE_BALANCE_TO_BLACK(a->character.row[i],  b->character.row[i],  2);
            g += RULE_BALANCE_TO_BLACK(b->character.row[i],  a->character.row[i],  2);
            g += RULE_BALANCE_TO_BLACK(a->character.row2[i], b->character.row2[i], 2);
            g += RULE_BALANCE_TO_BLACK(b->character.row2[i], a->character.row2[i], 2);
        }
    } else {
        /* animals: full mutual exchange across both halves */
        for (i = 0; i < BLOB_ROWS; i++) {
            g += RULE_MUTUAL(a->character.row[i], b->character.row[i], 1, 4, 0, 0);
            g += RULE_MUTUAL(a->character.row[i], b->character.row[i], 2, 2, 0, 0);
            g += RULE_MUTUAL(a->character.row[i], b->character.row[i], fc, 7, 0, 0);
            g += RULE_MUTUAL(a->character.row2[i], b->character.row2[i], 1, 4, 0, 0);
            g += RULE_MUTUAL(a->character.row2[i], b->character.row2[i], 2, 2, 0, 0);
        }
    }

    /* successful transfers feed the characters (clamped: 1.4.0 wrapped) */
    e = (unsigned int)a->character.energy + (unsigned int)(g >> 1);
    a->character.energy = (e > 255) ? 255 : (unsigned char)e;
    e = (unsigned int)b->character.energy + (unsigned int)(g >> 1);
    b->character.energy = (e > 255) ? 255 : (unsigned char)e;

    last_g = (unsigned char)g;
    if (g >= 8) g_event = 1;
}

/* ------------------------------------------------------------------ */
/* Status bars (void strip - bars only, no text)                      */
/* ------------------------------------------------------------------ */

/*
 * Energy bars + affinity pips for both blobs, painted into the void
 * (rows 176/188 - never touched by the field, shimmer or blobs).
 * Direct row_ptr writes: the void is black by construction and has no
 * shadow backing.  Legal bytes only: PAPER/INK attrs + 0x7F pixels
 * and 0x20|n (PIX_BIT5, always-lit) for the pips.
 */
static void draw_status_bars(void)
{
    static const unsigned char bar_y[2] = { 176, 188 };
    static const unsigned char bar_ink[2] = { 5, 3 };   /* magenta / yellow */
    Blob *bl[2];
    Blob *b;
    unsigned char *dst, *p;
    unsigned char ink, pip;
    int i, seg;

    bl[0] = &blobL;
    bl[1] = &blobR;
    for (i = 0; i < 2; i++) {
        b = bl[i];
        dst = row_ptr[bar_y[i]];
        for (seg = 0; seg < 8; seg++) {
            p = dst + 2 + seg * 3;
            ink = (b->character.energy > (unsigned char)(seg * 32))
                ? bar_ink[i] : 0;
            p[0] = 0x10;                       /* PAPER black             */
            p[1] = ink;                        /* INK 0 = unlit (hidden)  */
            p[2] = 0x7F;                       /* solid pixel byte        */
        }
        /* affinity pip: green=plant, red=animal, black=wall (invisible) */
        pip = (b->character.affinity == 1) ? 2
            : (unsigned char)((b->character.affinity == 2) ? 1 : 0);
        dst[26] = 0x10;
        dst[27] = pip;
        dst[28] = (unsigned char)(0x20 | 0x1F);
    }
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

extern void shimmer_step();

/* shimmer.s must never reference C BSS symbols directly (the vendored
   xa resolves .s->BSS operands with a stale high byte - screen writes
   landed outside the HIRES region and scribbled code).  C injects the
   correct table/shadow addresses into shimmer's self-patch slots. */
extern unsigned char shm_rl[], shm_rh[], shm_sl1[], shm_sh1[], shm_sl2[], shm_sh2[];
static void shim_patch_ptrs(void)
{
    unsigned int a;
    a = (unsigned int)row_off_lo;
    shm_rl[1] = (unsigned char)a;  shm_rl[2] = (unsigned char)(a >> 8);
    a = (unsigned int)row_off_hi;
    shm_rh[1] = (unsigned char)a;  shm_rh[2] = (unsigned char)(a >> 8);
    a = (unsigned int)shadow_screen;
    shm_sl1[1] = (unsigned char)a; shm_sh1[1] = (unsigned char)(a >> 8);
    shm_sl2[1] = (unsigned char)a; shm_sh2[1] = (unsigned char)(a >> 8);
    a = (unsigned int)displayInstructions;
    sd_dil[0] = (unsigned char)a;  sd_dih[0] = (unsigned char)(a >> 8);
    a = (unsigned int)pattern_bank;
    sd_pbl[0] = (unsigned char)a;  sd_pbh[0] = (unsigned char)(a >> 8);
    a = (unsigned int)row_ptr;
    ar_rpb[0] = (unsigned char)a;  ar_rpb2[0] = (unsigned char)(a >> 8);
}

void main(void)
{
    unsigned int current_pos = POS_INITIAL;
    char edit_mode = 0;
    int i;
#if STOY_AUTOG
    autog_left = 180;
#endif

    for (i = 0; i < 6; i++)
        cellStates[i].step_fn = (i & 1) ? step_orbit : step_dancer;

    update_cell_sources();
    init_geometry();
    init_safe_map();
    cb_init();

    shm_via = (unsigned char)(STOY_NO_VIA_RNG ? 0 : 1);
#if STOY_KBD_MODE == 1
    chain_irq_handler(&kbd_irq_cb);
#endif

    shm_init();
    setflags(SCREEN + NOKEYCLICK);
    poke(0x24E, 5);
    poke(0x24F, 2);
    poke(0x26A, 10);

    print_help_screen();
    synth_safe();

    while (1) {
        kp = poll_key();
#if STOY_AUTOHIRES
        if (!first_auto_done) { first_auto_done = 1; kp = KEY_HIRES_MODE; }
#endif
#if STOY_AUTOG
        if (!edit_mode && kp == 0 && autog_left) {
            if (--autog_left == 0) kp = KEY_HIRES_MODE;
        }
#endif
#if STOY_CHECK
        if (kp == KEY_QUIT) spin_kq();   /* AUTOG never injects Q: phantom = key-buffer trampled */
        if (!edit_mode) chk_rp(8);       /* menu stage */
#endif
        if (kp == KEY_QUIT) break;
        frame_tick();

        if (!edit_mode) {
            if (kp == KEY_HIRES_MODE) {
                dump_preset_details(current_pos);  /* last TEXT-mode view */
                edit_mode = 1;
                hires();
                poke(0x26A, 10);
                clear_screen_full();           /* wipe $BF40+ text tail */
                gen_random_colors();
                gen_pattern_bank();
                validate_field();
                shim_patch_ptrs();         /* fix shimmer's BSS refs  */
                cells_init();              /* start the cell rain     */
                cb_init();
                last_g = 0;
                g_event = 0;
                frame_count = 0;

                /* left: living, right: wall; bounce ranges keep them
                   apart until the exchange/wall rules take over */
                blob_init(&blobL,  2,  2, 20, +1, +1, 1);
                blob_init(&blobR, 37, 19, 37, -1, +1, 0);
                mailbox_update();

                load_sound_table(current_pos);
#if STOY_CHECK
                chk_rp(0);
#endif
#if STOY_MUTE_IRQ
                mute_irq();                    /* after rnd8 init: no ROM IRQ */
#endif
                continue;
            }

            if (kp == KEY_POS_INCREMENT) {
                current_pos++;
                printf("\np:%x", current_pos);
            }
            if (kp == KEY_POS_DECREMENT) {
                current_pos--;
                printf("\np:%x", current_pos);
            }
            if (kp == KEY_RESET_PRESET) {
                current_pos = POS_INITIAL;
                printf(".\np:%x", current_pos);
            }
            if (kp == KEY_RESET_PING) {
                current_pos = POS_KEYCLICK;
                printf(".\np:%x", current_pos);
            }
            if (kp == KEY_SET_POSITION) {
                unsigned int v;
                printf("Pos: 0x");
                v = read_hex4();
                current_pos = v;
                printf("\nv:%x p:%x", v, current_pos);
            }
            if (kp == KEY_HELP) print_help_screen();
            if (kp == KEY_SOUND_TOGGLE) {
                should_play = (unsigned char)(!should_play);
                if (should_play) printf("\nsound on\n");
                else             printf("\nsound off\n");
            }
#if !STOY_NO_SYNTH_ON_KEY
            /* raw cursor codes (0x9A/0x9D > 32) must not count as keys */
            if (should_play &&
                (kp == KEY_TRIGGER_SPACE || kp > 32) &&
                kp != KEY_CURSOR_LEFT  && kp != KEY_CURSOR_RIGHT &&
                kp != KEY_CURSOR_UP    && kp != KEY_CURSOR_DOWN &&
                kp != KEY_SOUND_TOGGLE)
                trigger_current_sound(current_pos);
#endif
            continue;
        }

        /* ---- HIRES mode ---- */
        if (kp == KEY_HIRES_MODE) {
            edit_mode = 0;
            text();
            continue;
        }

        validate_ui_bounds(kp);
        cell_ui_update();
#if STOY_CHECK
        chk_rp(1);
#endif
        cell_anim_update();
#if STOY_CHECK
        chk_rp(2);
#endif
        if (blast_hold > 0) {
            blast_hold--;
            if (blast_hold == 0) blast_revert();
        }
#if STOY_CHECK
        chk_rp(3);
#endif

        frame_count++;
        mailbox_update();                    /* publish BEFORE exchange  */
        if ((frame_count & 63) == 0)
            validate_field();                /* video-attr invariant net */
        if ((frame_count & 1) == 0)
            draw_status_bars();              /* void energy bars         */
#if STOY_CHECK
        chk_rp(4);
#endif
        blob_exchange(&blobL, &blobR);
#if STOY_CHECK
        chk_rp(5);
#endif
        blob_move(&blobL);
        blob_move(&blobR);
        blob_separate();
#if STOY_CHECK
        chk_rp(6);
#endif

        /* attribute weather drives the shimmer budget... */
        shm_flips = (unsigned char)(12 +
            ((blobL.character.energy + blobR.character.energy) >> 5));
        shimmer_step();
        cells_tick();                      /* fall/slide/scroll rules */
#if STOY_CHECK
        chk_rp(7);
#endif

        /* ...and the sound: energy/affinity modulate the AY table,
           bursts / deaths / revivals retrigger it */
        if (g_event || (last_g > 0 && (frame_count & 31) == 0)) {
            currentSound[0] = (unsigned char)(blobL.character.energy >> 4);
            currentSound[1] = (unsigned char)(blobR.character.energy >> 4);
            currentSound[2] = (blobL.character.affinity != 0) ? 15 : 4;
            currentSound[5] = (blobR.character.affinity != 0) ? 12 : 2;
            synth_safe();
            g_event = 0;
        }

        /* blit first, then blobs on top: blobs are the top layer and
           dancers can no longer scribble over them (1.4.0 order made
           blob_restore erase dancers mid-blob) */
#if STOY_CHECK
        chk_rp(8);
#endif
        call((unsigned int)&displayInstructions[0]);
        blob_restore(&blobL);
        blob_restore(&blobR);
        blob_draw(&blobL, &blobR);
        blob_draw(&blobR, &blobL);
    }

    text();
    load_sound_table(POS_INITIAL);
    synth_safe();
}
