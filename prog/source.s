BITS 32
section .text
SYSCALL_STDOUT equ 0

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
pop eax
pop ecx

mov esp, ebp
pop ebp

ret

global amain
amain:

extern main
call main

jmp $
