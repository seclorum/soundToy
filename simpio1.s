; *******************************************************************
; Oric-1/Atmos Assembly Language Basics
; Demonstrating four key principles:
; 1. Memory-Mapped I/O
; 2. Character Input and Output
; 3. Simple Sound Generation
; 4. Program Flow Control
; *******************************************************************

        .org $0500          ; Set program load address in memory

; *** 1. Memory-Mapped I/O ***
; The Oric uses specific memory locations for hardware interaction.
; For example, the keyboard and display are mapped to known addresses.

TEXT_SCREEN  = $BB80  ; Start of text screen memory
KEYBOARD     = $0300  ; Keyboard input location
AY_REGISTER  = $0310  ; Sound chip register select
AY_WRITE     = $0311  ; Sound chip write data

; *** 2. Character Input and Output ***
; Read a keypress and display it on the screen

        LDX #0            ; X register = 0 (column position)
READ_KEY:
        LDA KEYBOARD      ; Load key from keyboard buffer
        BEQ READ_KEY      ; If no key is pressed, loop
        STA TEXT_SCREEN,X ; Store key at screen position X
        INX              ; Move to next column
        CPX #40          ; Limit to one line (40 columns)
        BNE READ_KEY      ; Loop for next key

; *** 3. Simple Sound Generation ***
; Play a simple sound using the AY-3-8912 sound chip

        LDA #7           ; Select register 7 (Mixer control)
        STA AY_REGISTER  
        LDA #%11111010   ; Enable tone on channel A
        STA AY_WRITE     
        
        LDA #0           ; Select register 0 (Channel A fine tone)
        STA AY_REGISTER  
        LDA #$20         ; Set frequency low byte
        STA AY_WRITE     
        
        LDA #1           ; Select register 1 (Channel A coarse tone)
        STA AY_REGISTER  
        LDA #0           ; Set frequency high byte
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

; *** 4. Program Flow Control ***
; Implement a simple delay routine

DELAY:
        LDY #$FF         ; Outer loop
LOOP1:  LDX #$FF         ; Inner loop
LOOP2:  DEX              ; Decrement X
        BNE LOOP2        ; If not zero, loop again
        DEY              ; Decrement Y
        BNE LOOP1        ; If not zero, loop again
        RTS              ; Return from subroutine

        .end

