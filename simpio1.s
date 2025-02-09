; *******************************************************************
; Oric-1/Atmos Assembly Language Basics
; Demonstrating four key principles:
; 1. Memory-Mapped I/O
; 2. Character Input and Output
; 3. Simple Sound Generation
; 4. Program Flow Control with Randomized Sound
; *******************************************************************

        .org $0500          ; Set program load address in memory

; *** 1. Memory-Mapped I/O ***
TEXT_SCREEN  = $BB80  ; Start of text screen memory
KEYBOARD     = $0300  ; Keyboard input location
AY_REGISTER  = $0310  ; Sound chip register select
AY_WRITE     = $0311  ; Sound chip write data
RANDOM_SEED  = $0240  ; Random number generator seed

; *** 2. Character Input and Output with Sound ***
; Read a keypress, display it on the screen, and trigger a random sound

        LDX #0            ; X register = 0 (column position)
READ_KEY:
        LDA KEYBOARD      ; Load key from keyboard buffer
        BEQ READ_KEY      ; If no key is pressed, loop
        STA TEXT_SCREEN,X ; Store key at screen position X
        INX              ; Move to next column
        CPX #40          ; Limit to one line (40 columns)
        BNE TRIGGER_SOUND ; Jump to play sound when a key is pressed

; *** 3. Simple Sound Generation with Randomized Frequency ***
TRIGGER_SOUND:
        JSR RANDOM        ; Generate a random frequency
        LDA #0           ; Select register 0 (Channel A fine tone)
        STA AY_REGISTER  
        LDA RANDOM_SEED   ; Load random value for frequency
        STA AY_WRITE     
        
        LDA #1           ; Select register 1 (Channel A coarse tone)
        STA AY_REGISTER  
        LDA #0           ; Set frequency high byte
        STA AY_WRITE     
        
        LDA #7           ; Select register 7 (Mixer control)
        STA AY_REGISTER  
        LDA #%11111010   ; Enable tone on channel A
        STA AY_WRITE     
        
        LDA #8           ; Select register 8 (Amplitude for channel A)
        STA AY_REGISTER  
        LDA #15          ; Max volume
        STA AY_WRITE     
        
        JSR DELAY        ; Call delay subroutine
        
        LDA #8           ; Turn off sound
        STA AY_REGISTER  
        LDA #0           
        STA AY_WRITE     
        JMP READ_KEY      ; Loop back to wait for another keypress

; *** 4. Random Number Generator ***
RANDOM:
        LDA RANDOM_SEED   ; Load the current seed
        EOR #$1D         ; Apply XOR with a fixed value
        STA RANDOM_SEED   ; Store new random seed
        RTS              ; Return with a new random value

; *** 5. Program Flow Control ***
DELAY:
        LDY #$FF         ; Outer loop
LOOP1:  LDX #$FF         ; Inner loop
LOOP2:  DEX              ; Decrement X
        BNE LOOP2        ; If not zero, loop again
        DEY              ; Decrement Y
        BNE LOOP1        ; If not zero, loop again
        RTS              ; Return from subroutine

        .end

