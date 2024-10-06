
; Pseudo-random number seed
RandSeed: .byte $4D       ; Initial random seed

; Regular memory to store the table pointer
ptrL: .byte 0             ; Low byte of the table pointer
ptrH: .byte 0             ; High byte of the table pointer

; Temporary memory to calculate the effective address for table lookup
tmpPtrL: .byte 0          ; Temporary low byte for calculated pointer
tmpPtrH: .byte 0          ; Temporary high byte for calculated pointer

; RandomByte - Generates a pseudo-random number (0-255) in the accumulator (A)
RandomByte:
    lda RandSeed           ; Load the current random seed
    eor #$A7               ; XOR with a constant for randomness
    asl                    ; Arithmetic shift left for more randomness
    rol RandSeed           ; Rotate left through the seed
    sta RandSeed           ; Update the random seed
    rts                    ; Return the random number in A

; Function: FindRandomNumber
; Inputs:
;   A: Pointer to the table (low byte)
;   Y: Pointer to the table (high byte)
; Outputs:
;   A: Random number not found in the table
; Clobbers: A, X, Y

_FindRandomNumber:
    sta ptrL               ; Store low byte of the table pointer in regular memory
    tya                    ; Transfer high byte of the table pointer to A
    sta ptrH               ; Store high byte of the table pointer in regular memory

NextRandom:
    jsr RandomByte         ; Generate a random number in A
    tax                    ; Transfer the random number to X for comparison

    ldy #0                 ; Start index at 0 for scanning the table

CheckTable:
    ; Calculate the effective address (ptrL + Y) into tmpPtrL/tmpPtrH
    lda ptrL               ; Load low byte of the pointer
    clc                    ; Clear carry for addition
    adc #0                 ; Add the index (Y) to the low byte
    sta tmpPtrL            ; Store the result in a temporary low byte

    lda ptrH               ; Load high byte of the pointer
    adc #0                 ; Add carry to the high byte
    sta tmpPtrH            ; Store the result in a temporary high byte

    ; Combine tmpPtrH and tmpPtrL into a full address and load the table value
    lda tmpPtrL            ; Load the low byte of the calculated address
    sta $fe                ; Store the address in page zero indirect (Oric page zero pointer)
    lda tmpPtrH            ; Load the high byte
    sta $ff                ; Store the high byte indirect address

    ldy #0                 ; Use Y to scan through the table
    lda ($fe),y            ; Load from the table using absolute indexed address
    cpx                    ; Compare the table value with the random number
    beq GenerateAgain      ; If match, generate a new random number
    iny                    ; Increment index (Y)
    cpy #0                 ; Check if Y has wrapped around (i.e., scanned the full 256 bytes)
    bne CheckTable         ; If not done scanning, continue checking

    ; If we reach here, the random number is not in the table
    txa                    ; Move the valid random number from X back to A
    clc                    ; Clear carry (success)
    rts                    ; Return with the random number in A

GenerateAgain:
    jmp NextRandom         ; Generate another random number if found in the table

