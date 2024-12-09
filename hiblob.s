; HIRES screen base address
HIRES_BASE   = $A000
LINE_LENGTH  = 40        ; Bytes per line in HIRES mode
NUM_LINES    = 50        ; Number of lines to scroll

; Zero-page pointers
SourcePtr    = $00
DestPtr      = $02

_hiblob:
    ; Set source pointer to the start of the screen
    lda #<HIRES_BASE
    sta SourcePtr
    lda #>HIRES_BASE
    sta SourcePtr+1

    ; Set destination pointer to the bottom where lines will be copied
    lda #<(HIRES_BASE + (200 - NUM_LINES) * LINE_LENGTH)
    sta DestPtr
    lda #>(HIRES_BASE + (200 - NUM_LINES) * LINE_LENGTH)
    sta DestPtr+1

ScrollDown:
    ldy #0                  ; Y will track bytes within a line
LineCopy:
    lda (SourcePtr),y       ; Load byte from source line
    sta (DestPtr),y         ; Store byte to destination line
    iny                     ; Next byte in the line
    cpy #LINE_LENGTH        ; End of the line?
    bne LineCopy            ; Continue if not

    ; Move to the next line
    clc                     ; Clear carry for addition
    lda SourcePtr
    adc #LINE_LENGTH
    sta SourcePtr
    lda SourcePtr+1
    adc #0
    sta SourcePtr+1

    lda DestPtr
    adc #LINE_LENGTH
    sta DestPtr
    lda DestPtr+1
    adc #0
    sta DestPtr+1

    ; Decrement line counter
    dec NUM_LINES
    bne ScrollDown          ; Continue if lines remain

    rts                     ; Done

