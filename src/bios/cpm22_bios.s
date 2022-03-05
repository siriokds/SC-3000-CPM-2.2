; ==================================================
; MEMORY
; ------
RESET	= 0x0000	; Reset entry point
BDOS	= 0x0005	; BDOS entry point
FCB	= 0x005C	; Default FCB

IOBYTE	= 0x0003
CCP	= 0xD300	; 0xD300 = CCP start 	(0xD300 - 0xDB06)	2054 bytes	= 18 sectors
BDOS0	= 0xDC00	; 0xDC00 = BDOS start	(0xDC00 - 0xEA00)	3584 bytes	= 28 sectors
BIOS	= 0xEA00	; 0xEA00 = BIOS start	(0xEA00 - 0xFC00)	4096 bytes

STACK 	= 0x8FFF


; I/O
; ---
STATUS	=	0x00		; 6850 status register
CONTROL	=	0x00		; 6850 control register
DATA	=	0x01		; 6850 data register



; BIOS JUMP TABLE
; ===============
BOOT:	JP	JBOOT
WBOOT:	JP	JWBOOT
CONST:	JP	JCONST
CONIN:	JP	JCONIN
CONOUT:	JP	JCONOUT
LIST:	JP	JLST
PUNCH:	JP	JPUNCH
READER:	JP	JREADER

TRACK0: JP	FDC_HOME

SELDSK:	JP	JSELDSK

SETTRK:	JP	FDC_SETTRK
SETSEC:	JP	FDC_SETSEC
SETDMA:	JP	FDC_SETDMA
READ:	JP	FDC_READ
WRITE:	JP	FDC_WRITE

PRSTAT:	JP	JPRSTAT
SECTRN:	JP	JSECTRN

; INITIAL ZERO-PAGE CONTENTS
; --------------------------
BASE:	
	JP	WBOOT		; Jump to BIOS to Warm Boot			(3 bytes)		0-1-2
	.dw	0x0000		; IOBYTE, DRIVE					(2 bytes)		3-4
	JP	BDOS0+6		; BDOS function call entry			(3 bytes)		5-6-7

;---------------------------------------------   8 bytes
OS_INFO:  
	.db	0x0C	;ctrl L -> CLS
	;        0123456789012345678901234567890123456789
	;       |                                        |
        .ascii	"SEGA SC-3000 CP/M-80 (P2DOS 2.3)\r\n"
        .ascii  "Copyright (c) by Digital Research\r\n\r\n"
	.db	0



BDOS_LOADED:
	.ascii " BDOS Loaded."
	.db	0

CCP_LOADED:
	.ascii "\r\n* CCP  Loaded.\r\n"
	.db	0


; COLD BOOT - Entered on hard reset
; ---------------------------------
JBOOT:	
	di
	im	1
	ld	sp, #STACK

	ld	de, #BIOS_END
	ld	hl, #0xFFFF
	xor	a
	call	fill_mem

	LD	HL,#BASE
	LD	DE,#0x0000
	LD	BC,#8
	LDIR

	ld	a, #0xC9
	ld	(0x0066), a

	call	BIOS_INIT
	

	ld	a, #ASC_CLS
	call	conio_write_char

	;ld	hl, #BIOS_INFO
	ld	hl, #OS_INFO
	call	conio_write_text

	
;=========================================================
	ld	a, #">"
	call	conio_write_char

	ld	de, #BDOS0-128		; dest = 0xDC00
	;ld	bc, #0x0200		; START SEC|TRK
	ld	bc, #0x0100		; START SEC|TRK
	ld	l, #14			; 3584 / 256
	call	fdc_readsectors

	call	fdc_motor_req_on	; FORCE MOTOR TO STAY ON


	ld	hl, #BDOS_LOADED
	call	conio_write_text

;	ld	hl, #CCP_LOADED
;	call	conio_write_text
;	call	fdc_stop_disk

;	jp	0
  
  
; WARM BOOT - soft reset
; ---------------------------------
JWBOOT:
	ld	sp, #top_of_stack
  	call	FDC_RESET


	ld	de, #CCP		; dest = 0xD300
	ld	bc, #0x0102		; START SEC|TRK
;	ld	bc, #0x0F01		; START SEC|TRK
	ld	l, #8
	call	fdc_readsectors


	;LD	bc, #0x0080		; Default DMA address 
	ld	bc, #DIRBF
	call	FDC_SETDMA

	
	ld	hl, #CCP_LOADED
	call	conio_write_text

	ei
	call	psg_beep_1

	xor	a
	ld	c, a			; C = uuuudddd (u = user, d = drive no)
	ld	b, a
	JP	CCP			; Start CP/M by jumping to the CCP.
		


; CONSOLE I/O VIA A 6850 SERIAL ACIA
; ==================================
JCONST:	
	call	KEYB_BUFTEST
	ld	a, #0xFF
	jr	nz, j_nokey
	xor	a
j_nokey:
	ret


JCONIN:
	call	KEYB_WAITFORCHAR
	ret

JCONOUT:
	;out (0xd2), a
	di
	call	conio_write_char
	ei
	ret
		
JREADER:
	LD	a,#0x1a				; EOF and RETurn.
	ret
JSECTRN:
	ld  	h, b				; pass the logical
	ld	l, c       			; sector to HL
	inc	hl				; convert to phisical
JLST:   
JPUNCH: 
JPRSTAT:
	xor	a
	ret


; DISK ACCESS
; ===========
JSELDSK:
	call	FDC_RESET_AND_SPIN
	ld	hl,#DPH				; Return same DPH
	ret

DPH:	
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw 0x0000
	.dw DIRBF		; 128-byte buffer
	.dw B_DPB
	.dw B_CSV
	.dw B_ALV

B_DPB:	
; http://www.sharpmz.org/dpb.htm
; 256 bytes/sector, 16 sectors/track, 40 tracks, 1 heads
	.dw 32      ; (SPT) LSECTORS PER TRACK (32 logical-sectors * 128 bytes = 4096 bytes/track)
	
	.DB 3       ; (BSH) BLOCK SHIFT FACTOR; block size => 1024
	.DB 7       ; (BLM) BLOCK MASK
	
	.DB 0       ; (EXM) NULL MASK
	.dw 159     ; (DSM) DISK SIZE-1					Total bytes / block size = 160
	.dw 63      ; (DRM) DIRECTORY MASK = DIR ENTRIES - 1

	.DB 0xC0    ; (AL0) ALLOC 0
	.DB 0x00    ; (AL1) ALLOC 1		; 0xC000 => 1100 0000 0000 0000 (first 2 bit => first 2 block reserved for dir => 2048)
	.dw 16      ; (CKS) CHECK AREA SIZE	; Number of directory entries to check for disk change
	.dw 3       ; (OFF) TRACK OFFSET	; Number of system reserved tracks at the beginning of the ( logical ) disk

;DIRBF:	.ds	128	; 128-byte buffer
;B_CSV:	.ds	16	; CKS bytes...
;B_ALV:	.ds	40	; Allocation vector = 2 * ((DSM / 8) + 1) 


.include "sc3k_bios.s"


