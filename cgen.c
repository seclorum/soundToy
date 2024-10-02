#include <stdio.h>
#include <string.h>

#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*3)

#define ROW_SIZE 40
#define BLOCK_HEIGHT 8

#define NUM_CB_CELLS 4

#define STOY1 0
#define STOY2 1
#define STOY3 2
#define STOY4 3

// Predefined op code block (LDA, STA sequence for 8 rows)
unsigned char cellOpArray[NUM_CB_CELLS][51] = {

  {
   0xA2, 0x00,			// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,		// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,		// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xEA},			// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,			// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,		// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,		// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xEA},			// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,			// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,		// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,		// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xEA},			// NOP ( RTS at last ll ]

  {
   0xA2, 0x00,			// LDX #$00 (initialize X register)
   0xBD, 0x00, 0x00,		// LDA $0000,X (placeholder for source address)
   0x9D, 0x00, 0x00,		// STA $0000,X (placeholder for destination address)
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 2)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 3)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 4)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 5)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 6)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 7)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0xBD, 0x00, 0x00,		// LDA $0000,X (row 8)
   0x9D, 0x00, 0x00,		// STA $0000,X
   0x60},			// RTS (return from subroutine)

};

// Function to modify the block with correct addresses based on cell index
void
genCellCopy (int cell, int sourceX, int sourceY, int destX, int destY)
{
  unsigned int source_base =
    SOURCE_BUFFER_START + (sourceY * ROW_SIZE * BLOCK_HEIGHT) + (sourceX * 1);
  unsigned int dest_base =
    DESTINATION_BUFFER_START + (destY * ROW_SIZE * BLOCK_HEIGHT) +
    (destX * 1);
  unsigned int source_addr;
  unsigned int dest_addr;

  if (cell >= NUM_CB_CELLS)
    {
    }
  else
    {
      // Modify the source and destination addresses for each row (8 rows)
      int pos;
      for (pos = 0; pos < BLOCK_HEIGHT; pos++)
	{
	  // Source address modification
	  source_addr = source_base + (pos * ROW_SIZE);
	  cellOpArray[cell][3 + pos * 6] = source_addr & 0xFF;	// Low byte
	  cellOpArray[cell][4 + pos * 6] = (source_addr >> 8) & 0xFF;	// High byte

	  // Destination address modification
	  dest_addr = dest_base + (pos * ROW_SIZE);
	  cellOpArray[cell][6 + pos * 6] = dest_addr & 0xFF;	// Low byte
	  cellOpArray[cell][7 + pos * 6] = (dest_addr >> 8) & 0xFF;	// High byte
	}
    }
}

int
generateCells ()
{
  static int pos;
  static int level;
  static int rando;

  int key;

  // simple example
  level = 2;
  rando = 3;
  while (1)
    {
      rando++;
      if (rando > 24)
	rando = 0;


      genCellCopy (STOY1, rando + 1, 0, pos++, 0);
      genCellCopy (STOY2, rando + 2, 0, pos++, level++);
      genCellCopy (STOY3, rando + 3, 0, pos++, level++);
      genCellCopy (STOY4, rando + 4, 0, pos++, level++);

      if (level > 10)
	level = 5;
      if (pos > 20)
	pos = 4;

      call (&cellOpArray[0]);
    }

  return;
}
