section .bss
align 16
stack_bot:
	resb 16384 		; 16 KiB
stack_top:

section .text
global _start:function (_start.end - _start)
_start:
	mov rsp, stack_top
	mov rbp, rsp
	; Save the Kickstart bootinfo pointer
	push rax

	cli
;extern _init:function
;	call _init

	pop rdi			; Move bootinfo given by Kickstart to the first argument 
extern kernel_arch_init:function
	call kernel_arch_init

	; TODO: Add literally everything else
.hang:
	hlt
	jmp .hang
.end:
