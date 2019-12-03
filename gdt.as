global flush_tlb
global set_page_directory
global enable_paging
extern serial_write
extern printf

global get_esp

get_esp:
	push ebp
	mov ebp, esp

	mov eax, esp

	mov esp, ebp
	pop ebp

	ret

set_page_directory:
	; w cr3 musi znaleść się adres katalogu stron
	push ebp
	mov ebp, esp

	mov ecx, [esp + 8]
	mov cr3, ecx

	mov esp, ebp
	pop ebp

	ret

fmt:
	dd "esp: %x", 0x10, 0x0

enable_paging:
	push ebp
	mov ebp, esp
	; 
	; Włączanie obsługi stron o wielkości 4 Mib
	;mov ecx, cr4
	;or ecx, 0x00000010
	;mov cr4, ecx

	; Ustawianie bitów pg i ;pe
	mov ecx, cr0
	or ecx, 0x80000001
	mov cr0, ecx

	mov esp, ebp
	pop ebp

	ret

textbuff:
	dd "Flushing tlb", 10, 0

flush_tlb:
	mov eax, cr3
	mov cr3, eax
;	push textbuff
;	call serial_write
;	pop eax
	ret

global invl_pg

invl_pg:
	pop eax
	invlpg [eax]
	push eax
	ret

global gdt_flush

gdt_flush:
	mov eax, [esp + 4]
	lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush
.flush:
	ret

global load_idt
global irqdefm
global irqdefs
global irq1
global page_except

extern irqmaster_handler
extern irqslave_handler
extern irq1_handler
extern page_except_handler

load_idt:
	mov edx, [esp+4]
	lidt [edx]
	sti
	ret

irqdefm:
	pusha
	call irqmaster_handler
	popa
	iret

irqdefs:
	pusha
	call irqslave_handler
	popa
	iret

irq1:
	pusha
	call irq1_handler
	popa
	iret

page_except:
	push eax
	mov eax, [esp + 4]
	pusha
	push eax
	mov eax, cr2
	push eax
	call page_except_handler
	pop eax
	pop eax

	popa
	pop eax
	pop eax
	iret

extern double_fault_handler
global double_fault
double_fault:
	pusha
	call double_fault_handler
	popa
	iret

global halt
halt:
	hlt
	jmp halt

;page_except2:
;	push ebp
;	mov ebp, esp



;	pop ebp