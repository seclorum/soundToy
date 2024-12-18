

### soundtoy - a simple toy for exploring the Oric-1/Atmos synthesizer capabilities

The Oric-1/Atmos computers (released: 1983) were a wonderful 8-bit series of computers which included some nice synthesizer capabilitiies in the form of the  AY-3-8912 sound chip.

soundtoy gives you a way of experimenting with the synthesizer of your Oric-1/Atmos.  

It does this by giving you a means of loading the address of 14 registers, and then calling the onboard Oric "SPLAY - Sound Play"  (#FA86) function, to be played.

From the ORIC ROM Disassembly, comments for SPLAY (#FA86):

 "This routine takes X and Y as the low and high halves of the start address of a table to send data to the sound chip from. 14 bytes are sent to the 8912 starting with register 0 and  working up in order until register D. The data from the table is used starting from the low address..."

So, to use this in BASIC it would be nice to just put the address of 14 bytes of DATA into #00(LO)/#01(HI) with POKE, and then CALL #B000, which will prepare X and Y for #FA86 .. so we need a small 'loader' for the X/Y registers

We use some assembly to set up the register address for the call to SPLAY (#FA86)

```
	            .org $B000          ; Example start address in RAM
	SOUNDROUTINE:
	            LDX $00             ; Load low byte of sound data address from zero-page ($00)
	            LDY $01             ; Load high byte of sound data address from zero-page ($01)
	            JSR $FA86           ; Jump to the sound routine in ROM (#FA86)
	            RTS                 ; Return to BASIC
```

An example, in BASIC, of using the ORIC SPLAY command (#FA86) to play sound effects, which includes the above 'loader' code:


```
	0 ?"SOUNDHAK:"
	1 ?"e.g. POKE#00,#LO:POKE#01,#HI:CALL#B000"
	2 FORI=#B000TO#B009:READA:POKEI,A:NEXT:REM $B000-$B009
	3 DATA #AE,#00,#00,#AC,#01,#00,#20,#86,#FA,#60
	4 REM PING EXPLODE ZAP..
	6 POKE#00,#A7:POKE#01,#FA:CALL#B000:WAIT65
	8 POKE#00,#B0:POKE#01,#FA:CALL#B000:WAIT85
	10 POKE#00,#C0:POKE#01,#FA:CALL#B000:WAIT75
	11 REM ..AND FROM ROM: FAA7,FABD,FAD3,FB10..
	12 POKE#00,#AA:POKE#01,#FA:CALL#B000:WAIT100
	13 POKE#00,#BD:POKE#01,#FA:CALL#B000:WAIT20
	14 POKE#00,#D3:POKE#01,#FA:CALL#B000:WAIT40
	15 POKE#00,#10:POKE#01,#FB:CALL#B000:WAIT80
	16 POKE#00,#BD:POKE#01,#FA:CALL#B000:WAIT10
	17 POKE#00,#75:POKE#01,#FB:CALL#B000:WAIT20
	18 POKE#00,#10:POKE#01,#FB:CALL#B000:WAIT24
	19 CALL#B000
	20 GOTO 12
```


soundToy can be used to discover the addresses of cool sounds, and these can then be used in BASIC for a wide array of purposes .. sound effects, ui prompts, annoying alert sounds, maybe even some songs could be written ..


V0.0.1	- Basic first version - use to explore the Oric ROM data to use as
		  seeds to the 8192 synthesizer registers.  Use 'j' and 'k' keys to step
		  through ROM addresses to listen to the various 'presets' available in
		  your Oric - use the '?' key to get help and some example addresses to
		  try out.

V0.0.2	- Added HIRES video synth mode.  TODO: connect both synthesizers in pleasing, absttract ways.

V0.0.3  - Exploring the RANDOM nature of things.  The colour-bar that is generated in
HIRES ("G") mode of soundtoy, represents a playground of colourful 'sprites' which have been pre-filtered to ensure no attribute colour-clash occurs when they are placed adjacent to each other .. At this stage, I am exploring/hacking around, and things don't quite work properly yet, but the controls are there to drive into interesting territory.


Lets hear of your discoveries!  :)

--
seclorum

![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-11-27%20at%2013.22.33.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-02%20at%2017.21.29.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-02%20at%2023.18.07.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-06%20at%2018.41.44.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-06%20at%2019.24.09.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-11%20at%2013.42.30.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-11%20at%2013.42.43.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-11%20at%2013.43.04.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-11%20at%2013.44.12.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-11%20at%2014.34.30.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-16%20at%2022.16.47.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-16%20at%2022.19.30.png)
![](https://github.com/seclorum/soundToy/blob/main/screenshots/Screenshot%202024-10-16%20at%2022.34.59.png)
