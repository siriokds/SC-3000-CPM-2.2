;############################################################à
; Bitbuster by Team Bomba
;############################################################à
; In: HL = source
;     DE = destination
;
unPack_1::
      exx
      ld			bc, #128        ; b' = 0 (register loading optimize)
              								; c' = bits from bitstream
      exx

unPack_loop_1:
      exx
      call    getBit_1
      exx
      jr      c,unPack_outCompress_1  ; if set, we got LZ77 compression

unPack_outLiteral_1:
      ldi                     ; copy byte from compressed data to destination
      jr      unPack_loop_1   ; handle more compressed data

unPack_outCompress_1:
      ld      a,(hl)          ; get lowest 7 bits of offset, plus the offset
                              ; extension bit
      inc     hl

      or      a
      jr      z,unPack_outRle_1       ; offset = 0, RLE compression used

unPack_outMatch_1:
      exx
      ld      e,a
      ld      d,b             ; b' should be always clear when entering this part
      rlca                    ; offset extension bit set?
      jr      nc,unPack_outMatch1_1   ; no need to get extra bits if carry not set

      call    getBit_1        ; get offset bit 10
      rl      d
      call    getBit_1        ; get offset bit 9
      rl      d
      call    getBit_1        ; get offset bit 8
      rl      d
      call    getBit_1        ; get offset bit 7

      jr      c,unPack_outMatch1_1    ; since extension mark already makes bit 7 set
      res     7,e             ; only clear it if the bit should be cleared
unPack_outMatch1_1:
      inc     de
      call    getGammaValue_0_1       ; get the match length
                              ; HL' = length

      push    hl              ; save compressed data pointer
      exx
      push    hl              ; save match length
      push    de              ; save match offset
      exx

      ld      h,d             ; destination in HL
      ld      l,e
      pop     bc              ; load match offset length
      sbc     hl,bc           ; calculate source address
      pop     bc              ; load match length
      ldir

      pop     hl              ; load compressed data pointer
      jr      unPack_loop_1

unPack_outRle_1:
      call    getGammaValue_1
      ret     c               ; HL' = repeat length

      push    hl              ; save compressed data pointer
      exx
      push    hl              ; save repeat length
      exx
      pop     bc              ; load repeat length

      ld      h,d             ; source = destination - 1
      ld      l,e
      dec     hl
      ldir

      pop     hl              								; load compressed data pointer
      jr      unPack_loop_1

getBit_1:
      sla     c               								; shift out new bit
      ret     nz              								; if remaining value != 0, we're done

      exx
      ld      a,(hl)          								; get 8 bits from the compressed stream
      inc     hl
      exx

      ld      c,a             								; 8 bits in C'
      sla     c               								; shift out new bit
      inc     c               								; set bit 0 so C' will be zero after shifting 8 times
      ret

getGammaValue_1:
      exx                     								; get number of bits used to encode value
getGammaValue_0_1:
      ld      hl,#1            								; initial length
      ld      b,#1             								; bitcount

getGammaValue_size_1:
      call    getBit_1        								; get more bits
      jr      nc,getGammaValue_sizeEnd_1      ; if bit is not set, bit length is known
      inc     b               								; increase bitcount
      jr      getGammaValue_size_1

getGammaValue_bits_1:
      call    getBit_1        								; get next bit of value from the compressed stream
      adc     hl,hl           								; insert new bit in HL
getGammaValue_sizeEnd_1:
      djnz    getGammaValue_bits_1            ; repeat if more bits to go

getGammaValue_end_1:
      inc     hl              								; correct HL (was stored as length - 2)
      exx
      ret
