; shimmer.s - attribute-exploiting animation hot loop for toy.c
;
; Per frame:
;   1. TWINKLE  : EOR #$80 on _shm_flips random PIXEL bytes.  Bit 7 never
;                 converts a pixel into an attribute and the field never
;                 contains video attributes, so toggling inverse is always
;                 safe - every cell flips between its two paper/ink
;                 pairings.  Row picker is a round-robin over 9 bands
;                 covering the seed field (top 1/3, rows 0..66) so every
;                 field row animates evenly.
;   2. WAVEFRONT: EOR #$80 on one random ATTRIBUTE byte (pixels skipped).
;                 Inverting an INK/PAPER attribute recolours every byte
;                 to its right on that scanline - a single byte write.
;   3. SWEEP    : write a [PAPER][INK] attribute PAIR at a random spot:
;                 paper at col, ink at col+1, so the whole segment to the
;                 right gets BOTH colours (the old sweep fixed only the
;                 paper and left the ink stale).
;
; Both the live screen ($A000 + row*40 + col) and the C-side shadow
; (shadow_screen) are updated so the shadow stays authoritative.
;
; Uses C globals: _row_off_lo / _row_off_hi (row*40 split, rows 0..66)
;                 _shadow_screen (8000 bytes)
; Exports:        _shm_flips     (C tunes the twinkle budget per frame)

; Seed field = top 1/3 of the screen (rows 0..66).  Keep FIELD_ROWS
; in sync with toy.c.
FIELD_ROWS  = 67
BANDS       = 9            ; ceil(67/8) band round-robin
BAND_ROWS   = 8

        .zero
; CRITICAL: nothing may live in .zero -- link65 parks it at $7C+, the
; kernel ROM workspace ($84-$86 are read/written by the 1ms FIRQ/EDIT
; code, and the $3C-$4F "hole" is BASIC/FIRQ scratch too: the IRQ
; handler touches $46/$47 at $FDDA).  All scratch vars live in the
; data segment instead (a few cycles slower, actually safe).  Keep
; shm_band immediately after shm_seed: the LFSR reads shm_seed+1.

        .text
shm_seed    .byt $9E
shm_band    .byt 0
shm_rowbase .byt 0
shm_col     .byt 0
shm_ofs_lo  .byt 0
shm_ofs_hi  .byt 0
shm_val     .byt $10
shm_tmp     .byt 0
_shm_init
        lda #$9E
        sta shm_seed
        lda #$00
        sta shm_band
        sta shm_rowbase
        sta shm_col
        sta shm_ofs_lo
        sta shm_ofs_hi
        sta shm_tmp
        lda #$10
        sta shm_val
        rts

_shm_flips  .byt 24            ; twinkle budget, C adjusts per frame

_shimmer_step
.(
        ldx _shm_flips
        beq shm_wave
shm_tloop:
        jsr _shm_pick_band     ; round-robin band row + random column
        jsr _shm_twinkle       ; flip bit7 on screen + shadow
        dex
        bne shm_tloop

        ; --- wavefront: invert one random ATTRIBUTE byte (segment shift) ---
shm_wave:
        jsr _shm_pick_rand     ; fully random row within the field
        lda shm_ofs_hi
        clc
        adc #$A0
        sta shm_fr+2
        lda shm_ofs_lo
        sta shm_fr+1
shm_fr: lda $ffff
        sta shm_tmp
        and #$60
        bne shm_sweep          ; pixel byte - twinkle already covers it
        lda shm_tmp
        and #$78               ; INVARIANT: never touch a style attr
        cmp #$08               ; (0x08-0x0F/0x88-0x8F) or a VIDEO-CONTROL
        beq shm_sweep          ; attr (0x18-0x1F/0x98-0x9F) - the latter
        cmp #$18               ; flips 50/60Hz + TEXT/HIRES machine-wide!
        beq shm_sweep
        lda shm_tmp
        eor #$80               ; 0x0i<->0x8i or 0x1i<->0x9i, always legal
        sta shm_val
        jsr _shm_store

        ; --- sweep: [PAPER][INK] pair, recolours the segment to the right ---
shm_sweep:
        jsr _shm_pick_rand
        lda shm_col            ; need col+1 for the INK byte
        cmp #39
        bcc shm_sw1
        dec shm_col            ; clamp pair to cols 38/39
        dec shm_ofs_lo
shm_sw1:
        jsr _shm_rand
        and #7
        ora #$10               ; 0x10|c = PAPER attribute
        sta shm_val
        jsr _shm_store
        inc shm_col
        inc shm_ofs_lo         ; +1 with carry (col 7/15/23/31 can hit $FF)
        bne shm_sw2
        inc shm_ofs_hi
shm_sw2:
        jsr _shm_rand
        and #7                 ; 0x00|c = INK attribute
        sta shm_val
        jsr _shm_store
        rts
.)

; pick a row from the band round-robin (all field rows covered evenly)
_shm_pick_band
.(
        inc shm_band
        lda shm_band
        cmp #BANDS
        bcc shm_pb1
        lda #0
        sta shm_band
shm_pb1:
        asl
        asl
        asl                    ; band * 8  (0..64)
        sta shm_rowbase
        jsr _shm_rand
        and #(BAND_ROWS-1)
        clc
        adc shm_rowbase        ; row 0..71
        cmp #FIELD_ROWS
        bcc shm_pb2
        sbc #FIELD_ROWS        ; wrap 67..71 -> 0..4 (carry set by cmp)
shm_pb2:
        tay
        jmp _shm_col
.)

