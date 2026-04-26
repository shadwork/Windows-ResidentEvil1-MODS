; macro to define functions
%macro func 2
	global _%1
	_%1:
	jmp[.a]
	.a: dd %2
%endmacro

; ---------------------------------------
; - FUNCTIONS						-
; ---------------------------------------
section .text

func Add_tile,		0x443350
