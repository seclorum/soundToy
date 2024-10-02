#include <stdio.h>
#include <string.h>

#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*3)

#define ROW_SIZE 40
#define BLOCK_HEIGHT 8


// Predefined opcode block (LDA, STA sequence for 8 rows)
unsigned char pregenCodeBlock[] = {
    0xA2, 0x00,             // LDX #$00 (initialize X register)
    0xBD, 0x00, 0x00,       // LDA $0000,X (placeholder for source address)
    0x9D, 0x00, 0x00,       // STA $0000,X (placeholder for destination address)
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 2)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 3)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 4)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 5)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 6)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 7)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0xBD, 0x00, 0x00,       // LDA $0000,X (row 8)
    0x9D, 0x00, 0x00,       // STA $0000,X
    0x60                    // RTS (return from subroutine)
};

// Function to modify the block with correct addresses based on cell index
void genCellCopy(int sourceX, int sourceY, int destX, int destY) {
    unsigned int source_base = SOURCE_BUFFER_START + (sourceY * ROW_SIZE * BLOCK_HEIGHT) + (sourceX * 1);
    unsigned int dest_base = DESTINATION_BUFFER_START + (destY * ROW_SIZE * BLOCK_HEIGHT) + (destX * 1);
	unsigned int source_addr;
	unsigned int dest_addr;

    // Modify the source and destination addresses for each row (8 rows)
	int pos;
    for (pos = 0; pos < BLOCK_HEIGHT; pos++) {
        // Source address modification
        source_addr = source_base + (pos * ROW_SIZE);
        pregenCodeBlock[3 + pos * 6] = source_addr & 0xFF;       // Low byte
        pregenCodeBlock[4 + pos * 6] = (source_addr >> 8) & 0xFF; // High byte

        // Destination address modification
        dest_addr = dest_base + (pos * ROW_SIZE);
        pregenCodeBlock[6 + pos * 6] = dest_addr & 0xFF;         // Low byte
        pregenCodeBlock[7 + pos * 6] = (dest_addr >> 8) & 0xFF;  // High byte
    }
}

int cgen_test() {
	int pos;
	int level;
	static int rando;
	int key;

	// simple example

//	while ((key = get()) != 'Q') {
	while (1) {
			rando+=2; if (rando>10) rando = 0;
			genCellCopy(rando, 0, rando, 0); call(&pregenCodeBlock);
			genCellCopy(rando, 0, rando, rando); call(&pregenCodeBlock);
			genCellCopy(rando, 0, 0, rando); call(&pregenCodeBlock);
#if 0
   	 		//rando  = qrandomJ(peek(0x276)) % 255 / 24;
			genCellCopy(rando, 0, 0, 0); call(&pregenCodeBlock);
			//genCellCopy(1, 0, 0, 1); call(&pregenCodeBlock);
			genCellCopy(2, 0, 0, 2); call(&pregenCodeBlock);
			//genCellCopy(2, 0, 0, 3); call(&pregenCodeBlock);
			genCellCopy(1, 0, 0, 4); call(&pregenCodeBlock);
			genCellCopy(1, 0, 1, 4); call(&pregenCodeBlock);
			genCellCopy(1, 0, 2, 4); call(&pregenCodeBlock);
			genCellCopy(1+rando, 0, 3, 4); call(&pregenCodeBlock);
			//genCellCopy(0, 0, 0, 5); call(&pregenCodeBlock);
			//genCellCopy(3, 0, 0, 6); call(&pregenCodeBlock);
			genCellCopy(4+rando, 0, 0, 7); call(&pregenCodeBlock);
			genCellCopy(4+rando, 0, 2, 7); call(&pregenCodeBlock);
			genCellCopy(4+rando, 0, 3, 7); call(&pregenCodeBlock);
			genCellCopy(4+rando, 0, 4, 7); call(&pregenCodeBlock);
			genCellCopy(4+rando, 0, 6, 7); call(&pregenCodeBlock);
			//genCellCopy(3, 0, 0, 8); call(&pregenCodeBlock);
			genCellCopy(1, 0, 0, 9); call(&pregenCodeBlock);
			genCellCopy(1, 0, 0, 10); call(&pregenCodeBlock);
//
      		genCellCopy(3+rando, 0, 1, 0); call(&pregenCodeBlock);
			genCellCopy(2+rando, 0, 1, 1); call(&pregenCodeBlock);
			genCellCopy(1+rando, 0, 1, 2); call(&pregenCodeBlock);
			genCellCopy(0+rando, 0, 1, 3); call(&pregenCodeBlock);
 
     		genCellCopy(3+rando, 0, 2, 0); call(&pregenCodeBlock);
			genCellCopy(2+rando, 0, 2, 1); call(&pregenCodeBlock);
			genCellCopy(1+rando, 0, 2, 2); call(&pregenCodeBlock);
			genCellCopy(0+rando, 0, 2, 3); call(&pregenCodeBlock);
     
 			genCellCopy(0+rando, 0, 3, 0); call(&pregenCodeBlock);
			genCellCopy(1+rando, 0, 3, 1); call(&pregenCodeBlock);
			genCellCopy(2+rando, 0, 3, 2); call(&pregenCodeBlock);
			genCellCopy(3+rando, 0, 3, 3); call(&pregenCodeBlock);

  			genCellCopy(0+rando, 0, 5, 0); call(&pregenCodeBlock);
			genCellCopy(1+rando, 0, 5, 1); call(&pregenCodeBlock);
			genCellCopy(2+rando, 0, 5, 2); call(&pregenCodeBlock);
			genCellCopy(3+rando, 0, 5, 3); call(&pregenCodeBlock);
 
		level=3;
		for (pos=0;pos<24;pos++) {
   	 		rando  = qrandomJ(peek(0x276)) % 255 / 24;
   	 		genCellCopy(0+rando, 0, pos, level); call(&pregenCodeBlock);
    		genCellCopy(1+rando, 0, pos+1, level++); call(&pregenCodeBlock);
    		genCellCopy(2+rando, 0, pos+2, level++); call(&pregenCodeBlock);
    		genCellCopy(3+rando, 0, pos+3, level); call(&pregenCodeBlock);
			level = 5;
    		genCellCopy(23-rando, 0, pos+1, level++); call(&pregenCodeBlock);
    		genCellCopy(22-rando-1, 0, pos+2, level++); call(&pregenCodeBlock);
    		genCellCopy(21-rando-2, 0, pos+3, level++); call(&pregenCodeBlock);
    		genCellCopy(20-rando-3, 0, pos+4, level++); call(&pregenCodeBlock);

   	 		genCellCopy(6+rando, 0, 1, 1); call(&pregenCodeBlock);
		}
#endif
	}
 

  //  genCellCopy(2, 1, 7, 7);
   // call(&pregenCodeBlock);

#if 0
    // Example output of the modified block
    print("Modified block for copying 8x8 cell:\n");
    for (pos = 0; pos < sizeof(pregenCodeBlock); pos++) {
        printf("%02X ", pregenCodeBlock[pos]);
        if ((pos + 1) % 8 == 0) printf("\n");  // Format in rows
    }
#endif

    // At this point, you would jump to the `pregenCodeBlock` to execute it.

    return 0;
}

