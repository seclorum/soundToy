

### soundtoy - a simple toy for exploring the Oric-1/Atmos synthesizer capabilities

The Oric-1/Atmos computers (released: 1983) were a wonderful 8-bit series of computers which included some nice synthesizer capabilitiies in the form of the  AY-3-8912 sound chip.

soundtoy gives you a way of experimenting with the synthesizer of your Oric-1/Atmos.  

It does this by giving you a means of loading the address of 14 registers, and then calling the onboard Oric "SPLAY - Sound Play"  (#FA86) function, to be played.

From the ORIC ROM Disassembly, comments for SPLAY (#FA86):

 "This routine takes X and Y as the low and high halves of the start address of a table to send data to the sound chip from. 14 bytes are sent to the 8912 starting with register 0 and  working up in order until register D. The data from the table is used starting from the low address..."

So, to use this in BASIC it would be nice to just put the address of 14 bytes of DATA into #00(LO)/#01(HI) with POKE, and then CALL #BFE0, which will prepare X and Y for #FA86 .. so we need a small 'loader' for the X/Y registers

We use some assembly to set up the register address for the call to SPLAY (#FA86)

```
	            .org $BFE0          ; Example start address in RAM
	SOUNDROUTINE:
	            LDX $00             ; Load low byte of sound data address from zero-page ($00)
	            LDY $01             ; Load high byte of sound data address from zero-page ($01)
	            JSR $FA86           ; Jump to the sound routine in ROM (#FA86)
	            RTS                 ; Return to BASIC
```

An example, in BASIC, of using the ORIC SPLAY command (#FA86) to play sound effects, which includes the above 'loader' code:


```
	0 ?"SOUNDHAK:"
	1 ?"e.g. POKE#00,#LO:POKE#01,#HI:CALL#BFE0"
	2 FORI=#BFE0TO#BFE9:READA:POKEI,A:NEXT:REM $BFE0-$BFE9
	3 DATA #AE,#00,#00,#AC,#01,#00,#20,#86,#FA,#60
	4 REM PING EXPLODE ZAP..
	6 POKE#00,#A7:POKE#01,#FA:CALL#BFE0:WAIT65
	8 POKE#00,#B0:POKE#01,#FA:CALL#BFE0:WAIT85
	10 POKE#00,#C0:POKE#01,#FA:CALL#BFE0:WAIT75
	11 REM ..AND FROM ROM: FAA7,FABD,FAD3,FB10..
	12 POKE#00,#AA:POKE#01,#FA:CALL#BFE0:WAIT100
	13 POKE#00,#BD:POKE#01,#FA:CALL#BFE0:WAIT20
	14 POKE#00,#D3:POKE#01,#FA:CALL#BFE0:WAIT40
	15 POKE#00,#10:POKE#01,#FB:CALL#BFE0:WAIT80
	16 POKE#00,#BD:POKE#01,#FA:CALL#BFE0:WAIT10
	17 POKE#00,#75:POKE#01,#FB:CALL#BFE0:WAIT20
	18 POKE#00,#10:POKE#01,#FB:CALL#BFE0:WAIT24
	19 CALL#BFE0
	20 GOTO 12
```


A naive implementation of a "!SYNTH" command which does all of the above:
```
10 REM INSTALL !SYNTH COMMAND
20 REM MEMORY LOCATION FOR CODE
30 CODEBASE = 49152 : REM $C000
40 HOOK = 757 : REM $02F5
50 REM WRITE _currentSound DATA
60 DATA 238,2,0,0,0,0,0,62,16,0,0,208,7,0,0,0
70 FOR I=0 TO 15
80 READ B : POKE CODEBASE + I, B
90 NEXT I
100 REM WRITE _BootSound DATA
110 DATA 238,2,0,0,0,0,0,62,16,0,0,208,7,0,0,0
120 FOR I=0 TO 15
130 READ B : POKE CODEBASE + 16 + I, B
140 NEXT I
150 REM WRITE _Synth ROUTINE
160 DATA 162,0,160,0,32,134,250,96
170 FOR I=0 TO 7
180 READ B : POKE CODEBASE + 32 + I, B
190 NEXT I
200 REM INSTALL HOOK
210 DATA 162,0,189,0,192,32,96
220 FOR I=0 TO 6
230 READ B : POKE HOOK + I, B
240 NEXT I
250 PRINT "READY FOR !SYNTH"

```


soundToy can be used to discover the addresses of cool sounds, and these can then be used in BASIC for a wide array of purposes .. sound effects, ui prompts, annoying alert sounds, maybe even some songs could be written ..


Note that soundToy also has a hack/playground for the HIRES mode, where strange things are happening - eventually the sound exploration and HIRES colour synthesis aspects of soundToy will be integrated somehow, to make it an explosion of colour and sound, but for now things aren't quite wired up while the fun details are being played with ..


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
