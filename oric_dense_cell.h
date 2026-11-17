/*
 * Oric Atmos / Oric-1 HIRES serial-attribute colour-density rules
 * (OSDK / cc65 style).  All values are unsigned char (byte).
 *
 * A byte is an ATTRIBUTE iff (v & 0x60) == 0.
 *   → bits 5 and 6 both clear  →  value 0..31 or 128..159
 * Everything else is PIXEL DATA (6 pixels in bits 0-5, bit 7 = inverse).
 *
 * Colour density without attribute clash on neighbouring regions is only
 * possible inside the ranges below.  Inverse (bit 7) gives the complementary
 * PAPER/INK pair for free, doubling the usable colours inside a 6-pixel cell
 * without an extra attribute byte.
 */

/* ------------------------------------------------------------------ */
/* 1. Single-byte ranges                                              */
/* ------------------------------------------------------------------ */

/* Pure attributes (force 6 pixels of current PAPER, change state) */
#define ATTR_INK_LO     0x00   /* 0..7   – set INK                */
#define ATTR_INK_HI     0x07
#define ATTR_STYLE_LO   0x08   /* 8..15  – charset / dbl / blink  */
#define ATTR_STYLE_HI   0x0F
#define ATTR_PAPER_LO   0x10   /* 16..23 – set PAPER              */
#define ATTR_PAPER_HI   0x17
#define ATTR_VIDEO_LO   0x18   /* 24..31 – 50/60 Hz + TEXT/HIRES  */
#define ATTR_VIDEO_HI   0x1F

/* Inverse attributes (same effect + local inverse of the 6 PAPER pixels) */
#define ATTR_INV_INK_LO     0x80
#define ATTR_INV_INK_HI     0x87
#define ATTR_INV_STYLE_LO   0x88
#define ATTR_INV_STYLE_HI   0x8F
#define ATTR_INV_PAPER_LO   0x90
#define ATTR_INV_PAPER_HI   0x97
#define ATTR_INV_VIDEO_LO   0x98
#define ATTR_INV_VIDEO_HI   0x9F

/* Safe monochrome / two-colour pixel data (bit 6 forced, bit 5 free) */
#define PIX_NORMAL_LO   0x40   /* 64..127  – non-inverse pixels   */
#define PIX_NORMAL_HI   0x7F
#define PIX_INVERSE_LO  0xC0   /* 192..255 – inverse pixels       */
#define PIX_INVERSE_HI  0xFF

/* Pixel data that still has bit 5 set (bit 6 may be clear).
 * One pixel is forced ON; useful for denser packing when you can live
 * with a permanently lit bit. */
#define PIX_BIT5_LO     0x20   /* 32..63 / 160..191               */
#define PIX_BIT5_HI     0x3F
#define PIX_BIT5_INV_LO 0xA0
#define PIX_BIT5_INV_HI 0xBF

/* ------------------------------------------------------------------ */
/* 2. Two-byte (byte-pair) interaction windows                        */
/*    (left byte changes state, right byte shows dense colour)        */
/* ------------------------------------------------------------------ */

/*
 * Legal non-clashing pairs that keep the rest of the scan-line
 * free of unwanted attribute side-effects:
 *
 *   PAPER then INK   (or INK then PAPER)
 *   + optional inverse on the second byte
 *
 * Ranges that may be written as a consecutive pair without
 * polluting neighbouring 6-pixel cells on the same line:
 */
static const unsigned char safe_pair_paper_ink[][2] = {
    /* normal PAPER + normal INK */
    {0x10,0x00},{0x11,0x01},{0x12,0x02},{0x13,0x03},
    {0x14,0x04},{0x15,0x05},{0x16,0x06},{0x17,0x07},
    /* normal PAPER + inverse INK (extra complementary colours) */
    {0x10,0x80},{0x11,0x81},{0x12,0x82},{0x13,0x83},
    {0x14,0x84},{0x15,0x85},{0x16,0x86},{0x17,0x87},
    /* inverse PAPER + normal INK */
    {0x90,0x00},{0x91,0x01},{0x92,0x02},{0x93,0x03},
    {0x94,0x04},{0x95,0x05},{0x96,0x06},{0x97,0x07},
    /* inverse PAPER + inverse INK */
    {0x90,0x80},{0x91,0x81},{0x92,0x82},{0x93,0x83},
    {0x94,0x84},{0x95,0x85},{0x96,0x86},{0x97,0x87}
};

/* ------------------------------------------------------------------ */
/* 3. Three-byte (byte + byte-pair) “maximum density” window          */
/* ------------------------------------------------------------------ */

/*
 * The densest colour region that does not force clash onto any other
 * cell on the same scan-line is a three-byte sequence:
 *
 *   [PAPER attr] [INK attr] [pixel data with optional inverse]
 *
 * or the swapped order [INK][PAPER][pixel].
 *
 * After the two attribute bytes the third byte may freely use the
 * full 6-bit pixel field (OR 0x40) and/or the inverse bit (OR 0x80).
 * All eight colours (and their eight inverses) are therefore available
 * inside a 18-pixel-wide strip without side-effects left or right.
 *
 * Forbidden in the third byte: any value with (v & 0x60) == 0
 * (would become a new attribute and re-colour everything to its right).
 */
#define DENSE_PIXEL_MASK  0x3F   /* six pixel bits                */
#define DENSE_FORCE_BIT6  0x40   /* guarantee “not attribute”     */
#define DENSE_INVERSE     0x80   /* complementary PAPER/INK pair  */

/* Helper: build a dense pixel byte */
static inline unsigned char dense_pixel(unsigned char bits6, int inverse)
{
    return (bits6 & DENSE_PIXEL_MASK) | DENSE_FORCE_BIT6 | (inverse ? DENSE_INVERSE : 0);
}

/* ------------------------------------------------------------------ */
/* 4. Values that MUST be avoided for non-clashing dense regions      */
/* ------------------------------------------------------------------ */

/* Video-control attributes (24-31 / 152-159) change 50/60 Hz and/or
 * TEXT↔HIRES for the whole machine on the next frame.  They are
 * global side-effects and destroy any local density guarantee. */
#define IS_VIDEO_CTRL(v)  (((v) & 0x7F) >= 0x18 && ((v) & 0x7F) <= 0x1F)

/* Any attribute at all collides with a pure pixel cell */
#define IS_ATTRIBUTE(v)   (((v) & 0x60) == 0)

/* ------------------------------------------------------------------ */
/* 5. Practical “write a colour-dense cell” macro (OSDK style)        */
/* ------------------------------------------------------------------ */

/*
 * Write a maximal-density 6-pixel cell at address p that does not
 * impose colour clash on any other cell on the same scan-line.
 * paper / ink are 0..7; bits6 is the 6-bit pixel pattern; inv selects
 * the complementary pair.
 */
#define WRITE_DENSE_CELL(p, paper, ink, bits6, inv)   \
    do {                                              \
        *(p)++ = 0x10 | ((paper) & 7);                \
        *(p)++ = 0x00 | ((ink)   & 7);                \
        *(p)   = dense_pixel((bits6), (inv));         \
    } while (0)

