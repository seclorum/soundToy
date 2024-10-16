#include <stdio.h>
#include <string.h>

#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*3)

#define ROW_SIZE 40
#define BLOCK_HEIGHT 8

// be sure to update cellOpArray
#define MAX_CB_CELLS 24

#define MAX_CB_X 40
#define MAX_CB_Y 16


#define STOY1 0
#define STOY2 1
#define STOY3 2
#define SAVE1 3
#define CURS1 4
#define CURS2 5
#define ANIM1 7
#define ANIM2 8
#define ANIM3 9
#define ANIM4 10
#define ANIM5 11
#define ANIM6 12
#define MONO1 14
#define MONO2 15
#define MONO3 16
#define MONO4 17
#define MONO5 18
#define MONO6 19
#define COLR1 20
#define COLR2 21
#define COLR3 22
#define COLR4 23
#define COLR5 24
#define COLR6 25



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
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0x60 },					// RTS (return from subroutine)
 

 	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0x60 },					// RTS (return from subroutine)
 
 	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0x60 },					// RTS (return from subroutine)
 
	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0xEA },					// NOP ( RTS at last ll ]

	{
	 0xA2, 0x00,				// LDX #$00 (initialize X register)
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
	 0x60 },					// RTS (return from subroutine)

};

// Function to modify the block with correct addresses based on cell index
// X,Y values are 22x40
void genCellCopy(int cell, int sourceX, int sourceY, int destX, int destY)
{

	unsigned int source_base =
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

#define CGENDIR_INIT_ 0x11

enum {
	CGENDIR_INIT = CGENDIR_INIT_,
	CGENDIR_FORWARD = 1,
	CGENDIR_BACKWARD = -1,
	CGENDIR_QUIT = 0x10
};



void cellRandomPlayer(unsigned char kp)
{

	static int rando;			// random
	static int fil;				// not too much random: filter

	static int xp;				// xpos
	static int yp;				// ypos
	static int r;				// some randomite

	static int st = CGENDIR_INIT_;	// state, 0xFF=initialize

	static int cx;				// cursor x
	static int cy;				// cursor y

	static int ipx;				// input pos x
	static int ipy;				// input pos y

	unsigned int source_base;

/*
  // simple example
  static int pos;   // pos
  static int level; // level
  level = 2;
  rando = 8;
  genCellCopy (STOY1, rando + 1, 0, pos, 0);
  genCellCopy (STOY2, rando + 2, 0, pos, level);
  genCellCopy (STOY3, rando + 3, 0, pos, level);
  genCellCopy (SAVE1, rando + 4, 0, pos, level);
*/

	if (st == CGENDIR_INIT) {
		st = CGENDIR_FORWARD;

		yp = 12;
		xp = 12;

		cx = 3;
		cy = 3;

		ipx = 1;
		ipy = 1;
	}
//  initTable();  // Initialize the table with values
//  check_rantab(); 
	xp += st;
	yp += st;

	if (kp == 'C') {
		// capture the current cell
		printf("in: %d,%d", ipx, ipy);
		source_base = SOURCE_BUFFER_START + (ipy * ROW_SIZE * BLOCK_HEIGHT) + (ipx* 1);
    	hexDump(source_base, 8);
	}

	if (kp == 'A') {
		cx -= 1;
	};
	if (kp == 'D') {
		cx += 1;
	};
	if (kp == 'W') {
		cy -= 1;
	};
	if (kp == 'S') {
		cy += 1;
	};

	if (kp == 'J') {
		ipx -= 1;
	};
	if (kp == 'L') {
		ipx += 1;
	};
	if (kp == 'I') {
		ipy -= 1;
	};
	if (kp == 'K') {
		ipy += 1;
	};

	if (yp > MAX_CB_Y) {
		yp = 2;
	};
	if (yp < 2) {
		yp = MAX_CB_Y;
	};
	if (xp > MAX_CB_X) {
		st = CGENDIR_BACKWARD;
	};
	if (xp < 2) {
		st = CGENDIR_FORWARD;
	};

	if (cy > MAX_CB_Y) {
		cy = 2;
	};
	if (cy < 2) {
		cy = MAX_CB_Y;
	};
	if (cx > MAX_CB_X) {
		cx = MAX_CB_X;
	};
	if (cx < 2) {
		st = CGENDIR_FORWARD;
	};

	printf("c:%d,%d,i:%d,%d p:%d,%d\r", cx, cy, ipx, ipy, xp, yp);

	r = qrandomJ(peek(0x276)) % 22;
	if (r < 12) {
		yp -= st;
	};

	genCellCopy(STOY1, 1, r, xp, yp + (r / 4));
	genCellCopy(STOY2, 2, r, xp, yp + 4);
	genCellCopy(STOY3, 3, r, xp, yp + (r / 4));
	genCellCopy(SAVE1, cx, cy, 0, 0);
	genCellCopy(CURS1, ipx, ipy, cx, cy);
	genCellCopy(CURS2, ipx, ipy, 20, 20);

	genCellCopy(ANIM1, cx, cx, 		8, 8);
	genCellCopy(ANIM2, ipx, ipy, 	8, 9);
	genCellCopy(ANIM3, cx, cx, 		8, 10);
	genCellCopy(ANIM4, 1, 1, 		8, 11);
	genCellCopy(ANIM5, 2, 2, 		8, 12);
	genCellCopy(ANIM6, 3, 1, 		8, 13);

 	genCellCopy(COLR1, cx, cx, 		8, 8);
	genCellCopy(COLR2, ipx, ipy, 	8, 9);
	genCellCopy(COLR3, cx, cx, 		8, 10);
	genCellCopy(COLR4, 1, 1, 		8, 11);
	genCellCopy(COLR5, 2, 2, 		8, 12);
	genCellCopy(COLR6, 3, 1, 		8, 13);
 
	call(&cellOpArray[0]);

}

 
void cellDemo(unsigned char ik)
{
	unsigned char kp='D';
	int iter;

   	for (iter=0;iter<99;iter++) { 
		cellRandomPlayer(kp);
		if (iter%8) {
			kp = 'A';
		}
	}
}

 
//
// Tick for a cell with input key
//
void cellTick(unsigned char ik)
{
	//cellDemo(ik);
	cellRandomPlayer(ik);
}
 
