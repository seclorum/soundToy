_currentSound
	.byt 238,2,0,0,0,0,0,62,16,0,0,208,7,0,0,0

_BootSound
	.byt 238,2,0,0,0,0,0,62,16,0,0,208,7,0,0,0
	
_Synth	
	ldx #<_currentSound
	ldy #>_currentSound
	jsr $FA86
	rts
