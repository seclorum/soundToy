    ; Constants
ADDR_START = $A000
ADDR_END   = $BF3F

    ; Pseudo-random number generator function (_localJrandom)
_localJrandom:
    ldy #8
    lda $0
_spmback:
    asl
    rol $FF
    bcc _spmnoEor
    eor #$39
_spmnoEor:
    dey
    bne _spmback
    sta $0
    rts

    ; Main routine
_spamit:
    ldx #<ADDR_START     ; Load low byte of start address into X
    ldy #>ADDR_START     ; Load high byte of start address into Y

_main_loop:
    jsr _localJrandom    ; Call random number generator
    lda $0               ; Load generated random value into A register

_check_filter:
    ; Check if value is in the first excluded range (0x08-0x18)
    cmp #$08
    bcc _store_value     ; Value is less than 0x08, so it's valid
    cmp #$18
    bcc _get_next_random ; Value is between 0x08 and 0x18, skip

    ; Check if value is in the second excluded range (0x88-0x98)
    cmp #$88
    bcc _store_value     ; Value is less than 0x88, so it's valid
    cmp #$98
    bcc _get_next_random ; Value is between 0x88 and 0x98, skip

_store_value:
    sta $0000, x         ; Store the value at the address $YXX (address is YX combined)

    inx                  ; Increment the low byte of the address
    bne _main_loop       ; If X didn't overflow, continue the main loop

    iny                  ; If X overflowed, increment Y (high byte of the address)
    cpy #>ADDR_END
    bcc _main_loop       ; If Y hasn't reached the end, continue the main loop

_done:
    rts                  ; Return from subroutine

_get_next_random:
    jsr _localJrandom    ; Get a new random value
    lda $0               ; Load the new random value into A register
    jmp _check_filter    ; Re-check the filter
