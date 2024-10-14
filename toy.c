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

#define POS_START 		((unsigned int)0x0000)
#define POS_END   		((unsigned int)0xFFFF)
#define POS_INITIAL_POSITION ((unsigned int)0xFAA7)
#define POS_KEYCLICK1 	((unsigned int)0xFB32)
#define HIRES_START   	((unsigned char *)0xA000)
#define HIRES_END 		((unsigned char *)0xBF3F)
extern unsigned char currentSound[];
extern unsigned char BootSound[];


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


// from picture.s
extern unsigned char OverlayLabel[];


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

	volatile char r;
	char s;

	// s = peek(0x276);
	// seed_lfsr(s);

	// j .. memory .. k
	j = (unsigned int) HIRES_START;
	k = (unsigned int) HIRES_START + 320 * 3;
	//k = (unsigned int)HIRES_END;

	do {
		// r = 16+((qrandomJ(peek(0x276)) % 255) & 7);
		// // Somewhat slow C-based implementation with assembly RNG - works
		do {
			r = qrandomJ(peek(0x276)) % 255;
		}
		while (((r & 0x78) == 0x08 || (r & 0x78) == 0x18)
			   || ((r & 0x78) == 0x88 || (r & 0x78) == 0x98));

		// r  = qrandomJ(peek(0x276)) % 255;

		// Linear-feedback shift register method
		// r = lfsr_random();

		// table-based with 
		// r = fastbloop();

		// Chema's randgen:
		// r == randgen();

		poke(j, r);
		// printf("j: %x r: %x\n", j, r);

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

	printHelp();
	Synth();

	while ((kp = key()) != APP_QUIT) {
	
		if (e_mode) {
			cellTick(kp);
			continue;
		}

		if (kp == APP_SOUND_ONOFF) {
			shouldPlay = !shouldPlay;
		} 
		else 
		if (kp == APP_POS_POSITIVE) {
			position++;
			if (position >= POS_END) {
				position = POS_START;
			}
			printf("\np:%x", position);
		} 
		else 
		if (kp == APP_POS_MINUS) {
			if (position-- < POS_START) {
				position = POS_END;
			}
			printf("\np:%x", position);
		} 
		else 
		if (kp == APP_RESET) {
			printf("reset!\n");
			position = (unsigned int) &BootSound;
			//p=POS_KEYCLICK1;
		} 
		else 
		if (kp == APP_RESET_POS) {
			printf(".\n");
			position = POS_INITIAL_POSITION;
		} 
		else 
		if (kp == APP_POSITION) {
			unsigned int v;
			printf("P:");
			scanf("%x", &v);
			position = v;
			printf("\nv:%x p:%x", v, position);
		} 
		else 
		if (kp == APP_HELP) {
			printHelp();
		} 
		else 
		if (kp == APP_HIRES) {

			e_mode = 1;

			hires();

			// memcpy((unsigned char*)0xa000, OverlayLabel, 8000);

 			// spamit();
			// randcolorgen();
			// randcogtab();

			gen_rnd_colors();

			printf("pos: mode:%x\n", position, e_mode);
			loadTable(position);
			hexDump((unsigned char *) position, 16);

			dumpPitch (CHAN_A);
			dumpPitch (CHAN_B);
			dumpPitch (CHAN_C);

		} 
		else 
		if (kp == APP_MEMHAK) {
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
		}


		if (shouldPlay) {
//			Synth();
			unsigned char H;
			unsigned char L;
			L = position << 8;
			H = position & 0xFF00;
			//printf("\nZP:%x %x %x\n", position, H, L);
			SynthZP(H, L);

			shouldPlay = 0;
		}

	}

	text();

//  loadTable ((unsigned int) &BootSound);
//  Synth ();
}
