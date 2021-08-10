ENTRY	= 0x05		;entry point for the cp/m bdos.
PROGRAM	= 0x100

.area _HEADER (ABS)
	.ORG	PROGRAM

	LD	C, #9
	LD	DE, #TEXT
	CALL	ENTRY		; print string

	LD	C, #1
	CALL	ENTRY		; wait for char

	RST	0		; exit program


TEXT:	.ascii	"Test program. Click to continue...$"
