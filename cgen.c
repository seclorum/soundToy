#include <stdio.h>
#include <string.h>

#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*3)

#define ROW_SIZE 40
#define BLOCK_HEIGHT 8

#define MAX_CB_CELLS 4

#define MAX_CB_X 40
#define MAX_CB_Y 16


#define STOY1 0
#define STOY2 1
#define STOY3 2
#define STOY4 3
 
// A lookup table to filter out values from random quickly
// 256 numbers 
//unsigned char table[256];

// Assembly routine to do the search
//extern unsigned char FindRandomNumber(unsigned char *table);

// Predefined op code block (LDA, STA sequence for 8 rows)

// The raw address of source and destinations in the cell copy
unsigned int cell_source_addr;
unsigned int cell_dest_addr;

unsigned char cellOpArray[MAX_CB_CELLS][51] = {

  {
   0xA2, 0x00,					// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,			// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,			// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xEA},						// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,					// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,			// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,			// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xEA},						// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,					// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,			// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,			// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xEA},						// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,					// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,			// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,			// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0xBD, 0x00, 0x00,			// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,			// STA $0000,X
   0x60},						// RTS (return from subroutine)

};

// Function to modify the block with correct addresses based on cell index
// X,Y values are 22x40
void genCellCopy (int cell, int sourceX, int sourceY, int destX, int destY)
{

  unsigned int source_base = SOURCE_BUFFER_START + (sourceY * ROW_SIZE * BLOCK_HEIGHT) + (sourceX * 1);
  unsigned int dest_base = DESTINATION_BUFFER_START + (destY * ROW_SIZE * BLOCK_HEIGHT) + (destX * 1);

  if (cell >= MAX_CB_CELLS)
	{
	}
  else
	{
	  // Modify the source and destination addresses for each row (8 rows)
	  int pos;
	  for (pos = 0; pos < BLOCK_HEIGHT; pos++)
		{
		  // Source address modification
		  cell_source_addr = source_base + (pos * ROW_SIZE);
		  cellOpArray[cell][3 + pos * 6] = cell_source_addr & 0xFF;	// Low byte
		  cellOpArray[cell][4 + pos * 6] = (cell_source_addr >> 8) & 0xFF;	// High byte

		  // Destination address modification
		  cell_dest_addr = dest_base + (pos * ROW_SIZE);
		  cellOpArray[cell][6 + pos * 6] = cell_dest_addr & 0xFF;	// Low byte
		  cellOpArray[cell][7 + pos * 6] = (cell_dest_addr >> 8) & 0xFF;	// High byte
		}
	}
}

#if 0
// Initialize the table with some data (e.g., odd numbers)
void initTable() {
	int i;

    for (i = 0; i < 256; i++) {
        table[i] = i * 2 + 1;  // Fill with odd numbers as an example
    }
}

int check_rantab() {
    // Call the assembly function
    unsigned char randomValue;

	randomValue = FindRandomNumber(table);

    // Print the result
    printf("Random number not in the table: %u\n", randomValue);

    return 0;
}
#endif 

int generateCells ()
{

  static int rando; // random
  static int fil; // not too much random: filter

  static int xp; // xpos
  static int yp; // ypos
  static int r;  // some randomite
  static int st; // state

  int kp;

/*
  // simple example
  static int pos;   // pos
  static int level; // level
  level = 2;
  rando = 8;
  genCellCopy (STOY1, rando + 1, 0, pos, 0);
  genCellCopy (STOY2, rando + 2, 0, pos, level);
  genCellCopy (STOY3, rando + 3, 0, pos, level);
  genCellCopy (STOY4, rando + 4, 0, pos, level);
*/

  st = 1;

  yp = 12;
  xp = 12;

//  initTable();  // Initialize the table with values
//  check_rantab();

  while ((kp = key()) != 'Q')
	{

	  xp += st;
	  yp += st;

	  if (yp > MAX_CB_Y) { yp = 2; };
	  if (yp < 2) { yp = MAX_CB_Y; };
	  if (xp > MAX_CB_X) { st = -1; };
	  if (xp < 2) { st = 1; };

  	  r = qrandomJ (peek (0x276)) % 22;

	  if (r < 12) { yp -= st; };

	  genCellCopy (STOY1, 1, r, xp, yp + (r/4));
	  genCellCopy (STOY2, 2, r, xp, yp + 4);
	  genCellCopy (STOY3, 3, r, xp, yp + (r/4));
	  genCellCopy (STOY4, 2, r, xp + (r/4), yp + 1);

	  call (&cellOpArray[0]);

	};

	printf("cgen!");

	//return;


}
