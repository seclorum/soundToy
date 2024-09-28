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

