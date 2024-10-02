; Function: execute_code_block
; This function jumps to the given address passed from C
; Input: A 16-bit address from C

_execute_code_block:
        ; C passes the address on the stack
        pla                 ; Pull low byte of the address
        sta low_byte        ; Store in low_byte variable

        pla                 ; Pull high byte of the address
        sta high_byte       ; Store in high_byte variable

        ; Load the address for jump
        sta $00             ; Store low byte in $00 (could use a different location if needed)

        lda high_byte       ; Load high byte into A

        jmp ($00)           ; Jump to the address in $00/$01

_execute_code_block2:
        ; C passes the address on the stack
        ; Low byte is first, high byte is second.

        ; Pull the low byte of the address from the stack
        pla                 ; Pull low byte
        sta low_byte        ; Store in a local variable

        ; Pull the high byte of the address from the stack
        pla                 ; Pull high byte
        sta high_byte       ; Store in a local variable

        ; Prepare for indirect jump
        lda low_byte        ; Load low byte into A
        ldx high_byte       ; Load high byte into X

        ; Use indirect jump
;        jmp (a, x)   ; Jump to the address stored in low_byte and high_byte

low_byte: .byt 1          ; Reserve 1 byte for the low byte
high_byte: .byt 1         ; Reserve 1 byte for the high byte

