; macro to define functions
%macro func 2
	global _%1
	_%1:
	jmp[.a]
	.a: dd %2
%endmacro

; ---------------------------------------
; - FUNCTIONS
; ---------------------------------------
section .text

; rendering
func Add_line,			0x443C40
func Add_tile,			0x443350

; projection / GTE emulation
func GsSetRotMatrix,	0x433F40
func GsSetTransMatrix,	0x433F20
func GsRotTransPers,	0x433FB0
