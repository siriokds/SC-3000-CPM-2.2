

; *******************************************************************
; info at: https://www.seasip.info/Cpm/bdos.html
;
; To make a BDOS call in an 8-bit CP/M, use:
;    ld      c, FUNCTION
;    ld      de, PARAMETER	(if required)
;    call    BDOS			; print string
; *******************************************************************
BDOS        = 0x05	; entry point for the cp/m bdos.
C_READ      = 1 	; Console input
C_WRITESTR  = 9		; Output string



.area _CODE

    ld      c, #C_WRITESTR	; print string
    ld      de, #TEXT		; address of $-terminated string
    call    BDOS			; print string

    ld      c, #C_READ		; wait for char
    call    BDOS		

    RST     0	; exit program


TEXT:	.ascii	"Test program. Click to continue...$"


	.area	_DATA		; 0xC000
