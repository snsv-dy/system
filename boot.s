global loader
extern kernel_main
global boot_page_directory

extern _kernel_start
extern _kernel_end

								; tych komentarzy nie napisałem, ale zostawiłem je, bo wyjaśniają co się dzieje
FLAGS    equ  0x0 				; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum of above, to prove we are multiboot

KERNEL_STACK_SIZE equ 16384

kernel_addr equ 0xC0000000
kernel_page_number equ (kernel_addr >> 22)

section .data
align 4096
boot_page_directory:
	dd 0x00000083
	times (kernel_page_number - 1) dd 0
	dd 0x00000083
	times (1024 - kernel_page_number) dd 0

section .text
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

loader:
	; mapowanie ostatniej pozycji katalogu stron do siebie
	;
	mov dword ebx, (boot_page_directory - kernel_addr)
	;shl ebx, 12
	or ebx, 3
	mov ecx, (boot_page_directory - kernel_addr)
	add ecx, 1023 * 4
	mov [ecx], ebx
	mov eax, ebx

	mov ecx, (boot_page_directory - kernel_addr)
	mov cr3, ecx

	mov ecx, cr4
	or ecx, 0x00000010
	mov cr4, ecx

	mov ecx, cr0
	or ecx, 0x80000000
	mov cr0, ecx

	lea ebx, [higher_half]
	jmp ebx


higher_half:
	mov dword [boot_page_directory], 0
	invlpg [0]
;	mov ebx, cr3
;	mov cr3, ebx

	
	mov esp, kernel_stack + KERNEL_STACK_SIZE
	push eax

	push _kernel_end
	push _kernel_start

	call kernel_main
.loop:
	hlt
	jmp .loop

section .bss
align 4096
kernel_stack:
	resb KERNEL_STACK_SIZE
