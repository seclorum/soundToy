/*
 * soundtoy - a sound toy for Oric.
 * 
 * Author: Jay Vaughan
 *
 * Intended as a scratchpad/workbench for exploring the Orics' synthesis capabilities.
 *
 * The Oric is capable of an extraordinary level of synthesis, and the purpose of 
 * this program is to explore that capability in creative and fun ways.
 *
 * V1.0.0 - Basic "factory preset patch" memory concept added - will take the seed
 * values (14 registers of the 8192) from any location in Oric ROM and use them to 
 * generate a sound.  This gives us a huge library of sounds to start with, out of 
 * the box. ;)
 *
 */
#include <lib.h>

#include "gencode.h"

extern unsigned char currentSound[];
extern unsigned char BootSound[];
// from picture.s

void Synth();

#define POS_START 		((unsigned int)0x0000)
#define POS_END   		((unsigned int)0xFFFF)
#define POS_INITIAL_POSITION ((unsigned int)0xFAA7)
#define POS_KEYCLICK1 	((unsigned int)0xFB32)
#define HIRES_START   	((unsigned char *)0xA000)
#define HIRES_END 		((unsigned char *)0xBF3F)

extern unsigned char OverlayLabel[];

/* Notes:
$272,$273    Keyboard timer.
$274,$275    Cursor timer.
$276,$277    Spare counter — also used by WAIT (and printer in V1.0).
*/

#define CGENSTATE_INIT_ 0x11

enum {
	CGENSTATE_INIT = CGENSTATE_INIT_,
	CGENSTATE_FORWARD = 1,
	CGENSTATE_BACKWARD = -1,
	CGENSTATE_QUIT = 0x10
};

#define CHAN_A (0)
#define CHAN_B (2)
#define CHAN_C (4)
#define CHAN_INDEX(x) (x / 2)

#define PITCH_WORD(x) ((unsigned int)currentSound[x] + ((unsigned int)currentSound[x+1] << 8))
#define CHAN_PITCH(c) (PITCH_WORD(c))

#define APP_SOUND_ONOFF		'H'
#define APP_POS_POSITIVE	'J'
#define APP_POS_MINUS		'K'
#define APP_RESET			'R'
#define APP_RESET_POS		'.'
#define APP_POSITION		'P'
#define APP_HELP			'?'
#define APP_HIRES			'G'
#define APP_MEMHAK 			'M'
#define APP_QUIT 			'Q'
#define APP_CAPTURE 		'C'
#define APP_SPACE			' '
#define APP_CURSOR_L		'A'
#define APP_CURSOR_R		'D'
#define APP_CURSOR_U		'W'
#define APP_CURSOR_D		'S'
#define APP_INPUT_L			'J'
#define APP_INPUT_R			'L'
#define APP_INPUT_U			'I'
#define APP_INPUT_D			'K'
#define APP_BLAST			'B'


// cgen
#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*3)

#define ROW_SIZE 40
#define COL_SIZE 22
#define BLOCK_HEIGHT 8


#define MAX_CB_X 40
#define MAX_CB_Y MAX_CB_CELLS


// aliases for positions in the displayInstructions
#define STOY1 0
#define STOY2 1
#define STOY3 2
#define SAVE1 3
#define CURS1 4
#define CURS2 5
#define ANIM1 6
#define ANIM2 7
#define ANIM3 8
#define ANIM4 9
#define ANIM5 10
#define ANIM6 11
#define MONO1 12
#define MONO2 13
#define COLR1 14
#define COLR2 15
#define UIRI1 16
#define UIRI2 17
#define UIRI3 18

/*
#define MONO5 16
#define MONO6 17
#define COLR1 18
#define COLR2 19
#define COLR3 20
#define COLR4 21
#define COLR5 22
#define COLR6 23
*/

#define UI_Y 20
#define UI_STOR1 2
#define UI_STOR2 4
#define UI_STOR3 6


// A lookup table to filter out values from random quickly
// 256 numbers 
//unsigned char table[256];

//static int rando;             // random
//static int fil;               // not too much random: filter

// The animated x/y cell
static int anim_x;				// xpos
static int anim_y;				// anim_yos

static int stoy_x;				// set toy x
static int stoy_y;				//

int r_val;						// some randomite

static int g_state = CGENSTATE_INIT_;	// state, 0xFF=initialize


// The current cursor position
static int cursor_x;			// cursor x
static int cursor_y;			// cursor y

// The current 'cell' from which input is derived
static int input_x;				// input pos x
static int input_y;				// input pos y

