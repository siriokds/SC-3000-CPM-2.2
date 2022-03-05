.include "sc3k_bios_fdc.HAL.765.inc"


fdc_status	= 0xe0
fdc_data	= 0xe1

SECTS_SIZE	= 256
SECTS_PER_TRACK = 16

FDC_WAIT1SEC:
	push	bc
	ld	bc, #869                              ; wait for 3581154 cycles = 1.000s
	call	FDC_WAIT
	pop	bc
	ret

FDC_WAIT:   
	xor    a
FDC_WAIT2:
	dec    a
	jr     nz, FDC_WAIT2
	
	dec    bc
	ld     a,b
	or     c
	jr     nz, FDC_WAIT
	ret


;-----------------------------------
sf7k_reset:
;-----------------------------------
	di
sf7k_reset_loop:
	call	disk_initialise
	jr	c, sf7k_reset_loop
	ei


	ret



;-----------------------------------
fdc_readsectors::
;-----------------------------------
;#######################################
; READ A SET OF SECTORS TO HL
; Parameters:
; de = destination buffer
;  b = start sector
;  c = start track
;  l = num sectors 
	
	call	fdc_motor_req_on

read_set_from_disk_loop_n:
	push	bc
	push	hl
	call	fdc_readsector_inner
	pop	hl
	pop	bc

	dec	l
	ret	z
	
	inc	d					; next 256 bytes
	inc	b					; next sector
	ld	a, b
	cp	#SECTS_PER_TRACK + 1			
	jr	nz, read_set_from_disk_loop_n

	ld	b, #1
	inc	c
	jr	read_set_from_disk_loop_n

	call	fdc_motor_req_off

	ret


;-----------------------------------
;-----------------------------------
fdc_readsector::
;-----------------------------------
;-----------------------------------
	call	fdc_motor_req_on

	call	fdc_readsector_inner

	call	fdc_motor_req_off
	ret
	
;-----------------------------------
fdc_readsector_inner::
;#######################################
; 256 BYTES PHYSICAL SECTOR READ TO HL
; Parameters:
; de = destination buffer
; b = sector number on disk + 1
; c = track number on disk
; Destroys af
; returns carry on error
; checks it can read back without error, does not verify data read back


	push	bc
	push	de
	push	hl


	ld     hl, #fdc_command_loop_counter
	ld     (hl), #4                              ; try 4 times

read_from_disk_outer_4:   
	call   seek_track_c
	
	jr     c,read_from_disk_error
	ld     hl, #fdc_command_loop_counter_2
	
	ld     (hl),#10                             ; try 10 times

read_from_disk_inner_10:    
	call   read_track_c_sector_b_to_de
	jr     z, read_from_disk_exit
	
	ld     hl, #fdc_command_loop_counter_2       ; failed: loop inner (reading)
	dec    (hl)
	
	jr     nz, read_from_disk_inner_10
	
	
	ld     hl, #fdc_command_loop_counter         ; failed a lot: loop outer (seeking)
	dec    (hl)
	jr     z, read_from_disk_error
	
	call   calibrate_and_seek_to_track_0       ; so the seek will be recalibrated too
	jr     nc, read_from_disk_outer_4

read_from_disk_error:    
	scf
	.db 0x06		; will merge with next opcode to make "ld b, 0xb7" and thus not affect flags

read_from_disk_exit:
	or	a               ; reset carry flag

	pop	hl
	pop	de
	pop	bc
	ret
		

read_track_c_sector_b_to_de:
	ld	a, #fdc_command_read | fdc_command_modifier_mfm
	call	fill_and_write_to_fdc_9bytes
	ret	nz                               ; return if
	push	bc
	push	de
	ex	de,hl                            ; hl = dest address
	call	try_raw_read_sector_from_disk
	pop	de
	pop	bc
	
	call	 fdc_wait_for_int_result	; wait for SF7 PA0 (FDC INT)
	ret



try_raw_read_sector_from_disk:
	;di					; DII
	ld     b, #0x00                         ; counter: read 256 bytes
	ld     c, #fdc_data
	ld     e, #0b01000000                   ; used for a mask in a moment

try_raw_read_sector_loop:  
	call	fill_fdc_wait_1		; wait for FDC status bit 7 (RQM)
	and    	e                               ; return if FDC status bit 5 (EXM) is not set (?)
	ret    	z
	
	ini                                     ; read a byte from fdc_data to hl
	jr     nz, try_raw_read_sector_loop     ; loop until b = 0, ie. 256 times
	
	ld     a, #0b00000101                   ; set SF7 PC2 (FDD TC - transaction complete?) to 1
	out    (sf7_ppi_control),a
	dec    a                                ; then set it to 0, ie. just strobe it
	out    (sf7_ppi_control),a
	;ei					; EII
	ret