; pick a fully random row 0..FIELD_ROWS-1 (mod by subtraction loop)
_shm_pick_rand
.(
        jsr _shm_rand
shm_pr1:
        cmp #FIELD_ROWS
        bcc shm_pr2
        sbc #FIELD_ROWS        ; carry set by cmp -> A = A - FIELD_ROWS
        jmp shm_pr1            ; 255 needs at most 3 passes
shm_pr2:
        tay
        jmp _shm_col
.)

; add random column: shm_ofs = row*40 + col, shm_col = col
_shm_col
.(
        lda _row_off_lo,y
        sta shm_ofs_lo
        lda _row_off_hi,y
        sta shm_ofs_hi
        jsr _shm_rand
        and #63
        tay
        lda shm_coltab,y
        sta shm_col
        clc
        adc shm_ofs_lo
        sta shm_ofs_lo
        lda shm_ofs_hi
        adc #0
        sta shm_ofs_hi
        rts
.)

; EOR #$80 at (shm_ofs + $A000) and (shm_ofs + shadow_screen)
_shm_twinkle
.(
        lda shm_ofs_hi
        clc
        adc #$A0               ; screen base $A000
        sta shm_hi_scr

        clc
        lda shm_ofs_lo
        adc #<_shadow_screen
        sta shm_ofs_slo
        lda shm_ofs_hi
        adc #>_shadow_screen
        sta shm_ofs_shi

        lda shm_ofs_lo
        sta shm_fs+1
        sta shm_st+1
        lda shm_hi_scr
        sta shm_fs+2
        sta shm_st+2
shm_fs: lda $ffff
        sta shm_tmp
        and #$60
        beq shm_tw_attr        ; attribute: must be PAPER/INK family
shm_tw_flip:
        lda shm_tmp
        eor #$80
shm_st: sta $ffff

        ; --- shadow side (same byte in the C-authoritative copy) ---
        lda shm_ofs_slo
        sta shm_fs2+1
        sta shm_st2+1
        lda shm_ofs_shi
        sta shm_fs2+2
        sta shm_st2+2
shm_fs2: lda $ffff
        eor #$80
shm_st2: sta $ffff
        rts
shm_tw_attr:
        ; INVARIANT: style (xx001xxx) and video-control (xx011xxx)
        ; attributes are NEVER written - flipping one turns 0x18-0x1F
        ; into 0x98-0x9F and flips 50/60Hz machine-wide.
        lda shm_tmp
        and #$78
        cmp #$08
        beq shm_tw_skip
        cmp #$18
        beq shm_tw_skip
        jmp shm_tw_flip
shm_tw_skip:
        rts
.)

; store shm_val at (shm_ofs + $A000) and (shm_ofs + shadow_screen)
_shm_store
.(
        lda shm_ofs_hi
        clc
        adc #$A0
        sta shm_hi_scr

        clc
        lda shm_ofs_lo
        adc #<_shadow_screen
        sta shm_ofs_slo
        lda shm_ofs_hi
        adc #>_shadow_screen
        sta shm_ofs_shi

        lda shm_ofs_lo
        sta shm_ss+1
        lda shm_hi_scr
        sta shm_ss+2
shm_ss: sta $ffff

        lda shm_ofs_slo
        sta shm_ss2+1
        lda shm_ofs_shi
        sta shm_ss2+2
shm_ss2: sta $ffff
        rts
.)

; hardware-flavoured 8-bit RNG.
; VIA mode (Chema's mix, see qrandom.s): NOTE - reading T1CL/T2CL
; ACKS and clears the VIA timer IRQ latches the ROM relies on for its
; 50 Hz tick; calling this from main can silently kill $A0 and the ROM
; FIRQ work.  Safe mode mixes the ROM frame counter ($A0) instead and
; never touches the timers.  C selects via _shm_via (1=VIA, 0=safe).
_shm_via    .byt 1

_shm_rand
.(
        lda _shm_via
        beq shm_r_safe
        lda shm_seed
        ora $308               ; VIA T2CL  (acks T2 latch!)
        rol
        adc $304               ; VIA T1CL  (acks T1 latch!)
        sta shm_seed
        sbc shm_seed+1
        rol
        sta shm_seed+1
        rts
shm_r_safe:
        lda shm_seed
        clc
        adc $A0                ; ROM 50 Hz frame counter (no latches)
        rol
        adc shm_seed+1
        sta shm_seed
        eor shm_seed+1
        rol
        sta shm_seed+1
        rts
.)

shm_hi_scr  .byt 0
shm_ofs_slo .byt 0
shm_ofs_shi .byt 0

; column picker: seed&63 -> 0..39
shm_coltab:
        .byte  0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        .byte 10,11,12,13,14,15,16,17,18,19
        .byte 20,21,22,23,24,25,26,27,28,29
        .byte 30,31,32,33,34,35,36,37,38,39
        .byte  3,17,31, 8,22,36,13,27, 2,16
        .byte 30, 5,19,33, 8,22,36,11,25,39
        .byte  1,15,29, 7
