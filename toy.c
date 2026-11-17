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