seek_track_c:
	push   bc
	push   hl
	call   flush_fdc_status
	ld     hl, #fdc_command_data_buffer
	ld     (hl), #fdc_command_seek
	inc    hl
	ld     (hl), #0x00                            ; drive 0 side 0
	inc    hl
	ld     (hl),c                              ; track c
	ld     a, #3                                 ; 3 byte command
	
output_no_data_fdc_command_and_get_result:
	ld     hl, #fdc_command_data_buffer
	call   write_to_fdc
	jr     nz,return_carry_set
	
	call	 fdc_wait_for_int_result
	
	pop    hl
	pop    bc
	scf                                          ; return carry set if z flag not set, ie. FDC status indicates an error
	ret    nz
	ccf
	ret

return_carry_set:
	pop    hl
	pop    bc
	scf
	ret

calibrate_and_seek_to_track_0:
	push   bc
	push   hl
	call   flush_fdc_status
	ld     hl, #fdc_command_data_buffer
	ld     (hl), #fdc_command_recalibrate
	inc    hl
	ld     (hl), #0x00                            ; drive 0, side 0
	ld     a,#2                                 ; 2 byte command
	jr     output_no_data_fdc_command_and_get_result








;###################################
fdc_stop_disk::
;###################################
	call	fdc_motor_off
	ld	a, #0b00000001                         ; set SF7 PC0 (FDD /INUSE) to 1 = set FDD off
	out	(sf7_ppi_control),a
	ld	a, #0b00001010                         ; set SF7 PC5 (???) to 0
	out	(sf7_ppi_control),a
	ret

;====================================================
fdc_motor_on:
;====================================================
	ld	a, #0b00000010                         ; set SF7 PC1 (FDD /MOTOR ON) to 0 = turn motor on
	out	(sf7_ppi_control),a
	ld	(FDC_MOTOR_ACT), a
	ret

;====================================================
fdc_motor_req_off:
;====================================================
	xor	a
	ld	(FDC_MOTOR_ACT), a
	dec	a
	ld	(FDC_MOTOR_cnt), a
	ret
	
;====================================================
fdc_motor_off:
;====================================================
	ld	a, #0b00000011                         ; set SF7 PC1 (FDD /MOTOR ON) to 1 = turn FDD motor off
	out	(sf7_ppi_control),a
	ret

;====================================================
fdc_motor_req_on:
;====================================================
	in	a,(sf7_ppi_c)                       	; reset FDD /INUSE (ie. FDD on)
	bit	1,a                                 	; check FDD /MOTOR ON
	ret	z					; if already on, return
	call	fdc_motor_on
	call	FDC_WAIT1SEC
	ret



;-----------------------------------
fdc_writesector::
;-----------------------------------
	call	fdc_motor_req_on

	call	fdc_writesector_inner

	call	fdc_motor_req_off
	ret

;-----------------------------------
fdc_writesector_inner::
;-----------------------------------
;#######################################
; 256 BYTES PHYSICAL SECTOR READ TO HL
; Parameters:
; de = source buffer
; b = sector number on disk + 1
; c = track number on disk
; Destroys af
; returns carry on error
; checks it can read back without error, does not verify data read back
	push	bc
	push	de
	push	hl

	call   seek_track_c
	jr     c,write_to_disk_error
	ld     hl, #fdc_command_loop_counter
	ld     (hl), #0x00                            		; counter: 256 tries
	
write_to_disk_loop:   
	call   write_sector_from_de_to_fdc_track_c_sector_b
	jr     nz,write_to_disk_continue
	
	
	ld     a, #0xe0                               		; pause
write_to_disk_pause:    
	dec    a
	nop
	nop
	jr     nz,write_to_disk_pause
	
	
;	call   test_read_sector_from_fdc_track_c_sector_b 	; try to read it back (slow!)
;	jr     z,write_to_disk_exit				; exit if no error

	
write_to_disk_continue:    
	ld     hl, #fdc_command_loop_counter			; else decrement counter and loop
	dec    (hl)
	jr     nz, write_to_disk_loop
	
write_to_disk_error:
	scf
	.db 0x06						; "or a" eater
	
write_to_disk_exit:  
	or     a
	
	pop	hl
	pop	de
	pop	bc
	ret





write_sector_from_de_to_fdc_track_c_sector_b:
	ld	a, #fdc_command_write | fdc_command_modifier_mfm; FDC Read mode; b and c set to sector, track already
	call	fill_and_write_to_fdc_9bytes
	ret	nz
	push	bc
	push	de
	ex	de,hl
	call	write_256_bytes_from_hl_to_fdc_when_ready
	pop	de
	pop	bc
	
	call	 fdc_wait_for_int_result		; wait for SF7 PA0 (FDC INT)
	ret



write_256_bytes_from_hl_to_fdc_when_ready:
	;di						; DII
	ld     b, #0x00                               	; counter for 256 bytes
	ld     c, #fdc_data
	ld     e, #0b01000000

