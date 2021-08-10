;----------------------------------
fdc_setup:
;----------------------------------
	ld     hl, #fdc_specification_data           ; Set up FDC parameters
	ld     a, #fdc_specification_data_end-fdc_specification_data
	call   write_to_fdc
	ret

fdc_specification_data:
.db fdc_command_specify                          ; "specify"
.db (6<<4) | 0                                   ; Step Rate Time 6ms, Head Unload Time 0ms
.db (5<<1) | 1                                   ; Head Load Time 5ms, Non-DMA mode true
fdc_specification_data_end:


;----------------------------------
fill_fdc_command_data_buffer: 
;----------------------------------
; fills 9-byte fdc_command_data_buffer for command a, cylinder c, track b
; and default the other 5 bytes to work with this drive
; returns hl = fdc_command_data_buffer

; writes a, #0x0, c, 0x0, b, 0x1, 0x10, 0xe, 0xff to fdc_command_data_buffer
;        |   |  |   |  |   |    |   |    `-- DTL = data length  = unused when N=0
;        |   |  |   |  |   |    |   `------- GPL = gap 3 length = some low-level stuff
;        |   |  |   |  |   |    `----------- EOT = end of track = highest possible sector number
;        |   |  |   |  |   `---------------- N   = number       = number of bytes to transfer
;        |   |  |   |  `-------------------- R   = record       = sector number
;        |   |  |   `----------------------- H   = head         = head address
;        |   |  `--------------------------- C   = cylinder     = track number
;        |   `------------------------------ drive 0, side 0
;        `---------------------------------- command + flags
	ld	hl, #fdc_command_data_buffer
	push	hl
	ld	(hl),a			; command + flags
	inc	hl
	ld	(hl),#0x00		; drive 0, side 0
	inc	hl
	ld	(hl),c			; track number
	inc	hl
	ld	(hl),#0x00		; head address always 0
	inc	hl
	ld	(hl),b			; sector number
	inc	hl
	ld	(hl),#1			; amount to read/write - 1 byte!
	inc	hl
	ld	(hl),#16		; highest sector number
	inc	hl
	ld	(hl),#0x0e		; gap 3 stuff
	inc	hl
	ld	(hl),#0xff		; ignored
	pop	hl
	ret





;----------------------------------
fill_fdc_wait_1:    
;----------------------------------
	in     a, (fdc_status)                      ; wait for status bit 7 = 1 
	rlca
	jr     nc,fill_fdc_wait_1
	ret

;----------------------------------
fdc_wait_for_int_result:
;----------------------------------
	in	a, (sf7_ppi_a)                       ; wait for SF7 PA0 (FDC INT)
	rrca
	jr	nc, fdc_wait_for_int_result

	call	fill_fdc_command_result_buffer
	ret




;--------------------------------
fill_and_write_to_fdc_9bytes:
;--------------------------------
	call	fill_fdc_command_data_buffer

;	FALL DOWN...

;--------------------------
write_to_fdc_9bytes:
;--------------------------
	ld	a, #9

;	FALL DOWN...

;#######################################
; Write a bytes from (hl) to FDC
;#######################################
; Parameters:
; a = number of bytes to write
; hl = where to write from
; Returns:
; carry set on error
; zero set on success
; Clobbers: af
;--------------------------
write_to_fdc:
;--------------------------
	push	bc
	push	hl
	ld	b,a			; a = loop count
	ld	c, #fdc_data		; c = fdc data port
wait_bit_4:    
	in	a,(fdc_status)		; wait for fdc status bit 4 to be 0
	bit	4,a
	jr	nz, wait_bit_4
	
write_to_fdc_loop:
	call	fill_fdc_wait_1
					; wait for fdc status bit 7 to be 1
	rlca				; if fdc status bit 6 is 1, error
	jr	c, write_to_fdc_error
	outi                            ; else, output a byte
	jr	nz, write_to_fdc_loop
	pop	hl
	pop	bc
	xor	a
	ret
	
write_to_fdc_error:
	call   fill_fdc_command_result_buffer
	pop    hl
	pop    bc
	ret



; while FDC INT is set, read in the FDC status registers
;---------------------------------
flush_fdc_status:
;---------------------------------
	in	a,(sf7_ppi_a)                         ; if SF7 PA0 (FDC INT) set, fill status buffer and repeat
	rrca                                         ; else exit
	ret	nc
	call	fill_fdc_command_result_buffer
	jr	flush_fdc_status



; read the FDC status registers - however many it wants to return
; returns the contents of SR0 in a, and z set for normal termination status value
;---------------------------------
fill_fdc_command_result_buffer:
;---------------------------------
	push	hl
	ld	hl, #fdc_command_result_buffer
	call	fill_fdc_wait_1    
	rlca
	jr	c,fill_fdc_read                     ; if status bit 6 = 1 

	ld	a,#8                                 ; else output 8
	out	(fdc_data),a

fill_fdc_wait2:
	call	fill_fdc_wait_1
	rlca
	jr	nc,fill_fdc_exit                    ; if status bit 6 = 0

fill_fdc_read:    
	in     a,(fdc_data)                        ; else read byte to (hl++)
	ld     (hl),a
	inc    hl
	jr     fill_fdc_wait2

fill_fdc_exit:
	ld     a, (fdc_command_result_buffer)       ; return FDC SR0 contents in a <--+
	and    #fdc_status_0_ok_mask                ; return z if FDC SR0 indicates normal termination
	pop    hl
	ret


	



; FDC stuff
; FDC status byte meaning
;   bit  meaning
;    7   RQM = Request for Master = ready to send/recieve data
;    6   DIO = Data Input/Output  = 1 when CPU should read, 0 when it should write
;    5   EXM = Execution Mode     = 1 during "execution phase", 0 during "results phase"
;    4   CB  = Controller Busy    = set when controller is busy and cant accept anything
;   3-0  DnB = Drive n Busy       = set when a particular drive is busy
;
; FDC status registers
;  These 4 bytes contain extended status info after an operation. Some are returned after some commands.
;  bit  meaning
;  ST0:
;  7-6  Interrupt Code    00 = OK, 01 = abnormal termination, 10 = invalid command, 11 = FDD changed state and invalidated command
;   5   Seek End          set after seek
;   4   Equipment Check   FDD failure signal or recalibration failed
;   3   Not Ready         FDD not ready, or side unavailable
;   2   Head Address      Side of disk when interrupt happened
;  1-0  Unit Select       Drive number when interrupt happened

fdc_status_0_ok_mask 			= 0b11000000 ; mask for bits 7 and 6, zero if OK
;--------------------------------------------------------------------

;  ST1:
;   7   End of Cylinder   Sector number too large
;   6                     Unused
;   5   Data Error        CRC error reading disk
;   4   Overrun           FDC not serviced fast enough
;   3                     Unused
;   2   No Data           Cant find sector (read/write data/deleted data), or error reading ID, or error finding start sector (read diagnostic)
;   1   Not Writable      FDD says disk is write protected
;   0   Missing Address Mark Low level format stuff
;  ST2:
;   7                     Unused
;   6   Control Mark      Low level format stuff
;   5   Data Error in Data Field CRC error in data section
;   4   Wrong Cylinder    Low level format stuff
;   3   Scan Equal Hit    During scan, equal condition satisfied
;   2   Scan Not Satisfied During scan, condition not met
;   1   Bad Cylinder      Low level format stuff
;   0   Missing Address Mark in Data Field Low level format stuff?
;  ST3:
;   7   Fault             Copy of FDD signal
;   6   Write Protected   Copy of FDD signal
;   5   Ready             Copy of FDD signal
;   4   Track 0           Copy of FDD signal
;   3   Two Side          Copy of FDD signal
;   2   Head Address      Copy of FDD signal
;  1-0  Unit Select       Copy of FDD signal
;
; FDC commands:
; Modifiers for multitrack, MFM, skip deleted - only apply to some commands (see xyz below, respectively)

fdc_command_modifier_multitrack   = 0b10000000
fdc_command_modifier_mfm          = 0b01000000
fdc_command_modifier_skip_deleted = 0b00100000

;--------------------------------------------------------------------
fdc_command_read_diagnostic 	= 0x02

; 0x?2 = %0yz00010 %-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     "read diagnostic"

;--------------------------------------------------------------------
fdc_command_specify 					= 0x03

; 0x03 = %00000011 %ssssuuuu %llllllln
;     "specify" - set up some parameters
;     s = Step Rate Time (multiple of 16ms)
;     u = Head Unload Time (multiple of 16ms)
;     l = Head Load Time (multiple of 2ms)
;     n = Non-DMA Mode flag
;     Results: none

;--------------------------------------------------------------------
fdc_command_sense 						= 0x04

; 0x04 = %00001000 %-----sdd
;     "sense drive status"
;     s = side
;     d = drive
;     Results: ST3

;--------------------------------------------------------------------
fdc_command_write 						= 0x05

; 0x?5 = %xy000110 %-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     write data:
;       x  = multitrack mode
;       y  = MFM mode
;       dd = drive
;       s  = side
;       tt = track
;       hh = head address
;       rr = sector
;       nn = byte count
;       ee = final sector number
;       gg = gap length
;       ll = data length ll (if nn = 0)
;     for SF-7000, xy = 01, dd = 0, s = 0, hh = 0, ee = 16, gg = 14, ll = dont care if nn > 0
;     Results: ST0, ST1, ST2, ( 0xtt, 0xhh, 0xrr, 0xnn ) after operation

;--------------------------------------------------------------------
fdc_command_read 							= 0x06

; 0x?6 = %xyz00110 %-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     read data:
;       x  = multitrack mode
;       y  = MFM mode
;       z  = skip deleted data address mark
;       dd = drive
;       s  = side
;       tt = track
;       hh = head address
;       rr = sector
;       nn = byte count
;       ee = final sector number
;       gg = gap length
;       ll = data length ll (if nn = 0)
;     for SF-7000, xyz = 010, dd = 0, s = 0, hh = 0, ee = 16, gg = 14, ll = dont care if nn > 0
;     Results: ST0, ST1, ST2, ( 0xtt, 0xhh, 0xrr, 0xnn ) after operation

;--------------------------------------------------------------------
fdc_command_recalibrate 			= 0x07

; 0x07 = %00000111 %------dd
;     recalibrate drive dd, seek to track 0

;--------------------------------------------------------------------
fdc_command_sense_interrupt 	= 0x08

; 0x08 = %00001000
;     "sense interrupt status"
;     Returns: ST0, 0xtt = current track number

;--------------------------------------------------------------------
fdc_command_write_deleted 		= 0x09
; 0x?9 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x?? 0x??
;     "write deleted data"

;--------------------------------------------------------------------
fdc_command_read_id 					= 0x0a
; 0x?a = %0y001010 %-----sdd
;     "read sector id"
;     reads the first "correct ID information" on the current track
;     Returns: ST0, ST1, ST2, tt, hh, rr, nn

;--------------------------------------------------------------------
fdc_command_read_deleted 			= 0x0c

; 0x?c = %xyz01100 0x-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     "read deleted data"
;       x  = multitrack mode
;       y  = MFM mode
;       z  = skip deleted data address mark
;       dd = drive
;       s  = side
;       tt = track
;       hh = head address
;       rr = sector
;       nn = byte count
;       ee = final sector number
;       gg = gap length
;       ll = data length ll (if nn = 0)
;     for SF-7000, xyz = 010, dd = 0, s = 0, hh = 0, ee = 16, gg = 14, ll = dont care if nn > 0
;     Results: ST0, ST1, ST2, ( 0xtt, 0xhh, 0xrr, 0xnn ) after operation

;--------------------------------------------------------------------
fdc_command_format 						= 0x0D

; 0x?d = %0y001101 %-----sdd 0xnn 0xtt 0xgg 0xbb
;     "format a track"
;       y  = use MFM mode
;       s  = side
;       dd = drive
;       nn = bytes per sector
;       tt = sectors per track
;       gg = gap 3 length
;       bb = filler byte

;--------------------------------------------------------------------
fdc_command_seek 							= 0x0f
; 0x0f = %-----sdd 0xtt
;     seek drive dd, side s, track tt

;--------------------------------------------------------------------
fdc_command_scan_equal 				= 0x11
; 0x11 = %xyz10001 0x-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     "scan equal" - "data compared between the FDD and the host system"

;--------------------------------------------------------------------
fdc_command_scan_low_equal 		= 0x19
; 0x19 = %xyz11001 0x-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     "scan low or equal"

;--------------------------------------------------------------------
fdc_command_scan_high_equal 	= 0x1e
; 0x1e = %xyz11101 0x-----sdd 0xtt 0xhh 0xrr 0xnn 0xee 0xgg 0xll
;     "scan high or equal"

; PPI stuff
; PPI control has 2 modes:
;   if bit 7 is set, it controls input/output and overall mode
;     bit  meaning (bit 7 set)
;     6,5  port A+C(high) mode selection
;      4   port A input if set
;      3   port C upper nibble input if set
;      2   port B+C(low) mode selection
;      1   port B mode selection
;      0   port C lower nibble input if set
;   if bit 7 is reset, it sets or resets bits in port C - useful for some modes
;     bit  meaning (bit 7 reset)
;    6,5,4 ignored
;    3,2,1 bit selection (0-7)
;      0   bit value
; SF-7000 PPI port A
;   PA0 = FDC INT: in non-DMA mode, set when there is a byte to output
;   PA1 = printer port BUSY
;   PA2 = FDC INDEX: high when at the start of a track
; SF-7000 PPI port B
;   Printer port output - unused
; SF-7000 PPI port C
;   PC0 = FDD /INUSE
;   PC1 = FDD /MOTOR ON
;   PC2 = FDD TC = terminal count: terminates data transfers when set
;   PC3 = FDC RESET
;   PC4 = ?
;   PC5 = ?
;   PC6 = /ROM SEL = map ROM to lower 4KB when zero - note that writing to ROM will write to RAM.
;   PC7 = /STROBE = printer output
