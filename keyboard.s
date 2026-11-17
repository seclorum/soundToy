

; Example program for reading the Oric's     
; keyboard. All keys are scanned and a       
; virtual matrix of 8 bytes is updated at    
; each IRQ.                                  
;                                            
; Idea: Dbug                                 
; Main code: Twilighte                       
; Adaptation & final implementation: Chema   
;                                            
; 2010                                        

#include "params.h"

	.zero
; CRITICAL: nothing may live in .zero -- link65 parks it at $7C+, the
; kernel ROM workspace ($84-$86 are read/written by the 1ms FIRQ/EDIT
; code, and the $3C-$4F "hole" is BASIC/FIRQ scratch too: the IRQ
; handler touches $46/$47 at $FDDA).  Scratch vars live in the data
; segment instead (a few cycles slower, actually safe).

	.text 

oldKey 			.byt 0
zpTemp01			.byt 0
zpTemp02			.byt 0
tmprow				.byt 0
_KeyBank 		.dsb 8   ; The virtual Key Matrix

; Usually it is a good idea to keep 0 all the unused
; entries, as it speeds up things. Z=1 means no key
; pressed and there is no need to look in tables later on.
_KeyboardASCIIMapping 
tab_ascii
    .asc "7","N","5","V",KET_RCTRL,"1","X","3"
    .asc "J","T","R","F",0,KEY_ESC,"Q","D"
    .asc "M","6","B","4",KEY_LCTRL,"Z","2","C"
    .asc "K","9",38,"-",0,0,42,39
    .asc " ",",",".",KEY_UP,KEY_LSHIFT,KEY_LEFT,KEY_DOWN,KEY_RIGHT
    .asc "U","I","O","P",KEY_FUNCT,KEY_DEL,")","("
    .asc "Y","H","G","E",0,"A","S","W"
    .asc "8","L","0","/",KEY_RSHIFT,KEY_RETURN,0,"+"




; C-callable alias: scan the matrix once from main.  NOTE: this drives
; the AY/VIA keyboard lines with sei - calling it every frame races the
; ROM FIRQ (keyboard buffer + AY sound replay) and can freeze the ROM
; sound player.  Prefer _KbdIrqInstall (scan inside the IRQ chain).

_ReadKeyboard
.(
	sei
	jsr _ScanKeyboard
	cli
	rts
.)