static int ui_x;
static int ui_y;

// Assembly routine to do the search
//extern unsigned char FindRandomNumber(unsigned char *table);

// The raw address of source and destinations in the cell copy
unsigned int cell_source_addr;
unsigned int cell_dest_addr;
unsigned int cell_source_base;


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
void setDI(int cell, int sourceX, int sourceY, int destX, int destY)
{

	unsigned int buffer_base =
		SOURCE_BUFFER_START + (sourceY * ROW_SIZE * BLOCK_HEIGHT) +
		(sourceX * 1);
	unsigned int dest_base =
		DESTINATION_BUFFER_START + (destY * ROW_SIZE * BLOCK_HEIGHT) +
		(destX * 1);

	if (cell >= MAX_CB_CELLS) {
	} else {
		// Modify the source and destination addresses for each row (8 rows)
		int pos;
		for (pos = 0; pos < BLOCK_HEIGHT; pos++) {
			// Source address modification
			cell_source_addr = buffer_base + (pos * ROW_SIZE);
			displayInstructions[cell][3 + pos * 6] = cell_source_addr & 0xFF;	// Low byte
			displayInstructions[cell][4 + pos * 6] = (cell_source_addr >> 8) & 0xFF;	// High byte

			// Destination address modification
			cell_dest_addr = dest_base + (pos * ROW_SIZE);
			displayInstructions[cell][6 + pos * 6] = cell_dest_addr & 0xFF;	// Low byte
			displayInstructions[cell][7 + pos * 6] = (cell_dest_addr >> 8) & 0xFF;	// High byte
		}
	}
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
	x = 11;

	for (c = 0; c < MAX_CB_CELLS; c++) {
		setDI(c, ix, iy, c, 10);
		//printf("blast:%d,%d:%d,%d\n", ix, iy, x, c);
	}
	call((unsigned int) &displayInstructions[0]);


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

	if (input_x % 2) {
		setDI(CURS1, input_x, input_y, cursor_x, cursor_y);
		setDI(CURS2, input_x, input_y, cursor_x + 1, cursor_y + 1);
	} else {
		setDI(CURS1, input_x + 1, input_y, cursor_x, cursor_y + 1);
		setDI(CURS2, input_x, input_y + 1, cursor_x + 1, cursor_y);
	}

	call((unsigned int) &displayInstructions[0]);
}

void cellAnim(unsigned char kp)
{

	setDI(ANIM1, cursor_x, cursor_y, ui_x, 7);
	setDI(ANIM2, cursor_x, cursor_y, ui_x, 8);
	setDI(ANIM3, cursor_x, cursor_y, ui_x, 9);
	setDI(ANIM4, cursor_x, input_y, ui_x, 10);
	setDI(ANIM5, cursor_x, input_y, ui_x, 11);
	setDI(ANIM6, cursor_x, input_y, ui_x, 12);
	setDI(MONO1, cursor_x, input_y, ui_x, 13);
	setDI(MONO2, cursor_x, input_y, ui_x, 14);
	setDI(COLR1, cursor_x, input_y, ui_x, 15);
	setDI(COLR2, cursor_x, input_y, ui_x, 16);
	setDI(UIRI1, cursor_x, input_y, ui_x, 17);
	setDI(UIRI2, cursor_x, input_y, ui_x, 18);
	setDI(UIRI3, cursor_x, input_y, ui_x, 19);

	call((unsigned int) &displayInstructions[0]);
}


void cellHack(unsigned char kp)
{

//  initTable();  // Initialize the table with values
//  check_rantab(); 

//  printf("c:%d,%d,i:%d,%d p:%d,%d r:%d\r", cursor_x, cursor_y, input_x, input_y, anim_x, anim_y, r_val);

	if (0) {
		// the random stuff
		stoy_x += g_state;
		stoy_y += g_state;

		r_val = qrandomJ(peek(0x276)) % COL_SIZE;
		if (r_val < 12) {
			anim_y -= g_state;
		};

		setDI(STOY1, 1, r_val, stoy_x, stoy_y + 1);
		setDI(STOY2, 2, r_val, stoy_x, stoy_y + 2);
		setDI(STOY3, 3, r_val, stoy_x, stoy_y + 3);
		setDI(SAVE1, cursor_x, cursor_y, MAX_CB_X, MAX_CB_Y);
	}

	setDI(ANIM1, input_x, input_y, cursor_x, cursor_y);

	setDI(CURS2, cursor_x, cursor_x, 1, 1);
	setDI(CURS1, input_x, input_y, 2, 1);

	call((unsigned int) &displayInstructions[0]);

}