write_256_loop:    
	call	fill_fdc_wait_1				; loop until FDC status bit 7 (RQM) is set
	and    e
	ret    z                                   	; return if FDC status bit 5 (EQM) not set (?)
	
	outi                                       	; output a byte
	jr     nz, write_256_loop
	
	ld     a, #0b00000101                         	; strobe SF7 PC2 (FDD TC) ->1->0 = terminate any data transfers
	out    (sf7_ppi_control),a
	dec    a
	out    (sf7_ppi_control),a
	;ei						; EII
	ret















;#######################################
; disk_reset
;#######################################
disk_reset::
	call   fill_fdc_command_result_buffer        ; doing this twice will blank it and make the FDC get on with things
	call   fill_fdc_command_result_buffer
; ... fall down...

;#######################################
; disk_initialise
; API 0x08
;#######################################
; Initialises FDC and FDD
; returns carry on error, including disk not present
disk_initialise::
	push   bc
	push   de
	push   hl
	in     a,(sf7_ppi_c)                       ; reset FDD /INUSE (ie. FDD on)
	and    #0b11110010                         ; reset FDD TC
	or     #0b00100000                         ; reset FDC RESET (ie. FDC on)
	out    (sf7_ppi_c),a                       ; set SF7 PC5 = ???
	bit    1,a                                 ; check FDD /MOTOR ON
	jr     z, disk_init_detect_spin            ; if already on, go detect rotation speed


	; TURN MOTOR ON
	ld     a, #0b00000010                         ; set SF7 PC1 (FDD /MOTOR ON) to 0 = turn motor on
	out    (sf7_ppi_control),a

	call	FDC_WAIT1SEC




disk_init_detect_spin:    
	ld     d, #250                               ; outer loop 250 times - how may times to try to time a fast disk spin before failing
	ld     e, #7                                 ; inner loop 7 times - how many times to try to detect a spin before failing

disk_initialise_loop:
	ld     bc, #0x0000                            ; wait for FDC INDEX (PA2) to be set
disk_init_check_high:    
	in     a,(sf7_ppi_a)
	and    #0b00000100
	jr     nz, disk_init_high_ok                  ; passed check (nz), carry on
	dec    bc
	ld     a,b
	or     c
	jr     nz,disk_init_check_high
	jr     disk_initialise_error                  	; flag was never set


disk_init_high_ok:    
	ld     hl, #0x0000                            	; wait for FDC INDEX (PA2) to be reset
			
disk_init_check_low:    
	in     a,(sf7_ppi_a)				; 11 cycles
	and    #0b00000100													;  4 cycles
	jr     z, disk_init_low_ok                 	;  7 passed check (z), carry on
	dec    hl					;  6
	ld     a,h					;  4
	or     l					;  4
	jr     nz, disk_init_check_low			; 12
																									; tot = 48 cycles
	
	jr     disk_initialise_error

disk_init_low_ok:    

	ld     bc, #15441                            ; 20 wait (~207ms) for FDC INDEX (PA2) to be set
disk_init_wait_high2:    
	in     a,(sf7_ppi_a)				; 11
	and    #0b00000100                           ;  4
	jr     nz, disk_init_high2_end             ;  7 (+5 on exit) passed check (nz), carry on
	dec    bc                                  ;  6
	ld     a,b                                 ;  4
	or     c                                   ;  4
	jr     nz, disk_init_wait_high2            ; 12 (always pass)
	
	; loop is 48 cycles + 31 since last port read, + 27 reaching successful exit
	
	; ran out of time waiting
	dec    e
	jr     nz,disk_initialise_loop             ; repeat disk_initialise_loop while e-- > 0 (inner loop)
	jr     disk_initialise_error
	
	; disk spin fully detected
	; see how fast its spinning - if bs was decremented to 2806 or less, it was too slow
	; reaching 2807 is equivalent to 12634 loops and 1 successful exit
	; which makes 606490 cycles between INDEX off and INDEX on
	; = 169ms = 354RPM

disk_init_high2_end:    
	ld     hl, #2807                             ; compare bc to 2807
	or     a
	sbc    hl,bc
	jr     nc, disk_init_spin_ok               ; if less, its good - disk is spinning fast
	dec    d                                   ; else, try again, looping on d (outer loop)
	jr     nz,disk_initialise_loop
	jr     disk_initialise_error

disk_init_spin_ok:
	call	fdc_setup
	jr	nz,disk_initialise_error

	call	calibrate_and_seek_to_track_0
	jr	c,disk_initialise_error

disk_initialise_exit:  
	pop	hl
	pop	de
	pop	bc
	ret

disk_initialise_error:
	scf                                          ; set carry = 1
	jr	disk_initialise_exit                      ; return


