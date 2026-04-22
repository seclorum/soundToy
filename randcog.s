; _randcolorgen
; Fills memory from 0xA000 to 0xBF3F with random values (0-255)
; but skips values 0x08, 0x18, 0x88, and 0x98

_randcolorgen:
        LDX #$00         ; Initialize X register for random seed
        LDY #$00         ; Initialize Y for offset

        LDA #$A0         ; Set A to high byte $A0
        STA $02          ; Store A in high byte of address ($02)
        LDA #$00         ; Set A to low byte $00 (start at $A000)
        STA $03          ; Store A in low byte of address ($03)

rcg_loop_start:
        JSR gen_random   ; Generate a random number in A

        ; Check for special values: 0x08, 0x18, 0x88, 0x98
        CMP #$08
        BEQ rcg_loop_start   ; If A == 0x08, branch back to random generation
        CMP #$18
        BEQ rcg_loop_start   ; If A == 0x18, branch back to random generation
        CMP #$88
        BEQ rcg_loop_start   ; If A == 0x88, branch back to random generation
        CMP #$98
        BEQ rcg_loop_start   ; If A == 0x98, branch back to random generation

        ; Store the random value in memory
        STA ($02),Y      ; Store A into the memory address in $02/$03

        ; Increment the address
        INC $03          ; Increment low byte of the memory address ($03)
        BNE skip_high_inc ; If low byte hasn't wrapped, skip high byte increment
        INC $02          ; Increment high byte if low byte wrapped

skip_high_inc:
        LDA $02          ; Load high byte of address
        CMP #$BF         ; Check if we have reached high byte $BF
        BNE rcg_loop_start   ; If not, continue filling
        LDA $03          ; Check if low byte reached 0x40 (end of $BF3F)
        CMP #$40         ; Compare with the limit
        BNE rcg_loop_start   ; If not yet $40, continue

        RTS              ; Return from subroutine when done


; randcogtab
; Fills memory from 0xA000 to 0xBF3F using random values from a lookup table,
; limited to the range 0x00..0x7F, excluding the values 0x08, 0x18.

dest_hi:   .byte $00     ; High byte of destination address
dest_lo:   .byte $00     ; Low byte of destination address

_randcogtab:
        LDX #$00         ; Initialize X register for random index into the table
        LDY #$00         ; Initialize Y register for memory address offset

        LDA #$A0
        STA dest_hi      ; Store high byte in dest_hi
        LDA #$00
        STA dest_lo      ; Store low byte in dest_lo

loop_start:
        JSR _qrandomJ   ; Generate a random index in A

        ; Limit the random index to the range of valid entries in the table
        AND #$7F         ; Mask to 0-127 (size of the lookup table with valid values)

        TAX              ; Transfer the random index to X for table lookup
        LDA valid_values,X ; Load the value from the lookup table

        ; Manually compute the target address using dest_hi and dest_lo
        LDY dest_lo      ; Load the low byte of the address into Y
        LDA dest_hi      ; Load the high byte of the address into A
        STA $FF          ; Temporary storage for high byte in zero page (pick an unused location)
        LDA dest_lo      ; Load the low byte of the address into A
        STA $FE          ; Temporary storage for low byte in zero page (pick another unused location)

        ; Now store value at the calculated address
        LDA valid_values,X ; Load value to store
        STA ($FE),Y      ; Store at address pointed by FE and FF (absolute address)

        ; Increment memory address
        INC dest_lo      ; Increment low byte of memory address
        BNE tskip_high_inc ; If low byte hasn't wrapped, skip high byte increment
        INC dest_hi      ; Increment high byte if low byte wrapped

tskip_high_inc:
        LDA dest_hi      ; Load high byte of address
        CMP #$BF         ; Check if we have reached high byte $BF
        BNE loop_start   ; If not, continue filling
        LDA dest_lo      ; Check if low byte reached 0x40 (end of $BF3F)
        CMP #$40         ; Compare with the limit
        BNE loop_start   ; If not yet $40, continue

        RTS              ; Return from subroutine when done

; Subroutine to generate random number
gen_random:
        ; Simple pseudo-random number generator
        LDA dest_hi      ; Load from dest_hi for randomness
        EOR dest_lo      ; XOR with dest_lo for randomness
        ASL              ; Shift left to mix up bits
        ADC dest_hi      ; Add with carry to further randomize
        STA dest_hi      ; Store result for the next iteration
        TXA              ; Transfer X to A for randomness variation
        EOR dest_hi      ; XOR with seed in dest_hi
        RTS              ; Return with random value in A

; Lookup table of valid values (excluding 0x08, 0x18, 0x88, 0x98)
valid_values:
        .byte $00, $01, $02, $03, $04, $05, $06, $07
        .byte $09, $0A, $0B, $0C, $0D, $0E, $0F, $10
        .byte $11, $12, $13, $14, $15, $16, $17, $19
        .byte $1A, $1B, $1C, $1D, $1E, $1F, $20, $21
        .byte $22, $23, $24, $25, $26, $27, $28, $29
        .byte $2A, $2B, $2C, $2D, $2E, $2F, $30, $31
        .byte $32, $33, $34, $35, $36, $37, $38, $39
        .byte $3A, $3B, $3C, $3D, $3E, $3F, $40, $41
        .byte $42, $43, $44, $45, $46, $47, $48, $49
        .byte $4A, $4B, $4C, $4D, $4E, $4F, $50, $51
        .byte $52, $53, $54, $55, $56, $57, $58, $59
        .byte $5A, $5B, $5C, $5D, $5E, $5F, $60, $61
        .byte $62, $63, $64, $65, $66, $67, $68, $69
        .byte $6A, $6B, $6C, $6D, $6E, $6F, $70, $71
        .byte $72, $73, $74, $75, $76, $77, $78, $79
        .byte $7A, $7B, $7C, $7D, $7E, $7F