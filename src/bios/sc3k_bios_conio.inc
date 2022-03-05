; ASCII
ASC_BS   = 0x08
ASC_FF   = 0x0C
ASC_CR   = 0x0D
ASC_LF 	 = 0x0A
ASC_HOME = 0x0B
ASC_CLS  = 0x0C
ASC_SP   = 0x20

vdp_tilemap_addr	= 0x0800
vdp_tilemap_size	= (screen_columns * screen_rows)

vdp_tiledat_addr = 0x1800
vdp_tiledat_size = 2048

cursor_x_home		= 0
screen_columns	= 40

cursor_y_home		= 0
screen_rows     = 24

CURSOR_ASCII		= 144
;CURSOR_ASCII		= 153
.globl	write_char_value, cursor_x, cursor_y, vram_copy_buffer
.globl	cursor_enabled, cursor_visible_ff, cursor_visible_cnt, cursor_char_saved
.globl	vdp_vram2ram, vdp_vram2vram, vdp_vramShiftBack


;.area	_CODE
; ==================================================

conio_reset::
		call	TEXT_CURSOR_RESET
		ret


conio_write_text::
		call	TEXT_CURSOR_OFF
		push	de
		push	hl

write_text:
		ld	a, (hl)
		inc	hl
		or	a
		jr	z, write_text_exit

		push	hl
		call	conio_write_char_inner
		pop	hl
		
		jr	write_text

write_text_exit:

		pop	hl
		pop	de
		call	TEXT_CURSOR_ON
		ret


conio_write_char::
		or	a
		ret	z
		call	TEXT_CURSOR_OFF
		call	conio_write_char_inner
		call	TEXT_CURSOR_ON
		ret



		




;       process the 40 col control codes
;conio_control_codes:
;				cp			#0x07									; BELL
;				jp			z, conio_ctrl_end
;				cp			#0x08									; BACKSPACE
;				jp			z, conio_ctrl_end
;				cp			#0x09									; TAB
;				jp			z, conio_ctrl_end
;				cp			#0x0D									; ENTER
;				jp			z, conio_ctrl_end
;				cp			#0x1B									; ESC
;				jp			z, conio_ctrl_end
;conio_ctrl_end:
;				ret




;#######################################
; Write char
;#######################################
; writes char in a

cursor_incX_wrap:
			call	cursor_incX
			ret		nz
			ld    a, #cursor_x_home													; wrap cursor to position 2 on next line
      ld    (cursor_x),a
			ret

cursor_incX:
      ld		a,(cursor_x)
      inc		a
      ld		(cursor_x),a
      cp		#cursor_x_home + screen_columns
			ret


cursor_decX_wrap:
			call	cursor_decX
			ret		nz
      ld    a, #cursor_x_home + screen_columns - 1                ; cursor x = end of line
      ld    (cursor_x),a
			ret

cursor_decX:
      ld		a,(cursor_x)
      dec		a
      ld		(cursor_x),a
      cp		#cursor_x_home-1
			ret





cursor_incY_wrap:
			call	cursor_incY
			ret		nz
			ld    a, #cursor_y_home													; wrap cursor to position 2 on next line
      ld    (cursor_y),a
			ret

cursor_incY:
      ld		a,(cursor_y)
      inc		a
      ld		(cursor_y),a
      cp		#cursor_y_home + screen_rows
			ret


cursor_decY_wrap:
			call	cursor_decY
			ret		nz
      ld    a, #cursor_y_home + screen_rows - 1                ; cursor x = end of line
      ld    (cursor_y),a
			ret

cursor_decY:
      ld		a,(cursor_y)
      dec		a
      ld		(cursor_y),a
      cp		#cursor_y_home-1
			ret




;-------------------------------------------------------
conio_write_char_inner::
;-------------------------------------------------------
	    push   af
	    push   bc
	    push   de
	    push   hl
	    push   ix

      ld     (write_char_value),a

	cp     #0x1F
	jp     nc, VALID_CHAR                  ; do nothing for rest


;      cp     #ASC_LF                                ; handle control codes
;      jp     z, write_newline

      cp     #ASC_CR                                ; handle control codes
      jp     z, write_CR
      cp     #ASC_LF                                ; handle control codes
      jp     z, write_LF


 
      cp     #ASC_FF
      jp     z, write_formfeed
 
      cp     #ASC_BS
      jp     z, write_backspace
	
	cp	#ASC_HOME
	jp	z, TEXT_CURSOR_HOME
  
      cp     #ASC_CLS
      jp     z, write_formfeed

	cp	#0x1C									; CURSOR RIGHT
	jp	z, write_cursor_right
	
	cp	#0x1D									; CURSOR LEFT
	jp	z, write_cursor_left
	
	cp	#0x1E									; CURSOR UP
	jp	z, write_cursor_up
	
	cp	#0x1F									; CURSOR DOWN
	jp	z, write_cursor_down

	cp     #SP                                 ; make sure char is in ASCII range (32<=n<128)
	jp     c, write_char_exit
	
