section .bss
align 16
stack_bot:
	resb 16384 		; 16 KiB
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
	mov rsp, stack_top

	; TODO: Add literally everything else
	cli
.hang:
	hlt
	jmp .hang
.end:
