#include <stdio.h>
#include <string.h>

#define SOURCE_BUFFER_START 0xA000
#define DESTINATION_BUFFER_START 0xA000 + (320*2)
#define ROW_SIZE 40
#define BLOCK_HEIGHT 8


// Predefined opcode block (LDA, STA sequence for 8 rows)
unsigned char pregen_8byte_put[] = {
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
    unsigned int source_base = SOURCE_BUFFER_START + (sourceY * ROW_SIZE * BLOCK_HEIGHT) + (sourceX * 8);
    unsigned int dest_base = DESTINATION_BUFFER_START + (destY * ROW_SIZE * BLOCK_HEIGHT) + (destX * 8);
	unsigned int source_addr;
	unsigned int dest_addr;

    // Modify the source and destination addresses for each row (8 rows)
	int pos;
    for (pos = 0; pos < BLOCK_HEIGHT; pos++) {
        // Source address modification
        source_addr = source_base + (pos * ROW_SIZE);
        pregen_8byte_put[3 + pos * 6] = source_addr & 0xFF;       // Low byte
        pregen_8byte_put[4 + pos * 6] = (source_addr >> 8) & 0xFF; // High byte

        // Destination address modification
        dest_addr = dest_base + (pos * ROW_SIZE);
        pregen_8byte_put[6 + pos * 6] = dest_addr & 0xFF;         // Low byte
        pregen_8byte_put[7 + pos * 6] = (dest_addr >> 8) & 0xFF;  // High byte
    }
}

int cgen_test() {
	int pos;
	int level;
	int rando;
	int key;

	// simple example

//	while ((key = get()) != 'Q') {
	while (1) {
		level=3;
		for (pos=0;pos<24;pos++) {
   	 		rando  = qrandomJ(peek(0x276)) % 255 / 24;
   	 		genCellCopy(0+rando, 0, pos, level); call(&pregen_8byte_put);
    		genCellCopy(1+rando, 0, pos+1, level++); call(&pregen_8byte_put);
    		genCellCopy(2+rando, 0, pos+2, level++); call(&pregen_8byte_put);
    		genCellCopy(3+rando, 0, pos+3, level); call(&pregen_8byte_put);
			level = 5;
    		genCellCopy(23-rando, 0, pos+1, level++); call(&pregen_8byte_put);
    		genCellCopy(22-rando-1, 0, pos+2, level++); call(&pregen_8byte_put);
    		genCellCopy(21-rando-2, 0, pos+3, level++); call(&pregen_8byte_put);
    		genCellCopy(20-rando-3, 0, pos+4, level++); call(&pregen_8byte_put);

   	 		genCellCopy(6+rando, 0, 1, 1); call(&pregen_8byte_put);
#endif
		}
	}
 

  //  genCellCopy(2, 1, 7, 7);
   // call(&pregen_8byte_put);

#if 0
    // Example output of the modified block
    print("Modified block for copying 8x8 cell:\n");
    for (pos = 0; pos < sizeof(pregen_8byte_put); pos++) {
        printf("%02X ", pregen_8byte_put[pos]);
        if ((pos + 1) % 8 == 0) printf("\n");  // Format in rows
    }
#endif

    // At this point, you would jump to the `pregen_8byte_put` to execute it.

    return 0;
}

