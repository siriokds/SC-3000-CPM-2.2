vdp_data      		= 0xbe
vdp_control				= 0xbf
vdp_tilemap_addr	= 0x0800

fore_back_colors	= 0x1E   	;0xF4

font_temp_area		= 0x1000	; 1792 bytes


.area	_CODE
; ==================================================

;-------------------------------------------------------
vdp_videomode_text40::
;-------------------------------------------------------
    	ld     b, #0x02

setup_vdp_registers_2x:
	push   bc
	call   read_vdp_status				; not sure why?
	ld     c, #0x00
	ld     a, #0b00000000				; reg 0: no line ints, turn off features
	call   vdp_set_register
	inc    c
	ld     a, #0b11010000				; reg 1: display enable, mode 1
	call   vdp_set_register
	inc    c
	ld     a, #(vdp_tilemap_addr) >> 10		; reg 2: tilemap address
	call   vdp_set_register
	inc    c
	ld     a, #0x00					; reg 3: colour table address
	call   vdp_set_register
	inc    c
	ld     a, #0x03					; reg 4: pattern generator address
	call   vdp_set_register
	inc    c
	ld     a, #0x00					; reg 5: sprite table address
	call   vdp_set_register
	inc    c
	ld     a, #0x00					; reg 6: sprite tile number modifier (?)
	call   vdp_set_register
	inc    c
	ld     a, #fore_back_colors			; reg 7: backdrop color (1)
	call   vdp_set_register
	
	ld     hl, #0x1800
	call   set_vram_write_address_to_hl

;------------------------------------------------------      
	xor    a
	ld     b,a
setup_vdp_regs_zero256:    
	call   wait_and_output_a_to_vdp
	djnz   setup_vdp_regs_zero256              ; output 256 zero bytes at VRAM 0x1800
;------------------------------------------------------      

	ld	hl, #font_data+4
	ld	de, #font_temp_area
	
	call	unPack_1
	
	ld	hl, #font_temp_area
	ld	bc, #1792
	
      ;ld     hl, #font_data                        ; output the font after that
      ;ld     bc, #font_data_end - font_data

setup_vdp_regs_font:    
	ld     a,(hl)
	call   wait_and_output_a_to_vdp
	inc    hl
	dec    bc
	ld     a,b
	or     c
	jr     nz, setup_vdp_regs_font
	

	ld     hl, #0x0800
	ld     bc, #960
	call   set_vram_write_address_to_hl
setup_vdp_regs_names_0:    
	ld     a, #0x20                               ; set 960 bytes from VRAM 0x0800 to 0x20
	call   wait_and_output_a_to_vdp
	dec    bc
	ld     a, b
	or     c
	jr     nz, setup_vdp_regs_names_0
	pop    bc
	
	djnz   setup_vdp_registers_2x                ; do the whole thing twice - why?!?
	
	
	ld     c, #0x01
	ld     a, #0b11110000                         ; reg 1
	call   vdp_set_register
	

;##############################################
vdp_int_hack::
read_vdp_status::
;##############################################
	    in     a,(vdp_control)
	    ret



;##############################################
vdp_set_register::
;##############################################
; a = data
; c = register
; clobbers a
	out    (vdp_control),a
	ld     a,c
	and    #0x07
	or     #0x80
	out    (vdp_control),a
	ret


;##############################################
set_vram_read_address_to_hl::
;##############################################
	push   af
	ld     a,l
	out    (vdp_control),a
	ld     a,h
	and    #0x3f
	out    (vdp_control),a
	pop    af
	ret

wait_and_read_from_vdp::
	    nop
	    nop
	    nop
	    nop
	    nop
	    in     a, (vdp_data)
	    ret


set_vram_write_address_to_home::
      ld     hl, #vdp_tilemap_addr

set_vram_write_address_to_hl::
	    push   af
      ld     a,l
      out    (vdp_control),a
      ld     a,h
      and    #0x3f
      or     #0x40                                 ; make it a write address
      out    (vdp_control),a
	    pop    af
	    ret

wait_and_output_a_to_vdp::
	    nop
	    nop
	    nop
	    nop
	    out    (vdp_data),a
	    ret



; hl = src
; iy = dst
; bc = len

vdp_vram2ram:
			push	bc
			push	de
			push	iy
			
			call	set_vram_read_address_to_hl
			ex	(sp),ix
			ex	(sp),ix

