; Declare multiboot 2 constants.
MAGIC			equ	0xE85250D6	; Multiboot 2 magic
ARCH			equ	0 		; i386 (&& x86-64)
MULTIBOOT_LENGTH	equ	multiboot_header.end - multiboot_header
CHECKSUM		equ	-(MAGIC + ARCH + MULTIBOOT_LENGTH)

	section .multiboot
	align 8
multiboot_header:
	dd MAGIC
	dd ARCH
	dd MULTIBOOT_LENGTH
	dd CHECKSUM
.tags:
	align 8
	; Multiboot2 information request tag
.info_req_tag:
 	dw 1
 	dw 0
 	dd (.info_req_tag_end - .info_req_tag)
 	dd 5 			; Framebuffer tag
 	dd 6			; Module alignment tag
.info_req_tag_end:
	align 8
	; Framebuffer tag
	dw 5
	dw 0
	dd 20
	dd 0			; Width  - No preference
	dd 0			; Height - No preference
	dd 32			; Depth  - 32-bits per pixel.
	; Module alignment tag
	align 8
	dw 6
	dw 0
	dd 8
	; The ending tag
	align 8
	dw 0
	dw 0
	dd 8
.end:

	section .bss
	align 16
stack_bot:
	resb 16384 		; 16KiB ought to be enough for Kickstart
stack_top:

	section .text
	global _start:function (_start.end - _start)
	extern _init
_start:
	mov esp, stack_top

	; TODO: Initialize everything properly
	call _init

	cli
.hang:
	hlt
	jmp .hang
.end:
