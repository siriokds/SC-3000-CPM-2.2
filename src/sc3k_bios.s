;.area	_CODE
; ==================================================

; BIOS JUMP TABLE
; ===============
BIOS_BOOT:	
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT
;	JP	COLDBOOT

		
;BIOS_INFO:		
;	.ascii "* SEGA SC-3000 CP/M BIOS 1.0\r\n"
;	.ascii "  Copyright (c) SiRioKD 2012\r\n\r\n"
;	.db	0

; COLD BOOT - Entered on hard reset
; ---------------------------------
COLDBOOT:
	di
	im	1
	ld	sp, #0x0600					; Initial stack 

	ld	de, #BIOS_END
	ld	hl, #0xFFFF
	xor	a
	call	fill_mem

	ld	sp, #top_of_stack				; Final stack 

	call	BIOS_INIT

	ei

	ld	a, #ASC_CLS
	call	conio_write_char

;	ld	hl, #BIOS_INFO
;	call	conio_write_text
	
	ld	a, #">"
	call	conio_write_char

	;ld	a, #0xFF
	;call	TEXT_INSERT_MODE

;=========================================================

el:	
	call	KEYB_WAITFORCHAR
	call	conio_write_char
	jr	el



BIOS_INIT::
	call	psg_reset
	call  	vdp_videomode_text40
	call	ppi_init

	call	fdc_reset_and_spin
	call	kbd_reset
	call	conio_reset

	ld	a,#0xC3          			; put a JP opcode
	ld	(0x0038),a       			; and the opcode again
	ld	hl, #Int38h     			; int handling routine address
	ld	(0x0039),hl      			; put it as a JP argument

	ld	a, #0x00
	call	TEXT_INSERT_MODE
	ret



; =============================================================
fill_mem:
; =============================================================
	push	af
	push	bc
	push	de
	push	hl

	or	a		; clear CF
	sbc	hl, de		; HL -= DE
	ld	b, h
	ld	c, l		; BC = HL - DE
	
	ld	h, d
	ld	l, e
	inc	de
	
	ld	(hl), a
	ldir

	pop	hl
	pop	de
	pop	bc
	pop	af
	ret


; =============================================================
Int38h:
; =============================================================
	push    af
	push    bc
	push    de
	push    hl
	push    ix
	push    iy
	
	exx
	ex      af,af'
	
	push    af
	push    bc
	push    de
	push    hl
	push    ix
	push    iy

	call	vdp_int_hack


	LD      HL,(FRAMES1)	; Fetch the first two bytes at FRAMES1.
        INC     HL		; Increment lowest two bytes of counter.
        LD      (FRAMES1),HL	; Place back in FRAMES1.
        LD      A,H		; Test if the result was zero.
        OR      L		;            
        JR      NZ, NOINC3	; Forward, if not, to KEY-INT

	LD	HL, #FRAMES3
        INC     (HL)		; otherwise increment FRAMES3 the third byte.
NOINC3:


	call	FDC_INTERRUPT
	call	keyboard_interrupt

int_end:
	pop     iy
	pop     ix
	pop     hl
	pop     de
	pop     bc
	pop     af
	
	ex      af,af'
	exx
	
	pop     iy
	pop     ix
	pop     hl
	pop     de
	pop     bc
	pop     af
	ei
	reti



.include "sc3k_bios_bitbuster.inc"
.include "sc3k_bios_vdp.inc"
.include "sc3k_bios_conio.inc"
.include "sc3k_bios_text.inc"
.include "sc3k_bios_psg.inc"
.include "sc3k_bios_ppi.inc"
.include "sc3k_bios_kbd.inc"
.include "sc3k_bios_fdc.inc"


;BIOS_END_S:	.ascii "END."
BIOS_END:

.area HEADER (ABS)
; ==================================================

;* FDC
;.org	0xFA00


; * KEYBOARD
.org	0xFA80
; --- 92 bytes
fdc_command_loop_counter::	.ds 1    ; 0xfc00 ; used for counting retries on fdc comands
fdc_command_loop_counter_2::	.ds 1    ; 0xfc01 ; used for counting inner loops on fdc comands
fdc_command_data_buffer::	.ds 9  	; 0xfc02 ; 9 bytes .. 0xfc0a
fdc_command_result_buffer::	.ds 7	; 0xfc0b ; 0-7 bytes .. 0xfc11 depending on command
fdc_format_buffer::		.ds 64	; 0xfc12 ; 64 bytes .. 0xfc51

FDC_MOTOR_ACT:			.ds 1
FDC_MOTOR_CNT:			.ds 1
FDC_TIME:			.ds 2
FDC_TRK:			.ds 2
FDC_SEC:			.ds 2
FDC_CACHE_TAG:			.ds 2
FDC_BUFPTR:			.ds 2
FDC_DMAPTR:			.ds 2



; * KEYB & CURSOR
.org	0xFB00
; --- 53 bytes
KEYBD_BUFFER:			.ds 32		; <<< 256 BYTES ALIGNED!!!
KEYBD_PUTPNT:			.ds 2
KEYBD_GETPNT:			.ds 2
keyboard_input_buffer::        	.ds 12		; 9 bytes .. 0xfc5e - first 7 bytes are low bytes of rasters, 8th is high bits, 9th is remaining control keys
keyboard_last_char::		.ds 2
keyboard_typematic_cnt::	.ds 2
keyboard_diff_char::		.ds 1
keyboard_int_counter::		.ds 1

; --- 17 bytes
cursor_insert_mode::		.ds 1
cursor_enabled::		.ds 1
cursor_visible_ff::		.ds 1
cursor_char_saved::		.ds 1
cursor_char_data_saved::	.ds 8
write_char_value::		.ds 1		; char being written to the screen
cursor_visible_cnt::		.ds 2
cursor_x::                     	.ds 1		; cursor x position (0-39)
cursor_y::                     	.ds 1		; cursor y position (0-23)


.org	0xFC00
vram_copy_buffer::             	.ds 256		; <<< 256 BYTES ALIGNED!!!


; * STACK2 (256 bytes)
.org	0xFD00
DIRBF:				.ds	128	; 128-byte buffer
B_CSV:				.ds	16	; CKS bytes...
B_ALV:				.ds	40	; Allocation vector = 2 * ((DSM / 8) + 1) 
;saved_stack_ptr::		.ds 2
;temp_stack::                   	.ds 253	; 0xFD00-0xFEFF
;temp_top_of_stack::            	.ds 1

; * STACK1 (256 bytes)
.org	0xFE00
bios_stack::                    .ds 239	; 0xFD00-0xFFEF
top_of_stack::                 	.ds 1

FRAMES1:			.ds 2	; 0xFEF0
FRAMES3:			.ds 1	; 0xFEF2
bios_vars:			.ds 13	

.org	0xFF00
FDC_BUF_0::			.ds 128			; 128-byte buffer
FDC_BUF_1::			.ds 128			; 128-byte buffer


;BIOS_END:			.ascii "END]"

.area	_DATA
.area _SCB

