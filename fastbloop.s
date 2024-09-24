_fastbloop

    ; Pointers and working registers
    lda $276           ; Load seed for random function
    sta seed

start_loop:
    ldx #$00           ; X-register for indexing the table
    lda seed
    jsr _blooprand      ; Call your custom random generator
    and #$FF           ; Ensure result is within a byte range
    tax                ; Store random result in X for indexing

    lda random_table, x ; Get a valid random value from the table
    sta $0200, y       ; Poke the random value into memory
    iny                ; Increment Y to poke the next address

    cpy #$FF           ; Compare Y with the low byte of $3FFF
    bne start_loop     ; If Y is not $FF, continue
    lda $02FF          ; Check the high byte of the address
    cmp #$3F           ; Compare with high byte of $3FFF
    beq done           ; If we’ve reached the end, jump to done
    bcc start_loop     ; Otherwise, loop back

done:
    rts                ; Return from the subroutine

; -----------------------------------------------------
; Random number generator (_blooprand)
_blooprand:
    ldy #8
    lda $00
_blooprandBack:
    asl
    rol $FF
    bcc _bloopNoEor
    eor #$39
_bloopNoEor:
    dey
    bne _blooprandBack
    sta $00
    cmp #0
    rts

; -----------------------------------------------------
; Random number table (pre-filtered values)
; Excludes values where (r & 0x78) == 0x08, 0x18, 0x88, 0x98

random_table:
    .byte $00, $01, $02, $03, $04, $05, $06, $07
    .byte $10, $11, $12, $13, $14, $15, $16, $17
    .byte $20, $21, $22, $23, $24, $25, $26, $27
    .byte $30, $31, $32, $33, $34, $35, $36, $37
    .byte $40, $41, $42, $43, $44, $45, $46, $47
    .byte $50, $51, $52, $53, $54, $55, $56, $57
    .byte $60, $61, $62, $63, $64, $65, $66, $67
    .byte $70, $71, $72, $73, $74, $75, $76, $77
    .byte $80, $81, $82, $83, $84, $85, $86, $87
    .byte $A0, $A1, $A2, $A3, $A4, $A5, $A6, $A7
    .byte $B0, $B1, $B2, $B3, $B4, $B5, $B6, $B7
    .byte $C0, $C1, $C2, $C3, $C4, $C5, $C6, $C7
    .byte $D0, $D1, $D2, $D3, $D4, $D5, $D6, $D7
    .byte $E0, $E1, $E2, $E3, $E4, $E5, $E6, $E7
    .byte $F0, $F1, $F2, $F3, $F4, $F5, $F6, $F7

    ; Seed for random number generation
seed: .byte $00
