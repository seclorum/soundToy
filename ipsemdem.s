; OSDK-compatible 6502 assembly code for Oric-1 HIRES character plotting
; Function name: ipsem_demo
; Callable from C, no zero-page usage, no .org directive
; Variables stored at $0400 and above

;   .export _ipsem_demo  ; Export symbol for C linkage (OSDK prepends underscore)

; Constants
CHARSET     = $9800      ; Base address of character set
TEMPCHAR    = $9F38      ; Temporary character address ($9800 + 127*8 = $9F38)
SCREEN_BASE = $A000      ; HIRES screen base address

; Variables (non-zero page, starting at $0400)
VARS_BASE:
X_POS:      .dsb 1       ; X coordinate (1 byte)
Y_POS:      .dsb 1       ; Y coordinate (1 byte)
CHAR_ASCII: .dsb 1       ; Current ASCII character (1 byte)
STR_PTR:    .dsb 2       ; String pointer (2 bytes)
N_LOOP:     .dsb 1       ; Outer loop counter (N)
I_LOOP:     .dsb 1       ; Inner loop counter (I)
A1:         .dsb 2       ; Character data address (2 bytes)
A2:         .dsb 2       ; Temp character address (2 bytes)
Z_LOOP:     .dsb 1       ; Inner loop for character data copy (Z)
MULT_TMP1:  .dsb 1       ; Temp for multiplication (A)
MULT_TMP2:  .dsb 1       ; Temp for multiplication (X)
TEMP_ADDR:  .dsb 2       ; Temporary address for absolute addressing (2 bytes)

; Function: ipsem_demo
; Entry point for C call
_ipsem_demo:
    ; Save registers (C expects A, X, Y to be preserved)
    pha
    txa
    pha
    tya
    pha

    ; Initialize HIRES mode
    jsr $EC33            ; ROM routine to switch to HIRES mode ($EC33 for Oric-1/Atmos)

    ; Initialize outer loop (N=1 to 6)
    lda #1
    sta N_LOOP

outer_loop:
    ; Check if N <= 6
    lda N_LOOP
    cmp #7
    bcs end_program      ; If N >= 7, exit

    ; Calculate X = 20 + N
    lda N_LOOP
    clc
    adc #20
    sta X_POS

    ; Calculate Y = 20 + N*20
    lda N_LOOP
    ldx #20
    jsr multiply         ; Multiply N by 20 (result in A)
    clc
    adc #20              ; Add base offset of 20
    sta Y_POS

    ; Initialize inner loop (I=1 to LEN(A$))
    lda #0
    sta I_LOOP
    lda #<string_data    ; Set string pointer
    sta STR_PTR
    lda #>string_data
    sta STR_PTR+1

inner_loop:
    ; Check string length (null-terminated string)
    ; Compute STR_PTR + I_LOOP
    lda STR_PTR
    clc
    adc I_LOOP
    sta TEMP_ADDR
    lda STR_PTR+1
    adc #0
    sta TEMP_ADDR+1
    lda TEMP_ADDR        ; Read character from string (absolute addressing)
    beq next_outer       ; If null, end of string, go to next N

    ; Get ASCII character
    sta CHAR_ASCII

    ; Call print_char subroutine
    jsr print_char

    ; Increment X = X + 6
    lda X_POS
    clc
    adc #6
    sta X_POS

    ; Increment I
    inc I_LOOP
    jmp inner_loop

next_outer:
    ; Increment N
    inc N_LOOP
    jmp outer_loop

end_program:
    ; Restore registers
    pla
    tay
    pla
    tax
    pla
    rts                  ; Return to C caller

; Subroutine to print character (equivalent to lines 1000-1060)
print_char:
    ; Save registers
    pha
    txa
    pha
    tya
    pha

    ; Calculate A1 = $9800 + C*8
    lda CHAR_ASCII
    asl                  ; Multiply by 8 (shift left 3 times)
    asl
    asl
    sta A1
    lda #0
    rol                  ; Handle carry from multiplication
    sta A1+1
    lda A1
    clc
    adc #<CHARSET
    sta A1
    lda A1+1
    adc #>CHARSET
    sta A1+1

    ; Set A2 = TEMPCHAR ($9F38)
    lda #<TEMPCHAR
    sta A2
    lda #>TEMPCHAR
    sta A2+1

    ; First half: Copy 4 bytes
    lda #0
    sta Z_LOOP
copy_loop1:
    lda Z_LOOP
    cmp #4
    bcs end_copy1
    ; Compute A1 + Z_LOOP
    lda A1
    clc
    adc Z_LOOP
    sta TEMP_ADDR
    lda A1+1
    adc #0
    sta TEMP_ADDR+1
    lda TEMP_ADDR        ; Read from character set
    pha
    ; Compute A2 + Z_LOOP
    lda A2
    clc
    adc Z_LOOP
    sta TEMP_ADDR
    lda A2+1
    adc #0
    sta TEMP_ADDR+1
    pla
    sta TEMP_ADDR        ; Write to temp character
    inc Z_LOOP
    jmp copy_loop1
end_copy1:
    ; CURSET X, Y, 0
    lda X_POS
    ldx Y_POS
    ldy #0               ; Mode 0 (clear)
    jsr $D9A8            ; ROM CURSET routine ($D9A8)

    ; CHAR 127, 0, 1
    lda #127             ; ASCII 127 (temp character)
    ldx #0               ; Rotation (0)
    ldy #1               ; Mode 1 (draw)
    jsr $D9B5            ; ROM CHAR routine ($D9B5)

    ; A1 = A1 + 4
    lda A1
    clc
    adc #4
    sta A1
    lda A1+1
    adc #0
    sta A1+1

    ; Second half: Copy next 4 bytes
    lda #0
    sta Z_LOOP
copy_loop2:
    lda Z_LOOP
    cmp #4
    bcs end_copy2
    ; Compute A1 + Z_LOOP
    lda A1
    clc
    adc Z_LOOP
    sta TEMP_ADDR
    lda A1+1
    adc #0
    sta TEMP_ADDR+1
    lda TEMP_ADDR        ; Read from character set
    pha
    ; Compute A2 + Z_LOOP
    lda A2
    clc
    adc Z_LOOP
    sta TEMP_ADDR
    lda A2+1
    adc #0
    sta TEMP_ADDR+1
    pla
    sta TEMP_ADDR        ; Write to temp character
    inc Z_LOOP
    jmp copy_loop2
end_copy2:
    ; CURSET X, Y+8, 0
    lda X_POS
    ldx Y_POS
    clc
    adc #8               ; Y+8
    tax
    ldy #0               ; Mode 0 (clear)
    jsr $D9A8            ; ROM CURSET routine

    ; CHAR 127, 0, 1
    lda #127             ; ASCII 127 (temp character)
    ldx #0               ; Rotation (0)
    ldy #1               ; Mode 1 (draw)
    jsr $D9B5            ; ROM CHAR routine

    ; Restore registers
    pla
    tay
    pla
    tax
    pla
    rts

; Subroutine: Multiply A by X, result in A
multiply:
    sta MULT_TMP1        ; Store A
    stx MULT_TMP2        ; Store X
    lda #0
    ldy MULT_TMP2
mult_loop:
    cpy #0
    beq mult_end
    clc
    adc MULT_TMP1
    dey
    jmp mult_loop
mult_end:
    rts

; Data section
string_data:
    .byte "Lorem ipsum dolor sit amet", 0

    .end
