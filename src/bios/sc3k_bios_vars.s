.module BIOS_VARS

;.area	_SCB
.area HEADER (ABS)
; ==================================================

.org	0xFC00	; 0xFC00
vars_start::


conio_wfifo_char::							.ds 1
conio_wfifo_len::								.ds 1

; * KEYBOARD (16 bytes)									; 0xFCBE-0xFCCF
keyboard_input_buffer::        .ds 12		; 9 bytes .. 0xfc5e - first 7 bytes are low bytes of rasters, 8th is high bits, 9th is remaining control keys
keyboard_last_char::					 .ds 2
keyboard_typematic_cnt::			 .ds 2

; * CONIO (48 bytes)										; 0xFCD0-FCFF
cursor_enabled::							 .ds 1
cursor_visible_ff::						 .ds 1
cursor_char_saved::						 .ds 1
cursor_visible_cnt::					 .ds 2
cursor_x::                     .ds 1		; cursor x position (0-39)
cursor_y::                     .ds 1		; cursor y position (0-23)
write_char_value::             .ds 1		; char being written to the screen
scrolling_buffer::             .ds 40		; text mode scrolling buffer


; * STACK (512 bytes)
temp_stack::                   .ds 253	; 0xFD00-0xFEFF
temp_top_of_stack::            .ds 1
saved_stack_ptr::							 .ds 2
stack::                     	 .ds 255	; 0xFD00-0xFEFF
top_of_stack::                 .ds 1

; * RAM CODE (256 bytes)
ram_code_area::                .ds 256	; 0xFF00-0xFFFF 

vars_end::
