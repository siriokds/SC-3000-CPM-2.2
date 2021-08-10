COMMAND_COM:
	di
	im	1
	ld	sp, #0x0600							; Initial stack 

	ld	de, #BIOS_END
	ld	hl, #0xFFFF
	xor	a
	call	fill_mem

	ld	sp, #top_of_stack				; Final stack 

	call	BIOS_INIT

	ei

	ld	a, #ASC_CLS
	call	conio_write_char

	ld	hl, #BIOS_INFO
	call	conio_write_text
	

;=========================================================

cmd_el:
	ld	a, #">"
	call	conio_write_char

	call	GETLINE	

	ld	a, #ASC_LF
	call	conio_write_char

	jr	cmd_el



GETLINE:
	CALL	KEYB_WAITFORCHAR

	push	af
	CALL	conio_write_char
	pop	af

	CP	#13
	RET	Z

	jr	GETLINE



.include "sc3k_bios.s"

.area HEADER (ABS)
; ==================================================

; * KEYBOARD
.org	0x8000

CMDLINE:
	.ds	1
	.ds	127

.area	_DATA
.area _SCB

