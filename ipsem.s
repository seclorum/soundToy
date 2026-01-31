; OSDK-compatible 6502 assembly code for Oric-1 HIRES character plotting
; Converted from provided BASIC code

    .org $0600       ; Place code in a safe memory area

; Constants
CHARSET     = $9800  ; Base address of character set
TEMPCHAR    = $9800 + 127*8 ; Temporary character address (ASCII 127)
SCREEN_BASE = $A000  ; HIRES screen base address

; Zero-page variables
X_POS       = $00    ; X coordinate (1 byte)
Y_POS       = $01    ; Y coordinate (1 byte)
CHAR_ASCII  = $02    ; Current ASCII character (1 byte)
STR_PTR     = $03    ; String pointer (2 bytes, low byte)
STR_PTR_H   = $04    ; String pointer high byte
N_LOOP      = $05    ; Outer loop counter (N)
I_LOOP      = $06    ; Inner loop counter (I)
A1          = $07    ; Character data address (2 bytes, low byte)
A1_H        = $08    ; Character data address high byte
A2          = $09    ; Temp character address (2 bytes, low byte)
A2_H        = $0A    ; Temp character address high byte
Z_LOOP      = $0B    ; Inner loop for character data copy (Z)

; Main program
start:
    ; Initialize HIRES mode
    jsr $EC33        ; ROM routine to switch to HIRES mode ($EC33 for Oric-1/Atmos)

    ; Initialize outer loop (N=1 to 6)
    lda #1
    sta N_LOOP

outer_loop:
    ; Check if N <= 6
    lda N_LOOP
    cmp #7
    bcs end_program  ; If N >= 7, exit

    ; Calculate X = 20 + N
    lda N_LOOP
    clc
    adc #20
    sta X_POS

    ; Calculate Y = 20 + N*20
    lda N_LOOP
    asl              ; Multiply by 2
    asl              ; Multiply by 4
    asl              ; Multiply by 8
    asl              ; Multiply by 16
    clc
    adc N_LOOP       ; Add N (effectively N*16 + N = N*17)
    asl              ; Multiply by 2 (N*34)
    asl              ; Multiply by 2 (N*68)
    sec
    sbc N_LOOP       ; Subtract N (N*68 - N = N*67)
    sec
    sbc N_LOOP       ; Subtract N (N*67 - N = N*66)
    sec
    sbc N_LOOP       ; Subtract N (N*66 - N = N*65)
    sec
    sbc N_LOOP       ; Subtract N (N*65 - N = N*64)
    sec
    sbc N_LOOP       ; Subtract N (N*64 - N = N*63)
    sec
    sbc N_LOOP       ; Subtract N (N*63 - N = N*62)
    sec
    sbc N_LOOP       ; Subtract N (N*62 - N = N*61)
    sec
    sbc N_LOOP       ; Subtract N (N*61 - N = N*60)
    sec
    sbc N_LOOP       ; Subtract N (N*60 - N = N*59)
    sec
    sbc N_LOOP       ; Subtract N (N*59 - N = N*58)
    sec
    sbc N_LOOP       ; Subtract N (N*58 - N = N*57)
    sec
    sbc N_LOOP       ; Subtract N (N*57 - N = N*56)
    sec
    sbc N_LOOP       ; Subtract N (N*56 - N = N*55)
    sec
    sbc N_LOOP       ; Subtract N (N*55 - N = N*54)
    sec
    sbc N_LOOP       ; Subtract N (N*54 - N = N*53)
    sec
    sbc N_LOOP       ; Subtract N (N*53 - N = N*52)
    sec
    sbc N_LOOP       ; Subtract N (N*52 - N = N*51)
    sec
    sbc N_LOOP       ; Subtract N (N*51 - N = N*50)
    sec
    sbc N_LOOP       ; Subtract N (N*50 - N = N*49)
    sec
    sbc N_LOOP       ; Subtract N (N*49 - N = N*48)
    sec
    sbc N_LOOP       ; Subtract N (N*48 - N = N*47)
    sec
    sbc N_LOOP       ; Subtract N (N*47 - N = N*46)
    sec
    sbc N_LOOP       ; Subtract N (N*46 - N = N*45)
    sec
    sbc N_LOOP       ; Subtract N (N*45 - N = N*44)
    sec
    sbc N_LOOP       ; Subtract N (N*44 - N = N*43)
    sec
    sbc N_LOOP       ; Subtract N (N*43 - N = N*42)
    sec
    sbc N_LOOP       ; Subtract N (N*42 - N = N*41)
    sec
    sbc N_LOOP       ; Subtract N (N*41 - N = N*40)
    clc
    adc #20          ; Add base offset of 20
    sta Y_POS

    ; Initialize inner loop (I=1 to LEN(A$))
    lda #0
    sta I_LOOP
    lda #<string_data ; Set string pointer
    sta STR_PTR
    lda #>string_data
    sta STR_PTR_H

inner_loop:
    ; Check string length (null-terminated string)
    ldy I_LOOP
    lda (STR_PTR),y
    beq next_outer   ; If null, end of string, go to next N

    ; Get ASCII character (C = ASC(MID$(A$,I,1)))
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
    rts              ; Return to caller

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
    asl              ; Multiply by 8 (shift left 3 times)
    asl
    asl
    sta A1
    lda #0
    rol              ; Handle carry from multiplication
    sta A1_H
    lda A1
    clc
    adc #<CHARSET
    sta A1
    lda A1_H
    adc #>CHARSET
    sta A1_H

    ; Set A2 = TEMPCHAR ($9800 + 127*8)
    lda #<TEMPCHAR
    sta A2
    lda #>TEMPCHAR
    sta A2_H

    ; First half: Copy 4 bytes (Z=0 to 3, but BASIC does Z/2)
    ldy #0
copy_loop1:
    cpy #4
    bcs end_copy1
    lda (A1),y       ; Read from character set
    sta (A2),y       ; Write to temp character
    iny
    jmp copy_loop1
end_copy1:
    ; CURSET X, Y, 0
    lda X_POS
    ldx Y_POS
    ldy #0           ; Mode 0 (clear)
    jsr $D9A8        ; ROM CURSET routine ($D9A8)

    ; CHAR 127, 0, 1
    lda #127         ; ASCII 127 (temp character)
    ldx #0           ; Rotation (0)
    ldy #1           ; Mode 1 (draw)
    jsr $D9B5        ; ROM CHAR routine ($D9B5)

    ; A1 = A1 + 4
    lda A1
    clc
    adc #4
    sta A1
    lda A1_H
    adc #0
    sta A1_H

    ; Second half: Copy next 4 bytes
    ldy #0
copy_loop2:
    cpy #4
    bcs end_copy2
    lda (A1),y       ; Read from character set
    sta (A2),y       ; Write to temp character
    iny
    jmp copy_loop2
end_copy2:
    ; CURSET X, Y+8, 0
    lda X_POS
    ldx Y_POS
    inx
    inx
    inx
    inx
    inx
    inx
    inx
    inx              ; Y+8
    ldy #0           ; Mode 0 (clear)
    jsr $D9A8        ; ROM CURSET routine

    ; CHAR 127, 0, 1
    lda #127         ; ASCII 127 (temp character)
    ldx #0           ; Rotation (0)
    ldy #1           ; Mode 1 (draw)
    jsr $D9B5        ; ROM CHAR routine

    ; Restore registers
    pla
    tay
    pla
    tax
    pla
    rts

; Data section
string_data:
    .byte "Lorem ipsum dolor sit amet", 0

    .end
