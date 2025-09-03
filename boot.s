global loader
extern kernel_main

extern _kernel_start
extern _kernel_end

MBALIGN   equ  1<<0				; to mówi loaderowi, żeby zrobił strukturę mapy pamięci
MEMINFO	 equ  1<<1				; to mówi loaderowi, żeby zrobił strukturę mapy pamięci
MBFLAGS    equ  MBALIGN | MEMINFO	; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS)   ; checksum of above, to prove we are multiboot

KERNEL_STACK_SIZE equ 16384		; rozmiar stacku kernela

section .multiboot
	align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

section .bss
align 16
stack_bottom:
	resb KERNEL_STACK_SIZE
stack_top:

section .text
loader:	
	mov esp, stack_top	; wstawienie do esp, ostatniego adresu pamięci stosu
	
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
