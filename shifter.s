; =============================================
; Arbitrary bit shift for 7-byte blob on Oric HIRES
; Entry: A = shift amount (positive = left, negative = right)
;        Blob at fixed address BLOB (e.g. $A000)
; Uses zero-page for temp if needed, but here we keep it simple and fast.
; =============================================

BLOB    = $A000     ; Top-left 7 bytes on HIRES screen

; Zero-page temps (safe in most OSDK setups; adjust if needed)
TMP     = $80       ; 1 byte temp
COUNT   = $81       ; Shift counter

; Main entry point (callable from C as _shift_blob)
_shift_blob:
        sta COUNT       ; Save shift amount
        bpl left_shift  ; Positive or zero -> left

        ; Right shift (make positive)
        eor #$FF
        clc
        adc #1          ; COUNT = -original (now positive right shifts)
        sta COUNT

right_shift_loop:
        ldx #6          ; Start from LSB (byte 6)
        clc
rshift_byte:
        lda BLOB,x
        ror             ; Shift right, carry in from previous
        sta BLOB,x
        dex
        bpl rshift_byte
        dec COUNT
        bne right_shift_loop
        rts

left_shift:
        beq done        ; Shift 0 = nothing

left_shift_loop:
        ldx #0          ; Start from MSB (byte 0)
        clc
lshift_byte:
        lda BLOB,x
        rol             ; Shift left, carry out to next
        sta BLOB,x
        inx
        cpx #7
        bcc lshift_byte
        dec COUNT
        bne left_shift_loop

done:
        rts

; Optional: Fill blob with random data (for testing)
; Call once at start
_random_fill:
        ldx #6
rand_loop:
        jsr $EA5E       ; Oric ROM random (or use your own)
        sta BLOB,x
        dex
        bpl rand_loop
        rts
