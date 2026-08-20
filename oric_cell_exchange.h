/*
 * oric_cell_exchange.h
 *
 * OSDK / cc65 style rule system for colour-balanced exchanges
 * between two dense HIRES CELLs.
 *
 * A CELL is the three-byte dense unit defined previously:
 *   [PAPER attr] [INK attr] [pixel byte (bit6 forced, optional inverse)]
 *
 * “Sub-pixels” are the individual bits of the 6-bit pattern.
 * Each bit is either the CELL’s current INK colour (or its inverse)
 * or the CELL’s current PAPER colour (or its inverse).
 *
 * An exchange moves one sub-pixel of a nominated colour from the
 * source CELL to the destination CELL.  The vacated bit in the
 * source is replaced according to the chosen rule (another legal
 * colour, black, or white).  The destination bit is forced to the
 * transferred colour (possibly by temporarily adjusting the
 * destination’s INK/PAPER pair if necessary).
 *
 * All macros operate on two Cell descriptors and are intended to be
 * called once per frame until the desired balance is reached.
 */

#ifndef ORIC_CELL_EXCHANGE_H
#define ORIC_CELL_EXCHANGE_H

#include "oric_dense_cell.h"   /* the previous header */

/* ------------------------------------------------------------------ */
/* Cell descriptor (lightweight, OSDK-friendly)                       */
/* ------------------------------------------------------------------ */

typedef struct {
    unsigned char paper;   /* 0..7 */
    unsigned char ink;     /* 0..7 */
    unsigned char bits;    /* 6-bit pattern (bits 0-5) */
    unsigned char inv;     /* 0 = normal, 1 = inverse */
} Cell;

/* Build a Cell from the three dense bytes */
#define CELL_FROM_BYTES(p, i, px)               \
    {                                           \
        .paper = ((p) & 7),                     \
        .ink   = ((i) & 7),                     \
        .bits  = ((px) & 0x3F),                 \
        .inv   = (((px) & 0x80) ? 1 : 0)        \
    }

/* Emit the three dense bytes from a Cell */
#define CELL_TO_BYTES(c, dest)                  \
    do {                                        \
        *(dest)++ = 0x10 | ((c).paper & 7);     \
        *(dest)++ = 0x00 | ((c).ink   & 7);     \
        *(dest)   = dense_pixel((c).bits, (c).inv); \
    } while (0)

/* ------------------------------------------------------------------ */
/* Colour helpers                                                     */
/* ------------------------------------------------------------------ */

/* Effective colour of a bit (0 = paper side, 1 = ink side) */
#define CELL_BIT_COLOUR(c, bit)                 \
    ( ((c).inv)                                 \
        ? ( ((bit) ? (c).ink : (c).paper) ^ 7 ) \
        : ( (bit) ? (c).ink : (c).paper ) )

/* Count of sub-pixels that currently display a given colour 0..7 */
#define CELL_WEIGHT(c, colour) (                \
    (CELL_BIT_COLOUR(c,0) == (colour)) +        \
    (CELL_BIT_COLOUR(c,1) == (colour)) +        \
    (CELL_BIT_COLOUR(c,2) == (colour)) +        \
    (CELL_BIT_COLOUR(c,3) == (colour)) +        \
    (CELL_BIT_COLOUR(c,4) == (colour)) +        \
    (CELL_BIT_COLOUR(c,5) == (colour))          \
)

/* ------------------------------------------------------------------ */
/* Core transfer primitive                                            */
/* ------------------------------------------------------------------ */

/*
 * Transfer one sub-pixel of ‘colour’ from src → dst.
 * The vacated bit in src becomes ‘leftover’ (0..7).
 * Returns 1 on success, 0 if no matching bit existed in src
 * or no free bit existed in dst.
 *
 * Implementation is deliberately macro-based so the compiler can
 * constant-fold the bit tests when colour/leftover are literals.
 */
#define CELL_TRANSFER(src, dst, colour, leftover) (     \
    cell_transfer_impl(&(src), &(dst), (colour), (leftover)) \
)