_ScanKeyboard
.(
	;Write Column Register Number to PortA 
	lda #$0E 
	sta via_porta 
	;Tell AY this is Register Number 
	lda #$FF 
	sta via_pcr 
	; Clear CB2, as keeping it high hangs on some orics.
	; Pity, as all this code could be run only once, otherwise
	ldy #$dd 
	sty via_pcr 

	ldx #7 
loop2   ;Clear relevant bank 
	lda #00 
	sta _KeyBank,x 
	;Write 0 to Column Register 
	sta via_porta 
	lda #$fd 
	sta via_pcr 
	lda #$dd
	sta via_pcr
	lda via_portb 
	and #%11111000
	stx zpTemp02
	ora zpTemp02 
	sta via_portb 
	;Wait 10 cycles for circuit to settle on new row 
	;Use time to load inner loop counter and load Bit 
	; CHEMA: Fabrice Broche uses 4 cycles (lda #8:inx) plus
	; the four cycles of the and absolute. That is 8 cycles.
	; So I guess that I could do the same here (ldy,lda)
	ldy #$80
	lda #8 
	;Sense Row activity 
	and via_portb 
	beq skip2 
	;Store Column 
	tya
loop1   
	eor #$FF 
	sta via_porta 
	lda #$fd 
	sta via_pcr 
	lda #$dd
	sta via_pcr
	lda via_portb 
	and #%11111000
	stx zpTemp02
	ora zpTemp02 
	sta via_portb 
	;Use delay(10 cycles) for setting up bit in _KeyBank and loading Bit 
	tya
	ora _KeyBank,x 
	sta zpTemp01 
	lda #8 
	;Sense key activity 
	and via_portb 
	beq skip1 
	;Store key 
	lda zpTemp01 
	sta _KeyBank,x 
skip1   ;Proceed to next column 
	tya 
	lsr 
	tay 
	bcc loop1 
skip2   ;Proceed to next row 
	dex 
	bpl loop2 
	rts
.)  


; Some more routines, not actualy needed, but quite useful
; for reading a single key (get the first active bit in 
; the virtual matrix) and returning his ASCII value.
; Should serve as an example about how to handle the keymap.

; Reads a key (single press, but repeating) and returns his ASCII value in reg X. 
; Z=1 if no keypress detected.

_ReadKey
.(
	ldx #7
loop
	lda _KeyBank,x
	and #%11101111    ; The column 4 of the matrix contains all the CTRL, SHIFT and FUNC keys
	beq skip
	
	ldy #$ff
loop2
	iny
	lsr
	bcc loop2

	txa
	asl
	asl
	asl
	sty tmprow
	clc
	adc tmprow
	tax
	lda tab_ascii,x  ; Get the code associated to the key

	tax
	rts

skip
	dex
	bpl loop

	ldx #0
	rts
.)


; Read a single key, same as before but no repeating.

_ReadKeyNoBounce
.(
	jsr _ReadKey
	cpx oldKey
	beq retz
	stx oldKey
	cpx #0
	beq retz
	rts
retz
	ldx #0
	rts
.)


; ---- C-callable poll entry points ---------------------------------
; OSDK C reads the low byte of a char return from X (high byte in A),
; see grsimple.s _key/grexit2.  So these return X = key, A = 0.

; Mode 3 (legacy): scan the matrix from main.  Reproducing the 1.5.0
; freeze is the point of this mode - the sei'd VIA scan races the ROM
; FIRQ's AY/keyboard handling.
_PollKeyMain
.(
	jsr _ReadKeyboard
	jsr _ReadKeyNoBounce
	lda #0
	rts
.)

; Mode 1: assumes _KbdIrqInstall has been called; the scan happens in
; the IRQ chain, main only debounces.
_PollKeyIrq
.(
	jsr _ReadKeyNoBounce
	lda #0
	rts
.)

; Mode 1 note: the IRQ installation goes through the OSDK library's
; official chain_irq_handler() (lib/irq.s) with a C callback into
; _ScanKeyboard - the wrapper saves A/X/Y and the C temporaries.
; See STOY_KBD_MODE in toy.c.


; Wait for a keypress
_WaitKey
.(
loop
	; jsr _WaitIRQ
	jsr _ReadKeyNoBounce
	beq loop
	rts	
.)


; debug harness: mute the RAM interrupt vectors by pointing them at an
; RTI stub (restores available via _unmute_irq).  Tests whether the
; first-hires-frame crash is a ROM-interrupt interaction.
_mute_irq
.(
        lda $0245
        sta irq_saved
        lda $0246
        sta irq_saved+1
        lda $0247
        sta irq_saved+2
        lda $0248
        sta irq_saved+3
        lda #<_irq_rti
        sta $0245
        sta $0247
        lda #>_irq_rti
        sta $0246
        sta $0248
        rts
.)
_unmute_irq
.(
        lda irq_saved
        sta $0245
        lda irq_saved+1
        sta $0246
        lda irq_saved+2
        sta $0247
        lda irq_saved+3
        sta $0248
        rts
.)
irq_saved	.byt 0,0,0,0
_irq_rti
        rti

; Pure-software SYNTH: writes AY reg0..15 straight from currentSound[].
; Replaces the OSDK Synth() stub that JSRs the ROM driver at $FA86 - that
; driver walks the ROM sound queue in $0534..$0570, which our program
; load (based at $550) has overwritten; the ROM's 50Hz handler then
; plays the garbage queue and STAs through wild pointers.
_synth_safe
.(
; The AY is wired through the VIA, NOT at $0308/$0310 (those are T2CL
; and the ACIA region - writing there corrupts the ROM's 50Hz handler).
; Canonical sequence (see _ScanKeyboard): register number on portA,
; CB2 high->low on PCR to latch it; data on portA, CB1 high->low to
; latch it.
        sei
        ldx #$00
sy_loop:
        txa
        sta via_porta
        lda #$ff
        sta via_pcr
        lda #$dd
        sta via_pcr
        lda _currentSound,x
        sta via_porta
        lda #$fd
        sta via_pcr
        lda #$dd
        sta via_pcr
        inx
        cpx #$10
        bne sy_loop
        cli
        rts
.)

; debug/atomicity helpers: maskable-interrupt mask.  The ROM's own
; display routine executes the $550 display-instruction chain from the
; 1ms IRQ; block patches must be atomic against it (half-patched
; operands made the ROM STA through garbage pointers).
_disable_irq
        sei
        rts
_enable_irq
        cli
        rts
