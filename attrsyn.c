
// Approach 1: Simple Masking and Bit Shifting
void map_to_synth1(unsigned char random_byte, unsigned char* synth_regs) {
    synth_regs[0] = random_byte & 0x0F;  // Low 4 bits for frequency
    synth_regs[1] = (random_byte >> 4) & 0x0F;  // High 4 bits for amplitude
    synth_regs[2] = (random_byte & 0xC0) >> 6;  // Use top 2 bits for noise
}

// Approach 2: Byte Division for Frequency and Volume
void map_to_synth2(unsigned char random_byte, unsigned char* synth_regs) {
    synth_regs[0] = random_byte % 16;  // Frequency (0-15)
    synth_regs[1] = (random_byte / 16) % 16;  // Volume (0-15)
    synth_regs[2] = random_byte % 3;  // Noise (0-2)
}

// Approach 3: XOR and Shift for Synth Registers
void map_to_synth3(unsigned char random_byte, unsigned char* synth_regs) {
    synth_regs[0] = random_byte ^ 0xAA;  // XOR with pattern
    synth_regs[1] = (random_byte << 2) & 0xFF;  // Shift left for frequency
    synth_regs[2] = (random_byte >> 3) & 0x1F;  // Shift right for volume
}


// Approach 4: Lookup Table Mapping
unsigned char lookup_table[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

void map_to_synth4(unsigned char random_byte, unsigned char* synth_regs) {
    synth_regs[0] = lookup_table[random_byte & 0x0F];  // Frequency from lower nibble
    synth_regs[1] = lookup_table[(random_byte >> 4) & 0x0F];  // Volume from upper nibble
    synth_regs[2] = lookup_table[(random_byte & 0xC0) >> 6];  // Noise from top 2 bits
}



// Dataset for Synth Bass on Channel 1, Pad/Lead on Channel 2, and Noise Rhythm on Channel 3

// Register layout for AY-3-8912
// Channel 1: Registers 0,1 for frequency, 2 for volume
// Channel 2: Registers 3,4 for frequency, 5 for volume
// Channel 3: Registers 6,7 for frequency, 8 for volume
// Noise: Register 9 for noise frequency, Register 10 for mixer control


void sample__syn_mapping() {

unsigned char _synth_regs[14];  // Synthesizer registers

unsigned char *synth_regs = &_synth_regs[0];

// Bass on Channel 1 (low frequency, high volume)
map_to_synth1(0x15, synth_regs);  // Frequency for bass
synth_regs[0] = 0x0C;  // Bass frequency low
synth_regs[1] = 0x02;  // Bass frequency high
synth_regs[2] = 0x0F;  // Full volume, no envelope

// Pad/Lead on Channel 2 (medium frequency, mid volume, slight vibrato)
map_to_synth1(0x73, synth_regs);  // Frequency for lead
synth_regs[3] = 0x40;  // Lead frequency low
synth_regs[4] = 0x03;  // Lead frequency high
synth_regs[5] = 0x08;  // Medium volume

// Noise rhythm on Channel 3 (noise frequency, rhythm pattern)
map_to_synth1(0xA5, synth_regs);  // Frequency for noise
synth_regs[6] = 0xFF;  // Noise-based frequency low
synth_regs[7] = 0x00;  // No tone frequency (pure noise)
synth_regs[8] = 0x0C;  // Noise volume

// Mixer control: Enable tone on Channel 1 and 2, noise on Channel 3
synth_regs[10] = 0x38;  // Enable noise on Channel 3, tone on Channel 1 & 2

// Noise frequency control
synth_regs[9] = 0x1F;  // Set noise frequency (fast rhythm)


}
