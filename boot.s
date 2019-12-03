global loader
extern kernel_main

extern _kernel_start
extern _kernel_end

MEMINFO	 equ  1<<1				; to mówi loaderowi, żeby zrobił strukturę mapy pamięci
FLAGS    equ  MEMINFO			; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum of above, to prove we are multiboot

KERNEL_STACK_SIZE equ 16384		; rozmiar stacku kernela

section .text
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

loader:	
	mov esp, kernel_stack + KERNEL_STACK_SIZE	; wstawienie do esp, ostatniego adresu pamięci stosu
	
	push eax				; flaga MAGIC
	push ebx				; adres struktury multiboot info
	mov ecx, _kernel_end	; adres końca kernela
	push ecx				
	mov ecx, _kernel_start	; adres początku kernela
	push ecx

	call kernel_main
.loop:
	hlt
	jmp .loop

section .bss
align 4096
kernel_stack:
	resb KERNEL_STACK_SIZE
