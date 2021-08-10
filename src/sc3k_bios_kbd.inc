.include	"sc3k_bios_kbd.HAL.inc"			; // KEYB_READ_MATRIX, KEYB_READ_RAWDATA
.include	"sc3k_bios_kbd.ASCII.inc"		; // KEYB_READ_ASCII


.globl	keyboard_input_buffer, keyboard_last_char, keyboard_typematic_cnt

;#######################################
kbd_reset::
;#######################################
	push	af
	xor	a
	ld	(keyboard_last_char), a
	ld	(keyboard_last_char+1), a
	ld	(keyboard_int_counter), a		
	inc	a
	ld	(keyboard_typematic_cnt), a

	ld	hl, #KEYBD_BUFFER
	ld	(KEYBD_PUTPNT), hl
	ld	(KEYBD_GETPNT), hl
	pop	af
	ret




KEYB_BUFTEST:
	ld	hl,(KEYBD_PUTPNT)
	ld	de,(KEYBD_GETPNT)
CP_HLDE:
	ld      a,h
	cp      d
	ret     nz
	ld      a,l
	cp      e
	ret



KEYB_WRAP_PTR:
        ld      a,l
        cp      #32
        ret     nz
	ld	hl, #KEYBD_BUFFER
	ret


;===========================================================
;===========================================================
keyboard_interrupt::
;===========================================================
;===========================================================
;	ld	a, (keyboard_int_counter)
;	inc	a
;	cp	#3
;	jr	nz, keyb_int

;	xor	a
;	ld	(keyboard_int_counter), a
;	ret
;keyb_int:
	
	call	KEYB_READ_MATRIX				; Update Matrix EVERY INT
	call	KEYB_READ_ASCII					; Get ASCII Char


	ld	hl, #keyboard_typematic_cnt

	ld	a, (keyboard_diff_char)
	or	a
	jr	z, noResetTypeMatic
	xor	a
	ld	(hl), a
noResetTypeMatic:
	ld	a, (hl)
	or	a
	jr	z, notypematic
	dec	a
	ld	(hl), a
	ret
notypematic:
	call	KEYB_BUFTEST
	ret	nz						; if HL != DE, buffer is NOT EMPTY, exit!


	
	ld	a, (keyboard_last_char)
	or	a
	ret	z

	call	KEYB_BUFFERPUTCHAR

; SET NEW TYPEMATIC VAL
	ld	b, #3
	ld	a, (keyboard_diff_char)
	or	a
	jr	z, tm_samechar

	ld	b, #15
tm_samechar:
	ld	a, b	
	ld	(keyboard_typematic_cnt), a

	ret



; IN: A = NEW CHAR
;===============================================	
KEYB_BUFFERPUTCHAR::
;===============================================	
	
        ld      hl,(KEYBD_PUTPNT)
        ld      (hl),a
        inc     hl

	call	KEYB_WRAP_PTR

        push    de
        ld      de,(KEYBD_GETPNT)
	call	CP_HLDE	
        pop     de
        ret     z

        ld      (KEYBD_PUTPNT),hl
        ret




; OUT: A = NEW CHAR

;===============================================	
KEYB_WAITFORCHAR::
;===============================================	
	call	KEYB_BUFTEST		; Test buffer for a KEY
        jr      nz, newKeyInBuffer
        ei				; Ensure INT is enabled
        halt				; Stop CPU and wait for next INT
        jr      KEYB_WAITFORCHAR	; Retry test
        
newKeyInBuffer:
        ld      hl, (KEYBD_GETPNT)
	ld	d, h
	ld	e, l			; save old GET buffer pointer into DE

        inc     hl			;\
	call	KEYB_WRAP_PTR		; > Increment GET buffer pointer and wrap around buffer size
        ld      (KEYBD_GETPNT),hl	;/

	ld	a, (de)			; get KEY from old GET buffer pointer

;>>>	or	a
;>>>	jr	z, KEYB_WAITFORCHAR	; test for ZERO (useful? buffer contains only NO-ZERO values) 
	ret


