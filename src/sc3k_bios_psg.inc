psg_port  = 0x7F

.area	_CODE
; ==================================================

psg_reset::
	ld     bc, #500                               	; repeat count
	
	psg_reset_once: 
;---------------------------
	ld     a, #0xff                                 ; pause: 4082 cycles = 1.14ms
pause_1ms_inner:
	dec   	a
	jr      nz, pause_1ms_inner
;---------------------------
	ld     a, #0x9f                                 ; PSG: silence all channels
	out    (psg_port),a
	ld     a, #0xbf
	out    (psg_port),a
	ld     a, #0xdf
	out    (psg_port),a
	ld     a, #0xff
	out    (psg_port),a
;---------------------------
	xor    a
	dec    bc
	ld     a,b
	or     c
	jr     nz, psg_reset_once                      ; repeat 1000 times - maybe to make it audible on error?
	ret



;---------------------------
psg_beep_1:
;---------------------------
    ld     a, #0x01
    jr     psg_beep

;---------------------------
psg_beep_2:
;---------------------------
    ld     a, #0x02
    jr     psg_beep

;---------------------------
psg_beep_3:
;---------------------------
    ld     a, #0x03
    ; fall through

;-------------------------------------------------------------------------------------------
; inputs: e = beep count
; never returns
; beeps are of the approximate form (eg. e=2)
; -__-__________________-__-__________________-__-___________
; 0s            1s            2s             3s            4s
; where 1 char is ~70ms, the beep section is variable length but the long pauses arent
;-------------------------------------------------------------------------------------------
psg_beep:
;-------------------------------------------------------------------------------------------
	ld     d,a                                   ; e -> d

psg_beep_loop:
	ld	a, #0b10001111				; PSG: ch 0 frequency = %0000111111 = 1775.57Hz
	out	(psg_port),a
	ld	a, #0b00000011
	out	(psg_port),a                            ; PSG: ch 0 volume = 0 = full
	ld	a, #0b10010000
	out	(psg_port),a

	ld	hl, #0x1efb
	call	psg_beep_pause

	ld	a, #0b10011111
	out	(psg_port),a

	ld	hl, #0x3dfb
	call	psg_beep_pause

	dec    d                                     ; repeat d times
	jr     nz, psg_beep_loop
	ret

psg_beep_pause:
	dec    hl
	ld     a,h
	or     l
	jr     nz, psg_beep_pause
	ret
