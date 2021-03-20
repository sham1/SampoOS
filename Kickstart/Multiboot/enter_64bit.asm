section .bss
extern program_entry_point:data
extern top_page_structure:data

[BITS 32]
section .text

global enter_64bit_kernel:function
enter_64bit_kernel:
	; Now we need to first set the long-mode big in the EFER MSR
	mov ecx, 0xC0000008
	rdmsr
	or eax, 1 << 8 		; Set the actual bit.
	wrmsr			; And write it

	; We must also enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; Let's now enable paging with our page table.
	; First set our page table as the one being used.
	mov eax, DWORD [top_page_structure]
	mov eax, DWORD [eax]
	mov cr3, eax

	xchg bx, bx

	; And now enable it.
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	; We are now in compatibility mode.
	; Load our new 64-bit GDT and go to 64-bit mode!
	lgdt [GDT64.Pointer]
	jmp GDT64.Code:JumpToKernel

[BITS 64]
JumpToKernel:
	; We are now in 64-bit mode!
	; Let's jump to the actual kernel image!
	mov rcx, QWORD [program_entry_point]
	jmp [rcx]

[BITS 32]
section .data
GDT64:                           ; Global Descriptor Table (64-bit).
.Null: equ $ - GDT64         ; The null descriptor.
	dw 0xFFFF                    ; Limit (low).
	dw 0                         ; Base (low).
	db 0                         ; Base (middle)
	db 0                         ; Access.
	db 1                         ; Granularity.
	db 0                         ; Base (high).
.Code: equ $ - GDT64         ; The code descriptor.
	dw 0                         ; Limit (low).
	dw 0                         ; Base (low).
	db 0                         ; Base (middle)
	db 10011010b                 ; Access (exec/read).
	db 10101111b                 ; Granularity, 64 bits flag, limit19:16.
	db 0                         ; Base (high).
.Data: equ $ - GDT64         ; The data descriptor.
	dw 0                         ; Limit (low).
	dw 0                         ; Base (low).
	db 0                         ; Base (middle)
	db 10010010b                 ; Access (read/write).
	db 00000000b                 ; Granularity.
	db 0                         ; Base (high).
.Pointer:                    ; The GDT-pointer.
	dw $ - GDT64 - 1             ; Limit.
	dd GDT64                     ; Base.
	dd 0			     ; Specified in two parts so NASM doesn't complain
