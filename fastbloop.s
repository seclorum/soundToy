; qrandom.s - experiment with different ways to generate 'valid' colour values randomly using a lookup table

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
seed: .byte $42
_fastbloop:

    lda $276           ; Load seed for random function
    sta seed

    lda #$02           ; Initialize high byte for storing in page $0200
    sta addr_high      ; Store in high byte variable
    lda #$00           ; Initialize low byte
    sta addr_low

    lda #$00           ; Initialize iteration counter
    sta iteration_count

start_loop:
    ldx #$00           ; X-register for indexing the table
    lda seed
    jsr _randgen        ; _qrandomJ      ; Call random generator
    tax                ; Store random result in X for indexing

    lda random_table, x ; Get a random value from the table

    ; Calculate the address to store the value
    lda addr_high       ; Load the high byte of the address
    sta temp_high       ; Store in temporary variable
    lda addr_low        ; Load the low byte of the address
    sta temp_low        ; Store in temporary variable

    ; Store the value at the address (absolute indexed)
    sta temp_store      ; Save the random value temporarily
    ldy #$00            ; Clear Y for addressing calculation

    ; Form full address manually
    lda temp_low        ; Load low byte of address
    sta $00             ; Save it to memory
    lda temp_high       ; Load high byte of address
    sta $01             ; Save it to memory

    ; Store value using absolute indexed addressing
    lda temp_store
    sta ($00), y        ; Store value at the address

    ; Increment the low byte of the address
    inc addr_low        
    lda addr_low        ; Check if low byte has overflowed
    bne continue_loop   ; If not overflowed, continue

    inc addr_high       ; Increment high byte if low byte overflowed

continue_loop:
    ; Increment the iteration counter
    inc iteration_count
    lda iteration_count
    cmp #$10            ; Set the loop to run for 16 iterations (or another value)
    beq done            ; If iterations reached the limit, exit the loop

    jmp start_loop      ; Otherwise, loop back

done:
    lda random_table, x  ; Return the last random value in A register
    rts                 ; Return to C with value in A


; Temporary variables for address calculations
temp_high:     .byte $00
temp_low:      .byte $00
temp_store:    .byte $00
addr_low:      .byte $00  ; High and low byte variables
addr_high:     .byte $00
iteration_count: .byte $00 ; Loop counter

