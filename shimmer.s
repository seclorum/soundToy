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
; CRITICAL: this module must NEVER reference C BSS symbols directly -
; the vendored xa resolves .s->BSS abs operands with a stale high byte
; (row_off_lo $90a4 was emitted as $51a4: screen writes landed OUTSIDE
; the $A000-$BFFF HIRES region and scribbled over program code).  The
; table/shadow addresses live in self-patched operand slots that C
; fills at HIRES entry (toy.c shim_patch_ptrs): C computes its OWN
; BSS addresses correctly.  Only $A000-relative screen bytes and the
; in-module data below are ever touched.
; Exports:        _shm_flips     (C tunes the twinkle budget per frame)
;                 _shm_rl/_shm_rh/_shm_sl1/_shm_sh1/_shm_sl2/_shm_sh2
;                 (3/3/2/2/2/2-byte self-patchable operand slots)

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
.)
_shm_rl:
        lda $ffff,y              ; operand = &row_off_lo  (C patches)
        sta shm_ofs_lo
_shm_rh:
        lda $ffff,y              ; operand = &row_off_hi  (C patches)
        sta shm_ofs_hi
.(
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
        ; hard bound: ofs = row*40+col <= 66*40+39 = $0A77.  Anything
        ; larger could NOT be a legal field offset - zero it so every
        ; later screen write stays inside the HIRES region, no matter
        ; what corrupted the tables.
        lda shm_ofs_hi
        cmp #$0B
        bcc shm_cl_ok
        lda #0
        sta shm_ofs_lo
        sta shm_ofs_hi
shm_cl_ok:
        rts
.)

; EOR #$80 at (shm_ofs + $A000) and (shm_ofs + shadow_screen)
_shm_twinkle
.(
        lda shm_ofs_hi
        clc
        adc #$A0               ; screen base $A000
        sta shm_hi_scr
.)
        clc
        lda shm_ofs_lo
_shm_sl1:
        adc #$ff                 ; operand = <&shadow_screen (C patches)
        sta shm_ofs_slo
        lda shm_ofs_hi
_shm_sh1:
        adc #$ff                 ; operand = >&shadow_screen (C patches)
        sta shm_ofs_shi
.(
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
         ; MUST be guarded on ITS OWN class, not the screen byte's: a
         ; blob hides a field byte here, and flipping a shadow $1B/$0B
         ; produced $9B/$8B which blob_restore later copied back to the
         ; screen -> raster scans a video-control attr -> TEXT mode.
         lda shm_ofs_slo
         sta shm_fs2+1
         sta shm_st2+1
         lda shm_ofs_shi
         sta shm_fs2+2
         sta shm_st2+2
shm_fs2: lda $ffff
         sta shm_tmp
         and #$60
         beq shm_tw_sattr
shm_tw_sflip:
         lda shm_tmp
         eor #$80
shm_st2: sta $ffff
         rts
shm_tw_sattr:
         lda shm_tmp
         and #$78
         cmp #$08
         beq shm_tw_sskip
         cmp #$18
         beq shm_tw_sskip
         jmp shm_tw_sflip
shm_tw_sskip:
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
.)
        clc
        lda shm_ofs_lo
_shm_sl2:
        adc #$ff                 ; operand = <&shadow_screen (C patches)
        sta shm_ofs_slo
        lda shm_ofs_hi
_shm_sh2:
        adc #$ff                 ; operand = >&shadow_screen (C patches)
        sta shm_ofs_shi
