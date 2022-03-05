
sc_ppi_a                      = 0xdc
sc_ppi_b                      = 0xdd
sc_ppi_c                      = 0xde
sc_ppi_control                = 0xdf

sf7_ppi_a                     = 0xe4       ; (FDC, printer) status (input)
sf7_ppi_b                     = 0xe5       ; Printer port output - unused
sf7_ppi_c                     = 0xe6       ; (FDD, FDC, ROM mapping, printer) control (output)
sf7_ppi_control               = 0xe7



.area	_CODE
; ==================================================


; SC-3000
;===============================================================
; PORT           DIR     DEVICE          DESCR
;===============================================================
; PortA(7..0) = (Input)  KEYBOARD        DataIn

; PortB(7)		= (Input)  TAPE            DataIn
; PortB(6)		= (Input)  PRINTER         BUSY 
; PortB(5)		= (Input)  PRINTER         FAULT  (serial printer SP-400) 
; PortB(4)		= (Input)                  /CONT  (to the external terminal B-11) 
; PortB(3..0) = (Input)  KEYBOARD        DataIn

; PortC(7)		= (Output) PRINTER         /FEED  (serial printer SP-400)
; PortC(6)		= (Output) PRINTER         /RESET (serial printer SP-400) 
; PortC(5)		= (Output) PRINTER         Data 
; PortC(4)		= (Output) TAPE            DataOut 
; PortC(3)		= (Output) ...             Not Connected

; PortC(2..0) = (Output) KEYBOARD        Row Select


; SF-7000
;===============================================================
; PORT           DIR     DEVICE          DESCR
;===============================================================
; PortA(7..3) = (Input)  ...             Not Connected
; PortA(2)		= (Input)  FDC765          Pin 17 of the FDC ?
; PortA(1)		= (Input)  CENTRONICS PRN  BUSY
; PortA(0)		= (Input)  FDC             INT signal from input from FDC 

; PortB(7..0)	= (Output) CENTRONICS PRN  DataOut

; PortC(0)		= (Output) FDC             /INUSE signal to FDD 
; PortC(1)		= (Output) FDC             /MOTOR ON signal to FDD 
; PortC(2)		= (Output) FDC             TC signal to FDD 
; PortC(3)		= (Output) FDC             RESET signal to FDC 
; PortC(4)		= (Output) ...             Not Connected
; PortC(5)		= (Output) ...             Not Connected
; PortC(6)		= (Output) ROM             /ROMSEL: Switching between IPL ROM(0) and RAM(1)
; PortC(7)		= (Output) CENTRONICS PRN  /STROBE


ppi_init:
	call	sc3000_ppi_init
	jr	sf7000_ppi_init
	;call	sf7000_ppi_init
	;ret

		
PPI_PAUSE:  
	push    bc
ppi_p1_lp:  
	dec     c
        jp      nz, ppi_p1_lp
        nop     
        pop     bc
        ret     

; de = cycles
PPI_LONG_PAUSE:  
				ld      c, #0xFF
        call    PPI_PAUSE
        dec     de
        ld      a,d
        or      e
        jr      nz, PPI_LONG_PAUSE
        ret

;-------------------------------------
sc3000_ppi_init:
;-------------------------------------

	; * SC3000 PPI PORT SET
	; ---------------------------------------
	ld     a, #0b10010010                           				; 0x92; initialise SC PPI: set I/O to mode 0, A+B in, C out
	;            ||/||||`---- PortCL Dir   = CL   Output
	;            || |||`----- PortB  Dir   = B    Input
	;            || ||`------ PortB/PortCL = B/CL Mode 0
	;            || |`------- PortCH Dir   = CH   Output
	;						 || `-------- PortA Dir    = A    Input
	;            |`---------- PortA/PortCH = A/CH Mode 0
	;            `----------- 8255 CONFIG(1)
	out    (sc_ppi_control),a

		
	; * SC3000 PPI INIT
	; ---------------------------------------
        ld      a,#0x0C			; X000 1100  =>  bit(00110) => PortC(6) = 0		; Printer /RESET (ON)
        out     (sc_ppi_control),a		
        
        ld      a,#0x0F			; X000 1111  =>  bit(00111) => PortC(7) = 1		; Printer /FEED (OFF)
        out     (sc_ppi_control),a
        
        ld      a,#0x0A			; X000 1010  =>  bit(00101) => PortC(5) = 0		; Printer DATA (ZERO)
        out     (sc_ppi_control),a


        ld      de, #0x0064
        call	PPI_LONG_PAUSE

        ld      a,#0x0D			; X000 1101  =>  bit(00110) => PortC(6) = 1		; Printer /RESET (OFF)
        out     (sc_ppi_control),a

        ld      de,#0x012C
        call	PPI_LONG_PAUSE
		ret

;-------------------------------------
sf7000_ppi_init:
;-------------------------------------

	; * SF7000 PPI PORT SET
	; ---------------------------------------
	ld     a, #0b10010000                           				; 0x90; initialise SF PPI: set I/O to mode 0, A in, B+C out
	;            ||/||||`---- PortCL Dir   = CL   Output
	;            || |||`----- PortB  Dir   = B    Output
	;            || ||`------ PortB/PortCL = B/CL Mode 0
	;            || |`------- PortCH Dir   = CH   Output
	;	     || `-------- PortA Dir    = A    Input
	;            |`---------- PortA/PortCH = A/CH Mode 0
	;            `----------- 8255A CONFIG(1)
	out    (sf7_ppi_control),a
		

	; * SF7000 PPI INIT
	; ---------------------------------------
	ld     a, #0b01001011                           	; 0x4B; reset FDC, FDD turned off, ROM OFF
	;            ||  |||`---- FDD /INUSE
	;            ||  ||`----- FDD /MOTOR ON
	;            ||  |`------ FDD TC
	;            ||  `------- FDC RESET
	;            |`---------- /ROM SEL
	;            `----------- printer /STROBE
	out    (sf7_ppi_c),a
	nop                                          		; wait
	nop
	nop
	nop
	ld     a, #0b01000011                           	; 0x43; let FDC start up, FDD still off, ROM OFF
	;            ||  |||`---- FDD /INUSE
	;            ||  ||`----- FDD /MOTOR ON
	;            ||  |`------ FDD TC
	;            ||  `------- FDC RESET
	;            |`---------- /ROM SEL
	;            `----------- printer /STROBE
	out    (sf7_ppi_c),a
	ret