vdp_vram2ram_loop:
	    in    a, (vdp_data)
      ld		(iy), a
      inc		iy
			dec		bc
			ld		a,c
			or		b
      jr		nz, vdp_vram2ram_loop
      
      pop		iy
      pop		de
      pop		bc
			ret




; iy = src
; hl = dst
; bc = len

vdp_ram2vram:
	push	bc
	push	de
	push	iy
	
	call	set_vram_write_address_to_hl
	ex	(sp),ix
	ex	(sp),ix

vdp_ram2vram_loop:
	ld	a, (iy)
	out	(vdp_data),a
	inc	iy
	dec	bc
	ld	a,c
	or	b
	jr	nz, vdp_ram2vram_loop
	
	pop	iy
	pop	de
	pop	bc
	ret



; hl = dst
; bc = len
;  a = byte filler

vdp_fill2vram:
	push	bc
	push	de
	
	push	af
	call	set_vram_write_address_to_hl
	pop	hl					; L = A
	
	ex	(sp),ix
	ex	(sp),ix

vdp_fill2vram_loop:
	ld	a, l
	out	(vdp_data),a
	dec	bc
	ld	a,c
	or	b
	jr	nz, vdp_fill2vram_loop
	
	pop	de
	pop	bc
	ret


; ix = vdp src address
; iy = vdp dst address
; bc = area len
vdp_vramShiftForward:
	push	bc
	push	de
	push	ix
	push	iy

	ld		a, b
	or		a
	jr		z, cpSX2					

	add		ix, bc
	add		iy, bc
	ld		de, #0xFF00				; BLOCK256 INCREMENTER	-256

cpSB2:
	push	bc

	add		ix, de
	add		iy, de

	ld		bc, #0x0100

	push	iy

	push	iy
	ld		iy, #vram_copy_buffer
	push	ix
	pop		hl
	call	vdp_vram2ram
	pop		iy

	push	iy
	pop		hl
	ld		iy, #vram_copy_buffer
	call	vdp_ram2vram

	pop		iy

	pop		bc


	dec		b
	jr		nz, cpSB2


cpSX2:
	ld		a, c
	or		a
	jr		z, cpSXX2

	xor		a
	sub		c
	ld		c, a
	ld		b, #0xFF
	add		ix, bc
	add		iy, bc
	
	call	vdp_vram2vram

cpSXX2:			
	pop		iy
	pop		ix
	pop		de
	pop		bc
	ret




; ix = vdp src address
; iy = vdp dst address
; bc = area len
vdp_vramShiftBack:
			push	bc
			push	de
			push	ix
			push	iy

			ld		de, #0x0100				; BLOCK256 INCREMENTER
cpSB1:
			ld		a, b
			or		a
			jr		z, cpSX1					; STILL 256 BLOCKS?
			
			push	bc
			ld		c, #0
			call	vdp_vram2vram
			pop		bc

			dec		b
			add		ix, de
			add		iy, de

			jr		cpSB1

			ld		a, c
			or		a
			jr		z, cpSXX1

cpSX1:
			call	vdp_vram2vram
			
cpSXX1:
			pop		iy
			pop		ix
			pop		de
			pop		bc
			ret



; ix = src
; iy = dst
;  c = len
vdp_vram2vram:
			push	bc
			push	de
			push	hl
			push	ix
			push	iy

			push	bc
			push	ix
			pop		hl
			call	set_vram_read_address_to_hl
			ld		de, #vram_copy_buffer
			ex		(sp),ix
			ex		(sp),ix
v2v_loop1:
	    in    a, (vdp_data)
      ld		(de), a
      inc		de
			dec		c
      jr		nz, v2v_loop1
			pop		bc

			push	bc
			push	iy
			pop		hl
			call	set_vram_write_address_to_hl
			ld		de, #vram_copy_buffer
			ex		(sp),ix
			ex		(sp),ix
v2v_loop2:
      ld		a, (de)
	    out   (vdp_data),a
      inc		de
			dec		c
      jr		nz, v2v_loop2
			pop		bc

			pop 	iy
			pop 	ix
			pop 	hl
			pop 	de
			pop		bc
			ret





font_data::
	.include "sc3k_bios_vdp_font_pck.inc"
font_data_end::

