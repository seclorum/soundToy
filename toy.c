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

void Synth();

#define POS_START ((unsigned int)0x0000)
#define POS_END   ((unsigned int)0xFFFF)
#define POS_INITIAL_POSITION ((unsigned int)0xFAA7)
#define POS_KEYCLICK1 ((unsigned int)0xFB32)

extern unsigned char currentSound[];
extern unsigned char BootSound[];

#define CHAN_A (0)
#define CHAN_B (2)
#define CHAN_C (4)
#define CHAN_INDEX(x) (x / 2)

#define PITCH_WORD(x) ((unsigned int)currentSound[x] + ((unsigned int)currentSound[x+1] << 8))
#define CHAN_PITCH(c) (PITCH_WORD(c))


void printHelp()
{
	cls(); paper(0); ink(5);
    printf("\nsoundtoy::\nA toy for exploring Oric ROM sounds.\n");
	printf("(by seclorum)\n\n");
	printf("Key: Function:\n\n");
	printf(" ?   Display this help.\n");
	printf(" P   Set \"PRESET pointer\"\n     : enter HEX address.\n");
	printf(" 	   e.g. FAA7, FABD, FAD3, FB10..\n");
	printf(" J   Increment the \"PRESET pointer\".\n");
	printf(" K   Decrement the \"PRESET pointer\".\n");
	printf(" R   Reset to the \"bootup sound\".\n");
	printf(" . 	 Reset to the \"PING\" sound.\n\n");
	printf(" Q	 Quit from soundtoy.\n\n");
	printf("Any other key will trigger the synth.\n\n");
	printf("Explore the Oric ROM for more sounds!\n");
}

char *charHexStr(unsigned char value) {
	static char hexString[3];
    static const char hexLookup[] = "0123456789ABCDEF";
    hexString[0] = hexLookup[(value >> 4) & 0x0F];
    hexString[1] = hexLookup[value & 0x0F];
    hexString[2] = '\0';
    return hexString;
}

void hexDump(unsigned char *addr, int length)
{
	int cnt=0;
	int i=0;
	for (i=0;i<length;i++) {
		if (cnt++>7) { cnt = 0; printf("\n"); }
		printf(" %s", charHexStr(addr[i]));
		// if (i>13) printf(":"); else printf(" ");
	}
	printf("\n");
}

loadTable(unsigned int address)
{
	currentSound[0] = peek(address);
	currentSound[1] = peek(address+1);
	currentSound[2] = peek(address+2);
	currentSound[3] = peek(address+3);
	currentSound[4] = peek(address+4);
	currentSound[5] = peek(address+5);
	currentSound[6] = peek(address+6);
	currentSound[7] = peek(address+7);
	currentSound[8] = peek(address+8);
	currentSound[9] = peek(address+9);
	currentSound[10] = peek(address+10);
	currentSound[11] = peek(address+11);
	currentSound[12] = peek(address+12);
	currentSound[13] = peek(address+13);
}

dumpPitch(unsigned char channel)
{
	printf("[%d][p:%x]\n", CHAN_INDEX(channel), CHAN_PITCH(channel));
}

void main()
{   
	unsigned char k;		// key pressed
	unsigned int p;			// position
	unsigned int i; 		// input

	setflags(SCREEN+NOKEYCLICK);

	printHelp();
	Synth();

	p=POS_INITIAL_POSITION; // start of ROM
	
	while((k=get())!='Q') {

		if(k=='J') {
			if (p++ >= POS_END) { p=POS_START; } 
		}
		else
		if(k=='K') {
			if (p-- < POS_START) { p=POS_END; } 
		}
		else
		if(k=='R') {
			printf("reset!\n");
			p=(unsigned int)&BootSound;
			//p=POS_KEYCLICK1;
		}
		else
		if(k=='.') {
			printf(".\n");
			p=POS_INITIAL_POSITION;
		}
		else
		if(k=='P') {
			unsigned int v;
			printf("P:");
			scanf("%x", &v);
			p=v;
			printf("\nv:%x p:%x", v, p);
		}
		else 
		if(k=='?') {
			printHelp();
		}

		{
			loadTable(p);

			printf("\nP:%x\n", p);
			hexDump((unsigned char *)p, 16);

#if 0
			// disabled for now, will be the subject of more work in the near future
			dumpPitch(CHAN_A);
			dumpPitch(CHAN_B);
			dumpPitch(CHAN_C);
#endif

			Synth();
		}

	}

	loadTable((unsigned int)&BootSound);

	Synth();
}
