; HIRES screen base address
HIRES_BASE   = $A000
LINE_LENGTH  = 40        ; Bytes per line in HIRES mode
NUM_LINES    = 50        ; Number of lines to scroll
TOTAL_LINES  = 200       ; Total HIRES lines

; Precompute destination address
DEST_OFFSET  = (TOTAL_LINES - NUM_LINES) * LINE_LENGTH
DEST_BASE    = HIRES_BASE + DEST_OFFSET

_hiblob:
    ; Initialize source address
    lda #<HIRES_BASE         ; Load low byte of HIRES_BASE
    sta src_lo
    lda #>HIRES_BASE         ; Load high byte of HIRES_BASE
    sta src_hi

    ; Initialize destination address
    lda #<DEST_BASE          ; Load low byte of DEST_BASE
    sta dest_lo
    lda #>DEST_BASE          ; Load high byte of DEST_BASE
    sta dest_hi

ScrollDown:
    ldy #0                  ; Y will track bytes within a line
LineCopy:
    ; Compute source address
    lda src_lo
    clc
    adc y                   ; Add Y to the low byte of source address
    sta temp_lo             ; Store in temporary low byte
    lda src_hi
    adc #0                  ; Add carry to high byte of source address
    sta temp_hi             ; Store in temporary high byte

    ; Load byte from computed source address
    lda temp_lo             ; Load low byte of source address
    sta ptr_lo              ; Set pointer low byte
    lda temp_hi             ; Load high byte of source address
    sta ptr_hi              ; Set pointer high byte
    lda (ptr_lo),y          ; Load byte using indexed addressing

    ; Compute destination address
    lda dest_lo
    clc
    adc y                   ; Add Y to the low byte of destination address
    sta temp_lo             ; Store in temporary low byte
    lda dest_hi
    adc #0                  ; Add carry to high byte of destination address
    sta temp_hi             ; Store in temporary high byte

    ; Store byte to computed destination address
    lda temp_lo             ; Load low byte of destination address
    sta ptr_lo              ; Set pointer low byte
    lda temp_hi             ; Load high byte of destination address
    sta ptr_hi              ; Set pointer high byte
    sta (ptr_lo),y          ; Store byte using indexed addressing

    iny                     ; Next byte in the line
    cpy #LINE_LENGTH        ; End of the line?
    bne LineCopy            ; Continue if not

    ; Move to the next line
    clc                     ; Clear carry for addition
    lda src_lo
    adc #LINE_LENGTH
    sta src_lo
    lda src_hi
    adc #0
    sta src_hi

    lda dest_lo
    adc #LINE_LENGTH
    sta dest_lo
    lda dest_hi
    adc #0
    sta dest_hi

    ; Decrement line counter
    lda nlines
    sec
    sbc #1
    sta nlines
    bne ScrollDown          ; Continue if lines remain

    rts                     ; Done

src_lo:
    .byte 0                 ; Low byte of source address
src_hi:
    .byte 0                 ; High byte of source address
dest_lo:
    .byte 0                 ; Low byte of destination address
dest_hi:
    .byte 0                 ; High byte of destination address
temp_lo:
    .byte 0                 ; Temporary low byte for address computation
temp_hi:
    .byte 0                 ; Temporary high byte for address computation
ptr_lo:
    .byte 0                 ; Pointer low byte for indirect addressing
ptr_hi:
    .byte 0                 ; Pointer high byte for indirect addressing
nlines:
    .byte 50                ; Line counter (initialized to NUM_LINES)

