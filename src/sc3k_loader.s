; ****************************
; SCP/M Loader
; 
; This module is loaded by SF-7000 IPL to 0xFF00.
; It loads SCP/M BIOS to 0xEA00
;
; ****************************
.module LOADER


BIOS   = 0xEA00
HEADER = 0xFF00
LOADER = 0xFF20
STACK  = 0x8FFF

SECTS_SIZE	= 256
SECTS_PER_TRACK	= 16
BIOS_SIZE	= 4096
BIOS_SECTS	= BIOS_SIZE / SECTS_SIZE

.area 	HEADER (ABS)

;================================================
.org	HEADER
;head:
	.ascii "SYS:"
;title:
	.asciz " SC3K CPM 2.2  "
	.ascii "            "
;================================================
.org	LOADER
;init:
	di
	im  	1
	ld	sp, #STACK
    
    	call	0x08				; RESET FDC SYSTEM
    	jp	c, 0x0000			; IF ERROR - COLD BOOT (ROM IS STILL ENABLED)
   
	; 0xBC43
	; = track B + 0xC43
	; = track B + sector C + offset 0x43
	
	ld	de, #BIOS			; dest = 0x0000
;	ld	bc, #0x0101			; START SEC|TRK
	ld	bc, #0x0F00			; START SEC|TRK
	ld	l, #BIOS_SECTS

	; ========================================================================================

; ========================================================================================
read_set_from_disk_loop_n:
; ========================================================================================
	push	hl

;	call	read_from_disk
	call	0x10				; READ SECTOR, ROM API 0x10
	JP	C, 0x0000			; IF ERROR - COLD BOOT (ROM IS STILL ENABLED)

	pop	hl

	; ========================================================================================

	dec	l
	jr	z, load_exit
	
	;push	hl
	;ld	hl, #SECTS_SIZE
	;add	hl, de
	;ex	de, hl
	;pop	hl

	inc	d					; next 256 bytes

	inc	b						; next sector
	ld	a, b
	cp	#SECTS_PER_TRACK + 1						
	jr	nz, read_set_from_disk_loop_n

	ld	b, #1
	inc	c
	jr	read_set_from_disk_loop_n

	; ========================================================================================

load_exit:
;	ld	a, #0x03
;	out	(0xE7), a			; MOTOR OFF

	ld	a, #0x0D
	out	(0xE7), a			; DISABLE ROM

;	call	vdp_reset

	jp	BIOS


;vdp_reset::
;	ld	c, #0xBF			; vdp_control
;	in	a,(c)
;	in	a,(c)
	
;	ld	a, #0b00000000
;	out	(c),a
;	ld	a, #0x80
;	out	(c),a
;	ld	a, #0b10010000
;	out	(c),a
;	ld	a, #0x81
;	out	(c),a
	
;	in	a,(c)
;	ret


;#######################################
; Reads a sector (256B) from disk to memory
; API 0x10
;#######################################
; Parameters:
; de = destination buffer
; b = sector number on disk
; c = track number on disk
; Destroys af
; returns carry on error
; retries 40 times, re-seeking after 10 read errors


.area	_DATA
.area _SCB
