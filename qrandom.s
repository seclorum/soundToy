_qrandomJ
		ldy #8
		lda $0
_qrback
		asl
		rol $FF
		bcc _qrnoEor
		eor #$39
_qrnoEor
		dey
		bne _qrback
		sta $0
		cmp #0
		rts
