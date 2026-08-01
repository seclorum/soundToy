; SafeColourTble - 0 = safe, 1 = dangerous
; Covers low 5 bits; high bit (inverse) is handled by masking
_SafeColourTable
    .byt 0,0,0,0,0,0,0,0 	; 00-07 ink
	.byt 1,1,1,1,1,1,1,1	; 08-15 charset (dangerous)
	.byt 0,0,0,0,0,0,0,0	; 16-23 paper
	.byt 1,1,1,1,1,1,1,1	; 24-31 video/Hz (dangerous)

; Returns a guaranteed-safe attribute in A
; Clobbers X
_GetSafeColour
	jsr qrandomJ			; todo: find a faster random
	and #$1F				; keep only attribute bits
	tax
	lda _SafeColourTable,x
	bne _GetSafeColour		; retry if dangerous
	txa						; A = the safe value
	rts


