.include "sc3k_bios_fdc.HAL.inc"

;--------------------------------------------------------------
;--------------------------------------------------------------
FDC_INTERRUPT:
;--------------------------------------------------------------
;--------------------------------------------------------------
;       if the drive is active, then skip the motor timing routine

	LD	A,(FDC_MOTOR_ACT)       ;drive activity flag
        OR	A               	;is any drive active?
        RET	NZ

        LD      A,(FDC_MOTOR_CNT)	;get count to turn drive off
        OR      A               	;is it zero?
        RET	Z

        DEC     A               	;dec counter
        JR      NZ, FDC_INT_X        	;jump if not zero
	call	fdc_stop_disk
        XOR     A               	;A=0
FDC_INT_X:  
	LD      (FDC_MOTOR_CNT),A       ;store in variable
	RET


FDC_RESET_AND_SPIN:
	call	fdc_motor_req_on	; FORCE MOTOR TO STAY ON
	jr	FDC_RESET2
	
;-----------------------------------
FDC_RESET::
;-----------------------------------
	call	fdc_stop_disk
	call	sf7k_reset

FDC_RESET2::
	xor	a
	ld	(FDC_MOTOR_CNT), a
	ld	(FDC_MOTOR_ACT), a

	ld	l, a
	ld	h, a
	ld	(FDC_TRK), hl
	ld	(FDC_SEC), hl
	dec	hl
	ld	(FDC_CACHE_TAG), hl				; DEFAULT CACHE TAG = 0xFFFF
	
	ld	hl, #FDC_BUF_0
	ld	(FDC_BUFPTR), hl

	ld	hl, #0x0080
	ld	(FDC_DMAPTR), hl
	ret

;-----------------------------------
FDC_HOME::	
;-----------------------------------
	LD	bc, #0				; Select track zero
;-----------------------------------
FDC_SETTRK::
;-----------------------------------
	ld	(FDC_TRK), bc			; Store track
	ret
;-----------------------------------
FDC_SETSEC::
;-----------------------------------
	ld	(FDC_SEC), bc			; Store sector
	ret
;-----------------------------------
FDC_SETDMA::
;-----------------------------------
	ld	(FDC_DMAPTR),bc			; Set DMA address
	ret

		

; ================================================
FDC_READ::	
; ================================================
	call	FDC_TRY_READ_CACHE
	call	FDC_COPY_TO_DMA
	xor	a						; No checking for errors!
	ret

; ================================================
FDC_WRITE::	
; ================================================
	call	FDC_TRY_READ_CACHE
	call	FDC_COPY_FROM_DMA				; update data to buffer
		
	ld	bc, (FDC_CACHE_TAG)	
	inc	b						; BC  = SEC|TRK	(SEC: 0-15 => 1-16)
	ld	de, (FDC_BUFPTR)
	
	di
	call	fdc_writesector					; write it to floppy!
	ei

	xor	a						; No checking for errors!
	ret




;-----------------------------------
FDC_COPY_TO_DMA:
;-----------------------------------
	ld	hl,(FDC_DMAPTR)					; LOAD HL WITH DMA ADDRESS
	ex	de, hl
	ld	hl,(FDC_BUFPTR)					; GET RAM ADDRESS TO COPY
	jr	copy_128_bytes
;-----------------------------------
FDC_COPY_FROM_DMA:
;-----------------------------------
	ld	hl,(FDC_BUFPTR)					; LOAD HL WITH DMA ADDRESS
	ex	de, hl
	ld	hl,(FDC_DMAPTR)					; GET RAM ADDRESS TO COPY
copy_128_bytes:
	ld	BC,#128						; BC IS COUNTER FOR FIXED SIZE TRANSFER (128 BYTES)
	ldir
	ret



; 1 2 3 4 5 6 7 8 9 A B C D E F G
; 0 1 2 3 4 5 6 7 8 9 A B C D E F
; 0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7 
; 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 
		
		
; ================================================
FDC_TRY_READ_CACHE:
; ================================================
	; *** build tag
	ld	a,(FDC_TRK)
	ld	c, a				; c = TRACK

	ld	de, #FDC_BUF_0
	ld	a,(FDC_SEC)			; CPM SECTOR (1-16)
	dec	a				; TRANSFORM 1-16 => 0-15
	srl	a				; sec >>= 1, CF = sec & 1
	jr	nc, IDE_SEL_BLOCK_0

	ld	de, #FDC_BUF_1			; if (CF) PHYBUFFER = PHYBUF_1;

IDE_SEL_BLOCK_0: 
	ld	(FDC_BUFPTR),de			; BUFFER ADDRESS
	ld	b, a				; b = SECTOR - 1 (0-15)
	
	; *** compare tags
	ld	hl, (FDC_CACHE_TAG)		; get old tag
	ld	(FDC_CACHE_TAG),bc		; store new tag
	or	a				; clear CF
	sbc	hl,bc
	ret	z				; IS ALREADY IN CACHE, DO NOTHING
	
	; *** read from floppy
	inc	b				; BC  = SEC|TRK	(SEC: 0-15 => 1-16)
	ld	de, #FDC_BUF_0			; DST = read 256 bytes from disk

	di
	call	fdc_readsector
	ei

	ret

		
