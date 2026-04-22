; zpdump.s

.zero         ; Switch to zero-page memory
*= $50        ; Start allocating variables at $50

CURSOR_X   .byte 0  ; Cursor X position
CURSOR_Y   .byte 0  ; Cursor Y position
INPUT_X    .byte 0  ; Input X coordinate
INPUT_Y    .byte 0  ; Input Y coordinate
OUTPUT_X   .byte 0  ; Output X coordinate
OUTPUT_Y   .byte 0  ; Output Y coordinate

.text        ; Switch back to normal memory for code

    ; Initialize variables
    lda #10
    sta CURSOR_X
    lda #5
    sta CURSOR_Y

    lda #20
    sta INPUT_X
    lda #15
    sta INPUT_Y

    lda INPUT_X
    sta OUTPUT_X
    lda INPUT_Y
    sta OUTPUT_Y

    ; Call dump function
    jsr DUMP_VARS

    rts  ; Return from subroutine

;-------------------------------------
; DUMP_VARS - Prints the variables
;-------------------------------------
DUMP_VARS:
    ldx #0      ; X register used for indexing

    ; Print "Cursor: "
    lda #<MSG_CURSOR
    ldy #>MSG_CURSOR
    jsr PRINT_STRING

    ; Print Cursor X
    lda CURSOR_X
    jsr PRINT_DECIMAL

    lda #' '
    jsr PRINT_CHAR

    ; Print Cursor Y
    lda CURSOR_Y
    jsr PRINT_DECIMAL

    jsr NEWLINE

    ; Print "Input: "
    lda #<MSG_INPUT
    ldy #>MSG_INPUT
    jsr PRINT_STRING

    ; Print Input X
    lda INPUT_X
    jsr PRINT_DECIMAL

    lda #' '
    jsr PRINT_CHAR

    ; Print Input Y
    lda INPUT_Y
    jsr PRINT_DECIMAL

    jsr NEWLINE

    ; Print "Output: "
    lda #<MSG_OUTPUT
    ldy #>MSG_OUTPUT
    jsr PRINT_STRING

    ; Print Output X
    lda OUTPUT_X
    jsr PRINT_DECIMAL

    lda #' '
    jsr PRINT_CHAR

    ; Print Output Y
    lda OUTPUT_Y
    jsr PRINT_DECIMAL

    jsr NEWLINE

    rts

;-------------------------------------
; PRINT_STRING - Prints a string using ROM routine
;-------------------------------------
PRINT_STRING:
    sta $A000   ; Store low byte of address
    sty $A001   ; Store high byte of address
    ldy #0      ; Y = 0 (string index)

@loop:
    lda ($A000),y  ; Load character
    beq @done      ; If zero (end of string), exit
    jsr PRINT_CHAR ; Print the character
    iny            ; Next character
    jmp @loop      ; Repeat

@done:
    rts

;-------------------------------------
; PRINT_DECIMAL - Prints a number (0-99)
;-------------------------------------
PRINT_DECIMAL:
    pha            ; Save original number
    ldx #0        ; X = Tens digit

@loop_tens:
    sec
    sbc #10       ; Subtract 10
    bcc @print_units ; If carry clear, we have our tens digit
    inx           ; Increment tens counter
    jmp @loop_tens

@print_units:
    adc #10       ; Restore last valid value
    ora #$30      ; Convert to ASCII
    jsr PRINT_CHAR ; Print Tens

    txa
    ora #$30      ; Convert to ASCII
    jsr PRINT_CHAR ; Print Units

    pla           ; Restore original A
    rts

;-------------------------------------
; PRINT_CHAR - Prints a character using Oric ROM
;-------------------------------------
PRINT_CHAR:
    jsr $BB5A   ; Oric ROM print character routine
    rts

;-------------------------------------
; NEWLINE - Moves to the next line
;-------------------------------------
NEWLINE:
    lda #13
    jsr PRINT_CHAR
    rts

;-------------------------------------
; Strings
;-------------------------------------
MSG_CURSOR: .byte "Cursor: ",0
MSG_INPUT:  .byte "Input: ",0
MSG_OUTPUT: .byte "Output: ",0