/* The actual workhorse (still pure C, no library calls) */
static inline int cell_transfer_impl(Cell *src, Cell *dst,
                                     unsigned char colour,
                                     unsigned char leftover)
{
    int s, d;
    unsigned char src_bit, dst_bit;

    /* 1. Locate a source bit that currently shows ‘colour’ */
    for (s = 0; s < 6; ++s) {
        if (CELL_BIT_COLOUR(*src, s) == colour)
            break;
    }
    if (s == 6) return 0;               /* nothing to give */

    /* 2. Locate a destination bit that is NOT already ‘colour’ */
    for (d = 0; d < 6; ++d) {
        if (CELL_BIT_COLOUR(*dst, d) != colour)
            break;
    }
    if (d == 6) return 0;               /* nowhere to put it */

    /* 3. Clear the source bit → leftover colour */
    /*    We force the bit to the paper side or ink side that matches leftover */
    if (src->inv) {
        /* inverted cell: paper side shows paper^7, ink side shows ink^7 */
        if ((leftover ^ 7) == src->paper)
            src->bits &= ~(1 << s);     /* paper side */
        else if ((leftover ^ 7) == src->ink)
            src->bits |=  (1 << s);     /* ink side */
        else {
            /* leftover not in current pair – force paper side to leftover */
            src->paper = leftover ^ 7;
            src->bits &= ~(1 << s);
        }
    } else {
        if (leftover == src->paper)
            src->bits &= ~(1 << s);
        else if (leftover == src->ink)
            src->bits |=  (1 << s);
        else {
            src->paper = leftover;
            src->bits &= ~(1 << s);
        }
    }

    /* 4. Set the destination bit to ‘colour’ */
    if (dst->inv) {
        if ((colour ^ 7) == dst->paper)
            dst->bits &= ~(1 << d);
        else if ((colour ^ 7) == dst->ink)
            dst->bits |=  (1 << d);
        else {
            dst->ink = colour ^ 7;      /* adopt the colour as ink */
            dst->bits |=  (1 << d);
        }
    } else {
        if (colour == dst->paper)
            dst->bits &= ~(1 << d);
        else if (colour == dst->ink)
            dst->bits |=  (1 << d);
        else {
            dst->ink = colour;
            dst->bits |=  (1 << d);
        }
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/* High-level exchange rules (the public API)                         */
/* ------------------------------------------------------------------ */

/* Rule 1 – classic balance: move one pixel of colour, leave black */
#define RULE_BALANCE_TO_BLACK(src, dst, colour) \
    CELL_TRANSFER(src, dst, colour, 0)

/* Rule 2 – leave white instead of black */
#define RULE_BALANCE_TO_WHITE(src, dst, colour) \
    CELL_TRANSFER(src, dst, colour, 7)

/* Rule 3 – leave a nominated secondary colour */
#define RULE_BALANCE_TO_COLOUR(src, dst, colour, leftover) \
    CELL_TRANSFER(src, dst, colour, leftover)

/* Rule 4 – mutual exchange (A gives colourX, B gives colourY) */
#define RULE_MUTUAL(src, dst, colourX, colourY, leftX, leftY) \
    ( CELL_TRANSFER(src, dst, colourX, leftX) | \
      CELL_TRANSFER(dst, src, colourY, leftY) )

/* Rule 5 – “green-heavy” specialised balance (most common demo case) */
#define RULE_GREEN_BALANCE(src, dst) \
    RULE_BALANCE_TO_BLACK(src, dst, 2)   /* 2 = green */

/* Rule 6 – full RGB balance pass (one pixel of each primary) */
#define RULE_RGB_BALANCE(src, dst) (            \
    RULE_BALANCE_TO_BLACK(src, dst, 1) |        /* red   */ \
    RULE_BALANCE_TO_BLACK(src, dst, 2) |        /* green */ \
    RULE_BALANCE_TO_BLACK(src, dst, 4)          /* blue  */ \
)

/* ------------------------------------------------------------------ */
/* Convenience – apply a rule then write both cells to screen memory  */
/* ------------------------------------------------------------------ */

#define APPLY_AND_WRITE(rule, cellA, cellB, addrA, addrB) \
    do {                                                  \
        rule(cellA, cellB);                               \
        { unsigned char *p = (addrA); CELL_TO_BYTES(cellA, p); } \
        { unsigned char *p = (addrB); CELL_TO_BYTES(cellB, p); } \
    } while (0)

#endif /* ORIC_CELL_EXCHANGE_H */