void cellDemo(unsigned char ik)
{
	unsigned char kp = 'D';
	int iter;

	for (iter = 0; iter < 99; iter++) {
		cellHack(kp);
		if (iter % 8) {
			kp = 'A';
		}
	}

}



void printHelp()
{
	cls();
	paper(0);
	ink(5);
	printf("\nsoundtoy:: 1.1.0\nA toy for exploring Oric ROM sounds.\n");
	printf("(by seclorum)\n\n");
	printf("Key: Function:\n\n");
	printf(" ?   Display this help.\n");
	printf(" P   Set \"PRESET pointer\"\n     : enter HEX address.\n");
	printf(" 	   e.g. FAA7, FABD, FAD3, FB10..\n");
	printf(" J   Increment the \"PRESET pointer\".\n");
	printf(" K   Decrement the \"PRESET pointer\".\n");
	printf(" R   Reset to the \"bootup sound\".\n");
	printf(" . 	 Reset to the \"PING\" sound.\n");
	printf(" G 	 HIRES mode.\n");
	printf(" Q	 Quit from soundtoy.\n\n");
	printf("Any other key will trigger the synth.\n\n");
	printf("Explore the Oric ROM for more sounds!\n");
}




dumpPitch(unsigned char channel)
{
	printf("[%d][p:%x]\n", CHAN_INDEX(channel), CHAN_PITCH(channel));
}

void SynthZP(unsigned char HI, unsigned char LO)
{
	poke(0x14, HI);
	poke(0x15, LO);
	call(0xF590);
}




void gen_rnd_colors()
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

void main()
{
	unsigned char kp;			// key pressed
	unsigned int position;		// position
	unsigned int i;				// input
	unsigned char r;
	unsigned char shouldPlay;
	int j;
	int m;
	int e_mode = 0;				// edit mode

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
					(input_y * ROW_SIZE * BLOCK_HEIGHT) + (input_x * 1);
				hexDump((unsigned char *) cell_source_base, 8);
			}

			if (kp == APP_CURSOR_L) {
				cursor_x -= 1;
			};

			if (kp == APP_CURSOR_R) {
				cursor_x += 1;
			};

			if (kp == APP_CURSOR_U) {
				cursor_y -= 1;
			};

			if (kp == APP_CURSOR_D) {
				cursor_y += 1;
			};

			if (kp == APP_INPUT_L) {
				input_x -= 1;
			};

			if (kp == APP_INPUT_R) {
				input_x += 1;
			};

			if (kp == APP_INPUT_U) {
				input_y -= 1;
			};
			if (kp == APP_INPUT_D) {
				input_y += 1;
			};

			if (anim_y > MAX_CB_Y) {
				anim_y = 2;
			};

			if (anim_y < 2) {
				anim_y = MAX_CB_Y;
			};

			if (stoy_x > MAX_CB_X) {
				g_state = CGENSTATE_BACKWARD;
			};

			if (stoy_x < 2) {
				g_state = CGENSTATE_FORWARD;
			};

			if (stoy_y > MAX_CB_Y) {
				stoy_y = 2;
			};

			if (stoy_y < 2) {
				stoy_y = MAX_CB_Y;
			};

			if (cursor_y > MAX_CB_Y) {
				cursor_y = 2;
			};

			if (cursor_y < 2) {
				cursor_y = MAX_CB_Y;
			};

			if (cursor_x > MAX_CB_X) {
				cursor_x = MAX_CB_X;
			};
			if (cursor_x < 2) {
				g_state = CGENSTATE_FORWARD;
			};

			if (kp == APP_BLAST) {
				cellBlast(input_x, input_y);
			};

			if (kp == APP_SPACE) {
				triggerCurrent(cell_source_base);
			}

			ui_x++;
			ui_y++;
			if (ui_x < ROW_SIZE)
				ui_x = 0;
			if (ui_y < COL_SIZE)
				ui_y = 0;

			// tick
			cellHack(kp);
			cellUI(kp);
			cellAnim(kp);

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
			if (position < POS_START) {
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

			// cellHack
			e_mode = 1;

			hires();
			poke(0x26A, 10);	// disble keyclick and cursor

			// memcpy((unsigned char*)0xa000, OverlayLabel, 8000);

			// spamit();
			// randcolorgen();
			// randcogtab();

			gen_rnd_colors();

			printf("pos: mode:%x\n", position, e_mode);
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
