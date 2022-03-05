
;-------------------------------------------------------
TEXT_INSERT_ON::
;-------------------------------------------------------
		ld		a, #0xFF
		jr		TEXT_INSERT_MODE
;-------------------------------------------------------
TEXT_INSERT_OFF::
;-------------------------------------------------------
		xor		a
;-------------------------------------------------------
TEXT_INSERT_MODE::
;-------------------------------------------------------
		ld		(cursor_insert_mode), a
		ret



TEXT_PUTCH::
	push	af
	call	calc_cursor_address						; 1) Calc VRAM address
	call	set_vram_write_address_to_hl		; 2) Set WRITE address
	pop	af
	call   	wait_and_output_a_to_vdp				; 3) Write it
	ret


;-------------------------------------------------------
TEXT_CURSOR_ON::
;-------------------------------------------------------
	push	af
	
	call	TEXT_CURSOR_PTR									; 1) Calc VRAM address
	push	hl
	
	
	call	set_vram_read_address_to_hl			; 2) Set READ address
	call	wait_and_read_from_vdp					; 3) Read char value from Screen
	ld	(cursor_char_saved), a					; 4) Save it
	
	ld	h, #0
	ld	l, a
	add	hl, hl													; * 2
	add	hl, hl													; * 4
	add	hl, hl													; * 8
	
	ld	de, #vdp_tiledat_addr
	add	hl, de													; HL = vdp_tiledat_addr + (CHAR * 8)
	
	ld	iy, #cursor_char_data_saved
	ld	bc, #8
	call	vdp_vram2ram										; save char data
	
	
	; Copy saved char DATA to CharCODE 255
	ld		hl, #vdp_tiledat_addr + vdp_tiledat_size - 8			; Point to charcode 255
	call	set_vram_write_address_to_hl
	
	ld		hl, #cursor_char_data_saved
	ld		b, #8
invert_loop:
	ld		a, (hl)
	cpl
	inc		hl
	call  wait_and_output_a_to_vdp
	dec		b
	jr		nz, invert_loop
	
	
	;ld		 a, #CURSOR_ASCII								; 5) Set <CURSOR_ASCII> char as cursor
	ld		 a, #0xFF												; 5) Set 255 char as cursor
	pop		hl															
	call   set_vram_write_address_to_hl		; 6) Set WRITE address
	call   wait_and_output_a_to_vdp				; 7) Write it
	
	pop		af
	ret
	

;-------------------------------------------------------
TEXT_CURSOR_RESET:
;-------------------------------------------------------
			ld		a, #SP
			ld		(cursor_char_saved), a
			xor		a
			ld		(cursor_visible_ff), a
			
			call	TEXT_CURSOR_HOME
			call	TEXT_INSERT_ON
			call	TEXT_CURSOR_ON
			ret

;-------------------------------------------------------
TEXT_CURSOR_OFF::
;-------------------------------------------------------
	push	af
	push	hl
			
TEXT_CURSOR_OFF_IN:
	call	calc_cursor_address			; 1) Calc VRAM address

	call	set_vram_write_address_to_hl		; 2) Set WRITE address
	ld	a, (cursor_char_saved)			; 3) Set <cursor_char_saved> as cursor
	call	wait_and_output_a_to_vdp		; 4) Write it

	pop	hl
	pop	af
	ret



;-------------------------------------------------------
TEXT_CURSOR_HOME::
;-------------------------------------------------------
			push	af

			xor		a
	    ld    (cursor_y),a

	    ld    a, #cursor_x_home
	    ld    (cursor_x),a
			pop		af

			ret

;-------------------------------------------------------
TEXT_CLEAR_SCREEN:
;-------------------------------------------------------
      push		af
      push		bc
      push		hl

      call   	set_vram_write_address_to_home

      ld     	bc, #(screen_columns * screen_rows) / 4                ; fill tilemap with spaces
clear_screen_loop:

			ld     	a, #SP
      call   	wait_and_output_a_to_vdp
      call   	wait_and_output_a_to_vdp
      call   	wait_and_output_a_to_vdp
      call   	wait_and_output_a_to_vdp
      dec    	bc
      ld     	a,b
      or     	c
      jr     	nz,clear_screen_loop

			pop			hl
			pop			bc
			pop			af
			ret



; OUT: HL = VRAM PTR
;-------------------------------------------------------
TEXT_CURSOR_PTR:
			ld		hl, #cursor_x
			; FALL DOWN...		

;  IN: HL = COORDS (YX)
; OUT: HL = VRAM PTR
;-------------------------------------------------------
TEXT_COORDS_TO_PTR:
;-------------------------------------------------------
      ; calculate tilemap address of cursor and write a space there
      ; address = cursor_y * 40 + cursor_x + tilemap_address

	push		af
	push		bc
	push		de
	push		ix
	
	push		hl
	pop		ix

	ld     	hl,#0
	ld     	de, #screen_columns
	ld     	a, 1(IX)													; Y
	ld     	b, a
	inc    	b
calc_coords_address_loop:
	dec    	b
	jr     	z, calc_coords_address_exit
	add    	hl, de
	jr     	calc_coords_address_loop

calc_coords_address_exit:    
	ld     	a, 0(IX)
	ld     	d, #0x00
	ld     	e, a
	add    	hl, de
	ld     	de, #vdp_tilemap_addr
	add    	hl, de
      
      pop			ix
      pop    	de
      pop			bc
      pop			af
			ret		 


