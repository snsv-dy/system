BITS 32
section .text
SYSCALL_STDOUT equ 0
SYSCALL_EXIT equ 5

global syscall
syscall:
push ebp
mov ebp, esp

push ecx
mov ecx, ebp

push eax
push ebx

mov eax, [ebp + 12]
mov ebx, [ebp + 8]
int 80

pop ebx
pop ecx
; pop eax	eax zawiera wartość zwracaną przez syscalla, więc nie przywracam jej
pop ecx

mov esp, ebp
pop ebp

ret

global thread_wrap_start
extern thread_func_caller

thread_wrap_start:
	call thread_func_caller

	ret

global amain
amain:

extern main
call main

; syscall do zakończenia wątku
mov eax, SYSCALL_EXIT
push eax
push eax
call syscall

jmp $