;	cp     #0x80
;	jp     nc, write_char_exit                  ; do nothing for rest


	jp     write_char_exit                  ; do nothing for rest

VALID_CHAR:
	call   calc_cursor_address
	call   set_vram_write_address_to_hl
			
	ld	a, (cursor_insert_mode)
	or	a
	jr	z, no_insert_mode
	
	ld	de, #160
	call	insert_at_curr_address
			
      	call   set_vram_write_address_to_hl
no_insert_mode:

	ld	a,(write_char_value)                ; write value
	call	wait_and_output_a_to_vdp

	call	cursor_incX_wrap
	jp	nz, write_char_exit


;-------------------------------------------------------
write_CR:
;-------------------------------------------------------
	ld	a, #cursor_x_home
      	ld	(cursor_x),a
	jp	write_char_exit


;-------------------------------------------------------
write_newline: 																	
;-------------------------------------------------------
	ld	a, #cursor_x_home																			; wrap cursor to position 2 on next line
      	ld	(cursor_x),a

;-------------------------------------------------------
write_LF:
;-------------------------------------------------------
	call	cursor_incY
	jp	nz, write_char_exit

write_newline_scroll_screen:
	call	screen_scroll_up
	ld	a, #cursor_y_home+screen_rows-1		; put cursor at row 23
	ld	(cursor_y),a
	jp	write_char_exit

write_formfeed: ; clear screen, reset cursor
	call	TEXT_CURSOR_HOME
	call	TEXT_CLEAR_SCREEN
	jp	write_char_exit


write_cursor_left:
			call	 cursor_decX_wrap
      jr		 nz, write_char_exit

write_cursor_up:
			call	 cursor_decY_wrap
			jr		 write_char_exit

write_cursor_right:
			call	 cursor_incX_wrap
			jp		 nz, write_char_exit

write_cursor_down:
			call	 cursor_incY_wrap
			jr		 nz, write_char_exit

			ld		 a, #0
			ld     (cursor_y),a

			jr		 write_char_exit



write_backspace:
			call	 cursor_decX
      jr		 nz, delete_char_from_tilemap

write_backspace_wrap_to_previous_line:
      ld     a,(cursor_y)                        ; move cursor to end of prevous line
      dec    a
      ld     (cursor_y),a
      cp     #-1                                  ; check if its -1
      ld     a, #screen_columns-1                  ; cursor x = end of line
      ld     (cursor_x),a
      jr     nz, delete_char_from_tilemap         ; if it wasnt -1 then were done
      
write_backspace_reached_top_left:
			call		TEXT_CURSOR_HOME
      jr     	write_char_exit                     ; nothing to delete

delete_char_from_tilemap:
			call	 calc_cursor_address
      call   set_vram_write_address_to_hl
      ld     a, #SP                               ; write space = blank
      call   wait_and_output_a_to_vdp

write_char_exit:
	    pop    ix
	    pop    hl
	    pop    de
	    pop    bc
	    pop    af
	    ret


; hl = vram address
; de = len of area to move
;-------------------------------------------------------
insert_at_curr_address:
;-------------------------------------------------------
			push		bc
			push		de
			push		hl
			push		ix
			push		iy
			
			push		hl
			push		hl
			pop			ix						; IX = VRAM PTR
			pop			iy
			inc			iy						; IY = VRAM PTR + 1


			add			hl, de
			ex			de, hl
			ld			hl, #vdp_tilemap_addr + vdp_tilemap_size
			or			a
			sbc			hl, de																					; HL = VRAMEND - (CURSPTR+160)
			jp			p, noAddRow
			
			call		cursor_decY																			; if (CURSPTR+160 > VRAM END) { CURSY--; SCRY-- VSRC-=160; VDST-=160; }
			push		ix
			push		iy
			call	 	screen_scroll_up
			pop			iy
			pop			ix


			ld			hl, #0
			xor			a
			sbc			hl, de																					; HL = -DE
			ex			de, hl

			add			ix, de
			add			iy, de
noAddRow:
			ld			c, e
			call		vdp_vram2vram																		; *VDST = *VSRC

			pop			iy
			pop			ix
			pop			hl
			pop			de
			pop			bc
			ret



screen_scroll_up:
      ld     	ix, #vdp_tilemap_addr + screen_columns  	; from second row
      ld     	iy, #vdp_tilemap_addr                   	; copy to start of tilemap
			ld			bc, #screen_columns * (screen_rows - 1)		; 40*23 rows back 1 row
			call		vdp_vramShiftBack

;-------------------------------------------------------
clear_line_at_current_address:
;-------------------------------------------------------
      ld     b,#10                                			; loop 10 times (unrolled by 4)
      ld     a,#SP                               				; write a space
clr_line_loop:    
			call   wait_and_output_a_to_vdp
      call   wait_and_output_a_to_vdp
      call   wait_and_output_a_to_vdp
      call   wait_and_output_a_to_vdp
      djnz   clr_line_loop
			ret





;-------------------------------------------------------
calc_cursor_address:
;-------------------------------------------------------
	ld	hl, #cursor_x
	call	TEXT_COORDS_TO_PTR
	ret