.(
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

; ------------------------------------------------------------------
; set_display_instruction hot path (cell_anim_update + cell_ui_update
; call it 10x/frame; the C version costs ~500k cyc/frame).
; C fills sd_cell..sd_dy then calls _sdi_patch under disable_irq.
; Rewrites the 8x(LDA abs / STA abs) immediates of displayInstructions
; [cell] to blit pattern_bank block (sx,sy) -> screen block (dx,dy).
; displayInstructions/pattern_bank addresses arrive via the C-patched
; slots below (vendored-xa pass-1 BSS/data resolution bug workaround).
; ------------------------------------------------------------------
_sd_cell    .byt 0
_sd_sx      .byt 0
_sd_sy      .byt 0
_sd_dx      .byt 0
_sd_dy      .byt 0
_sd_dil     .byt 0            ; <&displayInstructions (C patches)
_sd_dih     .byt 0            ; >&displayInstructions
_sd_pbl     .byt 0            ; <&pattern_bank (C patches)
_sd_pbh     .byt 0            ; >&pattern_bank
sd_p1       .byt 0            ; di running ptr (cell*51+3+row*6)
sd_p2       .byt 0
sd_s1       .byt 0            ; src running ptr
sd_s2       .byt 0
sd_d1       .byt 0            ; dst running ptr
sd_d2       .byt 0
sd_svx      .byt 0            ; X/Y save (xa C: callees preserve X,Y)
sd_svy      .byt 0
_sdi_patch
        stx sd_svx
        sty sd_svy
.(
        ; --- di = &displayInstructions + cell*51 + 3 ---
        lda #0
        sta sd_p1
        sta sd_p2
        ldx _sd_cell
        beq sd_p1d
sd_p1l: clc
        lda sd_p1
        adc #51
        sta sd_p1
        lda sd_p2
        adc #0
        sta sd_p2
        dex
        bne sd_p1l
sd_p1d: clc
        lda sd_p1
        adc #3
        sta sd_p1
        lda sd_p2
        adc #0
        sta sd_p2
        clc
        lda sd_p1
        adc _sd_dil
        sta sd_p1
        lda sd_p2
        adc _sd_dih
        sta sd_p2
        ; --- src = &pattern_bank + sy*320 + sx ---
        lda #0
        sta sd_s1
        sta sd_s2
        ldx _sd_sy
        beq sd_s1d
sd_s1l: clc
        lda sd_s1
        adc #40
        sta sd_s1
        lda sd_s2
        adc #0
        sta sd_s2
        dex
        bne sd_s1l
sd_s1d: clc
        lda sd_s1
        adc _sd_pbl
        sta sd_s1
        lda sd_s2
        adc _sd_pbh
        sta sd_s2
        clc
        lda sd_s1
        adc _sd_sx
        sta sd_s1
        lda sd_s2
        adc #0
        sta sd_s2
        ; --- dst = $A000 + dy*320 + dx ---
        lda #0
        sta sd_d1
        sta sd_d2
        ldx _sd_dy
        beq sd_d1d
sd_d1l: clc
        lda sd_d1
        adc #40
        sta sd_d1
        lda sd_d2
        adc #0
        sta sd_d2
        dex
        bne sd_d1l
sd_d1d: clc
        lda sd_d1
        adc _sd_dx
        sta sd_d1
        lda sd_d2
        adc #0
        sta sd_d2
        clc
        lda sd_d2
        adc #$A0
        sta sd_d2
        ; --- 8 rows: patch the 4 immediate pairs, then execute them ---
        ldx #8
sd_row: lda sd_p1
        sta sd_wa+1
        sta sd_wb+1
        sta sd_wc+1
        sta sd_wd+1
        lda sd_p2
        sta sd_wa+2
        sta sd_wb+2
        sta sd_wc+2
        sta sd_wd+2
        ; page-crossing fixups for the +1/+3/+4 operand pairs
        lda sd_p1
        cmp #$FC
        bcc sd_rlo
        inc sd_wd+2
        cmp #$FD
        bcc sd_rlo
        inc sd_wc+2
        cmp #$FF
        bne sd_rlo
        inc sd_wb+2
sd_rlo: lda sd_s1
sd_wa:  sta $ffff               ; +0 src lo (self-modifying immediates)
        lda sd_s2
sd_wb:  sta $ffff               ; +1 src hi
        lda sd_d1
sd_wc:  sta $ffff               ; +3 dst lo
        lda sd_d2
sd_wd:  sta $ffff               ; +4 dst hi
        clc
        lda sd_p1
        adc #6
        sta sd_p1
        lda sd_p2
        adc #0
        sta sd_p2
        clc
        lda sd_s1
        adc #40
        sta sd_s1
        lda sd_s2
        adc #0
        sta sd_s2
        clc
        lda sd_d1
        adc #40
        sta sd_d1
        lda sd_d2
        adc #0
        sta sd_d2
        dex
        beq sd_fin
        jmp sd_row
sd_fin: ldx sd_svx
        ldy sd_svy
        rts
.)

; ------------------------------------------------------------------
; cells engine hot primitives (cells_tick was ~300k cyc/frame in C).
; _gc_ar_asm  = gc_affects_right(g): per row, prefix walk to x, then
;               compare "with cell" vs "$00 baseline" render states to
;               the right edge; returns 1 in A if the cell recolours.
; _gc_lok_asm = gc_left_ok(g): 8 prefix walks must match expect[r].
; C stores (int)g into _gc_gpl/_gc_gph before each call; row_ptr's
; address is C-patched into the ar_rll/ar_rlh operands (vendored-xa
; pass-1 BSS resolution bug workaround). GameCell layout: 0=x 1=y
; 2=live 3..10=snap 11..18=expect, stride 19.
; All labels here are plain globals (no local blocks) so the shared
; subroutines can be referenced across the unit.
; ------------------------------------------------------------------
_gc_gpl     .byt 0
_gc_gph     .byt 0
_ar_rpb     .byt 0            ; <&row_ptr (C patches); operands rebuilt
_ar_rpb2    .byt 0            ; >&row_ptr  per row (2y can exceed 255!)
ar_svx      .byt 0            ; X/Y save (xa C: callees must preserve)
ar_svy      .byt 0
ar_pw       .byt 0            ; "with cell" render state
ar_iw       .byt 0
ar_vw       .byt 0
ar_pb       .byt 0            ; "cell absent" baseline state
ar_ib       .byt 0
ar_vb       .byt 0
ar_r        .byt 0            ; row 0..7
ar_x        .byt 0            ; cell column
ar_bb       .byt 0            ; b & $80 of the byte under test
ar_a1       .byt 0            ; lw / uw / lb / ub scratch
ar_a2       .byt 0
ar_a3       .byt 0
ar_a4       .byt 0

; jsr: walk row[0..x-1] (operand ar_ld1) into ar_pw/iw/vw. A,X,Y clobbered.
ar_prefix
        lda #0
        sta ar_pw
        sta ar_iw
        sta ar_vw
        ldy ar_x
        beq arpd
        dey                    ; Y = x-1 downto 0 (counter AND index)
arp_l
ar_ld1  lda $ffff,Y           ; row[k], operand = &row[0] (per row)
        tax                   ; byte in X; Y untouched (loop counter)
        and #$60
        bne arpn              ; pixel byte: render state untouched
        txa
        and #$80
        sta ar_vw
        txa
        and #$10
        beq arpink
        txa
        and #$07
        sta ar_pw
        jmp arpn
arpink: txa
        and #$07
        sta ar_iw
arpn:   dey
        bpl arp_l
arpd:   rts

; jsr: patch both walk operands to row_ptr[g->y + ar_r]
ar_row
ar_g1l  lda $ffff             ; operand = g+1 (g->y), per call
        clc
        adc ar_r
        asl                   ; X = 2(y+r) mod 256, C = bit 8
        tax
        php
        lda _ar_rpb           ; lo operand (pristine each row)
        sta _ar_rll+1
        sta _ar_rlh+1
        lda _ar_rpb2
        plp
        adc #0                ; + carry of 2(y+r)
        sta _ar_rll+2
        sta _ar_rlh+2
        lda _ar_rpb2          ; (flags clobbered; C no longer needed)
_ar_rll lda $ffff,X           ; row_ptr lo
        sta ar_ld1+1
        sta ar_ld2+1
_ar_rlh lda $ffff,X           ; row_ptr hi
        sta ar_ld1+2
        sta ar_ld2+2
        rts

_gc_ar_asm
        stx ar_svx
        sty ar_svy
        jsr gcar_body
        ldx ar_svx
        ldy ar_svy
        rts
gcar_body
        lda _gc_gpl           ; snap operand = g+3
        clc
        adc #3
        sta ar_s1+1
        lda _gc_gph
        adc #0
        sta ar_s1+2
        lda _gc_gpl           ; y operand = g+1
        clc
        adc #1
        sta ar_g1l+1
        lda _gc_gph
        adc #0
        sta ar_g1l+2
        lda _gc_gpl           ; x operand = g+0
        sta ar_x1+1
        lda _gc_gph
        sta ar_x1+2
ar_x1   lda $ffff
        sta ar_x
        lda #0
        sta ar_r
ar_loop
        ldy ar_r
ar_s1   lda $ffff,Y           ; g->snap[ar_r]
        and #$60
        beq ar_rg
        jmp ar_next           ; pixel byte: row provably unaffected
ar_rg   sta ar_a1             ; keep b0 (attribute)
        jsr ar_row
        jsr ar_prefix         ; with-world prefix to x
        lda ar_pw             ; baseline starts identical...
        sta ar_pb
        lda ar_iw
        sta ar_ib
        lda #0
        sta ar_vb
        lda ar_a1             ; ...then b0 applies to "with" only
        and #$80
        sta ar_vw
        lda ar_a1
        and #$10
        beq ar_rink
        lda ar_a1
        and #$07
        sta ar_pw
        jmp ar_rc
ar_rink lda ar_a1
        and #$07
        sta ar_iw
ar_rc   lda ar_pw             ; already converged -> next row
        cmp ar_pb
        bne ar_suf
        lda ar_iw
        cmp ar_ib
        bne ar_suf
        lda ar_vw
        cmp ar_vb
        bne ar_suf
        jmp ar_next
        ; suffix double walk, Y = x+1..39
ar_suf  ldx ar_x
        inx
        txa
        tay
ar_sl   cpy #40
        bcc ar_sl01
        jmp ar_next
ar_sl01:
ar_ld2  lda $ffff,Y           ; row[k], operand = &row[0] (per row)
        tax
        and #$60
        bne ar_px1
        jmp ar_sat
ar_px1: ; pixel byte: recolour check needs bit6 + divergent states
        txa
        and #$40
        bne ar_px2
        jmp ar_sn
ar_px2:
        lda ar_pw
        cmp ar_pb
        bne ar_cb
        lda ar_iw
        cmp ar_ib
        bne ar_cb
        lda ar_vw
        cmp ar_vb
        bne ar_cb
        jmp ar_next           ; pixel + converged -> break row
ar_cb   txa                   ; (b&$40) && states differ: compare block
        and #$80
        sta ar_bb
        cmp ar_vw
        beq ar_e0
        lda ar_pw             ; ew=1: lit uses with-paper/ink
        sta ar_a1
        lda ar_iw
        sta ar_a2
        jmp ar_eb
ar_e0   lda ar_iw             ; ew=0: lit swaps with-paper/ink
        sta ar_a1
        lda ar_pw
        sta ar_a2
ar_eb   lda ar_bb
        cmp ar_vb
        beq ar_eb0
        lda ar_pb
        sta ar_a3
        lda ar_ib
        sta ar_a4
        jmp ar_c1
ar_eb0  lda ar_ib
        sta ar_a3
        lda ar_pb
        sta ar_a4
ar_c1   txa                   ; cond1: lit bits !=0 and lw != lb
        and #$3F
        beq ar_c2
        lda ar_a1
        cmp ar_a3
        bne ar_ret1
ar_c2   txa                   ; cond2: unlit bits !=$3F and uw != ub
        and #$3F
        cmp #$3F
        beq ar_sn
        lda ar_a2
        cmp ar_a4
        bne ar_ret1
        jmp ar_sn
ar_sat  txa                   ; attribute: apply to BOTH states
        and #$80
        sta ar_vw
        sta ar_vb
        txa
        and #$10
        beq ar_sin
        txa
        and #$07
        sta ar_pw
        sta ar_pb
        jmp ar_sc
ar_sin  txa
        and #$07
        sta ar_iw
        sta ar_ib
ar_sc   lda ar_pw             ; re-converged -> break row
        cmp ar_pb
        bne ar_sn
        lda ar_iw
        cmp ar_ib
        bne ar_sn
        lda ar_vw
        cmp ar_vb
        bne ar_sn
        jmp ar_next
ar_sn   iny
        jmp ar_sl
ar_next inc ar_r
        lda ar_r
        cmp #8
        bcc ar_lp2
        lda #0
        rts
ar_lp2  jmp ar_loop
ar_ret1 lda #1
        rts

_gc_lok_asm
        stx ar_svx
        sty ar_svy
        jsr gclok_body
        ldx ar_svx
        ldy ar_svy
        rts
gclok_body
        lda _gc_gpl           ; expect operand = g+11
        clc
        adc #11
        sta ar_e1+1
        lda _gc_gph
        adc #0
        sta ar_e1+2
        lda _gc_gpl           ; y operand = g+1
        clc
        adc #1
        sta ar_g1l+1
        lda _gc_gph
        adc #0
        sta ar_g1l+2
        lda _gc_gpl           ; x operand = g+0
        sta ar_x2+1
        lda _gc_gph
        sta ar_x2+2
ar_x2   lda $ffff
        sta ar_x
        bne lok_go
        lda #1                ; x==0: edge always OK
        rts
lok_go  lda #0
        sta ar_r
lok_r   jsr ar_row
        jsr ar_prefix
        ldy ar_r
ar_e1   lda $ffff,Y           ; g->expect[r]
        sta ar_a1
        lsr
        lsr
        lsr
        lsr
        and #$0F
        cmp ar_pw
        bne lok_no
        lda ar_a1
        and #$07
        cmp ar_iw
        bne lok_no
        inc ar_r
        lda ar_r
        cmp #8
        bcc lok_rp
        lda #1
        rts
lok_rp  jmp lok_r
lok_no  lda #0
        rts

; column picker: seed&63 -> 0..39
shm_coltab:
        .byte  0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        .byte 10,11,12,13,14,15,16,17,18,19
        .byte 20,21,22,23,24,25,26,27,28,29
        .byte 30,31,32,33,34,35,36,37,38,39
        .byte  3,17,31, 8,22,36,13,27, 2,16
        .byte 30, 5,19,33, 8,22,36,11,25,39
        .byte  1,15,29, 7
