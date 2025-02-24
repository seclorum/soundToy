; qrandom.s - experiment with different ways to generate 'valid' colour values randomly
_qrandomJ
		ldy #8
		lda $0
_qrback
		asl
		rol $FF
		bcc _qrnoEor
		eor #$39
_qrnoEor
		dey
		bne _qrback
;		jmp return_qrandomJ

_qrCont
	    cmp #24
    	bcc _qrnd_next		   	; If number is below 8, it's valid
    	cmp #28
    	bcc _qrandomJ      		; If number below 28, regen

_qrnd_next:
    	cmp #152
    	bcc return_qrandomJ
    	cmp #156
    	bcc _qrandomJ

return_qrandomJ:
		sta $0
		cmp #0
		rts

_lfsr
   .byt 20,42        ; 16-bit LFSR storage (2 bytes)

; Seed the LFSR with an initial value
; Input: A = low byte, X = high byte (seed values)
_seed_lfsr:
    sta _lfsr          ; Store A in the low byte of LFSR
    stx _lfsr+1        ; Store X in the high byte of LFSR
    rts               ; Return

; LFSR random number generator
; Output: A = random number between 0–255, excluding 8-18 and 88-98
_lfsr_random:
    ; Load the current LFSR value
    lda _lfsr
    ldx _lfsr+1

    ; Perform the LFSR shift and feedback (16-bit LFSR with polynomial)
    lda _lfsr+1        ; Load high byte
    lsr               ; Shift right (low bit goes into carry)
    ror _lfsr          ; Rotate carry into low byte
    lda _lfsr+1        ; Reload high byte
    ror               ; Rotate carry into high byte
    lda _lfsr          ; Load low byte (A)
    eor #$B4          ; XOR feedback with a chosen polynomial value
    sta _lfsr          ; Store back into LFSR low byte

    ; Get the new random number (low byte of LFSR)
    lda _lfsr

    ; Check for exclusion range 8-18 (0x08–0x12)
check_exclusion:
    cmp #8
    bcc return_random   ; If number is below 8, it's valid
    cmp #19
    bcs check_next      ; If number is above 18, go to next check

    ; If we are here, the number is in the excluded range 8-18, generate again
    jmp _lfsr_random

check_next:
    ; Check for exclusion range 88-98 (0x58–0x62)
    cmp #88
    bcc return_random   ; If number is below 88, it's valid
    cmp #99
    bcs return_random   ; If number is above 98, it's valid

    ; If we are here, the number is in the excluded range 88-98, generate again
    jmp _lfsr_random

return_random:
    rts               ; Return with the valid random number in A

; Example usage:
; 1. Call 'seed_lfsr' to initialize the LFSR.
; 2. Call '_lfsr_random' to obtain random numbers, excluding 8-18 and 88-98.


// Chema's RNG from https://forum.defence-force.org/viewtopic.php?p=19108&hilit=random#p19108
; A real random generator... 
randseed .word $dead 	; will it be $dead again? 

_randgen
.(
   lda randseed     	; get old lsb of seed. 
   ora $308		; lsb of VIA T2L-L/T2C-L. 
   rol			; this is even, but the carry fixes this. 
   adc $304		; lsb of VIA TK-L/T1C-L.  This is taken mod 256. 
   sta randseed     	; random enough yet. 
   sbc randseed+1   	; minus the hsb of seed... 
   rol			; same comment than before.  Carry is fairly random. 
   sta randseed+1   	; we are set. 

    cmp #8
    bcc return_randgen   ; If number is below 8, it's valid
    cmp #19
    bcs check_nxt      ; If number is above 18, go to next check

    ; If we are here, the number is in the excluded range 8-18, generate again
    jmp _randgen

check_nxt:
    ; Check for exclusion range 88-98 (0x58–0x62)
    cmp #88
    bcc return_randgen   ; If number is below 88, it's valid
    cmp #99
    bcs return_randgen   ; If number is above 98, it's valid

    ; If we are here, the number generated is in the excluded range, so regenerate 
    jmp _randgen

return_randgen:
   rts			; see you later alligator. 
.)



// Chema's RNG from https://forum.defence-force.org/viewtopic.php?p=19108&hilit=random#p19108
; A real random generator... 
_randgenJ
.(
   lda randseed     	; get old lsb of seed. 
   ora $308		; lsb of VIA T2L-L/T2C-L. 
   rol			; this is even, but the carry fixes this. 
   adc $304		; lsb of VIA TK-L/T1C-L.  This is taken mod 256. 
   sta randseed     	; random enough yet. 
   sbc randseed+1   	; minus the hsb of seed... 
   rol			; same comment than before.  Carry is fairly random. 
   sta randseed+1   	; we are set. 

    cmp #24
    bcc check_nxtJ		   	; If number is below 8, it's valid
    cmp #28
    bcc _randgenJ      		; If number below 28, regen
check_nxtJ:
    cmp #152
    bcc return_randgenJ
    cmp #156
    bcc _randgenJ

return_randgenJ:
   rts			; see you later alligator. 
.)


;_HIGH .byte #2b
;_LOW .byte #42        ; 8-bit value (1 byte)
;_VALUE .byte #30
;
;
;_cmpRange
;.(  
;    ldx #VALUE      ; Load the value into X register
;    cpx #LOW       ; Compare X with lower bound
;    bcc set_zero   ; Branch if X < LOW (out of range low)
;    cpx #HIGH+1    ; Compare X with (HIGH+1)
;    bcs set_zero   ; Branch if X > HIGH (out of range high)
;    jmp done       ; Value is in range, keep it
;
;set_zero:
;    ldx #0         ; Set X to 0
;
;done:
;    stx #VALUE      ; Store result back into memory
;	rts
;.)

